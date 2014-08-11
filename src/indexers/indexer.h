/*
 * indexer.h
 *
 *  Created on: Dec 29, 2012
 *      Author: Reyad Attiyat
 *      Copyright 2012-2014
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

#ifndef INDEXER_H_
#define INDEXER_H_

#include <apr_xml.h>
#include "apr_tables.h"
#include "apr_dbd.h"
#include "database/db_typedefs.h"
#include "database/db_object.h"
#include "indexers/indexer_typedefs.h"
#include "error_messages.h"
#include "output.h"
#include "input.h"

typedef int (*indexer_init_cb_t)(apr_pool_t* pool, indexer_t** indexer);
typedef int (*indexer_on_fork_cb_t)(apr_pool_t* child_pool, indexer_t* indexer);
typedef int (*indexer_query_cb_t)(indexer_t* indexer, input_t* input, output_t* output);
typedef int (*indexer_add_cb_t)(indexer_t* indexer);

struct query_words_ {
	int num_words;
	const char** words;
};

typedef struct indexer_callbacks_ {
	//Callback functions
	//init apps is run when the server stats only run once
	indexer_init_cb_t indexer_init_cb;
	//Reattach app runs when a new proccess is created by apache
	indexer_on_fork_cb_t indexer_on_fork_cb;
	//Run query is run when a new request comes into apache
	indexer_query_cb_t indexer_query_cb;
} indexer_callbacks_t;

struct indexer_ {
	const char* id;
	const char* friendly_name;

	apr_pool_t* pool;

	error_messages_t* error_messages;

	const void* data;

	apr_array_header_t* db_objects;	

	const indexer_callbacks_t* callbacks;
};

struct indexer_node_ {
	indexer_t* indexer;
	indexer_node_t* next;
};




int indexer_init(apr_pool_t* pool, error_messages_t* error_messages, db_manager_t* db_manager, const char* indexer_id, const char* xml_config_path, const indexer_callbacks_t* indexer_callbacks, indexer_t** indexer_ptr);
int indexer_add_db_object (indexer_t* indexer, db_object_t* new_object);
int indexer_async_add_db_object (indexer_t* indexer, db_object_t* new_object, indexer_add_cb_t db_obj_add_cb);

int indexer_remove_db_object (indexer_t* indexer, db_object_t* db_obj);

int indexer_query (indexer_t* indexer, db_query_t* query);

#endif
