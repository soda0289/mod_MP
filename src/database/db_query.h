
#ifndef DB_QUERY_H
#define DB_QUERY_H
#include "database/db_config.h"

struct db_query_ {
	//ID of query
	const char* id;
	//Query type (Select, Insert, Update)
	const char* type;

	//Database Parameters for query
	db_config_t* db_config;

	apr_array_header_t* table_pointers_array;

	//Num of select or insert columns
	int num_columns;
	
	//Tables needed by query
	//apr_array_header_t* tables;
	const char* table_join_string;

	//Columns to select
	apr_array_header_t* select_columns;
	const char* select_columns_string;

	//Columns to insert data into
	apr_array_header_t* insert_values;

	//THIS SHOULD BE MOVED TO QUERY PARAMETERS
	const char* group_by_string;

	//Query parameters
	//db_query_params_t* query_params;
	
	//Custom query parameters used to pass
	//aditional information to an app
	apr_array_header_t* custom_parameters;
};

struct db_results_row_ {
	const char** results;
};

struct db_query_results_ {
	apr_array_header_t* row_array;
};
int db_query_run (db_query_t* db_query, db_query_parameters_t* db_query_parameters, db_query_results_t** db_query_results);
int db_query_parameter_find_custom_by_friendly(db_query_parameters_t* db_query_parameters, const char* friendly_name, custom_parameter_t** custom_parameter);
int db_query_parameter_add_where(db_query_parameters_t* query_parameters, db_table_column_t* column, const char* condition);

#endif
