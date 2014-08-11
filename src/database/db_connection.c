#include "database/db_typedefs.h"
#include "database/db_config.h"
#include "database/db_connection.h"

int db_connection_init (db_config_t* db_config, const char* connection_params, db_connection_t** db_connection_ptr){
	apr_status_t rv;
	const char* error_string;
	const char* mysql_params;
	
	db_connection_t* db_connection = *db_connection_ptr = apr_pcalloc(db_config->pool, sizeof(db_connection_t));

	db_connection->db_config = db_config;
	
	//Create lock for multi-threading connection use
	rv = apr_thread_mutex_create(&(db_connection->mutex), APR_THREAD_MUTEX_DEFAULT, db_config->pool);
	if(rv != APR_SUCCESS){
		error_messages_add(db_config->error_messages, ERROR, "Failed to connect to database", "Failed to create mutex"); 
		return -2;
	}
	
	//Read db params and create mysql params
	mysql_params = apr_pstrcat(db_config->pool ,"host=", db_config->db_params->hostname,",user=",db_config->db_params->username, NULL);

	rv = apr_dbd_open_ex(db_config->dbd_driver, db_config->pool, mysql_params, &(db_connection->dbd_handle), &error_string);
	if (rv != APR_SUCCESS){
		error_messages_add(db_config->error_messages, ERROR, "Failed to connect to database", error_string); 
		return -2;
	}

	return 0;
}

void db_connection_close(db_connection_t* db_conn){

	apr_dbd_close(db_conn->db_config->dbd_driver, db_conn->dbd_handle);
	db_conn->dbd_handle = NULL;
}
