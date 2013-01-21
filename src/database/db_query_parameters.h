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
#include "database/dbd.h"
#include "apr.h"

typedef 	enum{
	EQUAL =0,
	LIKE,
	GREATER,
	LESS,
	BETWEEN
}condition_operator;
typedef enum{
	LIMIT = 0,
	OFFSET,
	GROUP_BY,
	ORDER_BY
}sql_clauses;

#define NUM_SQL_CLAUSES 4
//SQL Where conditions
typedef struct{
	column_table_t* column;
	condition_operator operator;
	const char* condition;
}query_where_condition_t;

//SQL clauses struct (Row Count, Group By, ...)
typedef struct{
	const char* freindly_name;
	const char* value;
}query_sql_clauses_t;


typedef struct custom_parameter_t_{
	const char* freindly_name;
	const char* type;
	const char* value;
}custom_parameter_t;

//Query parameters including SQL Clauses and Custom parameters
typedef struct query_parameters_t_{
	//Parameters set allows us to cache out queries for
	//each query type. Each column gets one binary digit
	//and 3 binary digits are reserved for row_count,
	//row_offset, and group_by
	uint64_t parameters_set;
	char num_columns;
	apr_array_header_t* query_where_conditions;
	query_sql_clauses_t* query_sql_clauses;
	char num_custom_parameters;
	apr_array_header_t* query_custom_parameters;
}query_parameters_t;
int init_query_parameters(apr_pool_t* pool, query_parameters_t** query_parameters);
void setup_sql_clause(query_sql_clauses_t** clauses,sql_clauses type,const char* fname);
int find_custom_parameter_by_friendly(apr_array_header_t*,const char* friendly_name, custom_parameter_t** custom_parameter);
int add_where_query_parameter(query_parameters_t* query_parameters,column_table_t* column,const char* condition);

#endif /* DB_QUERY_PARAMETERS_H_ */
