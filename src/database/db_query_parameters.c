/*
 * db_query_parameters.c
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

#include <stdlib.h>
#include <database/db_query_parameters.h>
#include "database/dbd.h"
#include <ctype.h>

void setup_sql_clause(query_sql_clauses_t** clauses,sql_clauses type,const char* fname){
	(*clauses)[type].freindly_name = fname;
}

int init_query_parameters(apr_pool_t* pool, query_parameters_t** query_parameters){
	//INITALIZE query parameters

	*query_parameters = apr_pcalloc(pool,sizeof(query_parameters_t));

	(*query_parameters)->query_sql_clauses = apr_pcalloc(pool,sizeof(query_sql_clauses_t )* NUM_SQL_CLAUSES);
	setup_sql_clause(&((*query_parameters)->query_sql_clauses),LIMIT,"limit");
	setup_sql_clause(&((*query_parameters)->query_sql_clauses),OFFSET,"offset");
	//Group by has no friendly name because it cannot be accessed by client
	setup_sql_clause(&((*query_parameters)->query_sql_clauses),GROUP_BY,NULL);
	setup_sql_clause(&((*query_parameters)->query_sql_clauses),ORDER_BY,"sort_by");


	(*query_parameters)->query_where_conditions = apr_array_make(pool,10,sizeof(query_where_condition_t));

	return 0;
}

int find_custom_parameter_by_friendly(apr_array_header_t* parameter_array,const char* friendly_name, custom_parameter_t** custom_parameter){
	int i;
	for(i =0;i < parameter_array->nelts;i++){
		custom_parameter_t* cus_par = &(((custom_parameter_t*)parameter_array->elts)[i]);
		if(apr_strnatcmp(cus_par->freindly_name,friendly_name) == 0){
			*custom_parameter = cus_par;
			return 0;
		}
	}
	return -1;
}

int add_where_query_parameter(apr_pool_t* pool, query_parameters_t* query_parameters,column_table_t* column,const char* condition){
	query_where_condition_t* query_where_condition;
	condition_operator operator = EQUAL;


	if(column->type == VARCHAR){
		//Varchar string
		int i;
		//check if valid chars in condition string
		for(i = 0; i < strlen(condition); i++){
			if(!isalnum	(condition[i]) && !isprint(condition[i])){
				//Error not alphanumeric
				return -1;
			}
		}

		//Check for * Wild Card
		if(strchr(condition,'*') == NULL){
			operator = EQUAL;
		}else{
			//Query word contains asterisk
			//Replace them all with % sign for MySQL
			char* asterisk = condition;
			while((asterisk = strchr(asterisk, '*')) != NULL){
				asterisk[0] = '%';
			}
			//Since query word contains % change to operator to LIKE
			operator = LIKE;
		}

		//wrap double quotes around it
		condition = apr_pstrcat(pool,"\"", condition, "\"", NULL);

	}else if(column->type == DATETIME){
		//Convert to date and time


	}else{//INT
		int i = 0;
		//check if int
		for(i = 0; i < strlen(condition); i++){
			//Check if any char is not a digit
			if(!isdigit(condition[i]) && condition[i] != ','){
				//Error not a digit
				return -1;
			}
		}

		//TODO
		//check for two commas in a row ,,,
		//thats bad syntax use strtok look for null tokens
		//i think would work

		//Check for , commas
		if(strchr(condition, ',') != NULL){
			operator = IN;
			//Add parentheses to create array
			condition = apr_pstrcat(pool,"(", condition, ")", NULL);
		}
	}

	//No errors
	query_where_condition = apr_array_push(query_parameters->query_where_conditions);
	query_where_condition->column = column;
	query_where_condition->condition = condition;
	query_where_condition->operator = operator;
	return 0;
}

