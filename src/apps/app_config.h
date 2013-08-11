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

#include <apr_xml.h>
#include "apr_tables.h"
#include "apr_dbd.h"
#include "database/db_typedef.h"
#include "app_typedefs.h"
#include "error_handler.h"
#include "output.h"

typedef int (*init_app_fnt)(apr_pool_t* global_pool,apr_array_header_t* db_queries, apr_xml_elem* xml_params, const void** global_context, error_messages_t* error_messages);
typedef int (*reattach_app_fnt)(apr_pool_t* child_pool, error_messages_t* error_messages,const void* global_context);
typedef int (*run_query_fnt)(input_t* input, output_t* output, const void* global_context);

struct query_words_{
	int num_words;
	const char** words;
};


struct app_{
	const char* id;
	const char* friendly_name;

	const void* global_context;

	//Database queires the app will use
	const char* xml_config_file_path;
	apr_array_header_t* db_queries;	

	apr_xml_elem* xml_parameters;

	//Callback functions
	//init apps is run when the server stats only run once
	init_app_fnt init_app;
	//Reattach app runs when a new proccess is created by apache
	reattach_app_fnt reattach_app;
	//Run query is run when a new request comes into apache
	run_query_fnt run_query;
};


struct app_node_{
	app_t* app;
	app_node_t* next;
};


struct app_list_{
	int count;

	apr_pool_t* pool;  //Pool to allocate Linked List
	error_messages_t* error_messages;

	//Linked List
	app_node_t* first_node;
};

int app_process_uri(input_t* input, app_list_t* app_list, app_t** app);


int config_app(app_list_t* apps,const char* id,const char* xml_file_path,init_app_fnt init_app, reattach_app_fnt reattach_app, run_query_fnt run_query);
int allocate_app_prepared_statments(apr_pool_t* pool, app_t* app);
int init_app_manager(apr_pool_t* pool, error_messages_t* error_messages, app_list_t** apps_ptr);
void reattach_apps(app_list_t* app_list,apr_pool_t* child_pool, error_messages_t* error_messages);
int init_apps(app_list_t* app_list,apr_array_header_t* db_array);


#endif /* APP_CONFIG_H_ */
