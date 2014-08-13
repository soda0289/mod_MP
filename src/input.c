#include <apr.h>
#include <apr_strings.h>
#include "input.h"
#include "upload.h"

static int get_indexer_from_uri(apr_pool_t* pool, const char* uri, const char** indexer_string_ptr) {
	const char* slash_index;
	const char* uri_start = uri + 1;
	int uri_length = strlen(uri);

	//Find first slash
	slash_index = strchr((char*) uri_start, '/');	
	
	if (slash_index == NULL || (int) (slash_index - uri_start) > uri_length) {
		slash_index = (const char*) (uri + uri_length);
	}

	if (slash_index > uri_start) {
		int size = (slash_index - uri_start);
		*indexer_string_ptr = apr_pstrmemdup(pool, uri_start, size);
	}else{
		return -1;
	}

	return 0;
}

static int get_command_from_uri(apr_pool_t* pool, const char* uri, const char** command_string_ptr) {
	const char* uri_start = uri + 1;
	const char* slash_index;
	int uri_length = strlen(uri);

	//First slash after inital character
	slash_index = strchr((char*) uri_start, '/');	

	if (slash_index == NULL) {
		return -1;
	}

	if (slash_index > uri) {
		int size = uri_length - ((slash_index + 1) - uri);
		*command_string_ptr = apr_pstrmemdup(pool, slash_index + 1, size);
	}

	return 0;
}

int input_init(apr_pool_t* pool, const char* uri, int method_num, input_t** input_ptr){
	input_t* input = *input_ptr = apr_pcalloc(pool, sizeof(input_t));

	//Preapre Input Struct
	input->pool = pool;
	input->method = method_num;
	input->uri = uri;

	get_indexer_from_uri(pool, uri, &(input->indexer_string));
	get_command_from_uri(pool, uri, &(input->command_string));

	return 0;
}

