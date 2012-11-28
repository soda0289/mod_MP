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
/*
int add_error_list(error_messages_t* error_messages,const char* message_type,const char* message){
	if(error_messages == NULL || message_type == NULL || message == NULL || error_messages->pool == NULL || error_messages->error_table == NULL)
	{
		return -1;
	}

	apr_table_merge(error_messages->error_table,
			apr_itoa(error_messages->pool, error_messages->num_errors),
			apr_psprintf(error_messages->pool, "\"header\" : \"%s\",\"info\" : \"%s\"",  json_escape_char(error_messages->pool, message_type), json_escape_char(error_messages->pool, message)));
	error_messages->num_errors++;
	return 0;
}
*/

int add_error_list(error_messages_t* error_messages, enum error_type type, const char*error_header, const char* error_message){
	error_messages->messages[error_messages->num_errors].type = type;
	strncpy(error_messages->messages[error_messages->num_errors].header, error_header, 255);
	//Add one to num errors
	strncpy(error_messages->messages[error_messages->num_errors++].message ,error_message, 255);
	return 0;
}


