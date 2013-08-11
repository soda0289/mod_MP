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
#include "database/db_query_config.h"
#include "database/dbd.h"
#include "database/db_query_parameters.h"
#include "database/db_typedef.h"
#include "database/db_config.h"


//Setup db config by creating a pool, the database connection with APR,
// a lock for threads to use and  error_messages
int setup_db_config(apr_pool_t* parent_pool,db_config_t* db_config){
	apr_status_t rv;
	const char* mysql_params;
	//Create pool
	db_config->parent_pool = parent_pool;
	rv = apr_pool_create_ex(&(db_config->pool), parent_pool, NULL, NULL);
	if (rv != APR_SUCCESS){
		db_config->pool = NULL;
		//Run error function
		return rv;
	}

	//Read db params and create mysql params
	mysql_params = apr_pstrcat(parent_pool,"host=", db_config->db_params->hostname,",user=",db_config->db_params->username, NULL);
	//Use apr dbd to do all the hard work
	//Find driver and connect to database
	rv = apr_dbd_get_driver(db_config->pool, db_config->db_params->driver_name, &(db_config->dbd_driver));
	if (rv != APR_SUCCESS){
		//Run error function
		return rv;
	}

	rv = apr_dbd_open(db_config->dbd_driver, db_config->pool, mysql_params, &(db_config->dbd_handle));
	if (rv != APR_SUCCESS){
		//Run error function
		return rv;
	}

	//Create error messages
	db_config->database_errors = apr_pcalloc(db_config->pool, sizeof(error_messages_t));
	db_config->database_errors->num_errors = 0;

	return 0;
}


//Create database struct using the global pool
//and set up database parameters, apr dbd, and error messages
int connect_database(apr_pool_t* global_pool, db_params_t* db_params){
	apr_status_t rv;
	db_config_t* db_config;
	
	db_config = apr_pcalloc(global_pool, sizeof(db_config_t));

	//Wow is this loop really needed
	db_config->db_params = db_params;
	db_params->db_config = db_config;

	//Create lock for muli threading
	rv = apr_thread_mutex_create(&(db_config->mutex),APR_THREAD_MUTEX_DEFAULT,global_pool);
	if(rv != APR_SUCCESS){
		return rv;
	}

	//Setup the database connection including thread lock and memory pool
	rv = setup_db_config(global_pool, db_config);
	if(rv == APR_SUCCESS){
		//All is good
		db_config->connected = 1;
	}

	return rv;
}

void close_database(db_config_t* db_config){
	//Clear Prepared Statments
	memset(&(db_config->statements),0, sizeof(struct statments_) );

	apr_dbd_close(db_config->dbd_driver,db_config->dbd_handle);
	db_config->dbd_driver = NULL;
	db_config->dbd_handle = NULL;

	//Clear pool
	if(db_config->pool != NULL){
		apr_pool_destroy(db_config->pool);
		db_config->pool = NULL;
	}
}

int reconnect_database(db_config_t* db_config){
	int error_num;


	close_database(db_config);

	error_num = setup_db_config(db_config->parent_pool, db_config);
	if(error_num == 0){
		db_config->connected = 1;
		error_num = prepare_database(db_config);
	}


	return error_num;
}

