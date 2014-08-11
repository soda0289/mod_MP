/*
 * db_query_parameters.h
 *
 *  Created on: Jan 18, 2013
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

#ifndef DB_QUERY_PARAMETERS_H_
#define DB_QUERY_PARAMETERS_H_

#include "apr_dbd.h"
#include "apr_tables.h"
#include "apr.h"

#include "database/db_config.h"

typedef enum{
	EQUAL =0,
	LIKE,
	GREATER,
	LESS,
	BETWEEN,
	IN
}condition_operator;

typedef enum{
	LIMIT = 0,
	OFFSET,
	GROUP_BY,
	ORDER_BY
}sql_clauses;

#define NUM_SQL_CLAUSES 4


//SQL Where conditions
struct query_where_condition_{
	db_table_column_t* column;
	condition_operator operator;
	const char* condition;
};

//SQL clauses struct (Limit, Group By, ...)
struct query_sql_clasuses_{
	const char* freindly_name;
	const char* value;
};


struct custom_parameter_{
	const char* freindly_name;
	const char* type;
	const char* value;
};


//Query parameters including SQL Clauses and Custom parameters
struct db_query_parameters_{
	apr_pool_t* pool;

	char num_columns;

	apr_array_header_t* where_conditions;

	query_sql_clauses_t* sql_clauses;

	char num_custom_parameters;
	apr_array_header_t* custom_parameters;
};


int db_query_parameters_init (apr_pool_t* pool, db_query_parameters_t** db_query_parameters);
int db_query_parameters_add_where(db_query_parameters_t* db_query_parameters, db_table_column_t* column, const char* condition);

int db_query_parameters_find_custom_by_friendly(db_query_parameters_t* db_query_parameters, const char* friendly_name, custom_parameter_t** custom_parameter);
#endif /* DB_QUERY_PARAMETERS_H_ */
