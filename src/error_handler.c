/*
 * error_handler.c
 *
 *  Created on: Sep 26, 2012
 *      Author: Reyad Attiyat
 */
#include "mod_mediaplayer.h"



int add_error_list(error_messages_t* error_messages,const char* message_type,const char* message){
	if(error_messages == NULL || message_type == NULL || message == NULL || error_messages->pool == NULL || error_messages->error_table == NULL)
	{
		return -1;
	}
	apr_table_merge(error_messages->error_table,
			apr_itoa(error_messages->pool, error_messages->num_errors),
			apr_psprintf(error_messages->pool, "\"%s\" : \"%s\"",  json_escape_char(error_messages->pool, message_type), json_escape_char(error_messages->pool, message)));
	//apr_snprintf((char *)&error_messages->errors[error_messages->num_errors], sizeof(error_messages->errors[error_messages->num_errors]), "%s %s", message_type, message);
	error_messages->num_errors++;
	return 0;
}