int prepare_database(db_config_t* db_config){
	int error_num = 0;

	//Set Databse name
	error_num = apr_dbd_set_dbname(db_config->dbd_driver, db_config->pool, db_config->dbd_handle, "mediaplayer");
	if (error_num != 0){
		return error_num;
	}

	//SOON THIS WILL ALL BE DELETED. WE WILL GENERATE THIS AUTOMATICALLY FROM XML EVENTUALLY
	error_num = apr_dbd_prepare(db_config->dbd_driver, db_config->pool, db_config->dbd_handle, "SELECT LAST_INSERT_ID();",NULL, &(db_config->statements.select_last_id));
	if (error_num != 0){
		return error_num;
	}
	error_num = apr_dbd_prepare(db_config->dbd_driver, db_config->pool, db_config->dbd_handle, "SELECT links.songid FROM links LEFT JOIN Songs ON Songs.id = links.songid  WHERE links.artistid = %d AND links.albumid = %d AND Songs.name = %s;",NULL, &(db_config->statements.select_song));
	if (error_num != 0){
		return error_num;
	}
	error_num = apr_dbd_prepare(db_config->dbd_driver, db_config->pool, db_config->dbd_handle, "SELECT Sources.id FROM Sources LEFT JOIN links ON links.sourceid = Sources.id  WHERE links.songid = %d;",NULL, &(db_config->statements.select_sources));
	if (error_num != 0){
		return error_num;
	}
	error_num = apr_dbd_prepare(db_config->dbd_driver, db_config->pool, db_config->dbd_handle, "SELECT path FROM Sources  WHERE id = %d;",NULL, &(db_config->statements.select_file_path));
	if (error_num != 0){
		return error_num;
	}
	error_num = apr_dbd_prepare(db_config->dbd_driver, db_config->pool, db_config->dbd_handle, "SELECT id FROM Artists WHERE name = %s;",NULL, &(db_config->statements.select_artist));
	if (error_num != 0){
		return error_num;
	}
	error_num = apr_dbd_prepare(db_config->dbd_driver, db_config->pool, db_config->dbd_handle, "SELECT id FROM Albums WHERE name = %s;",NULL, &(db_config->statements.select_album));
	if (error_num != 0){
		return error_num;
	}
	error_num = apr_dbd_prepare(db_config->dbd_driver, db_config->pool, db_config->dbd_handle, "UPDATE Albums, Artists, Songs, Sources, links "
			"SET Albums.name = %s, Artists.name=%s, Songs.name=%s, Songs.length = %d, links.track_no = %d, links.disc_no = %d, Sources.mtime = %ld "
			"WHERE Sources.path = %s  AND links.artistid = Artists.id AND links.albumid = Albums.id AND links.songid = Songs.id;" ,NULL, &(db_config->statements.update_song));
	if (error_num != 0){
		return error_num;
	}
	error_num = apr_dbd_prepare(db_config->dbd_driver, db_config->pool, db_config->dbd_handle, "SELECT mtime FROM Sources WHERE path = %s;", NULL, &(db_config->statements.select_mtime));
	if (error_num != 0){
		return error_num;
	}
	error_num = apr_dbd_prepare(db_config->dbd_driver, db_config->pool, db_config->dbd_handle, "INSERT INTO `Artists` (name) VALUE (%s)",NULL, &(db_config->statements.add_artist));
	if (error_num != 0){
		return error_num;
	}
	error_num = apr_dbd_prepare(db_config->dbd_driver, db_config->pool, db_config->dbd_handle, "INSERT INTO `Albums` (name) VALUE (%s);",NULL, &(db_config->statements.add_album));
	if (error_num != 0){
		return error_num;
	}
	error_num = apr_dbd_prepare(db_config->dbd_driver, db_config->pool, db_config->dbd_handle, "INSERT INTO `Songs` (`name`, `musicbrainz_id`, `length`, `play_count`) VALUE (%s, %s, %d,%d);",NULL, &(db_config->statements.add_song));
	if (error_num != 0){
		return error_num;
	}
	error_num = apr_dbd_prepare(db_config->dbd_driver, db_config->pool, db_config->dbd_handle, "INSERT INTO `Sources` (`type`, `path`, `quality`,`mtime`) VALUE (%s, %s, %d,%lld);",NULL, &(db_config->statements.add_source));
	if (error_num != 0){
		return error_num;
	}
	error_num = apr_dbd_prepare(db_config->dbd_driver, db_config->pool, db_config->dbd_handle, "INSERT INTO `links` (artistid, albumid, songid, sourceid, feature, track_no, disc_no) VALUES (%d, %d, %d, %d, %d, %d, %d);",NULL, &(db_config->statements.add_link));
	if (error_num != 0){
		return error_num;
	}
	return error_num;
}

