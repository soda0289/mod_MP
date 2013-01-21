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
#include "database/dbd.h"
#include "database/db_query_parameters.h"


decoding_job_t* get_decoding_job_queue(queue_t* queue){
	//Lock queue
	apr_thread_mutex_lock(queue->mutex);

	decoding_job_t* dec_job = queue->head;
	//Check if last item
	if(queue->head == queue->tail){
		queue->tail = NULL;
	}
	if(queue->head != NULL){
		queue->head = queue->head->next;
	}

	//unlock queue
	apr_thread_mutex_unlock(queue->mutex);
	return dec_job;
}


void * APR_THREAD_FUNC encoder_thread(apr_thread_t* thread, void* ptr){
	int status, error_num;
	transcode_thread_t* transcode_thread = (transcode_thread_t*)ptr;
	apr_pool_t* pool = transcode_thread->pool;

	encoding_options_t enc_opt = {0};
	decoding_job_t** decoding_job =  &(transcode_thread->queue->decoding[transcode_thread->key]);
	char error_message[512];


	//Lock queue
	apr_thread_mutex_lock(transcode_thread->queue->mutex);
	++transcode_thread->queue->num_working_threads;
	apr_thread_mutex_unlock(transcode_thread->queue->mutex);

	while((*decoding_job = get_decoding_job_queue(transcode_thread->queue)) != NULL){
		char* file_path = (*decoding_job)->output_file_path;
		char* source_id;
		input_file_t* input_file = (*decoding_job)->input_file;
		//Open input file and prepare it for processing
		status = input_file->open_input_file(pool,&(input_file->file_struct),input_file->file_path,&enc_opt);
		if(status != 0){
			add_error_list(transcode_thread->error_messages,ERROR,apr_pstrcat(pool,"Error opening input file", input_file->file_path,NULL),"Error");
			continue;
		}
		(*decoding_job)->progress = &(enc_opt.progress);
		status = (*decoding_job)->encoder_function(pool,input_file,&enc_opt,file_path);
		if(status != 0){
			apr_strerror(status,error_message,512);
			add_error_list(transcode_thread->error_messages,ERROR,apr_pstrcat(pool,"Error opening input file", file_path,NULL),error_message);
			continue;
		}
		status = input_file->close_input_file(input_file->file_struct);
		if(status != 0){
			add_error_list(transcode_thread->error_messages,ERROR,"Error closing file",apr_itoa(pool,status));
			continue;
		}
		//Write to database with new source

		//add row to soruce table
		const char* args[4];
		//`type`, `path`, `quality`,`mtime`
		args[0] = (*decoding_job)->output_type;
		args[1] = (*decoding_job)->output_file_path;
		args[2] = apr_itoa(pool, 100);
		args[3] = apr_ltoa(pool,1337);
		error_num = insert_db(&source_id, transcode_thread->dbd_config, transcode_thread->dbd_config->statements.add_source, args);
		if (error_num != 0){
			add_error_list(transcode_thread->error_messages, ERROR, "DBD insert_source error:", apr_dbd_error(transcode_thread->dbd_config->dbd_driver, transcode_thread->dbd_config->dbd_handle, error_num));
			continue;
		}


			const char* links_args[7];
			//artistid, albumid, songid, sourceid, feature, track_no, disc_no
			links_args[0] = (*decoding_job)->artist_id;
			links_args[1] =(*decoding_job)->album_id;
			links_args[2] =(*decoding_job)->song_id;
			links_args[3] = source_id;
			links_args[4] = "0";
			links_args[5] = "0";
			links_args[6] = "0";

			error_num = insert_db(NULL, transcode_thread->dbd_config, transcode_thread->dbd_config->statements.add_link, links_args);
			if (error_num != 0){
				add_error_list( transcode_thread->error_messages, ERROR, "DBD insert_link error:", apr_dbd_error( transcode_thread->dbd_config->dbd_driver,  transcode_thread->dbd_config->dbd_handle, error_num));
				continue;
			}

	}

	//Lock queue
	apr_thread_mutex_lock(transcode_thread->queue->mutex);
	--transcode_thread->queue->num_working_threads;
	apr_thread_mutex_unlock(transcode_thread->queue->mutex);

	return 0;
}

int add_decoding_job_queue(decoding_job_t* job,queue_t* queue){
	//Lock queue
	apr_thread_mutex_lock(queue->mutex);
	if(queue->tail==NULL ){
		queue->tail= job;
		queue->head = job;
	}else{
		queue->tail->next = job;
		queue->tail = job;
	}
	//Lock queue
	apr_thread_mutex_unlock(queue->mutex);
	return 0;
}

