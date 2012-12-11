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

int add_error_list(error_messages_t* error_messages, enum error_type type, const char*error_header, const char* error_message){
	if((error_messages->num_errors) >=1024 || error_messages == NULL){
		//Should write to apache error log or somthing
		return -1;
	}
		error_messages->messages[error_messages->num_errors].type = type;
		strncpy(error_messages->messages[error_messages->num_errors].header, error_header, 255);
		//Add one to num errors
		strncpy(error_messages->messages[error_messages->num_errors++].message ,error_message, 255);

	return 0;
}