static int get_insert_last_id (char** id, db_config_t* db_config){
	int error_num = 0;
	int row_count;
	apr_pool_t* pool = db_config->pool;

	apr_dbd_results_t* results = NULL;
	apr_dbd_row_t *row = NULL;

	//Get id of new title
	error_num = apr_dbd_pvselect(db_config->dbd_driver, pool, db_config->dbd_handle,  &results, db_config->statements.select_last_id, 0);
	if (error_num != 0){
		add_error_list(db_config->database_errors, ERROR, "DBD insert_db_song error", apr_dbd_error(db_config->dbd_driver, db_config->dbd_handle, error_num));
		return error_num;
	}
	//If it does cylce throgh all of them to clear cursor
	for (row_count = 0, error_num = apr_dbd_get_row(db_config->dbd_driver, pool, results, &row, -1);
			error_num != -1;
			row_count++,error_num = apr_dbd_get_row(db_config->dbd_driver, pool, results, &row, -1)) {
			//only get first result
				*id = apr_pstrdup(pool,apr_dbd_get_entry (db_config->dbd_driver,  row, 0 ));
	}
	if (*id == NULL || strcmp(*id, "") == 0){
		return -2;
	}

		return 0;
}



static const char* operator_to_string(condition_operator operator){
	switch(operator){
		case (LESS):{
			return "<";
			break; //WHY
		}
		case (GREATER):{
			return ">";
			break; //WHY
		}
		case (EQUAL):{
			return "=";
			break; //WHY
		}
		case (LIKE):{
			return "LIKE";
			break; //WHY
		}
		case (BETWEEN):{
			return "BETWEEN";
			break;
		}
		case (IN):{
			return "IN";
			break;
		}
	}
	return NULL;
}

static int generate_sql_statement(db_config_t* db_config, query_parameters_t* query_parameters,db_query_t* db_query,const char** select,error_messages_t* error_messages){
	//int num_tables;
	apr_status_t status;

	apr_pool_t* statement_pool;

	//int query_par;
	//int num_query_parameters = __builtin_popcount(query->query_parameters_set);
	const char* select_columns = (db_query->select_columns_string) ? db_query->select_columns_string : "*";
	const char* tables = db_query->table_join_string;
	const char* order_by = NULL;
	const char* limit = NULL;
	const char* offset = NULL;
	const char* group_by = db_query->group_by_string;
	char* where = NULL;
	char* select_statement = NULL;

	int i;
	status = apr_pool_create(&statement_pool,db_config->pool);
	if (status != APR_SUCCESS){
		return -11;
	}

	//Start SQL Select Statement
	if(select_columns){
		select_statement = apr_psprintf(statement_pool,"SELECT %s",select_columns);
	}

	//Add SQL query from tables
	if(tables) {
		select_statement = apr_pstrcat(statement_pool, select_statement," FROM ", tables, NULL);
	}

	//Add SQL query parameters
	//WHERE, LIMIT, GROUP BY, ...
	if(query_parameters != NULL){
		//Add SQL query where parameters
		if(query_parameters->query_where_conditions != NULL){
			//Add together where conditions
			for(i = 0;i <query_parameters->query_where_conditions->nelts; i++){
				query_where_condition_t* query_where_condition = &(((query_where_condition_t*)query_parameters->query_where_conditions->elts)[i]);
				column_table_t* column = query_where_condition->column;
				char* where_condition = apr_pstrcat(statement_pool,column->table->name,".",column->name," ",operator_to_string(query_where_condition->operator)," ",query_where_condition->condition,NULL);
				where = (where) ? apr_pstrcat(statement_pool, where, " AND ", where_condition,NULL) : where_condition;
			}

			if(where){
				select_statement = apr_pstrcat(statement_pool,select_statement," WHERE ",where,NULL);
			}
		}
	}

	if (group_by){
		select_statement = apr_pstrcat(statement_pool,select_statement," GROUP BY ", group_by , NULL);
	}

	if(query_parameters != NULL){
		//Add SQL query parameters
		//GROUP BY, ORDER BY, LIMIT
		if(query_parameters->query_sql_clauses != NULL){
			order_by = query_parameters->query_sql_clauses[ORDER_BY].value;
			limit = query_parameters->query_sql_clauses[LIMIT].value;
			offset = query_parameters->query_sql_clauses[OFFSET].value;

			if (order_by){
				select_statement = apr_pstrcat(db_config->pool,select_statement," ORDER BY ",  order_by, NULL);
			}

			//LIMIT must come after order by
			if(limit){
				select_statement = apr_pstrcat(db_config->pool,select_statement," LIMIT ", limit,NULL);
				if(offset){
					select_statement = apr_pstrcat(db_config->pool,select_statement," OFFSET ",offset, NULL);
				}
			}
		}
	}


	if(select_statement){
		*select = apr_pstrcat(db_config->pool,select_statement,";",NULL);
		apr_pool_destroy(statement_pool);
	//	add_error_list(error_messages, WARN,"SELECT statment", *select);
	}else{
		return -1;
	}



	return 0;
}

