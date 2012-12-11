/*
 * dbd.c
 *
 *  Created on: Sep 26, 2012
 *      Author: Reyad Attiyat
 *      Copyright 2012 Reyad Attiyat
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
*/

#include "mod_mediaplayer.h"


apr_status_t connect_database(apr_pool_t* pool, db_config** dbd_config){
	apr_status_t rv;

	*dbd_config = apr_pcalloc(pool, sizeof(db_config));

	rv = apr_dbd_init(pool);
	if (rv != APR_SUCCESS){
	  //Run error function
	  return rv;
	}
	(*dbd_config)->driver_name = "mysql";
	(*dbd_config)->mysql_parms = "host=127.0.0.1,user=root";
	(*dbd_config)->pool = pool;


	rv = apr_dbd_get_driver(pool, (*dbd_config)->driver_name, &((*dbd_config)->dbd_driver));
	if (rv != APR_SUCCESS){
		//Run error function
		return rv;
	}

	rv = apr_dbd_open((*dbd_config)->dbd_driver, pool, (*dbd_config)->mysql_parms, &((*dbd_config)->dbd_handle));
	if (rv != APR_SUCCESS){
		//Run error function
		return rv;
	}
	(*dbd_config)->connected = 1;
	return rv;
}

static inline void set_columns_table_dependencies(enum parameter_types type, db_config* dbd_config, const char* column_list, const char* table_link, const char* group_by){
	dbd_config->column_table_dep[type][(dbd_config->num_column_dep[type])].columns = column_list;
	dbd_config->column_table_dep[type][dbd_config->num_column_dep[type]].group_by_columns = group_by;
	dbd_config->column_table_dep[type][(dbd_config->num_column_dep[type])++].table_dependcy = table_link;
}

