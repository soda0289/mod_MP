/*
 * db_query_config.c
 *
 *  Created on: Dec 17, 2012
 *     Author: Reyad Attiyat
 *		Copyright 2012 Reyad Attiyat
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
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#include <apr_xml.h>
#include <stdlib.h>
#include "db_query_config.h"
#include "dbd.h"
#include "db_typedef.h"


int get_xml_attr(apr_pool_t* pool,apr_xml_elem* elem,const char* attr_name,const char** attr_value){
	apr_xml_attr* attr;
	for(attr = elem->attr;attr != NULL;attr = attr->next){
		if(apr_strnatcmp(attr->name,attr_name) == 0){
			*attr_value = apr_pstrdup(pool,attr->value);
			return 0;
		}
	}
	return -1;
}

int find_query_by_id(db_query_t** query, apr_array_header_t* db_queries_arr, const char* id){
	int j, i;
	
	//Find db_queries with query
	for(j = 0; j < db_queries_arr->nelts; j++){
		db_queries_t* db_queries = &(((db_queries_t*)db_queries_arr->elts)[j]);
		apr_array_header_t* array = db_queries->queries;
		//Find Query
		for(i = 0;i < array->nelts;i++){
			if(apr_strnatcasecmp(APR_ARRAY_IDX(array, i, db_query_t).id,id)==0){
				*query = &(APR_ARRAY_IDX(array, i, db_query_t));
				return 0;
			}
		}
	}
	return -1;
}

static int find_table_by_id(table_t** element, apr_array_header_t*array, const char* id){
	int i;
	for(i = 0;i < array->nelts;i++){
		if(apr_strnatcasecmp(APR_ARRAY_IDX(array, i, table_t).id,id)==0){
			*element = &(APR_ARRAY_IDX(array, i, table_t));
			return 0;
		}
	}
	return -1;
}

static int find_column_by_id(column_table_t** element, apr_array_header_t*array, const char* id){
	int i;
	for(i = 0;i < array->nelts;i++){
		if(apr_strnatcasecmp(APR_ARRAY_IDX(array, i, column_table_t).id,id)==0){
			*element = &(APR_ARRAY_IDX(array, i, column_table_t));
			return 0;
		}
	}
	return -1;
}
/*
static int find_app_by_id(app_list_t* app_list,const char* id,app_t** app){
	app_node_t* app_node;
	for(app_node = app_list->first_node;app_node != NULL;app_node = app_node->next){
		if(apr_strnatcasecmp(id,app_node->app->id) == 0){
			*app = app_node->app;
			return 0;
		}
	}
	return -1;
}
*/

int find_select_column_from_query_by_table_id_and_query_id(column_table_t** select_column,db_query_t* query, const char* table_id, const char* column_id){
	int column_index;
	column_table_t* column;

	for(column_index = 0; column_index < query->select_columns->nelts;column_index++){
		column = ((column_table_t**)query->select_columns->elts)[column_index];
		if(apr_strnatcmp(column->id, column_id) == 0){
			if(apr_strnatcmp(column->table->id, table_id) == 0){
				*select_column = column;
				return 0;
			}
		}
	}


	return -1;
}

//long function name
int find_column_from_query_by_friendly_name(db_query_t* query,const char* friendly_name, column_table_t** column){
	table_t* table;
	column_table_t* column_temp;
	int table_index, col_index;

	for(table_index = 0;table_index < query->tables->nelts;table_index++){
		table = APR_ARRAY_IDX(query->tables,table_index,table_t*);
		//table = &(((table_t*)music->db_query->tables->elts)[table_elem]);
		for(col_index = 0;col_index < table->columns->nelts;col_index++){
			//For each column in query check if friendly names math
			column_temp = &(APR_ARRAY_IDX(table->columns,col_index,column_table_t));
			//Add to out column count used to calculate parameter set binary flag
			//column =(((column_table_t*)table.columns->elts)[col_elem]);
			if(apr_strnatcasecmp(friendly_name,column_temp->freindly_name) == 0){
				*column = column_temp;
				return 0;
			}
		}
	}
	return -1;
}