int get_column_results_for_row(db_query_t* db_query, results_table_t* query_results,column_table_t* column,int row_index,const char** column_result){
	int i;
	column_table_t* select_column;
	if(query_results->rows == NULL || query_results->rows->nelts == 0){
		return -2;
	}
	for(i = 0;i < db_query->select_columns->nelts; i++){
		select_column = ((column_table_t**)db_query->select_columns->elts)[i];
		if(column == select_column){
			//We have out index (i);
			*column_result = ((row_t*)query_results->rows->elts)[row_index].results[i];
			return 0;
		}
	}

	return -1;
}

//Run the database query and create results table if necesary
int run_query(db_query_t* db_query,query_parameters_t* query_parameters,results_table_t** query_results){
	apr_pool_t* pool = db_query->db_params->db_config->pool;
	error_messages_t* error_messages = db_query->db_params->db_config->database_errors;
	db_config_t* db_config = db_query->db_params->db_config;

	int error_num;

	apr_dbd_results_t* results = NULL;
	apr_dbd_row_t *row = NULL;
	const char* select = NULL;

	apr_thread_mutex_lock(db_config->mutex);
	(*query_results) = apr_pcalloc(pool, sizeof(results_table_t));

	error_num = generate_sql_statement(db_config,query_parameters,db_query,&select,error_messages);
	if(error_num != 0){
		add_error_list(error_messages, ERROR, "DBD generate_sql_statement error", apr_dbd_error(db_config->dbd_driver, db_config->dbd_handle, error_num));
		apr_thread_mutex_unlock(db_config->mutex);
		return error_num;
	}

	error_num = apr_dbd_select(db_config->dbd_driver,pool,db_config->dbd_handle,&results,select,0);
	if (error_num != 0){
		//check if not connected
		if (error_num == 2013){
			db_config->connected = 0;
		}
		add_error_list(error_messages, ERROR, "DBD select_db_range error", apr_dbd_error(db_config->dbd_driver, db_config->dbd_handle, error_num));
		apr_thread_mutex_unlock(db_config->mutex);
		return error_num;
	}
	(*query_results)->rows = apr_array_make(pool,1000,sizeof(row_t));

	//Cycle through all of them to clear cursor
	for (error_num = apr_dbd_get_row(db_config->dbd_driver, pool, results, &row, -1); //FOR
	error_num != -1; 																  //LOOP
	error_num = apr_dbd_get_row(db_config->dbd_driver, pool, results, &row, -1)) {    //FOR

		row_t* res_row = (row_t*)apr_array_push((*query_results)->rows);
		int i;
		int num_columns = apr_dbd_num_cols(db_config->dbd_driver, results);

		res_row->results = apr_pcalloc(pool,sizeof(char*) * num_columns);

		for(i = 0; i < num_columns; i++){
			//We must copy the row entry as it does not stay in memory for long
			res_row->results[i] = apr_pstrdup(pool,apr_dbd_get_entry (db_config->dbd_driver,  row, i));
		}

	}
	apr_thread_mutex_unlock(db_config->mutex);
	return 0;
}

