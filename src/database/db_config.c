/*
 *  db_config.c
 *
 *  Created on: Aug 4, 2013
 *  Author: Reyad Attiyat
 *	Copyright 2013 Reyad Attiyat
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
#include "db_query_config.h"


int find_db_by_id(apr_array_header_t* db_arrays, const char* db_id, db_params_t** db_params_ptr){
	int i;
	
	db_params_t* db_params = NULL;

	for(i = 0; i < db_arrays->nelts; i++){
		db_params = &(((db_params_t*)db_arrays->elts)[i]);
		if(apr_strnatcmp(db_params->id, db_id) == 0){
			*db_params_ptr = db_params;
			return 0;
		}
	}
	return -1;
}

static int read_columns(apr_pool_t* pool, table_t* table,apr_xml_elem* table_elem){
	apr_xml_elem* column_elem;
	apr_xml_elem* column_elem_child;
	column_table_t* column;
	apr_size_t max_element_size = 255;

	int status;

	table->columns = apr_array_make(pool,10,sizeof(column_table_t));
	for(column_elem = table_elem->first_child;column_elem != NULL;column_elem = column_elem->next){
		if(apr_strnatcmp(column_elem->name,"column") == 0){
			column = apr_array_push(table->columns);
			column->table = table;
			//Set column id
			status = get_xml_attr(pool,column_elem,"id",&column->id);
			if(status != 0){
				//No ID given
				return -5;
			}

			for(column_elem_child = column_elem->first_child;column_elem_child != NULL;column_elem_child = column_elem_child->next){
				if(apr_strnatcmp(column_elem_child->name,"name") == 0){
					 apr_xml_to_text(pool,column_elem_child,APR_XML_X2T_INNER,NULL,NULL,&(column->name),&max_element_size);
				}else if(apr_strnatcmp(column_elem_child->name,"fname") == 0){
					apr_xml_to_text(pool,column_elem_child,APR_XML_X2T_INNER,NULL,NULL,&(column->freindly_name),&max_element_size);
				}else if(apr_strnatcmp(column_elem_child->name,"type") == 0){
					const char* type_string;
					apr_xml_to_text(pool,column_elem_child,APR_XML_X2T_INNER,NULL,NULL,&(type_string),&max_element_size);
					if(apr_strnatcmp(type_string, "int") == 0){
						column->type = INT;
					}else if(apr_strnatcmp(type_string, "varchar") == 0){
						column->type = VARCHAR;
					}else if(apr_strnatcmp(type_string, "datetime") == 0){
						column->type = DATETIME;
					}else if(apr_strnatcmp(type_string, "bigint") == 0){
						column->type = BIGINT;
					}

				}
			}

			//Set column foreign key for joining tables in queries
			status = get_xml_attr(pool,column_elem,"fk", &table->foreign_key);
			if (status == 0){
				table->foreign_key = apr_pstrcat(pool,table->foreign_key," = ",table->name,".",column->name,NULL);
			}
		}
	}
	return 0;
}


static int read_tables(apr_pool_t* pool, apr_xml_elem* xml_tables, apr_array_header_t** tables_ptr){
	apr_xml_elem* table_elem;
	apr_xml_elem* table_elem_child;
	table_t* table;

	apr_array_header_t* tables;

	int status;

	apr_size_t max_element_size = 255;

	if(apr_strnatcmp(xml_tables->name,"tables")!=0){
		//Not a tables element
		return -1;
	}

	tables = *tables_ptr = apr_array_make(pool,10,sizeof(table_t));
	for (table_elem = xml_tables->first_child;table_elem != NULL; table_elem = table_elem->next){
		if(apr_strnatcmp(table_elem->name,"table") == 0){
			table = apr_array_push(tables);
			get_xml_attr(pool,table_elem,"id",&table->id);
			for(table_elem_child = table_elem->first_child;table_elem_child != NULL; table_elem_child = table_elem_child->next){
				if(apr_strnatcmp(table_elem_child->name,"name") == 0){
					apr_xml_to_text(pool,table_elem_child,APR_XML_X2T_INNER,NULL,NULL,&(table->name),&max_element_size);
				}else if(apr_strnatcmp(table_elem_child->name,"columns") == 0){
					status = read_columns(pool, table,table_elem_child);
					if(status != 0){
						return -7;
					}
				}
			}
		}
	}

	return 0;
}

int read_db_parameters(apr_pool_t* pool, apr_xml_elem* xml_parameters, db_params_t* db_params){
	apr_size_t max_element_size = 255;
	apr_xml_elem* param_elem;

	for(param_elem = xml_parameters->first_child; param_elem != NULL; param_elem = param_elem->next){
		if(apr_strnatcmp(param_elem->name, "driver") == 0){
			apr_xml_to_text(pool,param_elem,APR_XML_X2T_INNER,NULL,NULL,&(db_params->driver_name),&max_element_size);
		}else if(apr_strnatcmp(param_elem->name, "hostname") == 0){
			apr_xml_to_text(pool,param_elem,APR_XML_X2T_INNER,NULL,NULL,&(db_params->hostname),&max_element_size);
		}else if(apr_strnatcmp(param_elem->name, "username") == 0){
			apr_xml_to_text(pool,param_elem,APR_XML_X2T_INNER,NULL,NULL,&(db_params->username),&max_element_size);
		}else if(apr_strnatcmp(param_elem->name, "password") == 0){
			apr_xml_to_text(pool,param_elem,APR_XML_X2T_INNER,NULL,NULL,&(db_params->password),&max_element_size);
		}
	}

	return 0;
}



int read_db_conf(apr_pool_t* pool,apr_xml_elem* db_xml_elem, db_params_t* db_params, error_messages_t* error_messages){
	apr_xml_elem* xml_elem;

	int error_num;

	get_xml_attr(pool, db_xml_elem, "id", &(db_params->id));	
	for(xml_elem = db_xml_elem->first_child;xml_elem != NULL;xml_elem = xml_elem->next){
	 	//Setup Database parameters used for connecting
		if(apr_strnatcmp(xml_elem->name,"parameters") ==0){
			error_num = read_db_parameters(pool, xml_elem, db_params);
			if(error_num != 0){
				return error_num;
			}
		}
		//Setup Database table first
		if(apr_strnatcmp(xml_elem->name,"tables") ==0){
			error_num = read_tables(pool, xml_elem, &(db_params->tables));
			if(error_num != 0){
				 return error_num;
			}
		}
	}

	return 0;
}

int init_db_array(apr_pool_t* pool, const char* xml_db_dir, apr_array_header_t** db_array_ptr,error_messages_t* error_messages){
	int status = 0;
	apr_status_t rv = 0;
	apr_array_header_t* db_array;

	char xml_parse_error[255];
	const char* error_header = "Error init database array";
	
	apr_xml_parser* xml_parser;
	apr_xml_doc* xml_doc;
	apr_xml_elem* xml_elem;
	apr_xml_elem* db_elem;

	apr_dir_t* xml_dir;
	apr_file_t* xml_file;
	apr_finfo_t finfo;


	db_params_t* db_params;
	
	rv = apr_dir_open(&xml_dir, xml_db_dir, pool);
	if(rv != APR_SUCCESS){
		add_error_list(error_messages,ERROR,error_header,"Error opening database XML directory");
		return -5;
	}
	
	db_array = *db_array_ptr = apr_array_make(pool, 2, sizeof(db_params_t));
	while((rv = apr_dir_read(&finfo, APR_FINFO_MIN | APR_FINFO_NAME, xml_dir)) != APR_ENOENT){
		
		if(rv == APR_SUCCESS && finfo.name[0] != '.'){
			const char* file_path;
			file_path = apr_pstrcat(pool,xml_db_dir,"/", finfo.name, NULL);
			
			status = apr_file_open(&xml_file, file_path,APR_READ,APR_OS_DEFAULT ,pool);
			if(status != APR_SUCCESS){
					add_error_list(error_messages,ERROR,error_header, "Error opening DB XML file");
					return -1;
			}
			xml_parser = apr_xml_parser_create(pool);

			status = apr_xml_parse_file(pool, &xml_parser,&xml_doc,xml_file,20480);
			if(status != APR_SUCCESS){
				apr_xml_parser_geterror(xml_parser,(char*) &xml_parse_error, 255);
				add_error_list(error_messages,ERROR, error_header, xml_parse_error);
				return -1;
			}
		
			db_params = (db_params_t*)apr_array_push(db_array);
			xml_elem = xml_doc->root;
			if(apr_strnatcmp(xml_elem->name, "databases") == 0){
				//Multiple databases in this file
				for(db_elem = xml_doc->root->first_child; db_elem != NULL; db_elem = db_elem->next){
					read_db_conf(pool, db_elem, db_params, error_messages);	
				}
			
			}else if(apr_strnatcmp(xml_elem->name,"db") == 0){
				read_db_conf(pool, xml_doc->root, db_params, error_messages);	
			}

			apr_file_close(xml_file);
		}

	}

	apr_dir_close(xml_dir);


	return 0;
}
