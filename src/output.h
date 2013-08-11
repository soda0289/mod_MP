/*
 * output.h
 *
 *  Created on: Aug 7, 2013
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
#ifndef OUTPUT_H_
#define OUTPUT_H_

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
	int method;
	query_words_t* query_words;
	const char* uri;

	ap_filter_t* filters;
}input_t;

typedef struct output_{
	apr_pool_t* pool;
	apr_bucket_brigade* bucket_brigade;
	apr_bucket_alloc_t* bucket_allocator;
	apr_table_t* headers;
	const char* content_type;
	apr_off_t length;

	ap_filter_t* filters;
	error_messages_t* error_messages;	
}output_t;

#endif
