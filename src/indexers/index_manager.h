
#ifndef INDEX_MANAGER_H
#define INDEX_MANAGER_H

#include "error_messages.h"
#include "indexers/indexer_typedefs.h"

struct index_manager_ {
	int count;

	//Pool to allocate Linked List
	apr_pool_t* pool; 
	
	error_messages_t* error_messages;

	//Linked List of Indexers
	indexer_node_t* first_node;
};

int index_manager_init (apr_pool_t* pool, error_messages_t* error_messages, index_manager_t** index_manager_ptr);
int index_manager_add (index_manager_t* index_manager, indexer_t* indexer);
void index_manager_on_fork (index_manager_t* index_manager, apr_pool_t* child_pool, error_messages_t* error_messages);
int index_manager_find_indexer (index_manager_t* index_manager, const char* indexer_name, indexer_t** indexer_ptr);

#endif
