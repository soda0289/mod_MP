#include <apr.h>


#include "db_typedefs.h"
#include "db_query.h"
#include "database/db_config.h"
#include "db_connection.h"

#include "error_messages.h"

//Run the database query and create results table if necesary
int db_query_run (db_query_t* db_query, db_query_parameters_t* db_query_parameters, db_query_results_t** db_query_results_ptr){
	apr_pool_t* pool = db_query->db_config->pool;
	error_messages_t* error_messages = db_query->db_config->error_messages;
	db_config_t* db_config = db_query->db_config;

	db_connection_t* db_conn;

	/*
	int error_num;

	apr_dbd_results_t* results = NULL;
	apr_dbd_row_t *row = NULL;
	const char* select = NULL;
	*/

	db_query_results_t* db_query_results = *db_query_results_ptr = apr_pcalloc(pool, sizeof(db_query_results_t));
	if (db_query_results == NULL){
		return -1;
	}



	//Check if db connected/setup
	if(db_config_get_connection(db_config, &db_conn) != 0){
		error_messages_add(error_messages, ERROR, "Database Query", "Failed to get connection");
		return -90;
	}

	apr_thread_mutex_lock(db_conn->mutex);

	/*
	error_num = generate_sql_statement(db_config, db_query_parameters, db_query, &select, error_messages);
	if(error_num != 0){
		error_messages_add(error_messages, ERROR, "DBD generate_sql_statement error", apr_dbd_error(db_config->dbd_driver, db_conn->dbd_handle, error_num));
		apr_thread_mutex_unlock(db_conn->mutex);
		return error_num;
	}

	error_num = apr_dbd_select(db_config->dbd_driver,pool,db_conn->dbd_handle,&results,select,0);
	if (error_num != 0){
		//check if not connected
		if (error_num == 2013){
			//db_config->connected = 0;
		}
		error_messages_add(error_messages, ERROR, "DBD select_db_range error", apr_dbd_error(db_config->dbd_driver, db_conn->dbd_handle, error_num));
		apr_thread_mutex_unlock(db_conn->mutex);
		return error_num;
	}
	db_query_results->row_array = apr_array_make(pool,1000,sizeof(db_results_row_t));

	//Cycle through all of them to clear cursor
	for (error_num = apr_dbd_get_row(db_config->dbd_driver, pool, results, &row, -1); //FOR
		error_num != -1; 																  //LOOP
		error_num = apr_dbd_get_row(db_config->dbd_driver, pool, results, &row, -1))
	{ 

		db_results_row_t* res_row = (db_results_row_t*)apr_array_push(db_query_results->row_array);
		int i;
		int num_columns = apr_dbd_num_cols(db_config->dbd_driver, results);

		res_row->results = apr_pcalloc(pool,sizeof(char*) * num_columns);

		for(i = 0; i < num_columns; i++){
			//We must copy the row entry as it does not stay in memory for long
			res_row->results[i] = apr_pstrdup(pool,apr_dbd_get_entry (db_config->dbd_driver,  row, i));
		}

	}
	
	*/
	apr_thread_mutex_unlock(db_conn->mutex);
	


	return 0;
}


