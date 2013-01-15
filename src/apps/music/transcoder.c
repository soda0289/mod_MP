/*
 * transcoder.c
 *
 *  Created on: Jan 11, 2013
 *     Author: Reyad Attiyat
 *		Copyright 2012 Reyad Attiyat
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

#include <stdlib.h>
#include "apr.h"
#include "apr_errno.h"
#include "apps/music/music_query.h"
#include "apps/music/ogg_encode.h"
#include "database/db_query_config.h"
#include "apps/music/flac.h"

void * APR_THREAD_FUNC encoder_thread(apr_thread_t* thread, void* ptr){
	int status;
	int count = 0;
	transcode_thread_t* transcode_thread = (transcode_thread_t*)ptr;
	apr_pool_t* pool = transcode_thread->pool;

	encoding_options_t enc_opt = {0};
	decoding_job_t* decoding_job;


	for(decoding_job = transcode_thread->queue->head;transcode_thread->queue->head != NULL; transcode_thread->queue->head = transcode_thread->queue->head->next){
		input_file_t* input_file = decoding_job->input_file;
		//Open input file and prepare it for processing
		status = input_file->open_input_file(pool,&(input_file->file_struct),input_file->file_path,&enc_opt);
		if(status != 0){
			add_error_list(transcode_thread->error_messages,ERROR,"Error opening input file","Error");
		}
		status = decoding_job->encoder_function(pool,input_file,&enc_opt,apr_pstrcat(pool,decoding_job->output_file_path,"-",apr_itoa(pool,count++),NULL));

		status = input_file->close_input_file(input_file->file_struct);

		//Write to database with new source
	}


	return 0;
}

int get_decoding_job_queue(decoding_job_t* job, decoding_job_t*head){

	return 0;
}

int add_decoding_job_queue(decoding_job_t* job,queue_t* queue){
	if(queue->tail==NULL ){
		queue->tail= job;
		queue->head = job;
	}else{
		queue->tail->next = job;
		queue->tail = job;
	}

	return 0;
}

int transcode_audio(request_rec* r, db_config* dbd_config,music_query_t* music_query){
	int status;
	apr_status_t rv;

	const char* file_path;
	const char* file_type;
	char* tmp_file_path;
	const char* temp_dir;
	mediaplayer_srv_cfg* srv_conf;

	custom_parameter_t* output_type_parameter;

	input_file_t* input_file;

	apr_thread_t* trans_thread;

	decoding_job_t* decoding_job = apr_pcalloc(r->server->process->pool,sizeof(decoding_job_t));

	rv = apr_temp_dir_get(&temp_dir,r->pool);
	if(rv != APR_SUCCESS){

		return -1;
	}

	status = file_path_query(dbd_config,music_query->query_parameters,music_query->db_query,&file_path,&file_type, music_query->error_messages);
	if(status != 0){

		return -4;
	}

	tmp_file_path = apr_pstrcat(r->pool, temp_dir,"/", ((query_where_condition_t*)music_query->query_parameters->query_where_conditions->elts)[0].condition,".ogg", NULL);

	decoding_job->output_file_path = tmp_file_path;
	input_file = apr_pcalloc(r->server->process->pool,sizeof(input_file_t));

	//Determine proper decoder
	if(apr_strnatcmp(file_type,"flac") == 0){
		input_file->file_path = file_path;
		input_file->open_input_file = read_flac_file;
		input_file->close_input_file = close_flac;
		input_file->process_input_file = process_flac_file;
	}



	status = find_custom_parameter_by_friendly(music_query->query_parameters->query_custom_parameters,"output_type",&output_type_parameter);








	transcode_thread_t* transcode_thread = apr_pcalloc(r->server->process->pool,sizeof(transcode_thread_t));
	transcode_thread->pool = r->server->process->pool;

	//Determine proper encoder
	if(apr_strnatcmp(output_type_parameter->value,"ogg") == 0){
		decoding_job->output_type =output_type_parameter->value;
		decoding_job->encoder_function = ogg_encode;
		decoding_job->input_file = input_file;
	}


	srv_conf = ap_get_module_config(r->server->module_config, &mediaplayer_module);






	transcode_thread->queue = srv_conf->decoding_queue;

	if(srv_conf->decoding_queue->head == NULL){
		add_decoding_job_queue(decoding_job,srv_conf->decoding_queue);
		rv = apr_thread_create(&trans_thread,NULL,encoder_thread,transcode_thread,r->server->process->pool);
	}else{
		add_decoding_job_queue(decoding_job,srv_conf->decoding_queue);
	}
	return 0;
}
