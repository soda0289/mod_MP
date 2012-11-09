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

int prepare_database(db_config* dbd_config){
	int error_num;
	const char* Sort_By_Table_Names[] = {"Songs.name", "Albums.name", "Artists.name"};

	error_num = apr_dbd_set_dbname(dbd_config->dbd_driver, dbd_config->pool, dbd_config->dbd_handle, "mediaplayer");
	if (error_num != 0){
		return error_num;
	}
	error_num = apr_dbd_prepare(dbd_config->dbd_driver, dbd_config->pool, dbd_config->dbd_handle, "SELECT LAST_INSERT_ID();",NULL, &(dbd_config->statements.select_last_id));
	if (error_num != 0){
		return error_num;
	}
	error_num = apr_dbd_prepare(dbd_config->dbd_driver, dbd_config->pool, dbd_config->dbd_handle, "SELECT file_path FROM Songs WHERE id=%d;",NULL, &(dbd_config->statements.select_song));
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
	error_num = apr_dbd_prepare(dbd_config->dbd_driver, dbd_config->pool, dbd_config->dbd_handle, "UPDATE Albums, Artists, Songs, links "
			"SET Albums.name = %s, Artists.name=%s, Songs.name=%s, Songs.length = %d, links.track_no = %d, links.disc_no = %d, Songs.mtime = %ld "
			"WHERE Songs.file_path = %s  AND links.artistid = Artists.id AND links.albumid = Albums.id AND links.songid = Songs.id;" ,NULL, &(dbd_config->statements.update_song));
	if (error_num != 0){
		return error_num;
	}
	error_num = apr_dbd_prepare(dbd_config->dbd_driver, dbd_config->pool, dbd_config->dbd_handle, "SELECT mtime FROM Songs WHERE file_path=%s", NULL, &(dbd_config->statements.select_file_path));
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
	error_num = apr_dbd_prepare(dbd_config->dbd_driver, dbd_config->pool, dbd_config->dbd_handle, "INSERT INTO `Songs` (`name`, `musicbrainz_id`, `file_path`, `length`,`mtime`, `play_count`) VALUE (%s, %s, %s, %d,%lld,%d);",NULL, &(dbd_config->statements.add_song));
	if (error_num != 0){
		return error_num;
	}
	error_num = apr_dbd_prepare(dbd_config->dbd_driver, dbd_config->pool, dbd_config->dbd_handle, "INSERT INTO `links` (artistid, albumid, songid, feature, track_no, disc_no) VALUES (%s, %s, %s, %s, %s, %s);",NULL, &(dbd_config->statements.add_link));
	if (error_num != 0){
		return error_num;
	}


	/*
	 * Select songs from database:
	 * Use range, sortby and possibly id
	 */
	int i;
	//Select songs with no id
	for (i = 0; i < 3; i++){
		const char* statement_string = apr_pstrcat(dbd_config->pool, "SELECT Songs.file_path, Songs.name, Artists.name, Albums.name, Songs.length, links.track_no, links.disc_no FROM links LEFT JOIN Songs ON links.songid = Songs.id LEFT JOIN Artists ON links.artistid = Artists.id LEFT JOIN Albums ON links.albumid = Albums.id ORDER BY ", Sort_By_Table_Names[i]," LIMIT %d, %d;", NULL);
		error_num = apr_dbd_prepare(dbd_config->dbd_driver, dbd_config->pool, dbd_config->dbd_handle, statement_string,NULL, &(dbd_config->statements.select_songs_range[i]));
		if (error_num != 0){
			return error_num;
		}
	}
	//Select songs with artist id
	for (i = 0; i < 3; i++){
		const char* statement_string = apr_pstrcat(dbd_config->pool, "SELECT Songs.file_path, Songs.name, Artists.name, Albums.name, Songs.length, links.track_no, links.disc_no FROM links LEFT JOIN Songs ON links.songid = Songs.id LEFT JOIN Artists ON links.artistid = Artists.id LEFT JOIN Albums ON links.albumid = Albums.id WHERE Artists.id = %d ORDER BY ", Sort_By_Table_Names[i]," LIMIT %d, %d;", NULL);
		error_num = apr_dbd_prepare(dbd_config->dbd_driver, dbd_config->pool, dbd_config->dbd_handle, statement_string,NULL, &(dbd_config->statements.select_songs_by_artist_id_range[i]));
		if (error_num != 0){
			return error_num;
		}
	}
	//Select songs with album id
	for (i = 0; i < 3; i++){
		const char* statement_string = apr_pstrcat(dbd_config->pool, "SELECT Songs.file_path, Songs.name, Artists.name, Albums.name, Songs.length, links.track_no, links.disc_no FROM links LEFT JOIN Songs ON links.songid = Songs.id LEFT JOIN Artists ON links.artistid = Artists.id LEFT JOIN Albums ON links.albumid = Albums.id WHERE Albums.id = %d ORDER BY ", Sort_By_Table_Names[i]," LIMIT %d, %d;", NULL);
		error_num = apr_dbd_prepare(dbd_config->dbd_driver, dbd_config->pool, dbd_config->dbd_handle, statement_string,NULL, &(dbd_config->statements.select_songs_by_album_id_range[i]));
		if (error_num != 0){
			return error_num;
		}
	}
	for (i = 0;i < 2; i++){
		const char* order;
		((i+1) % 2) ? (order = "ASC") : (order = "DESC");
		const char* statement_string = apr_pstrcat(dbd_config->pool, "SELECT Artists.id, Artists.name FROM Artists ORDER BY Artists.name ",order, " LIMIT %d, %d;", NULL);
		error_num = apr_dbd_prepare(dbd_config->dbd_driver, dbd_config->pool, dbd_config->dbd_handle, statement_string,NULL, &(dbd_config->statements.select_artists_range[i]));
		if (error_num != 0){
			return error_num;
		}
	}
	for (i = 0;i < 2; i++){
		const char* order;
		((i+1) % 2) ? (order = "ASC") : (order = "DESC");
		const char* statement_string = apr_pstrcat(dbd_config->pool, "SELECT Albums.id, Albums.name FROM Albums ORDER BY Albums.name ", order, " LIMIT %d, %d;", NULL);
		error_num = apr_dbd_prepare(dbd_config->dbd_driver, dbd_config->pool, dbd_config->dbd_handle, statement_string,NULL, &(dbd_config->statements.select_albums_range[i]));
		if (error_num != 0){
			return error_num;
		}
	}
	for (i = 0;i < 2; i++){
		const char* order;
		((i+1) % 2) ? (order = "ASC") : (order = "DESC");
		const char* statement_string = apr_pstrcat(dbd_config->pool, "SELECT Albums.id, Albums.name FROM Albums JOIN links ON Albums.id = links.albumid WHERE links.artistid = %d AND links.track_no = 1  ORDER BY Albums.name ",  order, " LIMIT %d, %d;", NULL);
		error_num = apr_dbd_prepare(dbd_config->dbd_driver, dbd_config->pool, dbd_config->dbd_handle, statement_string,NULL, &(dbd_config->statements.select_albums_by_artist_id_range[i]));
		if (error_num != 0){
			return error_num;
		}
	}
	return error_num;
}
static char* get_insert_last_id (db_config* dbd_config){
	int no_errors = 0;
	int row_count;
	apr_pool_t* pool = dbd_config->pool;

	char* id = NULL;
	const char* error_message = NULL;

	apr_dbd_results_t* results = NULL;
	apr_dbd_row_t *row = NULL;

	//Get id of new title
		no_errors = apr_dbd_pvselect(dbd_config->dbd_driver, pool, dbd_config->dbd_handle,  &results, dbd_config->statements.select_last_id, 0);
		if (no_errors != 0){
			error_message = apr_dbd_error(dbd_config->dbd_driver, dbd_config->dbd_handle, no_errors);
			//ap_rputs(error_message,r);
		}
		//If it does cylce throgh all of them to clear cursor
		for (row_count = 0, no_errors = apr_dbd_get_row(dbd_config->dbd_driver, pool, results, &row, -1);
				no_errors != -1;
				row_count++,no_errors = apr_dbd_get_row(dbd_config->dbd_driver, pool, results, &row, -1)) {
				//only get first result
				if(row_count == 0){
					id = apr_pstrdup(pool,apr_dbd_get_entry (dbd_config->dbd_driver,  row, 0 ));
				}
		}
		return id;
}

