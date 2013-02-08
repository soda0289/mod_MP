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
#include "dbd.h"
#include "db_typedef.h"


int get_xml_attr(apr_pool_t* pool,apr_xml_elem* elem, const char* attr_name, char** attr_value){
	apr_xml_attr* attr;
	for(attr = elem->attr;attr != NULL;attr = attr->next){
		if(apr_strnatcmp(attr->name,attr_name) == 0){
			*attr_value = apr_pstrdup(pool,attr->value);
			return 0;
		}
	}
	return -1;
}

int read_columns(table_t* table,apr_xml_elem* table_elem, db_config* dbd_config){
	apr_xml_elem* column_elem;
	apr_xml_elem* column_elem_child;
	column_table_t* column;
	apr_size_t max_element_size = 255;

	int status;

	table->columns = apr_array_make(dbd_config->pool,10,sizeof(column_table_t));
	for(column_elem = table_elem->first_child;column_elem != NULL;column_elem = column_elem->next){
		if(apr_strnatcmp(column_elem->name,"column") == 0){
			column = apr_array_push(table->columns);
			column->table = table;
			//Set column id
			status = get_xml_attr(dbd_config->pool,column_elem,"id",&column->id);
			if(status != 0){
				//No ID given
				return -5;
			}

			for(column_elem_child = column_elem->first_child;column_elem_child != NULL;column_elem_child = column_elem_child->next){
				if(apr_strnatcmp(column_elem_child->name,"name") == 0){
					 apr_xml_to_text(dbd_config->pool,column_elem_child,APR_XML_X2T_INNER,NULL,NULL,&(column->name),&max_element_size);
				}else if(apr_strnatcmp(column_elem_child->name,"fname") == 0){
					apr_xml_to_text(dbd_config->pool,column_elem_child,APR_XML_X2T_INNER,NULL,NULL,&(column->freindly_name),&max_element_size);
				}
			}

			//Set column foreign key for joining tables in queries
			status = get_xml_attr(dbd_config->pool,column_elem,"fk", &table->foreign_key);
			if (status == 0){
				table->foreign_key = apr_pstrcat(dbd_config->pool,table->foreign_key," = ",table->name,".",column->name,NULL);
			}
		}
	}
	return 0;
}


int read_tables(apr_xml_elem* tables,db_config* dbd_config){
	apr_xml_elem* table_elem;
	apr_xml_elem* table_elem_child;
	table_t* table;

	int status;

	apr_size_t max_element_size = 255;

	if(apr_strnatcmp(tables->name,"tables")!=0){
		//Not a tables element
		return -1;
	}
	dbd_config->tables = apr_array_make(dbd_config->pool,10,sizeof(table_t));
	for (table_elem = tables->first_child;table_elem != NULL; table_elem = table_elem->next){
		if(apr_strnatcmp(table_elem->name,"table") == 0){
				table = apr_array_push(dbd_config->tables);
				get_xml_attr(dbd_config->pool,table_elem,"id",&table->id);
				for(table_elem_child = table_elem->first_child;table_elem_child != NULL; table_elem_child = table_elem_child->next){
					if(apr_strnatcmp(table_elem_child->name,"name") == 0){
						apr_xml_to_text(dbd_config->pool,table_elem_child,APR_XML_X2T_INNER,NULL,NULL,&(table->name),&max_element_size);
					}else if(apr_strnatcmp(table_elem_child->name,"columns") == 0){
						status = read_columns(table,table_elem_child,dbd_config);
						if(status != 0){
							return -7;
						}
					}
				}
		}
	}

	return 0;
}

int find_query_by_id(query_t** element, apr_array_header_t*array, const char* id){
	int i;
	for(i = 0;i < array->nelts;i++){
		if(apr_strnatcasecmp(APR_ARRAY_IDX(array, i, query_t).id,id)==0){
			*element = &(APR_ARRAY_IDX(array, i, query_t));
			return 0;
		}
	}
	return -1;
}

int find_table_by_id(table_t** element, apr_array_header_t*array, const char* id){
	int i;
	for(i = 0;i < array->nelts;i++){
		if(apr_strnatcasecmp(APR_ARRAY_IDX(array, i, table_t).id,id)==0){
			*element = &(APR_ARRAY_IDX(array, i, table_t));
			return 0;
		}
	}
	return -1;
}

