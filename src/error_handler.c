/*
 * error_handler.c
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
#include "mod_mediaplayer.h"

int init_error_messages(apr_pool_t* pool,error_messages_t** error_messages, const char* errors_shm_file){
	int status = 0;
	apr_shm_t* errors_shm;
	//Setup Shared Memory

	status = setup_shared_memory(&(errors_shm),sizeof(error_messages_t),errors_shm_file, pool);
	if(status != 0){
		return -1;
	}

	*error_messages = apr_shm_baseaddr_get(errors_shm);
	(*error_messages)->num_errors = 0;


	apr_pool_cleanup_register(pool, errors_shm,(void*) apr_shm_destroy	, apr_pool_cleanup_null);

	return 0;
}

int reattach_error_messages(apr_pool_t* pool,error_messages_t** error_messages, const char* errors_shm_file){
	apr_shm_t* errors_shm = NULL;
	apr_status_t rv;

	if(errors_shm_file){
		rv = apr_shm_attach(&errors_shm,errors_shm_file, pool);
		if(rv != APR_SUCCESS){
				//p_log_error(__FILE__,__LINE__, 0,APLOG_CRIT, rv, s, "Error reattaching shared memeory Error Messages");
		}
	}

	//Setup srv_conf
	*error_messages = apr_shm_baseaddr_get(errors_shm);

	return 0;
}

int copy_error_messages(error_messages_t** new,error_messages_t* old, apr_pool_t* pool){
	//Copy error messages from shared memory
	int i = 0;
	*new = apr_pcalloc(pool, sizeof(error_messages_t));
	(*new)->num_errors = old->num_errors;
	for(i = 0; i < old->num_errors; i++){
		(*new)->messages[i] = old->messages[i];
	}
	return 0;
}
int print_error_messages(apr_pool_t* pool,apr_bucket_brigade* bb,error_messages_t* error_messages){
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
	return 0;
}

int add_error_list(error_messages_t* error_messages, enum error_type type, const char*error_header, const char* error_message){
	if((error_messages->num_errors) >=1024 || error_messages == NULL){
		//Should write to apache error log or somthing
		return -1;
	}
		error_messages->messages[error_messages->num_errors].type = type;
		strncpy(error_messages->messages[error_messages->num_errors].header, error_header, MAX_ERROR_SIZE);
		//Add one to num errors
		strncpy(error_messages->messages[error_messages->num_errors++].message ,error_message, MAX_ERROR_SIZE);

	return 0;
}