int does_decoding_job_exsits(queue_t* queue,decoding_job_t** dec_job){
	decoding_job_t* decoding_job_node;
	int i;
	//Check the ones currently worked on
	for(i = 0;i < queue->max_num_workers;i++){
		//Check if source id and output_type are equal
		//Should also check if encoding options are equal such as quality
		if(queue->decoding[i] == NULL){
			//SKIP null index
			continue;
		}
		if(apr_strnatcmp(queue->decoding[i]->source_id,(*dec_job)->source_id) == 0 && apr_strnatcmp(queue->decoding[i]->output_type,(*dec_job)->output_type) == 0){
			*dec_job = queue->decoding[i];
			//Found decoding job
			(*dec_job)->status = apr_psprintf(queue->pool,"Decoding on thread: %d",i);
			return 1;
		}
	}

	//Check the waiting list in queue
	for(i = 0,decoding_job_node = queue->head;decoding_job_node != NULL;decoding_job_node = decoding_job_node->next,i++){
		//Check if source id and output_type are equal
		//Should also check if encoding options are equal such as quality
		if(apr_strnatcmp(decoding_job_node->source_id,(*dec_job)->source_id) == 0 && apr_strnatcmp(decoding_job_node->output_type,(*dec_job)->output_type) == 0){
			*dec_job = decoding_job_node;
			(*dec_job)->status = apr_psprintf(queue->pool,"Waiting in queue %d spots away from head",i);
			return 1;
		}
	}
	return 0;
}

int check_db_for_decoding_job(const char** output_source_id, apr_pool_t* pool,db_config* dbd_config,query_t* db_query,const char* song_id, column_table_t* song_id_col,const char* output_type,column_table_t* type,error_messages_t* error_messages){
	//Check database
	results_table_t* results_table;
	query_parameters_t* query_parameters;
	int status = 0;
	column_table_t* source_id_col;

	init_query_parameters(pool,&query_parameters);

	add_where_query_parameter(query_parameters,song_id_col,song_id);
	add_where_query_parameter(query_parameters,type,output_type);

	status = select_db_range(dbd_config,query_parameters,db_query,&results_table,error_messages);

	 status = find_select_column_from_query_by_table_id_and_query_id(&source_id_col,db_query,"links","sourceid");
	 if(status != 0){
		 return -13;
	 }

	 if(results_table->rows->nelts > 0){
		 status = get_column_results_for_row(db_query,results_table,source_id_col,0,output_source_id);
		 if(status != 0){
			 return -14;
		 }
	 }else{
		 return -1;
	 }

	return results_table->rows->nelts;

}

