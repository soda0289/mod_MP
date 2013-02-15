/*
 * app_config.h
 *
 *  Created on: Dec 29, 2012
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
 *   limitations under the License.
*/

#ifndef APP_CONFIG_H_
#define APP_CONFIG_H_

#include "apr_tables.h"
#include "apr_dbd.h"

#include "database/db_typedef.h"
#include "app_typedefs.h"
#include "error_handler.h"


typedef int (*init_app_fnt)(apr_pool_t* global_pool, error_messages_t* error_messages, const char* external_directory, const void** global_context);
typedef int (*reattach_app_fnt)(apr_pool_t* child_pool, error_messages_t* error_messages,const void* global_context);
typedef int (*run_query_fnt)(apr_pool_t* pool,apr_pool_t* global_pool, apr_bucket_brigade* output_bb, apr_table_t* output_headers, const char* output_content_type,error_messages_t* error_messages, db_config* dbd_config, query_words_t* query_words, apr_array_header_t* db_queries,const void* global_context);

struct query_words_{
	int num_words;
	const char** words;
};


struct app_{
	const char* id;
	const char* friendly_name;
	const void* global_context;
	query_words_t query_words;
	apr_array_header_t* db_queries; //Multi Column Queries for Database

	init_app_fnt init_app;
	reattach_app_fnt reattach_app;
	run_query_fnt run_query;
};


struct app_node_{
	app_t* app;
	app_node_t* next;
};


struct app_list_{
	int count;
	apr_pool_t* pool;  //Pool to allocate Linked List
	app_node_t* first_node;


};



int config_app(app_list_t* app_list,const char* freindly_name, const char* app_id,init_app_fnt init_app, reattach_app_fnt reattach_app, run_query_fnt run_query);
int app_process_uri(apr_pool_t* pool, const char* uri, app_list_t* app_list, app_t** app);
int allocate_app_prepared_statments(apr_pool_t* pool, app_t* app);
int init_apps(app_list_t* app_list,apr_pool_t* global_pool, error_messages_t* error_messages, const char* media_directory);
int reattach_apps(app_list_t* app_list,apr_pool_t* child_pool, error_messages_t* error_messages);

#endif /* APP_CONFIG_H_ */