const char* get_full_column_name(db_config* dbd_config, music_query* query, apr_dbd_results_t* results, int col_num){
	const char* mysql_name;
	const char* song_col_name[] = {"file_path", "title", "Artist", "Album", "length","track_no", "disc_no"};
	mysql_name = apr_dbd_get_name(dbd_config->dbd_driver,results, col_num);
	if (query->types == SONGS){
		return song_col_name[col_num];
	}else{
		return mysql_name;
	}

}

int select_db_range(db_config* dbd_config, music_query* query){
	int error_num;

	apr_dbd_results_t* results = NULL;
	apr_dbd_row_t *row = NULL;

	query->results = apr_pcalloc(dbd_config->pool, sizeof(results_table_t));

	//Check if id is set. If it is send it with select query
	if (query->by_id.id_type > 0){
		error_num = apr_dbd_pvselect(dbd_config->dbd_driver, dbd_config->pool, dbd_config->dbd_handle,  &results,query->statement, 0, query->by_id.id, query->range_lower, query->range_upper);
	}else{
		error_num = apr_dbd_pvselect(dbd_config->dbd_driver, dbd_config->pool, dbd_config->dbd_handle,  &results,query->statement, 0, query->range_lower, query->range_upper);
	}
	if (error_num != 0){
		//check if not connected
		if (error_num == 2013){
			dbd_config->connected = 0;
		}
		return error_num;
	}
	query->results->results = apr_table_make(dbd_config->pool, 6);
	//Cycle through all of them to clear cursor
	for (query->results->row_count = 0, error_num = apr_dbd_get_row(dbd_config->dbd_driver, dbd_config->pool, results, &row, -1);
			error_num != -1;
			(query->results->row_count)++,error_num = apr_dbd_get_row(dbd_config->dbd_driver, dbd_config->pool, results, &row, -1)) {
			//only get first result
					const char* key =  apr_itoa(dbd_config->pool, query->results->row_count);
					int i;
					for(i = 0; i < apr_dbd_num_cols(dbd_config->dbd_driver, results); i++){
						//Since three tables all use the same name we must substitue for what it actually is

						const char* value = apr_psprintf(dbd_config->pool, "\"%s\": \"%s\"", get_full_column_name(dbd_config,query,results, i), json_escape_char(dbd_config->pool, apr_dbd_get_entry (dbd_config->dbd_driver,  row, i)));
						apr_table_merge(query->results->results, key, value);
					}
					//return error_num;
		}

	return 0;
}