int insert_db(char** id, db_config_t* db_config, apr_dbd_prepared_t* query, const char** args){
	static int tries = 0;
	int error_num = 0;
	int nrows = 1;
	apr_pool_t* pool = db_config->pool;

	//Create new title
	error_num = apr_dbd_pquery(db_config->dbd_driver, pool, db_config->dbd_handle, &nrows, query, 0,args);

	if (id != NULL && error_num == 0){
		error_num = get_insert_last_id(id, db_config);
	}else if(error_num == 2013){
		//Database disconnected
		//reconnect
		error_num = reconnect_database(db_config);
		if(error_num == 0 && tries == 0){
			//recurse try again only once
			tries++;
			return insert_db(id, db_config, query, args);
		}
	}
	return error_num;
}

static int update_song(db_config_t* db_config, music_file* song, apr_time_t mtime){
	int error_num = 0;
	int nrows = 0;

	apr_pool_t* pool = db_config->pool;

	error_num = apr_dbd_pvquery(db_config->dbd_driver, pool, db_config->dbd_handle, &nrows, db_config->statements.update_song,song->album,song->title, apr_itoa(pool,  song->length), pool, song->track_no, song->disc_no, apr_ltoa(pool,(long) mtime));
	if (error_num != 0){
		add_error_list(db_config->database_errors, ERROR, "DBD update_song error", apr_dbd_error(db_config->dbd_driver, db_config->dbd_handle, error_num));
		return error_num;
	}
	return 0;
}

int get_file_path(char** file_path, db_config_t* db_config, char* id, apr_dbd_prepared_t* select){
	int error_num = 0;
	int row_count;
	apr_dbd_results_t* results = NULL;
	apr_dbd_row_t *row = NULL;
	apr_pool_t* pool = db_config->pool;

	error_num = apr_dbd_pvselect(db_config->dbd_driver, pool, db_config->dbd_handle,  &results, select, 0, id);
	if (error_num != 0){
		add_error_list(db_config->database_errors, ERROR, "DBD get file_path error", apr_dbd_error(db_config->dbd_driver, db_config->dbd_handle, error_num));
		return error_num;
	}
	//Cylce throgh all of them to clear cursor
	for (row_count = 0, error_num = apr_dbd_get_row(db_config->dbd_driver, pool, results, &row, -1);
			error_num != -1;
			row_count++, error_num = apr_dbd_get_row(db_config->dbd_driver, pool, results, &row, -1)) {
			//only get first result
					*file_path = apr_pstrdup(pool,apr_dbd_get_entry (db_config->dbd_driver,  row, 0));
		}
		if (*file_path == NULL || strcmp(*file_path, "") == 0){
			return -2;
		}
	//Return success since we have file_path
	return 0;
}


static int get_id(char** id, db_config_t* db_config,apr_dbd_prepared_t* prep, const char** args){
	int error_num = 0;
	apr_dbd_results_t* results = NULL;
	apr_dbd_row_t* row = NULL;
	apr_pool_t* pool = db_config->pool;

	error_num = apr_dbd_pselect(db_config->dbd_driver, pool, db_config->dbd_handle,  &results, prep, 0, 0, args);
	if (error_num != 0){
		add_error_list(db_config->database_errors, ERROR, "DBD get id error ", apr_dbd_error(db_config->dbd_driver, db_config->dbd_handle, error_num));
		return error_num;
	}
	//Cylce throgh all of them to clear cursor
	for (error_num = apr_dbd_get_row(db_config->dbd_driver, pool, results, &row, -1);
		error_num != -1;
		error_num = apr_dbd_get_row(db_config->dbd_driver, pool, results, &row, -1)) {
					*id = apr_pstrdup(pool,apr_dbd_get_entry (db_config->dbd_driver,  row, 0));
	}
	if (*id == NULL || strcmp(*id, "") == 0){
		return -2;
	}
		//Return success since we have file_path
	return 0;
}




