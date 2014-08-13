/*
 * error_handler.h
 *
 *  Created on: Oct 2, 2012
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
 * See the License for the specific language governing permissions and
*  limitations under the License.
*/

#ifndef ERROR_HANDLER_H_
#define ERROR_HANDLER_H_
#define MAX_ERROR_SIZE 1024

#include "apr_shm.h"
#include "httpd.h"

enum error_type{
	ERROR = 0,
	DEBUG,
	WARN
};

typedef struct {
	enum error_type type;
	char header[MAX_ERROR_SIZE] ;
	char message[MAX_ERROR_SIZE];
}error_message_t;

typedef struct {
	apr_pool_t* pool;

	int num_errors;
	//Create error memory table
	error_message_t messages[1024];

	///Shared memory file
	const char* shm_file;
}error_messages_t;

int error_messages_init_shared(apr_pool_t* pool,const char* errors_shm_file, error_messages_t** error_messages);
int error_messages_init(apr_pool_t* pool, error_messages_t** error_messages);
int error_messages_on_fork(error_messages_t** error_messages, apr_pool_t* new_pool);
int error_messages_add(error_messages_t* error_messages, enum error_type type, const char*error_header, const char* error_message);
int error_messages_addf (error_messages_t* error_messages, const enum error_type, const char* error_header, const char* error_message_format,...);
int error_messages_duplicate(error_messages_t* new_obj,error_messages_t* old_obj, apr_pool_t* pool);
int error_messages_print_json_bb(error_messages_t* error_messages, apr_pool_t* pool,apr_bucket_brigade* bb);
int error_messages_to_ap_log(error_messages_t* error_messages, server_rec* server);
#endif /* ERROR_HANDLER_H_ */
