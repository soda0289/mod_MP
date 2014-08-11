#include <apr.h>

#include "indexers/indexer.h"
#include "indexers/file/file_indexer.h"
#include "indexers/indexer_typedefs.h"

#include "input.h"
#include "output.h"

const indexer_callbacks_t file_indexer_callbacks = {
	.indexer_init_cb = file_indexer_init,
	.indexer_on_fork_cb = file_indexer_on_fork,
	.indexer_query_cb = file_indexer_query
};

int file_indexer_init (apr_pool_t* pool, indexer_t** indexer_ptr) {
	indexer_t* indexer = *indexer_ptr = apr_pcalloc(pool, sizeof(indexer_t));

	if(indexer == NULL){
		return -1;
	}

	return 0;
}

int file_indexer_on_fork (apr_pool_t* pool, indexer_t* indexer) {
	
	error_messages_on_fork(&(indexer->error_messages), pool);

	return 0;
}

int file_indexer_query (indexer_t* indexer, input_t* input, output_t* output) {
	if(indexer == NULL){
		return -1;
	}

	return 0;
}
