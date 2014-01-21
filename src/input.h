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

#include <httpd.h>
#include <http_protocol.h>
#include <http_config.h>
#include "apr_general.h"
#include "apr_buckets.h"
#include "apr_strings.h"
#include <stdlib.h>
#include "error_handler.h"
#include "apps/app_typedefs.h"

typedef struct input_{
	apr_pool_t* pool;
	error_messages_t* error_messages;

	int method;
	query_words_t* query_words;
	const char* uri;

	//Used for uploading
	ap_filter_t* filters;

	int eos; // 0 if not seen end of stream bucket

	const char* boundary;

	apr_array_header_t* files;

	apr_file_t* file_d;
	int headers_over;
}input_t;

int init_input(apr_pool_t* pool, const char* uri, int method_num, input_t** input_ptr);

#endif
