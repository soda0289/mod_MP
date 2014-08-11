#include <apr.h>
#include <apr_pools.h>
#include <apr_dbd.h>

#include "error_messages.h"
#include "database/db_typedefs.h"
#include "database/db_manager.h"
#include "database/db_config.h"
#include "database/db_connection.h"

int db_manager_init (apr_pool_t* pool, error_messages_t* error_messages, const char* db_xml_config_dir, db_manager_t** db_manager_ptr) {
	apr_status_t rv;
	db_manager_t* db_manager = *db_manager_ptr = apr_pcalloc(pool, sizeof(db_manager_t));

	//Initialize APR Database system
	rv = apr_dbd_init(pool);
	if (rv != APR_SUCCESS){
	//	ap_log_error(APLOG_MARK, APLOG_ERR, status, s, "Failed to initialize APR DBD.");
	  return rv;
	}

	db_manager->db_config_list = apr_pcalloc(pool, sizeof(db_config_list_t));

	return 0;
}


int db_manager_on_fork (apr_pool_t* child_pool, error_messages_t* child_error_messages, db_manager_t** db_manager) {



	return 0;
}
/*
static int read_columns(apr_pool_t* pool, db_table_t* table,apr_xml_elem* table_elem){
	apr_xml_elem* column_elem;
	apr_xml_elem* column_elem_child;
	db_table_column_t* column;
	apr_size_t max_element_size = 255;

	int status;

	table->columns = apr_array_make(pool,10,sizeof(db_table_column_t));
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


static int read_table(apr_pool_t* pool, apr_xml_elem* table_elem, apr_array_header_t** tables_ptr){
	apr_xml_elem* table_elem_child;
	db_table_t* table;

	apr_array_header_t* tables;

	int status;

	apr_size_t max_element_size = 255;


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

	return 0;
}
*/
/* 
static int read_db_conn_params(apr_pool_t* pool, apr_xml_elem* xml_parameters, db_config_t* db_config){
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
static int read_db_conf(apr_pool_t* pool,apr_xml_elem* db_xml_elem, db_config_t* db_config, error_messages_t* error_messages){
	apr_xml_elem* xml_elem;

	int error_num;

	//Set Database ID
	get_xml_attr(pool, db_xml_elem, "id", &(db_config->id));	

	for(xml_elem = db_xml_elem->first_child;xml_elem != NULL;xml_elem = xml_elem->next){
	 	//Setup Database parameters used for connecting
		if(apr_strnatcmp(xml_elem->name,"connection") ==0){
			error_num = read_db_conn_params(pool, xml_elem, db_config);
			if(error_num != 0){
				return error_num;
			}
		}
		//Setup Database table first
		if(apr_strnatcmp(xml_elem->name,"tables") ==0){
			error_num = read_tables(pool, xml_elem, db_config);
			if(error_num != 0){
				 return error_num;
			}
		}
	}

	return 0;
}
*/
int db_manager_find_db (db_manager_t* db_manager, const char* db_id){
	/*
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


	rv = apr_dir_open(&xml_dir, xml_db_dir, pool);
	if(rv != APR_SUCCESS){
		error_messages_add(error_messages,ERROR,error_header,"Error opening database XML directory");
		return -5;
	}
	
	db_array = *db_array_ptr = apr_array_make(pool, 2, sizeof(database_t));

	while((rv = apr_dir_read(&finfo, APR_FINFO_MIN | APR_FINFO_NAME, xml_dir)) != APR_ENOENT){
		
		if(rv == APR_SUCCESS && finfo.name[0] != '.'){
			const char* file_path;
			file_path = apr_pstrcat(pool,xml_db_dir,"/", finfo.name, NULL);
			
			status = apr_file_open(&xml_file, file_path,APR_READ,APR_OS_DEFAULT ,pool);
			if(status != APR_SUCCESS){
					error_messages_add(error_messages,ERROR,error_header, "Error opening DB XML file");
					return -1;
			}
			xml_parser = apr_xml_parser_create(pool);

			status = apr_xml_parse_file(pool, &xml_parser,&xml_doc,xml_file,20480);
			if(status != APR_SUCCESS){
				apr_xml_parser_geterror(xml_parser,(char*) &xml_parse_error, 255);
				error_messages_add(error_messages,ERROR, error_header, xml_parse_error);
				return -1;
			}
			
			//Setup new Database
			new_db = (database_t*)apr_array_push(db_array);
			new_db->tables = *tables_ptr = apr_array_make(pool,10,sizeof(table_t));

			xml_elem = xml_doc->root;
			read_db_conf(pool, xml_doc->root, new_db, error_messages);	

			apr_file_close(xml_file);
		}

	}

	apr_dir_close(xml_dir);
	
	*/

	return 0;
}
