/*
 * dbd.h
 *
 *  Created on: Sep 20, 2012
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
 * See the License for the specific language governing permissions and
*  limitations under the License.
*/

#ifndef DBD_H_
#define DBD_H_
#define MAX_NUM_COLUMNS 10
#define NUM_TABLES 5

#include "mod_mediaplayer.h"
#include "apps/app_config.h"
#include "apps/music/music_query.h"
#include "apps/app_config.h"
#include "db_query_parameters.h"
#include "db_typedef.h"
#include "apps/music/music_typedefs.h"




struct table_{
	const char* id;
	const char* name;
	apr_array_header_t* columns;
	const char* foreign_key;
};


struct column_table_{
		const char* id;
		const char* name;
		const char* freindly_name;
		table_t* table;
};



struct query_{
	const char* id;
	int num_columns;
	apr_array_header_t* tables;
	const char* table_join_string;
	apr_array_header_t* select_columns;
	const char* select_columns_string;
	const char* group_by_string;
	apr_array_header_t* custom_parameters;
};





struct db_config_{
	apr_pool_t* pool;
	apr_thread_mutex_t* mutex;
	apr_dbd_t *dbd_handle;
	const apr_dbd_driver_t *dbd_driver;
	const char* driver_name;
	const char* mysql_parms;
	apr_dbd_transaction_t * transaction;
	int connected;
	error_messages_t* database_errors;
	struct {
		apr_dbd_prepared_t* select_last_id;
		apr_dbd_prepared_t *add_song;
		apr_dbd_prepared_t *add_artist;
		apr_dbd_prepared_t *add_album;
		apr_dbd_prepared_t* add_source;
		apr_dbd_prepared_t *select_song;
		apr_dbd_prepared_t *select_artist;
		apr_dbd_prepared_t *select_album;
		apr_dbd_prepared_t *select_sources;
		apr_dbd_prepared_t *add_link;
		apr_dbd_prepared_t* select_file_path;
		apr_dbd_prepared_t *select_mtime;
		apr_dbd_prepared_t* update_song;
	}statements;
	//Setup by DB Query Configuration
	apr_array_header_t* tables;   //Tables on database
};


struct row_ {
	const char** results;
};


struct results_table_ {
	apr_array_header_t* rows;
};


apr_status_t connect_database(apr_pool_t* pool, error_messages_t* error_messages,db_config** dbd_config);
int prepare_database(app_list_t* app_list,db_config* dbd_config);
void close_database(db_config* dbd_config);
int sync_song(apr_pool_t* pool, db_config* dbd_config, music_file *song);
int select_db_range(apr_pool_t* pool, db_config* dbd_config,query_parameters_t* query_parameters, query_t* db_query,results_table_t** query_results,error_messages_t* error_messages);
int get_file_path(char** file_path, db_config* dbd_config, char* id, apr_dbd_prepared_t* select);
int get_column_results_for_row(query_t* db_query, results_table_t* query_results,column_table_t* column,int row_index,const char** column_result);
int insert_db(char** id, db_config* dbd_config, apr_dbd_prepared_t* query, const char** args);

#endif /* DBD_H_ */