int prepare_database(db_config* dbd_config){
	int error_num = 0;

	error_num = apr_dbd_set_dbname(dbd_config->dbd_driver, dbd_config->pool, dbd_config->dbd_handle, "mediaplayer");
	if (error_num != 0){
		return error_num;
	}
	error_num = apr_dbd_prepare(dbd_config->dbd_driver, dbd_config->pool, dbd_config->dbd_handle, "SELECT LAST_INSERT_ID();",NULL, &(dbd_config->statements.select_last_id));
	if (error_num != 0){
		return error_num;
	}
	error_num = apr_dbd_prepare(dbd_config->dbd_driver, dbd_config->pool, dbd_config->dbd_handle, "SELECT links.songid FROM links LEFT JOIN Songs ON Songs.id = links.songid  WHERE links.artistid = %d AND links.albumid = %d AND Songs.name = %s;",NULL, &(dbd_config->statements.select_song));
	if (error_num != 0){
		return error_num;
	}
	error_num = apr_dbd_prepare(dbd_config->dbd_driver, dbd_config->pool, dbd_config->dbd_handle, "SELECT Sources.id FROM Sources LEFT JOIN links ON links.sourceid = Sources.id  WHERE links.songid = %d;",NULL, &(dbd_config->statements.select_sources));
	if (error_num != 0){
		return error_num;
	}
	error_num = apr_dbd_prepare(dbd_config->dbd_driver, dbd_config->pool, dbd_config->dbd_handle, "SELECT path FROM Sources  WHERE id = %d;",NULL, &(dbd_config->statements.select_file_path));
	if (error_num != 0){
		return error_num;
	}
	error_num = apr_dbd_prepare(dbd_config->dbd_driver, dbd_config->pool, dbd_config->dbd_handle, "SELECT id FROM Artists WHERE name=%s;",NULL, &(dbd_config->statements.select_artist));
	if (error_num != 0){
		return error_num;
	}
	error_num = apr_dbd_prepare(dbd_config->dbd_driver, dbd_config->pool, dbd_config->dbd_handle, "SELECT id FROM Albums WHERE name=%s;",NULL, &(dbd_config->statements.select_album));
	if (error_num != 0){
		return error_num;
	}
	error_num = apr_dbd_prepare(dbd_config->dbd_driver, dbd_config->pool, dbd_config->dbd_handle, "UPDATE Albums, Artists, Songs, Sources, links "
			"SET Albums.name = %s, Artists.name=%s, Songs.name=%s, Songs.length = %d, links.track_no = %d, links.disc_no = %d, Sources.mtime = %ld "
			"WHERE Sources.path = %s  AND links.artistid = Artists.id AND links.albumid = Albums.id AND links.songid = Songs.id;" ,NULL, &(dbd_config->statements.update_song));
	if (error_num != 0){
		return error_num;
	}
	error_num = apr_dbd_prepare(dbd_config->dbd_driver, dbd_config->pool, dbd_config->dbd_handle, "SELECT mtime FROM Sources WHERE path = (%s)", NULL, &(dbd_config->statements.select_mtime));
	if (error_num != 0){
		return error_num;
	}
	error_num = apr_dbd_prepare(dbd_config->dbd_driver, dbd_config->pool, dbd_config->dbd_handle, "INSERT INTO `Artists` (name) VALUE (%s)",NULL, &(dbd_config->statements.add_artist));
	if (error_num != 0){
		return error_num;
	}
	error_num = apr_dbd_prepare(dbd_config->dbd_driver, dbd_config->pool, dbd_config->dbd_handle, "INSERT INTO `Albums` (name) VALUE (%s);",NULL, &(dbd_config->statements.add_album));
	if (error_num != 0){
		return error_num;
	}
	error_num = apr_dbd_prepare(dbd_config->dbd_driver, dbd_config->pool, dbd_config->dbd_handle, "INSERT INTO `Songs` (`name`, `musicbrainz_id`, `length`, `play_count`) VALUE (%s, %s, %d,%d);",NULL, &(dbd_config->statements.add_song));
	if (error_num != 0){
		return error_num;
	}
	error_num = apr_dbd_prepare(dbd_config->dbd_driver, dbd_config->pool, dbd_config->dbd_handle, "INSERT INTO `Sources` (`type`, `path`, `quality`,`mtime`) VALUE (%s, %s, %d,%lld);",NULL, &(dbd_config->statements.add_source));
	if (error_num != 0){
		return error_num;
	}
	error_num = apr_dbd_prepare(dbd_config->dbd_driver, dbd_config->pool, dbd_config->dbd_handle, "INSERT INTO `links` (artistid, albumid, songid, sourceid, feature, track_no, disc_no) VALUES (%d, %d, %d, %d, %d, %d, %d);",NULL, &(dbd_config->statements.add_link));
	if (error_num != 0){
		return error_num;
	}


	/*
	 * Select songs from database:
	 * Use range, sortby and ID,
	 * Source Type,
	 * Artist ID,
	 * Album ID
	 *
	 */
	set_columns_table_dependencies(SONGS,dbd_config, "links.track_no, links.disc_no", "links",NULL);
	set_columns_table_dependencies(SONGS,dbd_config,"links.songid, Songs.length", "Songs ON links.songid = Songs.id","links.songid");
	set_columns_table_dependencies(SONGS,dbd_config,"Artists.name, links.artistid", "Artists ON links.artistid = Artists.id","links.artistid");
	set_columns_table_dependencies(SONGS,dbd_config,"Albums.name,links.albumid", "Albums ON links.albumid = Albums.id","links.albumid");
	set_columns_table_dependencies(SONGS,dbd_config,"Sources.id, Sources.type, Sources.quality","Sources ON links.sourceid = Sources.id",NULL);

	set_columns_table_dependencies(ALBUMS,dbd_config,"links.albumid", "links","links.albumid");
	set_columns_table_dependencies(ALBUMS,dbd_config,"Albums.name", "Albums ON links.albumid = Albums.id", NULL);
	set_columns_table_dependencies(ALBUMS,dbd_config,"links.artistid, Artists.name", "Artists ON links.artistid = Artists.id", "links.artistid");

	set_columns_table_dependencies(ARTISTS,dbd_config,"links.artistid", "links", "links.artistid");
	set_columns_table_dependencies(ARTISTS,dbd_config,"links.albumid, Albums.name", "Albums ON links.albumid = Albums.id", "links.albumid");
	set_columns_table_dependencies(ARTISTS,dbd_config,"Artists.name", "Artists ON links.artistid = Artists.id", NULL);

	set_columns_table_dependencies(SOURCES,dbd_config,"links.sourceid, links.songid", "links", "links.sourceid");
	set_columns_table_dependencies(SOURCES,dbd_config,"Songs.length", "Songs ON links.songid = Songs.id", "Songs.id");

	set_columns_table_dependencies(PLAY,dbd_config,"Sources.id, Sources.type, Sources.quality, Sources.path","Sources", NULL);

	set_columns_table_dependencies(TRANSCODE,dbd_config,"Sources.id, Sources.type, Sources.quality, Sources.path","Sources", NULL);



	int num_query_types;
	unsigned int num_parameter_combos;
	for(num_query_types = 0; num_query_types < NUM_QUERY_TYPES; num_query_types++){
		int num_tables;
		const char* select_columns = NULL;
		const char* tables = NULL;
		const char* group_by = NULL;

		for(num_tables = 0; num_tables < dbd_config->num_column_dep[num_query_types]; num_tables++){
			select_columns = (select_columns) ? apr_pstrcat(dbd_config->pool,select_columns, ", ",dbd_config->column_table_dep[num_query_types][num_tables].columns,NULL) : dbd_config->column_table_dep[num_query_types][num_tables].columns;
			tables = (tables) ? apr_pstrcat(dbd_config->pool, tables," LEFT JOIN ",dbd_config->column_table_dep[num_query_types][num_tables].table_dependcy,NULL)  : dbd_config->column_table_dep[num_query_types][num_tables].table_dependcy;
			if (dbd_config->column_table_dep[num_query_types][num_tables].group_by_columns){
				group_by = (group_by) ? apr_pstrcat(dbd_config->pool,", ", dbd_config->column_table_dep[num_query_types][num_tables].group_by_columns ,NULL) :   dbd_config->column_table_dep[num_query_types][num_tables].group_by_columns;
			}
		}

		for(num_parameter_combos = 0;num_parameter_combos < (1 << NUM_QUERY_PARAMETERS);num_parameter_combos++){
			char* where =NULL;

			//Setup type specific parameters
			switch(num_query_types){
				case SONGS:{

					if(num_parameter_combos & (1<<ALBUM_ID)){
						const char* album_id = "Albums.id = %d";
						if (where == NULL){
							where = album_id;
						}else{
							where =apr_pstrcat(dbd_config->pool, where, " AND ", album_id,NULL);
						}
					}
					if(num_parameter_combos & (1<<ALBUM_NAME)){
											const char* album_id = "Albums.name LIKE %s";
											where = (where) ? apr_pstrcat(dbd_config->pool, where, " AND ", album_id,NULL) : album_id;
					}
					if(num_parameter_combos & (1<<ARTIST_ID)){
						const char* album_id = "Artists.id = %d";
						if (where == NULL){
							where = album_id;
						}else{
							where =apr_pstrcat(dbd_config->pool, where, " AND ", album_id,NULL);
						}
					}
					if(num_parameter_combos & (1<<ARTIST_NAME)){
											const char* album_id = "Artists.name LIKE %s";
											where = (where) ? apr_pstrcat(dbd_config->pool, where, " AND ", album_id,NULL) : album_id;
					}
					if(num_parameter_combos & (1<<SOURCE_TYPE)){
						const char* album_id = "Sources.type= %s";
						if (where == NULL){
							where = album_id;
						}else{
							where =apr_pstrcat(dbd_config->pool, where, " AND ", album_id,NULL);
						}
					}
					break;
				}
			}
			if(select_columns && tables){
				const char* select_statement = NULL;
				if(num_parameter_combos == 0 || where){
						select_statement = apr_psprintf(dbd_config->pool,"SELECT %s FROM %s",select_columns,tables);
				}
				if(where){
					select_statement = apr_pstrcat(dbd_config->pool,select_statement," WHERE ",where,NULL);
				}
				/*
				if(num_parameter_combos & (1 << ROWCOUNT)){
					select_statement = apr_pstrcat(dbd_config->pool,select_statement," LIMIT %d", NULL);
					if(num_parameter_combos & (1 << OFFSET)){
						select_statement = apr_pstrcat(dbd_config->pool,select_statement," OFFSET %d", NULL);
					}
				}
				*/
				if(select_statement){
					error_num = apr_dbd_prepare(dbd_config->dbd_driver, dbd_config->pool, dbd_config->dbd_handle, select_statement,apr_pstrcat(dbd_config->pool,apr_itoa(dbd_config->pool,num_query_types ),"#",apr_itoa(dbd_config->pool,(int)num_parameter_combos),NULL), &(dbd_config->statements.select_by_range[num_query_types][num_parameter_combos]));
					if (error_num != 0){
						return error_num;
					}
				}
			}
		}
	}


/*
	int i;
	//Select songs with no id
	for (i = 0; i < 3; i++){
		const char* statement_string = apr_pstrcat(dbd_config->pool, "SELECT Songs.id, Songs.name, Artists.name, Albums.name, Songs.length, links.track_no, links.disc_no FROM links LEFT JOIN Songs ON links.songid = Songs.id LEFT JOIN Artists ON links.artistid = Artists.id LEFT JOIN Albums ON links.albumid = Albums.id ORDER BY ", Sort_By_Table_Names[i]," LIMIT %d, %d;", NULL);
		error_num = apr_dbd_prepare(dbd_config->dbd_driver, dbd_config->pool, dbd_config->dbd_handle, statement_string,NULL, &(dbd_config->statements.select_songs_range[i]));
		if (error_num != 0){
			return error_num;
		}
	}
	//Select songs with artist id
	for (i = 0; i < 3; i++){
		const char* statement_string = apr_pstrcat(dbd_config->pool, "SELECT Songs.id, Songs.name, Artists.name, Albums.name, Songs.length, links.track_no, links.disc_no FROM links LEFT JOIN Songs ON links.songid = Songs.id LEFT JOIN Artists ON links.artistid = Artists.id LEFT JOIN Albums ON links.albumid = Albums.id WHERE Artists.id = %d ORDER BY ", Sort_By_Table_Names[i]," LIMIT %d, %d;", NULL);
		error_num = apr_dbd_prepare(dbd_config->dbd_driver, dbd_config->pool, dbd_config->dbd_handle, statement_string,NULL, &(dbd_config->statements.select_songs_by_artist_id_range[i]));
		if (error_num != 0){
			return error_num;
		}
	}
	//Select songs with album id
	for (i = 0; i < 3; i++){
		const char* statement_string = apr_pstrcat(dbd_config->pool, "SELECT Songs.id, Songs.name, Artists.name, Albums.name, Songs.length, links.track_no, links.disc_no FROM links LEFT JOIN Songs ON links.songid = Songs.id LEFT JOIN Artists ON links.artistid = Artists.id LEFT JOIN Albums ON links.albumid = Albums.id WHERE Albums.id = %d ORDER BY ", Sort_By_Table_Names[i]," LIMIT %d, %d;", NULL);
		error_num = apr_dbd_prepare(dbd_config->dbd_driver, dbd_config->pool, dbd_config->dbd_handle, statement_string,NULL, &(dbd_config->statements.select_songs_by_album_id_range[i]));
		if (error_num != 0){
			return error_num;
		}
	}
	//Select all artists
	for (i = 0;i < 2; i++){
		const char* order;
		((i+1) % 2) ? (order = "ASC") : (order = "DESC");
		const char* statement_string = apr_pstrcat(dbd_config->pool, "SELECT Artists.id, Artists.name FROM Artists ORDER BY Artists.name ",order, " LIMIT %d, %d;", NULL);
		error_num = apr_dbd_prepare(dbd_config->dbd_driver, dbd_config->pool, dbd_config->dbd_handle, statement_string,NULL, &(dbd_config->statements.select_artists_range[i]));
		if (error_num != 0){
			return error_num;
		}
	}
	//Select all albums
	for (i = 0;i < 2; i++){
		const char* order;
		((i+1) % 2) ? (order = "ASC") : (order = "DESC");
		const char* statement_string = apr_pstrcat(dbd_config->pool, "SELECT Albums.id, Albums.name FROM Albums ORDER BY Albums.name ", order, " LIMIT %d, %d;", NULL);
		error_num = apr_dbd_prepare(dbd_config->dbd_driver, dbd_config->pool, dbd_config->dbd_handle, statement_string,NULL, &(dbd_config->statements.select_albums_range[i]));
		if (error_num != 0){
			return error_num;
		}
	}
	//Select all albums with artist id
	for (i = 0;i < 2; i++){
		const char* order;
		((i+1) % 2) ? (order = "ASC") : (order = "DESC");
		const char* statement_string = apr_pstrcat(dbd_config->pool, "SELECT Albums.id, Albums.name FROM Albums JOIN links ON Albums.id = links.albumid WHERE links.artistid = %d GROUP BY Albums.id, Albums.name ORDER BY Albums.name ",  order, " LIMIT %d, %d;", NULL);
		error_num = apr_dbd_prepare(dbd_config->dbd_driver, dbd_config->pool, dbd_config->dbd_handle, statement_string,NULL, &(dbd_config->statements.select_albums_by_artist_id_range[i]));
		if (error_num != 0){
			return error_num;
		}
	}
	*/
	return error_num;
}
int get_insert_last_id (char** id, db_config* dbd_config){
	int error_num = 0;
	int row_count;
	apr_pool_t* pool = dbd_config->pool;

	apr_dbd_results_t* results = NULL;
	apr_dbd_row_t *row = NULL;

	//Get id of new title
	error_num = apr_dbd_pvselect(dbd_config->dbd_driver, pool, dbd_config->dbd_handle,  &results, dbd_config->statements.select_last_id, 0);
	if (error_num != 0){
		add_error_list(dbd_config->database_errors, ERROR, "DBD insert_db_song error", apr_dbd_error(dbd_config->dbd_driver, dbd_config->dbd_handle, error_num));
		return error_num;
	}
	//If it does cylce throgh all of them to clear cursor
	for (row_count = 0, error_num = apr_dbd_get_row(dbd_config->dbd_driver, pool, results, &row, -1);
			error_num != -1;
			row_count++,error_num = apr_dbd_get_row(dbd_config->dbd_driver, pool, results, &row, -1)) {
			//only get first result
				*id = apr_pstrdup(pool,apr_dbd_get_entry (dbd_config->dbd_driver,  row, 0 ));
	}
	if (*id == NULL || strcmp(*id, "") == 0){
		return -2;
	}

		return 0;
}

