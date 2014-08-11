/*
 * app_config.c
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

/*
 * config_app
 * app pointer
 * app type
 * freindly_name (safe to search)
 * app id
 * init app pointer function
 * get query pointer function
 * run query pointer function
 *
*/

#include <stdlib.h>
#include "apr_general.h"
#include "apr_lib.h"
#include "apr_strings.h"
#include <apr_xml.h>
#include "indexers/index_manager.h"

#include "database/db_query.h"
#include "database/db_config.h"

int index_manager_init (apr_pool_t* pool, error_messages_t* error_messages, index_manager_t** index_manager_ptr) {
	int status = 0;
	index_manager_t* index_manager;

	//Setup the indexer manager
	index_manager = *index_manager_ptr = apr_pcalloc(pool,sizeof(index_manager_t));
	index_manager->pool = pool;
	index_manager->error_messages = error_messages;

	return status;
}

void index_manager_on_fork (index_manager_t* index_manager, apr_pool_t* child_pool, error_messages_t* error_messages) {
	indexer_node_t* indexer_node;

	for(indexer_node = index_manager->first_node;indexer_node != NULL;indexer_node = indexer_node->next){
		indexer_t* indexer = indexer_node->indexer;
		indexer->pool = child_pool;
		indexer->callbacks->indexer_on_fork_cb (child_pool, indexer);
	}

}

int index_manager_add (index_manager_t* index_manager, indexer_t* indexer){
	int status = 0;

	indexer_node_t* indexer_node = NULL;
	
	indexer_node = apr_pcalloc(index_manager->pool,sizeof(indexer_node_t));
	indexer_node->next = NULL;

	//Add to linked list
	if(index_manager->first_node == NULL){
		//No apps in list
		index_manager->first_node = indexer_node;
	}else{
		indexer_node->next = index_manager->first_node;
		index_manager->first_node = indexer_node;
	}

	(index_manager->count)++;

	indexer_node->indexer = indexer;

	return status;
}

int index_manager_find_indexer (index_manager_t* index_manager, const char* indexer_name, indexer_t** indexer_ptr){
	indexer_node_t* indexer_node;

	for(indexer_node = index_manager->first_node; indexer_node != NULL; indexer_node = indexer_node->next){
		indexer_t* indexer = indexer_node->indexer;
		if(indexer->friendly_name != NULL && apr_strnatcmp(indexer->friendly_name, indexer_name) == 0){
			*indexer_ptr = indexer;
			return 0;
		}
	}

	return -1;
}