static int insert_db_artist(char** id, db_config* dbd_config, apr_dbd_prepared_t* query, music_file* song){
	int error_num = 0;
	int nrows = 1;
	apr_pool_t* pool = dbd_config->pool;

	//Create new title
	error_num = apr_dbd_pvquery(dbd_config->dbd_driver, pool, dbd_config->dbd_handle, &nrows, query, song->artist);
	if (error_num != 0){
		*id = get_insert_last_id(dbd_config);
	}
	return error_num;
}

static int insert_db_album(char** id, db_config* dbd_config, apr_dbd_prepared_t* query, music_file* song){
	int error_num = 0;
	int nrows = 1;
	apr_pool_t* pool = dbd_config->pool;

	//Create new title
	error_num = apr_dbd_pvquery(dbd_config->dbd_driver, pool, dbd_config->dbd_handle, &nrows, query, song->album);
	if (error_num != 0){
		*id = get_insert_last_id(dbd_config);
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
		*id = get_insert_last_id(dbd_config);
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
	return error_num;
}

static int update_song(db_config* dbd_config, music_file* song, apr_time_t mtime){
	int error_num = 0;
	int nrows = 0;
	int feature = 0;
	apr_pool_t* pool = dbd_config->pool;

	error_num = apr_dbd_pvquery(dbd_config->dbd_driver, pool, dbd_config->dbd_handle, &nrows, dbd_config->statements.update_song,song->album,song->title, apr_itoa(pool,  song->length), pool, song->track_no, song->disc_no, apr_ltoa(pool,(long) mtime));
					if (error_num != 0){
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

	//*id = apr_pcalloc(pool, 255);
		error_num = apr_dbd_pvselect(dbd_config->dbd_driver, pool, dbd_config->dbd_handle,  &results, select, 0, id);
		if (error_num != 0){
			return error_num;
		}
		//Cylce throgh all of them to clear cursor
		for (row_count = 0, error_num = apr_dbd_get_row(dbd_config->dbd_driver, pool, results, &row, -1);
				error_num != -1;
				row_count++, error_num = apr_dbd_get_row(dbd_config->dbd_driver, pool, results, &row, -1)) {
				//only get first result
						*file_path = apr_pstrdup(pool,apr_dbd_get_entry (dbd_config->dbd_driver,  row, 0));
						return error_num ;
			}
			if (*file_path == NULL || strcmp(*file_path, "") == 0){
				return -2;
			}
	return error_num;
}


static int get_id(char** id, db_config* dbd_config, char* title, apr_dbd_prepared_t* select){
	int error_num = 0;
	int row_count;
	apr_dbd_results_t* results = NULL;
	apr_dbd_row_t *row = NULL;
	apr_pool_t* pool = dbd_config->pool;

	//*id = apr_pcalloc(pool, 255);
		error_num = apr_dbd_pvselect(dbd_config->dbd_driver, pool, dbd_config->dbd_handle,  &results, select, 0, title);
		if (error_num != 0){
			return error_num;
		}
		//Cylce throgh all of them to clear cursor
		for (row_count = 0, error_num = apr_dbd_get_row(dbd_config->dbd_driver, pool, results, &row, -1);
				error_num != -1;
				row_count++, error_num = apr_dbd_get_row(dbd_config->dbd_driver, pool, results, &row, -1)) {
				//only get first result
						*id = apr_pstrdup(pool,apr_dbd_get_entry (dbd_config->dbd_driver,  row, 0));
						return error_num ;
			}
			if (*id == NULL || strcmp(*id, "") == 0){
				return -2;
			}
	return error_num;
}

static int get_mtime(apr_time_t* mtime, db_config* dbd_config, char* file_path, apr_dbd_prepared_t* select){
	int error_num = 0;
	int row_count;

	apr_dbd_results_t* results = NULL;
	apr_dbd_row_t *row = NULL;

	apr_pool_t* pool = dbd_config->pool;

	if (file_path != NULL){

		error_num = apr_dbd_pvselect(dbd_config->dbd_driver, pool, dbd_config->dbd_handle,  &results, select, 0,  apr_dbd_escape(dbd_config->dbd_driver,dbd_config->pool, file_path, dbd_config->dbd_handle));
		if (error_num != 0){
			return error_num;
		}
		//Cylce throgh all of them to clear cursor
		for (row_count = 0, error_num = apr_dbd_get_row(dbd_config->dbd_driver, pool, results, &row, -1);
				error_num != -1;
				row_count++,error_num = apr_dbd_get_row(dbd_config->dbd_driver, pool, results, &row, -1)) {
				//only get first result
						*mtime = (apr_time_t) apr_atoi64(apr_dbd_get_entry (dbd_config->dbd_driver,  row, 0));
						return error_num;
			}
			if (*mtime == 0 || row_count == 0){
				return -2;
			}
		}else{
			return -3;
		}
	return 0;
}

int sync_song(db_config* dbd_config, music_file *song, apr_time_t file_mtime, error_messages_t* error_messages){
	char* artist_id = NULL;
	char* album_id = NULL;
	char* song_id = NULL;
	apr_time_t db_mtime = 0;
	int error_num = 0;
	const char* error_message = NULL;
	apr_pool_t* pool = dbd_config->pool;

	//Check if song file path already exsits in database
	error_num = get_mtime(&db_mtime,dbd_config, song->file_path, dbd_config->statements.select_file_path);
	if (error_num == 0){
		if(file_mtime > db_mtime){
				//Update song
				error_num = update_song(dbd_config, song, file_mtime);
				if (error_num == 0){
					return 0;
				}else{
					//Report error
					error_message = apr_dbd_error(dbd_config->dbd_driver, dbd_config->dbd_handle, error_num);

					add_error_list(error_messages, apr_psprintf(pool,"DBD update  error(%d): %s", error_num,(char *) error_message), "dsdsdds");
					return 1;
				}
			}else if(file_mtime == db_mtime){
				//File is up to date
				return 0;
			}
	}else if(error_num == -2){
		//No file_path found so add new song
		error_num = insert_db_song(&song_id, dbd_config, dbd_config->statements.add_song, song, file_mtime);
		if (error_num != 0){
			add_error_list(error_messages, "DBD insert_song error:", apr_dbd_error(dbd_config->dbd_driver, dbd_config->dbd_handle, error_num));
			return -1;
		}
		error_num = get_id(&artist_id, dbd_config, song->artist, dbd_config->statements.select_artist);
		if (error_num  == -2){
			error_num = insert_db_artist(&artist_id, dbd_config, dbd_config->statements.add_artist, song);
			if (error_num != 0){
				add_error_list(error_messages,   apr_psprintf(pool,"DBD insert artist error(%d):", error_num), apr_dbd_error(dbd_config->dbd_driver, dbd_config->dbd_handle, error_num));
				return -1;
			}
		}else if(error_num != 0){
			add_error_list(error_messages,  apr_psprintf(pool,"DBD get artist id error(%d):", error_num), apr_dbd_error(dbd_config->dbd_driver, dbd_config->dbd_handle, error_num));
			return -1;
		}
		error_num = get_id(&album_id, dbd_config, song->album, dbd_config->statements.select_album);
		if(error_num == -2){
			error_num=  insert_db_album(&album_id, dbd_config, dbd_config->statements.add_album, song);
			if (error_num != 0){
				add_error_list(error_messages, "DBD insert_album error:", apr_dbd_error(dbd_config->dbd_driver, dbd_config->dbd_handle, error_num));
				return -1;
			}
		}else if (error_num != 0){
			add_error_list(error_messages, "DBD album get id error:", apr_dbd_error(dbd_config->dbd_driver, dbd_config->dbd_handle, error_num));
			return -1;
		}

			if (artist_id != NULL && album_id != NULL && song_id != NULL){
				error_num = insert_db_links(dbd_config, dbd_config->statements.add_link, song, artist_id, album_id, song_id);
				if (error_num != 0){
					//error_message = apr_dbd_error(dbd_config->dbd_driver, dbd_config->dbd_handle, error_num);
					add_error_list(error_messages, "DBD insert_song error:", apr_dbd_error(dbd_config->dbd_driver, dbd_config->dbd_handle, error_num));
					return -1;
				}
			}else{
				//add_error_list(error_messages, "DBD link error:", apr_psprintf(pool, "artist_id: %s album_id=%s song_id=%s", artist_id, album_id, song_id));
				return -1;
			}
	}else if(error_num == 2013){
		add_error_list(error_messages, "DBD not connected", apr_psprintf(pool, "(%d) (%s)",error_num, song->file_path));
		dbd_config->connected = 0;
		return -1;
	}else{
		add_error_list(error_messages, "DBD get mtime error", apr_psprintf(pool, "(%d) (%s)",error_num, song->file_path));
		return -1;
	}
	return 0;
}