const char* get_full_column_name(db_config* dbd_config, music_query* query, apr_dbd_results_t* results, int col_num){
	const char* mysql_name;
	const char* song_col_name[] = {"id","title", "Artist", "Album", "length","track_no", "disc_no", "source_id", "source_type","source_quality","unkown"};
	mysql_name = apr_dbd_get_name(dbd_config->dbd_driver,results, col_num);
	if (query->type == SONGS){
		return song_col_name[col_num];
	}else{
		return mysql_name;
	}

}

int select_db_range(db_config* dbd_config, music_query* query){
	int error_num;
	char* old_key;
	apr_dbd_results_t* results = NULL;
	apr_dbd_row_t *row = NULL;

	int num_parameters_set = __builtin_popcount(query->query_parameters_set);
	int parameters;
	unsigned int parameters_set = query->query_parameters_set;
	query->results = apr_pcalloc(dbd_config->pool, sizeof(results_table_t));
	if(query->query_parameters_set == 0){
		error_num = apr_dbd_pselect(dbd_config->dbd_driver, dbd_config->pool, dbd_config->dbd_handle,  &results,dbd_config->statements.select_by_range[query->type][0], 0,0,NULL);
	}else{
		//Create array of arguments
		//to pass too our prepared statement
		char* args[num_parameters_set];
		for(parameters = 0;parameters < num_parameters_set; parameters++){
			args[parameters] = query->query_parameters[__builtin_ctz(parameters_set)].parameter_value;
			parameters_set &= (parameters_set - 1);

		}
		error_num = apr_dbd_pselect(dbd_config->dbd_driver, dbd_config->pool, dbd_config->dbd_handle,  &results,dbd_config->statements.select_by_range[query->type][query->query_parameters_set], 0,0,(const char**)args);
	}




	if (error_num != 0){
		//check if not connected
		if (error_num == 2013){
			dbd_config->connected = 0;
		}
		add_error_list(query->error_messages, ERROR, "DBD select_db_range error", apr_dbd_error(dbd_config->dbd_driver, dbd_config->dbd_handle, error_num));
		return error_num;
	}
	query->results->song_results = apr_table_make(dbd_config->pool, 1000);
	query->results->sources_results = apr_table_make(dbd_config->pool,1000);
	//Cycle through all of them to clear cursor
	for (query->results->song_count = 0, error_num = apr_dbd_get_row(dbd_config->dbd_driver, dbd_config->pool, results, &row, -1);
			error_num != -1;
			error_num = apr_dbd_get_row(dbd_config->dbd_driver, dbd_config->pool, results, &row, -1)) {
					(query->results->song_count)++;
					const char* key =   apr_dbd_get_entry (dbd_config->dbd_driver,  row, 0);
					if(key == NULL){
						add_error_list(query->error_messages,ERROR,"select_db_range",apr_psprintf(dbd_config->pool,"No song id(key) Rowcount: %d", query->results->song_count));
						break;
					}
					/*
					if (old_key == key){
						const char* source_string = apr_psprintf(dbd_config->pool, "{\"id\": \"%s\", \"type\": \"%s\",\"quality\": \"%s\"}");
						apr_table_merge(query->results->sources_results,key, source_string);
					}else{
					*/

						int i;
						for(i = 1; i < apr_dbd_num_cols(dbd_config->dbd_driver, results); i++){
							//Since three tables all use the same name we must substitue for what it actually is
							const char* column_text = apr_dbd_get_entry (dbd_config->dbd_driver,  row, i);
							column_text = column_text ? column_text : "BLANK_BLANK_BLANK";
							const char* value = apr_psprintf(dbd_config->pool, "\"%s\": \"%s\"", get_full_column_name(dbd_config,query,results, i),column_text);
							apr_table_merge(query->results->song_results, key, value);
						}
						/*


						const char* source_string = apr_psprintf(dbd_config->pool, "{\"id\": \"%s\", \"type\": \"%s\",\"quality\": \"%s\"}", json_escape_char(dbd_config->pool, apr_dbd_get_entry (dbd_config->dbd_driver,  row, i+1)), json_escape_char(dbd_config->pool, apr_dbd_get_entry (dbd_config->dbd_driver,  row, i+2)), json_escape_char(dbd_config->pool, apr_dbd_get_entry (dbd_config->dbd_driver,  row, i+3)));
						apr_table_add(query->results->sources_results,key, source_string);
					}
					old_key = key;
					*/
		}
	if (query->results->song_count == 0){
		return 1;
	}
	return 0;
}

