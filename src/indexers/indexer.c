#include "indexer.h"
#include "error_messages.h"
#include "database/db_manager.h"
#include "database/db_object.h"

static int indexer_read_xml_config(indexer_t* indexer, db_manager_t* db_manager, const char* xml_config_file_path){
	error_messages_t* error_messages = indexer->error_messages;
	apr_xml_parser* xml_parser;
	apr_xml_doc* xml_doc;
	apr_xml_elem* xml_elem;

	apr_file_t* xml_file;

	apr_status_t status;

	apr_pool_t* pool = indexer->pool;
	apr_size_t max_element_size = 255;

	indexer->db_objects = apr_array_make(pool, 16, sizeof(db_object_t));
	
	status = apr_file_open(&xml_file, xml_config_file_path,APR_READ,APR_OS_DEFAULT ,pool);
	if(status != APR_SUCCESS){
		 	error_messages_add(error_messages,ERROR,"Error opening Database XML file","Check file permission or path");
		 	return -1;
	}
	xml_parser = apr_xml_parser_create(pool);

	status = apr_xml_parse_file(pool, &xml_parser,&xml_doc,xml_file,20480);
	if(status != APR_SUCCESS){
		 error_messages_add(error_messages,ERROR,"Error parsing Database XML file","Verify the XML syntax.");
		 return -1;
	}

	if(apr_strnatcmp(xml_doc->root->name, "indexer") != 0){
		 error_messages_add(error_messages,ERROR,"Invalid XML FILE","No element 'app' found as root.");
		 return -1;
	}

	for(xml_elem = xml_doc->root->first_child;xml_elem != NULL;xml_elem = xml_elem->next){
		//db_params_t* db_params;

		//Handle a database/queries for application
		if(apr_strnatcmp(xml_elem->name,"database") ==0){
			//get_xml_attr(pool,xml_elem, "db_id", &db_id);
		//Handle Friendly name for application
		}else if(apr_strnatcmp(xml_elem->name,"fname") ==0){
			apr_xml_to_text(pool,xml_elem,APR_XML_X2T_INNER,NULL,NULL,&(indexer->friendly_name),&max_element_size);
		}
	}

	apr_file_close(xml_file);
	return 0;
}

int indexer_init(apr_pool_t* pool, error_messages_t* error_messages, db_manager_t* db_manager, const char* indexer_id, const char* xml_config_path, const indexer_callbacks_t* indexer_callbacks, indexer_t** indexer_ptr){
	indexer_t* indexer = *indexer_ptr = apr_pcalloc(pool, sizeof(indexer_t));
	
	indexer->pool = pool;
	indexer->error_messages = error_messages;
	indexer->data = NULL;
	indexer->id = apr_pstrdup(pool, indexer_id);

	//Set callback functions
	indexer->callbacks = indexer_callbacks;

	indexer_read_xml_config(indexer, db_manager, xml_config_path);

	return 0;
}