int sync_song(apr_pool_t* pool, db_config_t* db_config, music_file *song){
	char* artist_id = NULL;
	char* album_id = NULL;
	char* song_id = NULL;
	char* source_id = NULL;
	char* db_mtime_string = NULL;
	apr_time_t db_mtime = 0;
	int error_num = 0;
	const char* error_message = NULL;


	//LOCK
	apr_thread_mutex_lock(db_config->mutex);

	//Check if song file path already exists in database
	{
	const char* args[1];
	args[0] = song->file->path;
	error_num = get_id(&db_mtime_string,db_config, db_config->statements.select_mtime,args);
	}
	if (error_num == 0){
		db_mtime = (apr_time_t) apr_atoi64(db_mtime_string);
		if(song->file->mtime > db_mtime){
				//Update song
				error_num = update_song(db_config, song, song->file->mtime);
				if (error_num == 0){
					apr_thread_mutex_unlock(db_config->mutex);return 0;
				}else{
					//Report error
					error_message = apr_dbd_error(db_config->dbd_driver, db_config->dbd_handle, error_num);

					add_error_list(db_config->database_errors, ERROR, apr_psprintf(pool,"DBD update  error(%d): %s", error_num,(char *) error_message), "dsdsdds");
					apr_thread_mutex_unlock(db_config->mutex);return -12;
				}
			}else if(song->file->mtime == db_mtime){
				//File is up to date
				apr_thread_mutex_unlock(db_config->mutex);return 0;
			}
	}else if(error_num == -2){
		//No file_path found so add new Source
		{
			const char* args[4];
			//`type`, `path`, `quality`,`mtime`
			args[0] = song->file->type_string;
			args[1] = song->file->path;
			args[2] = apr_itoa(pool, 100);
			args[3] = apr_ltoa(pool,song->file->mtime);
			error_num = insert_db(&source_id, db_config, db_config->statements.add_source, args);
			if (error_num != 0){
						add_error_list(db_config->database_errors, ERROR, "DBD insert_source error:", apr_dbd_error(db_config->dbd_driver, db_config->dbd_handle, error_num));
						apr_thread_mutex_unlock(db_config->mutex);return -11;
			}
		}
		//Find Artist ID
		{
			const char* args[1];
			args[0] = song->artist ? song->artist : "Unknown";
			error_num = get_id(&artist_id, db_config, db_config->statements.select_artist, args);
			if (error_num  == -2){
				error_num = insert_db(&artist_id, db_config, db_config->statements.add_artist, args);
				if (error_num != 0){
					add_error_list(db_config->database_errors, ERROR,   apr_psprintf(pool,"DBD insert artist error(%d):", error_num), apr_dbd_error(db_config->dbd_driver, db_config->dbd_handle, error_num));
					apr_thread_mutex_unlock(db_config->mutex);return -10;
				}
			}else if(error_num != 0){
				add_error_list(db_config->database_errors, ERROR,  apr_psprintf(pool,"DBD get artist id error(%d):", error_num), apr_dbd_error(db_config->dbd_driver, db_config->dbd_handle, error_num));
				apr_thread_mutex_unlock(db_config->mutex);return -9;
			}
		}
		//Find Album ID
		{
			const char* args[1];
			args[0] = song->album ? song->album : "Unknown";
			error_num = get_id(&album_id, db_config, db_config->statements.select_album, args);
			if(error_num == -2){
				error_num=  insert_db(&album_id, db_config, db_config->statements.add_album, args);
				if (error_num != 0){
					add_error_list(db_config->database_errors, ERROR, "DBD insert_album error:", apr_dbd_error(db_config->dbd_driver, db_config->dbd_handle, error_num));
					apr_thread_mutex_unlock(db_config->mutex);return -8;
				}
			}else if (error_num != 0){
				add_error_list(db_config->database_errors, ERROR, "DBD album get id error:", apr_dbd_error(db_config->dbd_driver, db_config->dbd_handle, error_num));
				apr_thread_mutex_unlock(db_config->mutex);return -2;
			}
		}
		//Find Song ID
		{
			//Create args
			const char* args[4];
			//`artist_id, album_id, title
			args[0] = artist_id;
			args[1] = album_id;
			args[2] = song->title;

			error_num = get_id(&song_id, db_config, db_config->statements.select_song, args);
			if (error_num  == -2){
				//`name`, `musicbrainz_id`, `length`, `play_count`
				args[0] = song->title;
				args[1] = song->mb_id.mb_release_id;
				args[2] = apr_itoa(pool, song->length);
				args[3] = apr_itoa(pool, 0);
				error_num = insert_db(&song_id, db_config, db_config->statements.add_song, args);
				if (error_num != 0){
					add_error_list(db_config->database_errors, ERROR,   apr_psprintf(pool,"DBD insert artist error(%d):", error_num), apr_dbd_error(db_config->dbd_driver, db_config->dbd_handle, error_num));
					apr_thread_mutex_unlock(db_config->mutex);return -10;
				}
			}else if(error_num != 0){
				add_error_list(db_config->database_errors, ERROR, "DBD get song id error:", apr_dbd_error(db_config->dbd_driver, db_config->dbd_handle, error_num));
				apr_thread_mutex_unlock(db_config->mutex);return -11;
		}
		}

		if (artist_id != NULL && album_id != NULL && song_id != NULL && source_id != NULL){
			//Add link
			{
				const char* args[7];
				//artistid, albumid, songid, sourceid, feature, track_no, disc_no
				args[0] = artist_id;
				args[1] = album_id;
				args[2] = song_id;
				args[3] = source_id;
				args[4] = apr_itoa(pool, 0);
				args[5] = song->track_no ? song->track_no : "0";
				args[6] = song->disc_no ? song->disc_no: "0";
				error_num = insert_db(NULL, db_config, db_config->statements.add_link, args);
				if (error_num != 0){
					add_error_list(db_config->database_errors, ERROR, "DBD insert_link error:", apr_dbd_error(db_config->dbd_driver, db_config->dbd_handle, error_num));
					apr_thread_mutex_unlock(db_config->mutex);return -3;
				}
			}
		}else{
			add_error_list(db_config->database_errors, ERROR, "DBD link NULL error:", apr_psprintf(pool, "artist_id: %s album_id=%s song_id=%s", artist_id, album_id, song_id));
			apr_thread_mutex_unlock(db_config->mutex);return -4;
		}
	}else if(error_num == 2013){
		add_error_list(db_config->database_errors, ERROR, "DBD not connected", apr_psprintf(pool, "(%d) (%s)",error_num, song->file->path));
		db_config->connected = 0;
		apr_thread_mutex_unlock(db_config->mutex); return -5;
	}else{
		add_error_list(db_config->database_errors, ERROR, "DBD get mtime error", apr_psprintf(pool, "(%d) (%s)",error_num, song->file->path));
		apr_thread_mutex_unlock(db_config->mutex); return -6;
	}
	apr_thread_mutex_unlock(db_config->mutex);
	return 0;
}

int output_db_result_json(results_table_t* results, db_query_t* db_query,output_t* output){
	int row_count;
	apr_bucket_brigade* bb = output->bucket_brigade;
	apr_pool_t* pool = output->pool;

	for(row_count = 0;row_count < results->rows->nelts;row_count++){
		row_t row;
		int column_index;
		row = APR_ARRAY_IDX(results->rows,row_count,row_t);
		apr_brigade_puts(bb, NULL,NULL, "\t\t\t{\n");
		for(column_index = 0;column_index < db_query->select_columns->nelts;column_index++){
			apr_brigade_printf(bb, NULL,NULL, "\t\t\t\t\"%s\": \"%s\"",APR_ARRAY_IDX(db_query->select_columns,column_index,column_table_t*)->freindly_name,json_escape_char(pool,row.results[column_index]));
			if(column_index+1 < db_query->select_columns->nelts){
				apr_brigade_puts(bb, NULL,NULL, ", ");
			}
			apr_brigade_puts(bb, NULL,NULL, "\n");
		}
		apr_brigade_puts(bb, NULL,NULL, "\t\t\t}");
		if(row_count+1 < results->rows->nelts){
			apr_brigade_puts(bb, NULL,NULL, ",");
		}
	}
	return 0;
}