static int insert_db(char** id, db_config* dbd_config, apr_dbd_prepared_t* query, const char** args){
	int error_num = 0;
	int nrows = 1;
	apr_pool_t* pool = dbd_config->pool;

	//Create new title
	error_num = apr_dbd_pquery(dbd_config->dbd_driver, pool, dbd_config->dbd_handle, &nrows, query, 0,args);

	if (id !=NULL && error_num == 0){
		error_num = get_insert_last_id(id, dbd_config);
	}
	return error_num;
}

/*
static int insert_db_artist(char** id, db_config* dbd_config, apr_dbd_prepared_t* query, music_file* song){
	int error_num = 0;
	int nrows = 1;
	apr_pool_t* pool = dbd_config->pool;

	//Create new title
	error_num = apr_dbd_pvquery(dbd_config->dbd_driver, pool, dbd_config->dbd_handle, &nrows, query, song->artist);

	if (error_num == 0){
		error_num = get_insert_last_id(id, dbd_config);
	}
	return error_num;
}

static int insert_db_album(char** id, db_config* dbd_config, apr_dbd_prepared_t* query, music_file* song){
	int error_num = 0;
	int nrows = 1;
	apr_pool_t* pool = dbd_config->pool;

	//Create new title
	error_num = apr_dbd_pvquery(dbd_config->dbd_driver, pool, dbd_config->dbd_handle, &nrows, query, song->album);
	if (error_num == 0){
		error_num = get_insert_last_id(id, dbd_config);
	}
	return error_num;
}

static int insert_db_song(char** id, db_config* dbd_config, apr_dbd_prepared_t* query, music_file* song, apr_time_t mtime){
	int error_num;
	int nrows = 1;
	apr_pool_t* pool = dbd_config->pool;

	//Create new title
	error_num = apr_dbd_pvquery(dbd_config->dbd_driver, pool, dbd_config->dbd_handle, &nrows, query, song->title, "aaa", song->file_path,apr_itoa(pool, song->length),  apr_ltoa(pool,mtime), apr_itoa(pool, 0));
	if (error_num == 0){
		error_num = get_insert_last_id(id, dbd_config);
	}
	return error_num;
}

static int insert_db_links(db_config* dbd_config, apr_dbd_prepared_t* query, music_file* song, char* artist_id, char* album_id, char* song_id){
	int error_num = 0;
	int nrows = 1;
	int feature = 0;
	apr_pool_t* pool = dbd_config->pool;

	if (song->track_no == NULL){
		song->track_no = apr_itoa(pool, 0);
	}
	if (song->disc_no == NULL){
		song->disc_no = apr_itoa(pool, 0);
	}
	error_num = apr_dbd_pvquery(dbd_config->dbd_driver, pool, dbd_config->dbd_handle, &nrows, dbd_config->statements.add_link,artist_id,album_id,song_id,apr_itoa(pool,  feature),song->track_no,song->disc_no);
	if (error_num != 0){
			add_error_list(dbd_config->database_errors, ERROR, "DBD insert_db_links error", apr_dbd_error(dbd_config->dbd_driver, dbd_config->dbd_handle, error_num));
			return error_num;
	}
	return error_num;
}
*/
static int update_song(db_config* dbd_config, music_file* song, apr_time_t mtime){
	int error_num = 0;
	int nrows = 0;
	int feature = 0;
	apr_pool_t* pool = dbd_config->pool;

	error_num = apr_dbd_pvquery(dbd_config->dbd_driver, pool, dbd_config->dbd_handle, &nrows, dbd_config->statements.update_song,song->album,song->title, apr_itoa(pool,  song->length), pool, song->track_no, song->disc_no, apr_ltoa(pool,(long) mtime));
	if (error_num != 0){
		add_error_list(dbd_config->database_errors, ERROR, "DBD update_song error", apr_dbd_error(dbd_config->dbd_driver, dbd_config->dbd_handle, error_num));
		return error_num;
	}
	return 0;
}

