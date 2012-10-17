/*
 * error_handler.h
 *
 *  Created on: Oct 2, 2012
 *      Author: Reyad Attiyat
 */

#ifndef ERROR_HANDLER_H_
#define ERROR_HANDLER_H_
typedef struct {
	int num_errors;
	apr_pool_t* pool;
	char errors[20000][512];
	apr_table_t*	error_table;
	apr_global_mutex_t* mutex;
}error_messages_t;

int add_error_list(error_messages_t* error_messages,const char* message_type,const char* message);

#endif /* ERROR_HANDLER_H_ */
