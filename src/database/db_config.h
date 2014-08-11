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

#include "mod_mp.h"
#include "indexers/indexer.h"
#include "db_query_parameters.h"
#include "db_query_xml_config.h"


enum column_types {
		INT = 0,
		VARCHAR,
		DATETIME,
		BIGINT
};

struct db_table_ {
	const char* id;
	const char* name;
	apr_array_header_t* columns;
	const char* foreign_key;
};


struct db_table_column_ {
		const char* id;
		const char* name;
		const char* freindly_name;
		enum column_types type;
		db_table_t* table;
};

//These values are filled by XML Database configuration file
struct db_params_{
	const char* driver_name;
	const char* hostname;
	const char* username;
	const char* password;

};

struct db_config_{
	const char* id;

	//Database memory pool should be used for all database operations
	apr_pool_t* pool;
	apr_pool_t* parent_pool;

	//APR DBD variables 
	const apr_dbd_driver_t *dbd_driver;
	
	//Database configuration errors
	error_messages_t* error_messages;

	db_params_t* db_params;

	//Database connection list
	db_conn_list_t* db_conn_list;

	//Tables of database are loaded from XML
	apr_array_header_t* tables_array;

	//Database queries that are used by applications
	apr_array_header_t* queries_array;
};


int db_config_init(apr_pool_t* parent_pool, error_messages_t* error_messages, db_params_t* db_params, db_config_t** db_config_ptr);
int db_config_get_connection (db_config_t* db_config, db_connection_t** db_conn);

int get_db_connection(db_config_t* db_config);
int prepare_database(db_config_t* db_config);
void destroy_database(db_config_t* db_config);

int get_column_results_for_row(db_query_t* db_query, db_query_results_t* query_results, db_table_column_t* column,int row_index,const char** column_result);
int output_db_result_json(db_query_results_t* results, db_query_t* db_query,output_t* output);


#endif /* DBD_H_ */