int get_file_path(char** file_path, db_config* dbd_config, char* id, apr_dbd_prepared_t* select){
	int error_num = 0;
	int row_count;
	apr_dbd_results_t* results = NULL;
	apr_dbd_row_t *row = NULL;
	apr_pool_t* pool = dbd_config->pool;

	error_num = apr_dbd_pvselect(dbd_config->dbd_driver, pool, dbd_config->dbd_handle,  &results, select, 0, id);
	if (error_num != 0){
		add_error_list(dbd_config->database_errors, ERROR, "DBD get file_path error", apr_dbd_error(dbd_config->dbd_driver, dbd_config->dbd_handle, error_num));
		return error_num;
	}
	//Cylce throgh all of them to clear cursor
	for (row_count = 0, error_num = apr_dbd_get_row(dbd_config->dbd_driver, pool, results, &row, -1);
			error_num != -1;
			row_count++, error_num = apr_dbd_get_row(dbd_config->dbd_driver, pool, results, &row, -1)) {
			//only get first result
					*file_path = apr_pstrdup(pool,apr_dbd_get_entry (dbd_config->dbd_driver,  row, 0));
		}
		if (*file_path == NULL || strcmp(*file_path, "") == 0){
			return -2;
		}
	//Return success since we have file_path
	return 0;
}


