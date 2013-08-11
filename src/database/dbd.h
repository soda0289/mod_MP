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

enum column_types {
		INT = 0,
		VARCHAR,
		DATETIME,
		BIGINT
};



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
		enum column_types type;
		table_t* table;
};



struct db_query_{
	//ID of query
	const char* id;
	//Query type (Select, Insert, Update)
	const char* type;

	//Database Parameters for query
	db_params_t* db_params;

	apr_array_header_t* tables;

	//Num of select or insert columns
	int num_columns;
	
	//Tables needed by query
	//apr_array_header_t* tables;
	const char* table_join_string;

	//Columns to select
	apr_array_header_t* select_columns;
	const char* select_columns_string;

	//Columns to insert data into
	apr_array_header_t* insert_values;

	//THIS SHOULD BE MOVED TO QUERY PARAMETERS
	const char* group_by_string;

	//Query parameters
	//db_query_params_t* query_params;
	
	//Custom query parameters used to pass
	//aditional information to an app
	apr_array_header_t* custom_parameters;
};

struct db_queries_{
	//Databases needed for queries
	db_params_t* db_params;
	apr_array_header_t* queries;

};



struct db_params_{
	const char* id;
	//Params filled by XML DB Config
	const char* driver_name;
	const char* hostname;
	const char* username;
	const char* password;
	//Tables on database also loaded from XML
	apr_array_header_t* tables;

	db_config_t* db_config;
};


struct db_config_{
	apr_pool_t* pool;
	apr_pool_t* parent_pool;
	
	//Thread lock as database can only send one query per proccess
	apr_thread_mutex_t* mutex;

	//APR DBD magic
	apr_dbd_t *dbd_handle;
	const apr_dbd_driver_t *dbd_driver;
	apr_dbd_transaction_t * transaction;
	
	//DB Parameters
	db_params_t* db_params;
	
	//Connected status
	int connected;

	//Database errors
	error_messages_t* database_errors;

	//WILL DELETE SOON ONCE QUERY PARAMETERS BECOMES MORE ADVANCE
	//ALL QUERIES SHOULD BE LOADED BY APPS NOT BY DATABSE CODE
	struct statments_{
		//Run after inserts to get id
		apr_dbd_prepared_t* select_last_id;
	
		//Used to add songs found when reading directories
		apr_dbd_prepared_t *add_song;
		apr_dbd_prepared_t *add_artist;
		apr_dbd_prepared_t *add_album;
		apr_dbd_prepared_t* add_source;

		//Used to select info when directory syncing
		apr_dbd_prepared_t *select_song;
		apr_dbd_prepared_t *select_artist;
		apr_dbd_prepared_t *select_album;
		apr_dbd_prepared_t *select_sources;


		apr_dbd_prepared_t *add_link;

		apr_dbd_prepared_t* select_file_path;
		apr_dbd_prepared_t *select_mtime;

		apr_dbd_prepared_t* update_song;
	}statements;

};


struct row_ {
	const char** results;
};


struct results_table_ {
	apr_array_header_t* rows;
};
int connect_database(apr_pool_t* global_pool, db_params_t* db_params);
int prepare_database(db_config_t* db_config);
void close_database(db_config_t* dbd_config);
int sync_song(apr_pool_t* pool, db_config_t* dbd_config, music_file *song);
int run_query(db_query_t* db_query,query_parameters_t* query_parameters,results_table_t** query_results);
int get_file_path(char** file_path, db_config_t* dbd_config, char* id, apr_dbd_prepared_t* select);
int get_column_results_for_row(db_query_t* db_query, results_table_t* query_results,column_table_t* column,int row_index,const char** column_result);
int insert_db(char** id, db_config_t* dbd_config, apr_dbd_prepared_t* query, const char** args);
int output_db_result_json(results_table_t* results, db_query_t* db_query,output_t* output);

int find_db_by_id(apr_array_header_t* db_arrays, const char* db_id, db_params_t** db_params_ptr);
int find_query_by_id(db_query_t** query, apr_array_header_t* db_queries_arr, const char* id);
#endif /* DBD_H_ */

