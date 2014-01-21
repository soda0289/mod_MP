#include <apr.h>
#include <httpd.h>
#include "input.h"
#include "mod_mediaplayer.h"
#include "error_handler.h"
#include "apps/music/tag_reader.h"
#include <string.h>
#include <stdio.h>

extern module AP_MODULE_DECLARE_DATA mediaplayer_module;

#define NO_BOUNDARY 0
#define PART_BOUNDARY 1
#define FINAL_BOUNDARY 2



int check_line_boundary(const char* line, apr_size_t len, const char* boundary){
		int i = 0;

		//Check if boundry
		if(line[0] == '-' && line[1] == '-'){
			for(i = 0;i < strlen(boundary); i++){
				if(line[i+2] != boundary[i]){
					return NO_BOUNDARY;
				}
			}
			//Check for final boundary
			if((line[i] == '-' && line[i+1] == '-' )&& i+2 == len){
				return FINAL_BOUNDARY;
			}else{
				return PART_BOUNDARY;
			}
		}
	return NO_BOUNDARY;	
}

int write_line_file(file_t* file, input_t* in, const char* line, apr_size_t len){
	const char* error_header = "Error writing line to file";
	int status = 0;
	apr_size_t length = len;

	if(file == NULL){
		//No file given ignore
		return -1;
	}

	if(in->file_d == NULL){
		status = apr_file_open(&(in->file_d), file->path, APR_WRITE | APR_CREATE, APR_OS_DEFAULT, in->pool);
		}else{
		status = apr_file_open(&(in->file_d), file->path, APR_WRITE | APR_APPEND, APR_OS_DEFAULT, in->pool);
	}
	if (status != APR_SUCCESS) { 
		add_error_list(in->error_messages, ERROR,error_header,"Error opening file");
	}

	status = apr_file_write(in->file_d, line, &length);


	if (status != APR_SUCCESS || length < len) { 
		add_error_list(in->error_messages, ERROR,error_header,"Error writing to file");
	}
	status = apr_file_close(in->file_d);
	in->file_d = (apr_file_t*)1; //NOT NULL
	
	return status;
}

int read_line_headers(file_t* file, input_t* in, const char* line, apr_size_t len){
	int status = 0;

	char* header_field;
	char* header_name;

	const char* colon;
	//Check if header exsits would contain : (colon)
	colon = strchr(line, ':');
	if(colon == NULL){
		return -1;
	}

	//Header name is from the begining of line to the colon
	header_name = apr_pstrndup(in->pool, line, colon-line); 

	header_field = apr_pstrndup(in->pool, colon+1, len - (colon-line));

	if(apr_strnatcasecmp("Content-Type", header_name) == 0){
		char* slash;

		slash = strchr(header_field, '/');

		if(slash != NULL){
		
		file->type_string = apr_pstrndup(in->pool, slash+1, 255);
		}

	}else if(apr_strnatcasecmp("Content-Disposition", header_name) == 0){
		const char* field = header_field;
		char* param;
		char* param_value;
		char* param_name;
		const char* semicolon;
		int field_len = strlen(field);

		for(semicolon = strchr(field, ';'); field < field_len + header_field; field = (semicolon+1)){
			int str_len;

			if((semicolon = strchr(field, ';')) == NULL){
				semicolon = strchr(field, '\0');
			}
			str_len = semicolon-field;
			

			param = apr_pstrndup(in->pool, field, str_len);
			
			if(isspace(param[0])){
				param++;
			}

			param_value = strchr(param, '=') + 1;
			if(param_value != NULL && param_value > param){
				*(param_value -1) = '\0';
			}

			param_name = param;


			if(apr_strnatcmp(param_name, "filename") == 0){
				if(param_value[0] == '\"' && param_value[strlen(param_value)-2] == '\"'){
					param_value++;
					param_value[strlen(param_value)-2] = '\0';
				}
				//Set filename
				file->path = apr_pstrcat(in->pool, "/tmp/", param_value, NULL);
			}

		}	
	}
	return status;
}

