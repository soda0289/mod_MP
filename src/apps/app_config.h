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


struct query_words_{
	int num_words;
	const char** words;
};


struct app_{
	const char* id;
	const char* friendly_name;
	query_words_t query_words;
	apr_array_header_t* db_queries; //Multi Column Queries for Database

	char* query;
	int (*get_query)(apr_pool_t*, error_messages_t*,app_query*,query_words_t*, apr_array_header_t*);
	int (*run_query)(request_rec* r, app_query,db_config*, apr_dbd_prepared_t****);
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



int config_app(app_list_t* app_list,const char* freindly_name, const char* app_id, int* init_app,int (*get_query)(apr_pool_t*, error_messages_t*,app_query*,query_words_t*, apr_array_header_t*),int (*run_query)(request_rec*, app_query,db_config*,apr_dbd_prepared_t****));
int app_process_uri(apr_pool_t* pool, const char* uri, app_list_t* app_list, app_t** app);
int allocate_app_prepared_statments(apr_pool_t* pool, app_t* app);

#endif /* APP_CONFIG_H_ */
