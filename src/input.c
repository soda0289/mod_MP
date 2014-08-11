#include <apr.h>
#include "input.h"
#include "upload.h"

int input_init(apr_pool_t* pool, const char* uri, int method_num, input_t** input_ptr){
	input_t* input = *input_ptr = apr_pcalloc(pool, sizeof(input_t));

	//Preapre Input Struct
	input->pool = pool;
	input->method = method_num;
	input->uri = uri;

	return 0;
}