int process_input_data(input_t* in,const char* data, apr_size_t length){
	int status = 0;
	
	apr_pool_t* pool;

	const char* line;
	const char* end_line;

	file_t* file = NULL;

	status = apr_pool_create(&pool, in->pool);
	if(status != APR_SUCCESS){
		return status;
	}

	//Read data by line
	for(line = data, end_line = strstr(line, "\r\n"); line < (data + length); line = (end_line + 2), end_line = strstr(line, "\r\n")){	
		apr_size_t str_len;

		//Check if we couldn't find \r\n
		if(end_line == NULL){
			end_line = (length + data);
		}

		//Check line length
		str_len =  end_line - line;
		if(str_len == 0){
			//We hit new area
			in->headers_over = 1;
			continue;
		}

		//Check if this line is a boundary
		switch(check_line_boundary(line, str_len, in->boundary)){
			case PART_BOUNDARY :
				//Each boundry indicates a new possible file
				file = apr_array_push(in->files);
				in->headers_over = 0;

				break;
			case FINAL_BOUNDARY :
				//Close down

				break;
			case NO_BOUNDARY :
				if(in->headers_over == 0){
					read_line_headers(file, in, line, str_len);
				}else if(in->headers_over == 1){
					write_line_file(file, in, line, (length - (line-data) > str_len+2) ? str_len+2 : length - (line-data));
				}
				break;
		}
	}

	apr_pool_destroy(pool);

	return status;
}

int content_type_header(input_t* input, apt_table_t* headers_in){
	char* content_type;
	char* semicolon;
	char* equal;


	//Get content type/mime boundry
	content_type = apr_table_get(headers_in, "Content-Type");
	if(content_type != NULL){
		//Check if there is a subtype/boundry
		if((semicolon = strchr(content_type, ';')) != NULL){
			content_type = apr_pstrndup(input->pool, content_type, (int)(semicolon - content_type));
			input->boundary = apr_pstrndup(input->pool, semicolon+1, 255);

			if(isspace(input->boundary[0])){
				(input->boundary)++;
			}

			if((equal = strchr(input->boundary, '=')) != NULL){
				*equal = '\0';
				
				if(apr_strnatcmp(input->boundary,"boundary") == 0){
					input->boundary = equal+1;
					return 0;
				}else{
					input->boundary = NULL;
					return -2;
				}
			}
		}else{
			return -3;
		}
	}

	return -1;
}

apr_status_t upload_filter(ap_filter_t *filter, apr_bucket_brigade *bbout, ap_input_mode_t mode, apr_read_type_e block, apr_off_t nbytes){

	int status = 0;	
	mediaplayer_srv_cfg* srv_conf;
	apr_bucket *e = NULL;

	const char*  error_header = "Error uploading file";
	char error_message[255];
	const char* data;
	apr_size_t length;

	apr_bucket* next_bucket;
	input_t* input = ap_get_module_config(filter->r->request_config, &mediaplayer_module);
	
	srv_conf = ap_get_module_config(filter->r->server->module_config, &mediaplayer_module);

	apr_bucket_brigade* input_bb = apr_brigade_create(filter->r->pool,filter->r->connection->bucket_alloc);

	input->error_messages = srv_conf->error_messages;

	if(input->eos != 0){
    	APR_BRIGADE_INSERT_TAIL(bbout, apr_bucket_eos_create(bbout->bucket_alloc));

		return 0;
	}

	do {
		status = ap_get_brigade(filter->next, input_bb, mode, block, nbytes);
		if(status != 0){
			add_error_list(srv_conf->error_messages, ERROR, error_header, apr_strerror(status, &(error_message[0]), 255));
			return status;	
		}


		for (e = APR_BRIGADE_FIRST(input_bb); e != APR_BRIGADE_SENTINEL(input_bb); e = next_bucket){
			//Set next bucket
			next_bucket = APR_BUCKET_NEXT(e);
			//Remove from brigade		
			APR_BUCKET_REMOVE(e);
			
			//Insert into out
			APR_BRIGADE_INSERT_TAIL(bbout, e);

			if (APR_BUCKET_IS_EOS(e)) {
				input->eos = 1;
    			APR_BRIGADE_INSERT_TAIL(bbout, apr_bucket_eos_create(bbout->bucket_alloc));
				return 0;	
			}

			//Skip metadata we cant read it
			if ( APR_BUCKET_IS_METADATA(e) ) {
				continue;	
			}

			//Read bucket
			status = apr_bucket_read(e, &data, &length, APR_BLOCK_READ);
			if (status != APR_SUCCESS && status != APR_EAGAIN) { 
				add_error_list(srv_conf->error_messages, ERROR,error_header, "Error reading bucket from input bukcet brigade");
			}

			process_input_data(input, data, length);
		}

		apr_brigade_cleanup(input_bb);
	} while (input->eos == 0);

	apr_brigade_destroy(input_bb);

	return 0;
 }
