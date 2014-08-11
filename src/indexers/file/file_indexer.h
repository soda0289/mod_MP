
#ifndef FILE_INDEXER_H_
#define FILE_INDEXER_H_

#include "indexers/indexer.h"

int file_indexer_init (apr_pool_t* pool, indexer_t** indexer);
int file_indexer_on_fork (apr_pool_t* pool, indexer_t* indexer);
int file_indexer_query (indexer_t* indexer, input_t* input, output_t* output);
const indexer_callbacks_t file_indexer_callbacks;

#endif
