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
	int num_errors;
	//Create error memory table
	error_message_t messages[1024];
}error_messages_t;

int add_error_list(error_messages_t* error_messages, enum error_type type, const char*error_header, const char* error_message);

#endif /* ERROR_HANDLER_H_ */
