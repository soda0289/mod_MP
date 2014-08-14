#include "indexer.h"
#include "error_messages.h"
#include "database/db_manager.h"
#include "database/db_object.h"

static const char* INDEXER_ERROR_HEADER = "Indexer Error";

static int indexer_read_xml_config(indexer_t* indexer, db_manager_t* db_manager, const char* xml_config_file_path){
	error_messages_t* error_messages = indexer->error_messages;

	apr_file_t* xml_file;

	apr_status_t status;

	apr_pool_t* pool = indexer->pool;

	indexer->db_objects = apr_array_make(pool, 16, sizeof(db_object_t));
	
	status = apr_file_open(&xml_file, xml_config_file_path,APR_READ,APR_OS_DEFAULT ,pool);
	if (status != APR_SUCCESS) {
		error_messages_addf(error_messages, ERROR, INDEXER_ERROR_HEADER, "Check file permission or path (%s)", xml_config_file_path);
		return -1;
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