static int get_id(char** id, db_config* dbd_config,  apr_dbd_prepared_t* select, const char** args){
	int error_num = 0;
	int row_count;
	apr_dbd_results_t* results = NULL;
	apr_dbd_row_t *row = NULL;
	apr_pool_t* pool = dbd_config->pool;

	error_num = apr_dbd_pselect(dbd_config->dbd_driver, pool, dbd_config->dbd_handle,  &results, select, 0, 0, args);
	if (error_num != 0){
		add_error_list(dbd_config->database_errors, ERROR, "DBD get id error", apr_dbd_error(dbd_config->dbd_driver, dbd_config->dbd_handle, error_num));
		return error_num;
	}
	//Cylce throgh all of them to clear cursor
	for (row_count = 0, error_num = apr_dbd_get_row(dbd_config->dbd_driver, pool, results, &row, -1);
			error_num != -1;
			row_count++, error_num = apr_dbd_get_row(dbd_config->dbd_driver, pool, results, &row, -1)) {
			//only get first result
					*id = apr_pstrdup(pool,apr_dbd_get_entry (dbd_config->dbd_driver,  row, 0));
		}
		if (*id == NULL || strcmp(*id, "") == 0){
			return -2;
		}
		//Return success since we have file_path
	return 0;
}


//should be static
int get_mtime(apr_time_t* mtime, db_config* dbd_config, const char* file_path, apr_dbd_prepared_t* select){
	int error_num = 0;
	int row_count;

	apr_dbd_results_t* results = NULL;
	apr_dbd_row_t *row = NULL;

	apr_pool_t* pool = dbd_config->pool;

	if (file_path != NULL){
		error_num = apr_dbd_pvselect(dbd_config->dbd_driver, pool, dbd_config->dbd_handle,  &results, select, 0,  file_path);
		if (error_num != 0){
			add_error_list(dbd_config->database_errors, ERROR, "DBD get mtime error", apr_dbd_error(dbd_config->dbd_driver, dbd_config->dbd_handle, error_num));
			return error_num;
		}
		//Cylce throgh all of them to clear cursor
		for (row_count = 0, error_num = apr_dbd_get_row(dbd_config->dbd_driver, pool, results, &row, -1);
				error_num != -1;
				row_count++,error_num = apr_dbd_get_row(dbd_config->dbd_driver, pool, results, &row, -1)) {
						*mtime = (apr_time_t) apr_atoi64(apr_dbd_get_entry (dbd_config->dbd_driver,  row, 0));
		}
		if (*mtime == 0 || row_count == 0){
			return -2;
		}
	}else{
			add_error_list(dbd_config->database_errors, ERROR, "DBD get mtime error", "NO file path");
			return -3;
			//No file path
	}
	return 0;
}

