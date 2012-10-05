/*
 * error_handler.h
 *
 *  Created on: Oct 2, 2012
 *      Author: reyad
 */

#ifndef ERROR_HANDLER_H_
#define ERROR_HANDLER_H_
typedef struct {
	int num_errors;
	apr_pool_t* pool;
	char errors[20000][512];
}error_messages_t;

int add_error_list(error_messages_t* error_messages, char* message_type, char* message);

#endif /* ERROR_HANDLER_H_ */