int transcode_audio(request_rec* r, db_config* dbd_config,music_query_t* music_query){
	int status;
	apr_status_t rv;

	const char* file_path;
	const char* file_type;

	const char* song_id;
	const char* album_id;
	const char* artist_id;

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

	column_table_t* file_path_col;
	column_table_t* file_type_col;
	column_table_t* song_id_col;
	column_table_t* album_id_col;
	column_table_t* artist_id_col;




	 status = find_select_column_from_query_by_table_id_and_query_id(&file_path_col,music_query->db_query,"sources","path");
	 if(status != 0){
		 return -11;
	 }
	 status = find_select_column_from_query_by_table_id_and_query_id(&file_type_col,music_query->db_query,"sources","type");
	 if(status != 0){
		 return -12;
	 }
	 status = find_select_column_from_query_by_table_id_and_query_id(&song_id_col,music_query->db_query,"links","songid");
	 if(status != 0){
		 return -13;
	 }
	 status = find_select_column_from_query_by_table_id_and_query_id(&artist_id_col,music_query->db_query,"links","artistid");
	 if(status != 0){
		 return -13;
	 }
	 status = find_select_column_from_query_by_table_id_and_query_id(&album_id_col,music_query->db_query,"links","albumid");
	 if(status != 0){
		 return -13;
	 }

	 status = get_column_results_for_row(music_query->db_query,music_query->results,file_path_col,0,&file_path);
	 if(status != 0){
		 return -14;
	 }
	 status = get_column_results_for_row(music_query->db_query,music_query->results,file_type_col,0,&file_type);
	 if(status != 0){
		 return -15;
	 }
	 status = get_column_results_for_row(music_query->db_query,music_query->results,song_id_col,0,&song_id);
	 if(status != 0){
		 return -15;
	 }
	 status = get_column_results_for_row(music_query->db_query,music_query->results,artist_id_col,0,&artist_id);
	 if(status != 0){
		 return -15;
	 }
	 status = get_column_results_for_row(music_query->db_query,music_query->results,album_id_col,0,&album_id);
	 if(status != 0){
		 return -15;
	 }

	//Setup input_file struct with decoding options
	input_file = apr_pcalloc(r->server->process->pool,sizeof(input_file_t));
	//Determine proper decoder
	if(apr_strnatcmp(file_type,"flac") == 0){
		input_file->file_path = file_path;
		input_file->open_input_file = read_flac_file;
		input_file->close_input_file = close_flac;
		input_file->process_input_file = process_flac_file;
	}else{
		//No decoder found
		return -20;
	}


	//Setup decoding job
	status = find_custom_parameter_by_friendly(music_query->query_parameters->query_custom_parameters,"output_type",&output_type_parameter);

	if(status != 0){
		return -99;
	}



	decoding_job->song_id = song_id;
	decoding_job->album_id = album_id;
	decoding_job->artist_id = artist_id;

	decoding_job->source_id = apr_pstrdup(r->server->process->pool,((query_where_condition_t*)music_query->query_parameters->query_where_conditions->elts)[0].condition);
	decoding_job->output_file_path = apr_pstrcat(r->server->process->pool, temp_dir,"/", ((query_where_condition_t*)music_query->query_parameters->query_where_conditions->elts)[0].condition,".ogg", NULL);
	//Determine proper encoder
	if(apr_strnatcmp(output_type_parameter->value,"ogg") == 0){
		decoding_job->encoder_function = ogg_encode;
	}else{
		return -5;
	}
	decoding_job->output_type = apr_pstrdup(r->server->process->pool,output_type_parameter->value);
	decoding_job->input_file = input_file;
	float zero = 0.0;
	float one_hundred = 100.0;
	decoding_job->progress = &zero;

	srv_conf = ap_get_module_config(r->server->module_config, &mediaplayer_module);
	//Check if decoding job already exists
	//Check Database
	char* output_source_id;
	status = check_db_for_decoding_job(&output_source_id,r->pool,dbd_config,music_query->db_query,song_id,song_id_col,decoding_job->output_type,file_type_col,music_query->error_messages);
	if(status > 0){

		decoding_job->new_source_id = output_source_id;
		decoding_job->status = apr_psprintf(r->server->process->pool,"Decoding job already exists in database");
		decoding_job->progress = &one_hundred;
	}else{
	//Look for matching source_id and output_type
	status = does_decoding_job_exsits(srv_conf->decoding_queue, &decoding_job);
	}
	if(status == 0){
		//decoding job does not exsits
		//add it to queue
		add_decoding_job_queue(decoding_job,srv_conf->decoding_queue);
		decoding_job->status = apr_psprintf(r->server->process->pool,"Added to queue of size %d", srv_conf->decoding_queue->size);

		if(srv_conf->decoding_queue->num_working_threads < srv_conf->decoding_queue->max_num_workers){
			//Get server config for global queue used for decoding jobs
			transcode_thread_t* transcode_thread = apr_pcalloc(r->server->process->pool,sizeof(transcode_thread_t));
			transcode_thread->pool = r->server->process->pool;
			transcode_thread->queue = srv_conf->decoding_queue;
			transcode_thread->dbd_config = dbd_config;
			transcode_thread->error_messages = srv_conf->error_messages;
			transcode_thread->key = 0;
			int i;
			int found = 0;
			//Find null value in array use index as thread key
			for(i = 0;i < srv_conf->decoding_queue->max_num_workers;i++){
				if(srv_conf->decoding_queue->decoding[i] == NULL){
					found = 1;
					transcode_thread->key = i;
					break;
				}
			}
			if(found ==0){
				/*BIG ERROR
				 * This means the queue is full
				 * but the number of working threads is less then the max
				 * ONE thread could have crashed.
				*/
				add_error_list(music_query->error_messages,ERROR,"BIG ERROR","Could not find null value in array one of the threads crashed");
			}
			rv = apr_thread_create(&trans_thread,NULL,encoder_thread,transcode_thread,r->server->process->pool);
		}
	}else if(status == 1){
		//decoding job does exsits
		//print decoding job


	}


	 output_status_json(r);

	ap_rprintf(r,"\"decoding_job\" :  { \"status\" :  \"%s\",\n \"input_source_id\" : \"%s\",\n \"output_type\" : \"%s\",\n \"progress\" : \"%.2f\",\n \"output_source_id\" : \"%s\",\n \"output_file_path\" : \"%s\"}\n",json_escape_char(r->pool, decoding_job->status),decoding_job->source_id,decoding_job->output_type,(*decoding_job->progress),decoding_job->new_source_id,json_escape_char(r->pool, decoding_job->output_file_path));
	ap_rputs("}\n", r);

	return 0;
}