int find_column_by_id(column_table_t** element, apr_array_header_t*array, const char* id){
	int i;
	for(i = 0;i < array->nelts;i++){
		if(apr_strnatcasecmp(APR_ARRAY_IDX(array, i, column_table_t).id,id)==0){
			*element = &(APR_ARRAY_IDX(array, i, column_table_t));
			return 0;
		}
	}
	return -1;
}

int find_arrary_element_by_id(void** element, apr_array_header_t*array, const char* id){
	int i;
	for(i = 0;i < array->nelts;i++){
		if(apr_strnatcasecmp(APR_ARRAY_IDX(array, i, table_t).id,id)==0){
			*element = &(APR_ARRAY_IDX(array, i, table_t));
			return 0;
		}
	}
	return -1;
}

int find_app_by_id(app_list_t* app_list,const char* id,app_t** app){
	app_node_t* app_node;
	for(app_node = app_list->first_node;app_node != NULL;app_node = app_node->next){
		if(apr_strnatcasecmp(id,app_node->app->id) == 0){
			*app = app_node->app;
			return 0;
		}
	}
	return -1;
}

int find_select_column_from_query_by_table_id_and_query_id(column_table_t** select_column,query_t* query, const char* table_id, const char* column_id){
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
int find_column_from_query_by_friendly_name(query_t* query,const char* friendly_name, column_table_t** column){
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

int generate_queries(app_list_t* app_list,apr_xml_elem* queries,db_config* dbd_config){
	apr_xml_elem* query_elem;
	apr_xml_elem* table_elem;
	apr_xml_elem* col_elem;
	apr_xml_elem* app_elem;
	apr_xml_elem* query_child_elem;
	apr_xml_elem* parameter_elem;
	apr_xml_elem* parameter_child_elem;

	const char* table_id = NULL;
	const char* column_id = NULL;
	char* app_id = NULL;

	query_t* query;
	table_t* table;
	table_t** table_ptr;
	column_table_t* column;
	column_table_t** column_ptr;
	int status;

	app_t* app;

	apr_size_t max_element_size = 512;


	//Read each app query and custom parameters
	for(app_elem = queries->first_child; app_elem != NULL;app_elem = app_elem->next){
		get_xml_attr(dbd_config->pool,app_elem,"id",&app_id);
		find_app_by_id(app_list,app_id,&app);
		app->db_queries = apr_array_make(dbd_config->pool,10,sizeof(query_t));
		//Read queries
		for(query_elem=app_elem->first_child;query_elem != NULL; query_elem = query_elem->next){
			if(apr_strnatcmp(query_elem->name,"query") == 0){
				query = apr_array_push(app->db_queries);
				get_xml_attr(dbd_config->pool,query_elem,"id",&query->id);
				query->select_columns = apr_array_make(dbd_config->pool,10,sizeof(column_table_t*));
				query->tables = apr_array_make(dbd_config->pool,10,sizeof(table_t*));

				//Read tables used in query
				for(query_child_elem = table_elem = query_elem->first_child;table_elem != NULL; query_child_elem = table_elem = table_elem->next){
					if(apr_strnatcmp(table_elem->name,"table") == 0){
						get_xml_attr(dbd_config->pool,table_elem,"id",&table_id);
						//Find table id in known tables
						status = find_table_by_id(&table,dbd_config->tables,table_id);
						if(status != APR_SUCCESS){
							//no table found
							return -1;
						}
						//Add table to query

						//Create table join string
						query->table_join_string = (query->table_join_string) ? apr_pstrcat(dbd_config->pool,query->table_join_string,  " LEFT JOIN ", table->name," ON ", table->foreign_key, NULL) : table->name;
						//Add table point to table list
						table_ptr = (table_t**)apr_array_push(query->tables);
						*table_ptr = table;

						//Read columns used in Select statement of Query
						for(col_elem = table_elem->first_child;col_elem != NULL;col_elem = col_elem->next){
							if(apr_strnatcmp(col_elem->name,"column") == 0){
								const char* table_column_name;
								status = get_xml_attr(dbd_config->pool,col_elem,"id",&column_id);
								if(status != APR_SUCCESS){
									//no column id found
									return -2;
								}
								status = find_column_by_id(&column,table->columns,column_id);
								if(status != APR_SUCCESS){
									//no column found
									add_error_list(dbd_config->database_errors,ERROR,"Database Query Config", apr_psprintf(dbd_config->pool,"Couldn't find coulmn with id:%s",column_id));
									return -3;
								}
								//Add column to column select array
								column_ptr = (column_table_t**)apr_array_push(query->select_columns);
								*column_ptr = column;

								//Append column name to Select Columns String
								table_column_name = apr_pstrcat(dbd_config->pool,table->name,".",column->name,NULL);
								 query->select_columns_string = (query->select_columns_string) ? apr_pstrcat(dbd_config->pool,query->select_columns_string,",",table_column_name,NULL) : table_column_name;
							}
						}
					}else if(apr_strnatcmp(query_child_elem->name,"group_by") == 0){
						apr_xml_to_text(dbd_config->pool,table_elem,APR_XML_X2T_INNER,NULL,NULL,&(query->group_by_string),&max_element_size);
					}else if(apr_strnatcmp(query_child_elem->name,"custom_parameters") == 0){

						query->custom_parameters =  apr_array_make(dbd_config->pool,5,sizeof(custom_parameter_t));

						for(parameter_elem = query_child_elem->first_child;parameter_elem  != NULL; parameter_elem  = parameter_elem ->next){
							custom_parameter_t* custom_parameter = apr_array_push(query->custom_parameters);
							for(parameter_child_elem = parameter_elem->first_child;parameter_child_elem != NULL; parameter_child_elem = parameter_child_elem->next){
								if(apr_strnatcmp(parameter_child_elem->name, "fname") == 0){
									apr_xml_to_text(dbd_config->pool,parameter_child_elem,APR_XML_X2T_INNER,NULL,NULL,&(custom_parameter->freindly_name),&max_element_size);
								}else if(apr_strnatcmp(parameter_child_elem->name, "type") == 0){
									apr_xml_to_text(dbd_config->pool,parameter_child_elem,APR_XML_X2T_INNER,NULL,NULL,&(custom_parameter->type),&max_element_size);
								}
							}
						}
					}
				}
			}
		}
		//Pre allocate space for prepared statements
		//allocate_app_prepared_statments(dbd_config->pool, app);
	}
	return 0;
}


int init_db_schema(app_list_t* app_list,char* file_path, db_config* dbd_config){
	apr_xml_parser* xml_parser;
	apr_xml_doc* xml_doc;
	apr_xml_elem* xml_elem;

	apr_file_t* xml_file;

	apr_status_t status;

	int error_num;

	 status = apr_file_open(&xml_file, file_path,APR_READ,APR_OS_DEFAULT ,dbd_config->pool);
	 if(status != APR_SUCCESS){
		 	add_error_list(dbd_config->database_errors,ERROR,"Error opening DB XML file","ERRRORROROOROR");
		 	return -1;
	 }
	 xml_parser = apr_xml_parser_create(dbd_config->pool);

	status = apr_xml_parse_file(dbd_config->pool, &xml_parser,&xml_doc,xml_file,20480);
	 if(status != APR_SUCCESS){
		 add_error_list(dbd_config->database_errors,ERROR,"Error parsing DB XML file","ERRRORROROOROR");
		 return -1;
	 }

	 if(apr_strnatcmp(xml_doc->root->name, "db") != 0){
		 add_error_list(dbd_config->database_errors,ERROR,"Invalid XML FILE","ERRRORROROOROR");
		 return -1;
	 }

	 for(xml_elem = xml_doc->root->first_child;xml_elem != NULL;xml_elem = xml_elem->next){
		 //Set up Database table first
		 if(apr_strnatcmp(xml_elem->name,"tables") ==0){
			 error_num = read_tables(xml_elem, dbd_config);
			 if(error_num != 0){
				 return error_num;
			 }
		 }
		 //Then process multi column queries
		 if(apr_strnatcmp(xml_elem->name,"queries") ==0){
		 	error_num = generate_queries(app_list,xml_elem, dbd_config);
		 	if(error_num != 0){
		 		return error_num;
		 	}
		 }
	 }
	return 0;
}