int generate_app_queries(apr_pool_t* pool, db_queries_t* db_queries,apr_xml_elem* queries, db_params_t* db_params, error_messages_t* error_messages){
	apr_xml_elem* query_elem;
	apr_xml_elem* table_elem;
	apr_xml_elem* col_elem;

	apr_xml_elem* query_child_elem;
	apr_xml_elem* parameter_elem;
	apr_xml_elem* parameter_child_elem;

	const char* table_id = NULL;
	const char* column_id = NULL;


	db_query_t* query;


	int status;

	apr_size_t max_element_size = 512;


	for(query_elem=queries->first_child;query_elem != NULL; query_elem = query_elem->next){
		if(apr_strnatcmp(query_elem->name,"query") == 0){
			//Init Query
			query = apr_array_push(db_queries->queries);
			status = get_xml_attr(pool,query_elem,"id",&query->id);
			if(status != 0){
				return -2;
			}
			query->select_columns = apr_array_make(pool,10,sizeof(column_table_t*));
			query->tables = apr_array_make(pool,10,sizeof(table_t*));
			query->db_params = db_params;

			//Read tables used in query
			for(query_child_elem = query_elem->first_child;query_child_elem != NULL; query_child_elem =query_child_elem->next){
				if(apr_strnatcmp(query_child_elem->name,"table") == 0){
					table_t* table;
					table_t** table_ptr;

					table_elem = query_child_elem;
					get_xml_attr(pool,table_elem,"id",&table_id);
					if(status != APR_SUCCESS){
							return -1;
						}
					//Find table id in known tables
					status = find_table_by_id(&table,db_queries->db_params->tables,table_id);
					if(status != APR_SUCCESS){
						//no table found
						return -1;
					}
					//Add table to query

					//Create table join string
					query->table_join_string = (query->table_join_string) ? apr_pstrcat(pool,query->table_join_string,  " LEFT JOIN ", table->name," ON ", table->foreign_key, NULL) : table->name;
					//Add table point to table list
					table_ptr = (table_t**)apr_array_push(query->tables);
					*table_ptr = table;

					//Read columns used in Select statement of Query
					for(col_elem = table_elem->first_child;col_elem != NULL;col_elem = col_elem->next){
						if(apr_strnatcmp(col_elem->name,"column") == 0){
							column_table_t* column;
							column_table_t** column_ptr;
							const char* table_column_name;


							status = get_xml_attr(pool,col_elem,"id",&column_id);
							if(status != APR_SUCCESS){
								//no column id found
								return -2;
							}
							status = find_column_by_id(&column,table->columns,column_id);
							if(status != APR_SUCCESS){
								//no column found
								add_error_list(error_messages,ERROR,"Database Query Config", apr_psprintf(pool,"Couldn't find coulmn with id:%s",column_id));
								return -3;
							}
							//Add column to column select array
							column_ptr = (column_table_t**)apr_array_push(query->select_columns);
							*column_ptr = column;

							//Append column name to Select Columns String
							table_column_name = apr_pstrcat(pool,table->name,".",column->name,NULL);
							 query->select_columns_string = (query->select_columns_string) ? apr_pstrcat(pool,query->select_columns_string,",",table_column_name,NULL) : table_column_name;
						}
					}
				}else if(apr_strnatcmp(query_child_elem->name,"group_by") == 0){
					apr_xml_to_text(pool,query_child_elem,APR_XML_X2T_INNER,NULL,NULL,&(query->group_by_string),&max_element_size);
				}else if(apr_strnatcmp(query_child_elem->name,"custom_parameters") == 0){

					query->custom_parameters =  apr_array_make(pool,5,sizeof(custom_parameter_t));

					for(parameter_elem = query_child_elem->first_child;parameter_elem  != NULL; parameter_elem  = parameter_elem ->next){
						custom_parameter_t* custom_parameter = apr_array_push(query->custom_parameters);
						for(parameter_child_elem = parameter_elem->first_child;parameter_child_elem != NULL; parameter_child_elem = parameter_child_elem->next){
							if(apr_strnatcmp(parameter_child_elem->name, "fname") == 0){
								apr_xml_to_text(pool,parameter_child_elem,APR_XML_X2T_INNER,NULL,NULL,&(custom_parameter->freindly_name),&max_element_size);
							}else if(apr_strnatcmp(parameter_child_elem->name, "type") == 0){
								apr_xml_to_text(pool,parameter_child_elem,APR_XML_X2T_INNER,NULL,NULL,&(custom_parameter->type),&max_element_size);
							}
						}
					}
				}else if(apr_strnatcmp(query_child_elem->name,"count") == 0){
					const char* select_count;
					apr_xml_elem* count_child_elem;
					for(count_child_elem = query_child_elem->first_child;count_child_elem != NULL; count_child_elem = count_child_elem->next){
						if(apr_strnatcmp(count_child_elem->name,"table") == 0){
							table_t* table;
						
							column_table_t* column;
							column_table_t** column_ptr;

							table_elem = count_child_elem;
							get_xml_attr(pool,table_elem,"id",&table_id);
							if(status != APR_SUCCESS){
									return -1;
							}
							//Find table id in known tables
							status = find_table_by_id(&table,db_params->tables,table_id);
							if(status != APR_SUCCESS){
								//no table found
								return -1;
							}

							column = apr_pcalloc(pool, sizeof(column_table_t));

							column->freindly_name=apr_pstrcat(pool, table->name, " Count", NULL);

							column_ptr = (column_table_t**)apr_array_push(query->select_columns);
							*column_ptr = column;


							//Create Select string
							//In this special case the columns are derived from select
							//statements. For example SELECT (SELECT COUNT(*) FROM Table1) as "Table 1 Count");
							select_count = apr_pstrcat(pool, "(SELECT COUNT(*) FROM ", table->name,")",  NULL);
							query->select_columns_string = (query->select_columns_string) ? apr_pstrcat(pool, query->select_columns_string,",",select_count,NULL)  : select_count;
						}
					}
				}
			}
		}
	}
	
	return 0;
}

