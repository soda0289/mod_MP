/*
 * error_handler.c
 *
 *  Created on: Sep 26, 2012
 *      Author: reyad
 */
#include "mod_mediaplayer.h"



int add_error_list(error_messages_t* error_messages, char* message_type, char* message){
	if(error_messages == NULL || message_type == NULL || message == NULL || error_messages->pool == NULL)
	{
		return -1;
	}
	apr_snprintf((char *)&error_messages->errors[error_messages->num_errors], sizeof(error_messages->errors[error_messages->num_errors]), "%s %s", message_type, message);
	error_messages->num_errors++;
	return 0;
}


