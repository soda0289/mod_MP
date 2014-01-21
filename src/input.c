#include <apr.h>
#include "apps/music/tag_reader.h"
#include "input.h"

int init_input(apr_pool_t* pool, const char* uri, int method_num, input_t** input_ptr){
	input_t* input = *input_ptr = apr_pcalloc(pool, sizeof(input_t));

	//Preapre Input Struct
	input->pool = pool;
	input->method = method_num;
	input->uri = uri;

	input->files = apr_array_make(pool, 50, sizeof(file_t));
	return 0;
}

