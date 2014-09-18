/*
 * input.h
 *
 *  Created on: Aug 13, 2013
 *      Author: Reyad Attiyat
 *      Copyright 2013 Reyad Attiyat
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
 *  limitations under the License.
*/
#ifndef INPUT_H_ 
#define INPUT_H_

#include <stdlib.h>
#include "error_messages.h"
#include "indexers/indexer_typedefs.h"

typedef void* query_words_t;

typedef struct input_{
	apr_pool_t* pool;
	error_messages_t* error_messages;

	int method;

	const char* uri;

	//Used for uploading
//	ap_filter_t* filters;

	//Indexer used to handle input query
	const char* indexer_string;

	//Command sent to indexer
	const char* command_string;

	//HTTP query parameters
	apr_table_t* parameter_strings;
}input_t;

int input_init(apr_pool_t* pool, const char* uri, int method_num, input_t** input_ptr);

#endif
