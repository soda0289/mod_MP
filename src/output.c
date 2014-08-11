
#include "output.h"


int output_init_bb (apr_pool_t* pool, ap_filter_t* out_filters,output_t** output_ptr){

	output_t* output = *output_ptr = apr_pcalloc(pool, sizeof(output_t));

	//Prepare Output
	output->pool = pool;
	output->filters = out_filters;
	output->headers = apr_table_make(pool, 10);
	output->content_type = apr_pcalloc(pool, sizeof(char) * 255);
	output->bucket_allocator = apr_bucket_alloc_create(pool);

	output->error_messages = apr_pcalloc(pool, sizeof(error_messages_t));
	output->error_messages->num_errors = 0;

	output->bucket_brigade = apr_brigade_create(pool, output->bucket_allocator);
	output->length = 0;
	
	return 0;
}

int output_finalize_bb (output_t* output, apr_table_t* out_headers){
	apr_status_t rv;

	apr_bucket* eos_bucket;
	eos_bucket = apr_bucket_eos_create(output->bucket_allocator);

	APR_BRIGADE_INSERT_TAIL(output->bucket_brigade, eos_bucket);

	apr_table_overlap(out_headers, output->headers, APR_OVERLAP_TABLES_SET);

	rv = apr_brigade_length(output->bucket_brigade,1,&(output->length));

	return rv;
}

int output_status_json(output_t* output){
	apr_table_add(output->headers,"Access-Control-Allow-Origin", "*");
	apr_cpystrn((char*)output->content_type, "application/json", 255);

	apr_brigade_puts(output->bucket_brigade, NULL,NULL, "{\n");

	error_messages_print_json_bb(output->error_messages, output->pool,output->bucket_brigade);

	apr_brigade_puts(output->bucket_brigade, NULL,NULL,"\n}\n");

	return 0;
}
