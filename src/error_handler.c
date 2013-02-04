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

int print_error_messages(request_rec* r,error_messages_t* error_messages){
	int i;
	ap_rputs("\t\t\"Errors\" : [\n", r);
				//Print Errors

			for (i =0;i < error_messages->num_errors; i++){
				ap_rprintf(r, "\t\t\t{\n\t\t\t\t\"type\" : %d,\n\t\t\t\t\"header\" : \"%s\",\n\t\t\t\t\"message\" : \"%s\"\n\t\t\t}\n", error_messages->messages[i].type,json_escape_char(r->pool,error_messages->messages[i].header), json_escape_char(r->pool,error_messages->messages[i].message));
				if (i+1 != error_messages->num_errors){
					ap_rputs(",",r);
				}
			ap_rputs("\n",r);
			}
			ap_rputs("\t\t]\n",r);
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


