#include <apr.h>
#include <apr_strings.h>
#include "input.h"
#include "upload.h"

static int get_indexer_from_uri(apr_pool_t* pool,
				const char* uri,
				const int uri_length,
				const char** indexer_string_ptr)
{
	const char* slash_index;
	const char* uri_start = uri + 1;

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

static int get_command_from_uri(apr_pool_t* pool,
				const char* uri,
				const int uri_length,
				const char** command_string_ptr)
{
	const char* uri_start = uri + 1;
	const char* slash_index;

	//First slash after inital character
	slash_index = strchr((char*) uri_start, '/');	

	if (slash_index == NULL) {
		return -1;
	}

	if (slash_index > uri) {
		const char* param_start_index;
		int size;

		param_start_index = strchr((char*) uri_start, '?');
		if(param_start_index)
			size = param_start_index - (slash_index + 1);
		else
			size = uri_length - ((slash_index + 1) - uri);
			
		*command_string_ptr = apr_pstrmemdup(pool, slash_index + 1, size);
	}

	return 0;
}

static int get_parameters_from_uri(apr_pool_t* pool,
				   const char* uri,
				   const size_t uri_length,
				   apr_table_t** parameter_strings_ptr)
{
	const char* query_start_index;
	const char* param_index;
	apr_table_t* parameter_strings;

	query_start_index = memchr(uri, '?', uri_length);

	if (query_start_index == NULL) {
		return -1;
	}

	parameter_strings = *parameter_strings_ptr = apr_table_make(pool, 10);

	//Check for both & and ; to indicate end of query parameter
	for(param_index = query_start_index + 1; param_index && param_index < (uri_length + uri); param_index++) {
		size_t param_len;
		size_t param_name_len;

		const char* param_name;
		const char* param_value;

		const char* equal_index;

		param_len = strcspn(param_index, "&;");
		if(!param_len)
			continue;

		param_name_len = (equal_index = memchr((char*) param_index, '=', param_len)) ? (size_t) (equal_index - param_index) : param_len;

		param_name = apr_pstrndup(pool, param_index, param_name_len);
		if(param_len - param_name_len)
			param_value = apr_pstrndup(pool, (param_index + param_name_len) + 1 , param_len - param_name_len - 1);
		else
			param_value = "";

		apr_table_mergen(parameter_strings, param_name, param_value);

		param_index += param_len;
	}
	return 0;
}

int input_init(apr_pool_t* pool, const char* uri, int method_num, input_t** input_ptr){
	input_t* input = *input_ptr = apr_pcalloc(pool, sizeof(input_t));

	//Prepare Input Struct
	input->pool = pool;
	input->method = method_num;
	input->uri = uri;

	if (uri) {
		const size_t uri_length = strlen(uri);
		

		get_indexer_from_uri(pool, uri, uri_length, &(input->indexer_string));
		get_command_from_uri(pool, uri, uri_length, &(input->command_string));
		get_parameters_from_uri(pool, uri, uri_length, &(input->parameter_strings));
	}

	return 0;
}

