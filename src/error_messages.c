/*
 * error_messages.c
 *
 *  Created on: Sep 26, 2012
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
 *  limitations under the License.
*/

#include <apr.h>
#include <apr_strings.h>
#include "error_messages.h"
#include "util_json.h"
#include "util_shmem.h"

int error_messages_init (apr_pool_t* pool, error_messages_t** error_messages_ptr) {
	error_messages_t* error_messages = *error_messages_ptr = apr_pcalloc(pool, sizeof(error_messages_t));
	
	if (error_messages == NULL) {
		return APR_ENOMEM;
	}

	error_messages->pool = pool;

	error_messages->num_errors = 0;
	return APR_SUCCESS;
}

int error_messages_init_shared (apr_pool_t* pool, const char* errors_shm_file, error_messages_t** error_messages_ptr) {
	int status = 0;
	apr_shm_t* errors_shm;
	error_messages_t* error_messages;
	//Setup Shared Memory

	status = setup_shared_memory(&(errors_shm), sizeof(error_messages_t), errors_shm_file, pool);
	if(status != 0){
		return APR_ENOMEM;
	}

	error_messages = *error_messages_ptr = apr_shm_baseaddr_get(errors_shm);
	if (error_messages == NULL) {
		return APR_ENOMEM;
	}

	error_messages->pool = pool;
	error_messages->num_errors = 0;
	error_messages->shm_file = errors_shm_file;

	apr_pool_cleanup_register(pool, errors_shm,(void*) apr_shm_destroy, apr_pool_cleanup_null);

	return APR_SUCCESS;
}

int error_messages_on_fork (error_messages_t** error_messages, apr_pool_t* pool) {
	const char* errors_shm_file = (*error_messages)->shm_file;
	apr_shm_t* errors_shm = NULL;
	apr_status_t rv;

	if(errors_shm_file){
		rv = apr_shm_attach(&errors_shm, errors_shm_file, pool);
		if(rv != APR_SUCCESS){
			return -1;
		}
	}

	//Swap error_messages value
	*error_messages = apr_shm_baseaddr_get(errors_shm);

	return APR_SUCCESS;
}

//Copy all error messages from one list to another
int error_messages_duplicate (error_messages_t* new,error_messages_t* old, apr_pool_t* pool) {
	int i = 0;
	int offset = new->num_errors;

	for(i = 0; i < old->num_errors; i++){
		int error_num = i + offset;
		new->messages[error_num].type = old->messages[i].type;
		strncpy(new->messages[error_num].header, old->messages[i].header, MAX_ERROR_SIZE);
		strncpy(new->messages[error_num].message ,old->messages[i].message, MAX_ERROR_SIZE);
		new->num_errors++;
	}
	return APR_SUCCESS;
}


//Print out error messages to JSON
int error_messages_print_json_bb (error_messages_t* error_messages, apr_pool_t* pool,apr_bucket_brigade* bb) {
	int i;
	apr_brigade_puts(bb, NULL,NULL,"\t\t\"Errors\" : [\n");
	//Print Errors
	for (i =0;i < error_messages->num_errors; i++){
		apr_brigade_printf(bb, NULL,NULL, "\t\t\t{\n\t\t\t\t\"type\" : %d,\n\t\t\t\t\"header\" : \"%s\",\n\t\t\t\t\"message\" : \"%s\"\n\t\t\t}\n", error_messages->messages[i].type,json_escape_char(pool,error_messages->messages[i].header), json_escape_char(pool,error_messages->messages[i].message));
		if (i+1 != error_messages->num_errors){
			apr_brigade_puts(bb, NULL,NULL,",");
		}
		apr_brigade_puts(bb, NULL,NULL,"\n");
	}
	apr_brigade_puts(bb, NULL,NULL,"\t\t]\n");

	return APR_SUCCESS;
}

/*
int error_messages_to_ap_log(error_messages_t* error_messages, server_rec* server){
	int i = 0;
	
	for (i = 0;i < error_messages->num_errors; i++){
		const char* err_header = error_messages->messages[i].header;
		const char* err_message = error_messages->messages[i].message;
		ap_log_error(APLOG_MARK, APLOG_ERR, 0, server, "%s: %s", err_header, err_message);
	}

	return 0;
}
*/

int error_messages_add (error_messages_t* error_messages, enum error_type type, const char* error_header, const char* error_message) {
	int error_num;

	if((error_messages->num_errors) >= 1024 || error_messages == NULL){
		//Maybe perform a cleanup
		return -1;
	}
	
	error_num = __sync_fetch_and_add(&(error_messages->num_errors), 1);

	error_messages->messages[error_num].type = type;
	strncpy(error_messages->messages[error_num].header, error_header, MAX_ERROR_SIZE);
	//Add one to num errors
	strncpy(error_messages->messages[error_num].message ,error_message, MAX_ERROR_SIZE);

	return APR_SUCCESS;
}

int error_messages_addf (error_messages_t* error_messages, const enum error_type type, const char* error_header, const char* error_message_format,...) {
	int status = APR_SUCCESS;
	const char* error_message;
	va_list arg_list;

	va_start(arg_list, error_message_format);

	error_message = apr_pvsprintf(error_messages->pool, error_message_format, arg_list);
	if (error_message != NULL) {
		status = error_messages_add (error_messages, type, error_header, error_message);
	}else{
		status = -1;
	}

	va_end(arg_list);

	return status;
}