int sync_song(db_config* dbd_config, music_file *song){
	char* artist_id = NULL;
	char* album_id = NULL;
	char* song_id = NULL;
	char* source_id = NULL;
	char* db_mtime_string = NULL;
	apr_time_t db_mtime = 0;
	int error_num = 0;
	const char* error_message = NULL;
	apr_pool_t* pool = dbd_config->pool;

	//Check if song file path already exsits in database
	error_num = get_id(&db_mtime_string,dbd_config, dbd_config->statements.select_mtime,(const char**)&(song->file.path));
	if (error_num == 0){
		db_mtime = (apr_time_t) apr_atoi64(db_mtime_string);
		if(song->file.mtime > db_mtime){
				//Update song
				error_num = update_song(dbd_config, song, song->file.mtime);
				if (error_num == 0){
					return 0;
				}else{
					//Report error
					error_message = apr_dbd_error(dbd_config->dbd_driver, dbd_config->dbd_handle, error_num);

					add_error_list(dbd_config->database_errors, ERROR, apr_psprintf(pool,"DBD update  error(%d): %s", error_num,(char *) error_message), "dsdsdds");
					return -12;
				}
			}else if(song->file.mtime == db_mtime){
				//File is up to date
				return 0;
			}
	}else if(error_num == -2){
		//No file_path found so add new Source
		{
		const char* args[4];
		//`type`, `path`, `quality`,`mtime`
		args[0] = song->file.type_string;
		args[1] = song->file.path;
		args[2] = apr_itoa(dbd_config->pool, 100);
		args[3] = apr_ltoa(dbd_config->pool,song->file.mtime);
		error_num = insert_db(&source_id, dbd_config, dbd_config->statements.add_source, args);
		if (error_num != 0){
					add_error_list(dbd_config->database_errors, ERROR, "DBD insert_source error:", apr_dbd_error(dbd_config->dbd_driver, dbd_config->dbd_handle, error_num));
					return -11;
		}
		}
		//Find Artist ID
		{
		const char* args[0];
		args[0] = song->artist ? song->artist : "Unknown";
		error_num = get_id(&artist_id, dbd_config, dbd_config->statements.select_artist, args);
		if (error_num  == -2){
			error_num = insert_db(&artist_id, dbd_config, dbd_config->statements.add_artist, args);
			if (error_num != 0){
				add_error_list(dbd_config->database_errors, ERROR,   apr_psprintf(pool,"DBD insert artist error(%d):", error_num), apr_dbd_error(dbd_config->dbd_driver, dbd_config->dbd_handle, error_num));
				return -10;
			}
		}else if(error_num != 0){
			add_error_list(dbd_config->database_errors, ERROR,  apr_psprintf(pool,"DBD get artist id error(%d):", error_num), apr_dbd_error(dbd_config->dbd_driver, dbd_config->dbd_handle, error_num));
			return -9;
		}
		}
		//Find Album ID
		{
		const char* args[0];
		args[0] = song->album ? song->album : "Unknown";
		error_num = get_id(&album_id, dbd_config, dbd_config->statements.select_album, args);
		if(error_num == -2){
			error_num=  insert_db(&album_id, dbd_config, dbd_config->statements.add_album, args);
			if (error_num != 0){
				add_error_list(dbd_config->database_errors, ERROR, "DBD insert_album error:", apr_dbd_error(dbd_config->dbd_driver, dbd_config->dbd_handle, error_num));
				return -8;
			}
		}else if (error_num != 0){
			add_error_list(dbd_config->database_errors, ERROR, "DBD album get id error:", apr_dbd_error(dbd_config->dbd_driver, dbd_config->dbd_handle, error_num));
			return -2;
		}
		}
		//Find Song ID
		{
		//Create args
		const char* args[3];
		//`artist_id, album_id, title
		args[0] = artist_id;
		args[1] = album_id;
		args[2] = song->title;

		error_num = get_id(&song_id, dbd_config, dbd_config->statements.select_song, args);
		if (error_num  == -2){
			//`name`, `musicbrainz_id`, `length`, `play_count`
			args[0] = song->title;
			args[1] = "aaaa";
			args[2] = apr_itoa(dbd_config->pool, song->length);
			args[3] = apr_itoa(dbd_config->pool, 0);
			error_num = insert_db(&song_id, dbd_config, dbd_config->statements.add_song, args);
			if (error_num != 0){
				add_error_list(dbd_config->database_errors, ERROR,   apr_psprintf(pool,"DBD insert artist error(%d):", error_num), apr_dbd_error(dbd_config->dbd_driver, dbd_config->dbd_handle, error_num));
				return -10;
			}
		}else if(error_num != 0){
			add_error_list(dbd_config->database_errors, ERROR, "DBD get song id error:", apr_dbd_error(dbd_config->dbd_driver, dbd_config->dbd_handle, error_num));
			return -11;
		}
		}

		if (artist_id != NULL && album_id != NULL && song_id != NULL && source_id != NULL){
			{
			const char* args[7];
			//artistid, albumid, songid, sourceid, feature, track_no, disc_no
			args[0] = artist_id;
			args[1] = album_id;
			args[2] = song_id;
			args[3] = source_id;
			args[4] = apr_itoa(dbd_config->pool, 0);
			args[5] = song->track_no ? song->track_no : "0";
			args[6] = song->disc_no ? song->disc_no: "0";
			error_num = insert_db(NULL, dbd_config, dbd_config->statements.add_link, args);
			if (error_num != 0){
				add_error_list(dbd_config->database_errors, ERROR, "DBD insert_link error:", apr_dbd_error(dbd_config->dbd_driver, dbd_config->dbd_handle, error_num));
				return -3;
			}
			}
		}else{
			add_error_list(dbd_config->database_errors, ERROR, "DBD link NULL error:", apr_psprintf(pool, "artist_id: %s album_id=%s song_id=%s", artist_id, album_id, song_id));
			return -4;
		}
	}else if(error_num == 2013){
		add_error_list(dbd_config->database_errors, ERROR, "DBD not connected", apr_psprintf(pool, "(%d) (%s)",error_num, song->file.path));
		dbd_config->connected = 0;
		return -5;
	}else{
		add_error_list(dbd_config->database_errors, ERROR, "DBD get mtime error", apr_psprintf(pool, "(%d) (%s)",error_num, song->file.path));
		return -6;
	}
	return 0;
}
