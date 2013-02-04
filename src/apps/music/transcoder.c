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

#include "apr.h"
#include "apr_errno.h"
#include "apps/music/music_query.h"
#include "apps/music/ogg_encode.h"
#include "database/db_query_config.h"
#include "apps/music/flac.h"
#include "database/dbd.h"
#include "database/db_query_parameters.h"
#include <stdio.h>
#include <stdlib.h>

#define LOCK_CHECK_ERRORS(lock, error_messages, error_header) if((rv = apr_global_mutex_lock(lock) != APR_SUCCESS))add_error_list(error_messages,ERROR,error_header,apr_strerror(rv, error_message,512))
#define UNLOCK_CHECK_ERRORS(lock, error_messages, error_header) if((rv = apr_global_mutex_unlock(lock) != APR_SUCCESS))add_error_list(error_messages,ERROR,error_header,apr_strerror(rv, error_message,512))


decoding_job_t* get_decoding_job_queue(queue_t* queue, error_messages_t* error_messages){
	//Lock queue
	decoding_job_t* dec_job;
	int index;

	apr_status_t rv;
	char error_message[512];

	uint64_t index_flag = 0;

	LOCK_CHECK_ERRORS(queue->mutex,queue->error_messages, "Error Locking in get decoding queue");

	if(queue->waiting){
		//Set dec job to head
		if(queue->head < 0 ){
			add_error_list(queue->error_messages,ERROR,"Errroroororor", "head is less than 0");
		}
		index = queue->head;
		index_flag = (1ull << (63 - index));
		dec_job = &(queue->decoding[index]);
		//remove job from waiting queue
		queue->waiting &= ~(index_flag);
		queue->head = dec_job->next;
	}else{
		dec_job = NULL;
	}

	queue->working |= index_flag;
	//unlock queue
	UNLOCK_CHECK_ERRORS(queue->mutex,queue->error_messages, "Error Unlocking in get decoding queue");
	return dec_job;

}


void * APR_THREAD_FUNC encoder_thread(apr_thread_t* thread, void* ptr){
	int status, error_num;
	transcode_thread_t* transcode_thread = (transcode_thread_t*)ptr;
	apr_pool_t* pool = transcode_thread->pool;

	encoding_options_t enc_opt = {0};
	decoding_job_t* decoding_job = NULL;

	apr_status_t rv;

	char error_message[512];


	//Lock queue
	LOCK_CHECK_ERRORS(transcode_thread->queue->mutex,transcode_thread->queue->error_messages, "Error Locking encoding thread");

	++transcode_thread->num_working_threads;
	++transcode_thread->queue->num_working_threads;
	rv = apr_global_mutex_unlock(transcode_thread->queue->mutex);

	UNLOCK_CHECK_ERRORS(transcode_thread->queue->mutex, transcode_thread->queue->error_messages,"Error Unlocking encoding thread");

	while((decoding_job = get_decoding_job_queue(transcode_thread->queue, transcode_thread->queue->error_messages)) != NULL){
		const char* args[4];
		const char* links_args[7];

		char* source_id;
		input_file_t input_file;

		int (*encoder_function)(apr_pool_t*, input_file_t*,encoding_options_t*,const char*);

		uint64_t index_flag = (1ull << (63 - decoding_job->index));

		//Determine proper decoder
		if(apr_strnatcmp(decoding_job->input_file_type,"flac") == 0){
			input_file.open_input_file = read_flac_file;
			input_file.close_input_file = close_flac;
			input_file.process_input_file = process_flac_file;
		}else{
			//No decoder found
			goto remove_from_queue;
		}

		//Open input file and prepare it for processing
		status = input_file.open_input_file(pool,&(input_file.file_struct),decoding_job->input_file_path,&enc_opt);
		if(status != 0){
			if(status < 0){
				apr_cpystrn(error_message,strerror(errno),512);
			}else{
				apr_strerror(status,error_message,512);
			}
			add_error_list(transcode_thread->error_messages,ERROR,apr_pstrcat(pool,"Error opening input file", decoding_job->input_file_path,NULL),error_message);
			goto remove_from_queue;
		}
		enc_opt.progress = &((decoding_job)->progress);
		if(apr_strnatcmp(decoding_job->output_file_type,"ogg") == 0){
			encoder_function = ogg_encode;
		}



		status = encoder_function(pool,&input_file,&enc_opt,decoding_job->output_file_path);
		if(status != 0){
			apr_strerror(status,error_message,512);
			add_error_list(transcode_thread->error_messages,ERROR,apr_pstrcat(pool,"Error encoding input file", decoding_job->input_file_path,NULL),error_message);
			goto remove_from_queue;
		}
		status = input_file.close_input_file(input_file.file_struct);
		if(status != 0){
			add_error_list(transcode_thread->error_messages,ERROR,"Error closing file",apr_itoa(pool,status));
			goto remove_from_queue;
		}
		//Write to database with new source

		if(decoding_job == NULL){
			add_error_list(transcode_thread->error_messages,ERROR,"WTF decoding job is now NULL",apr_itoa(pool,status));
			goto remove_from_queue;
		}

		//add row to soruce table
		rv = apr_thread_mutex_lock(transcode_thread->dbd_config->mutex);
		if(rv != APR_SUCCESS){
			add_error_list(transcode_thread->queue->error_messages,ERROR,"Error locking dbd",apr_strerror(rv, error_message,512));
		}
		//`type`, `path`, `quality`,`mtime`
		args[0] = (decoding_job)->output_file_type;
		args[1] = (decoding_job)->output_file_path;
		args[2] = apr_itoa(pool, 100);
		args[3] = apr_ltoa(pool,1337);
		error_num = insert_db(&source_id, transcode_thread->dbd_config, transcode_thread->dbd_config->statements.add_source, args);
		if (error_num != 0){
			add_error_list(transcode_thread->error_messages, ERROR, "DBD insert_source error(Encoder Thread)", apr_dbd_error(transcode_thread->dbd_config->dbd_driver, transcode_thread->dbd_config->dbd_handle, error_num));
			goto remove_from_queue;
		}



		//artistid, albumid, songid, sourceid, feature, track_no, disc_no
		links_args[0] = (decoding_job)->artist_id;
		links_args[1] =(decoding_job)->album_id;
		links_args[2] =(decoding_job)->song_id;
		links_args[3] = source_id;
		links_args[4] = "0";
		links_args[5] = "0";
		links_args[6] = "0";

		error_num = insert_db(NULL, transcode_thread->dbd_config, transcode_thread->dbd_config->statements.add_link, links_args);
		if (error_num != 0){
			add_error_list( transcode_thread->error_messages, ERROR, "DBD insert_link error:", apr_dbd_error( transcode_thread->dbd_config->dbd_driver,  transcode_thread->dbd_config->dbd_handle, error_num));
			goto remove_from_queue;
		}
		rv = apr_thread_mutex_unlock(transcode_thread->dbd_config->mutex);
		if(rv != APR_SUCCESS){
			add_error_list(transcode_thread->queue->error_messages,ERROR,"Error unlocking dbd",apr_strerror(rv, error_message,512));
		}

		remove_from_queue:

		LOCK_CHECK_ERRORS(transcode_thread->queue->mutex, transcode_thread->queue->error_messages,"Error Locking encoding thread");
		//Unset flag in working queue
		transcode_thread->queue->working &= ~index_flag;

		UNLOCK_CHECK_ERRORS(transcode_thread->queue->mutex, transcode_thread->queue->error_messages,"Error Unlocking encoding thread");
	}

	LOCK_CHECK_ERRORS(transcode_thread->queue->mutex,transcode_thread->queue->error_messages, "Error Locking encoding thread");

	--transcode_thread->num_working_threads;
	--transcode_thread->queue->num_working_threads;

	UNLOCK_CHECK_ERRORS(transcode_thread->queue->mutex, transcode_thread->queue->error_messages,"Error unlocking encoding thread");

	return 0;
}

int add_decoding_job_queue(decoding_job_t* job,queue_t* queue){
	int empty_index;
	apr_status_t rv;
	char error_message[512];
	uint64_t free;

	//Lock queue
	LOCK_CHECK_ERRORS(queue->mutex,queue->error_messages, "Error Locking in add decoding job");




	free = ~(queue->waiting | queue->working);
	if(!free){
		add_error_list(queue->error_messages,ERROR,"Error adding to queue","Queue is FULL!");
		return -1;
	}


	if(free){
		empty_index = __builtin_clzll(free);
	}else{
		//Queue empty
		//reset head and start at 0
		empty_index = 0;
	}
	//if waiting queue is empty
	if(queue->waiting == 0ull){
		//set head
		queue->head = empty_index;
	}

	//Add into free index
	queue->decoding[empty_index] = *job;
	queue->decoding[empty_index].index = empty_index;
	//Points no where since its last in list
	queue->decoding[empty_index].next = -1;

	//Mark in waiting
	queue->waiting |= (1ull << (63 - empty_index));

	//if queue->tail is set(not negative)
	if(queue->tail >= 0){
		//Update current tail
		queue->decoding[queue->tail].next = empty_index;
		//Set tail to new index
	}
	queue->tail = empty_index;
	//unlock queue
	UNLOCK_CHECK_ERRORS(queue->mutex,queue->error_messages, "Error Unlocking in add decoding job");

	return 0;
}

//This is better when most bits in x are 0
//It uses 3 arithmetic operations and one comparison/branch per "1" bit in x.
int popcount_4(uint64_t x) {
    int count;
    for (count=0; x; count++)
        x &= x-1;
    return count;
}


int does_decoding_job_exsits(queue_t* queue,decoding_job_t** dec_job){
	int i, found = 0;
	uint64_t has_job;

	apr_status_t rv;
	char error_message[512];

	if(*dec_job == NULL){
		return 0;
	}

	LOCK_CHECK_ERRORS(queue->mutex,queue->error_messages, "Error locking in does decoding job exsits");

	has_job = queue->waiting | queue->working;

	if(queue->waiting & queue->working){
		add_error_list(queue->error_messages,ERROR,"Working and waiting are set WTF MAN",apr_strerror(rv, error_message,512));
	}

	//i - Index is set to the offset, next decoding job in line, or the first one that is being worked on
	//which is equal too the array length minus the number of leading zeros.
	//i is incremented to each value of what is being worked on in the array and then through
	//each element in the queue.

	for(i = __builtin_clzll(has_job);
		has_job;
		i = __builtin_clzll(has_job &= ~(1ull << (63 - i)))){

		//Check if source id and output_type are equal
		//Should also check if encoding options are equal such as quality
		if(apr_strnatcmp(queue->decoding[i].source_id,(*dec_job)->source_id) == 0 && apr_strnatcmp(queue->decoding[i].output_file_type,(*dec_job)->output_file_type) == 0){
			const char* dec_job_status;

			*dec_job = &(queue->decoding[i]);
			//Found decoding job
			dec_job_status = apr_psprintf(queue->pool,"Decoding index: %d Working on",i);
			apr_cpystrn((*dec_job)->status,dec_job_status,512);

			//Job found return 1
			found++;
			//Should break but were debugging so try and find duplicates
		}
	}

	if(found > 1){
		add_error_list(queue->error_messages, ERROR, "Error found duplicate", "found duplicate in working/waiting queue");
	}

	UNLOCK_CHECK_ERRORS(queue->mutex,queue->error_messages, "Error Unlocking in does decoding job exists");

	return found;
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

	status = select_db_range(pool, dbd_config,query_parameters,db_query,&results_table,error_messages);

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
	const char* source_id;

	const char* output_file_path;
	const char* input_file_path;

	const char* temp_dir;
	mediaplayer_srv_cfg* srv_conf;

	custom_parameter_t* output_type_parameter;


	apr_thread_t* trans_thread;
	column_table_t* file_path_col;
	column_table_t* file_type_col;
	column_table_t* song_id_col;
	column_table_t* album_id_col;
	column_table_t* artist_id_col;

	decoding_job_t* decoding_job = apr_pcalloc(r->server->process->pool,sizeof(decoding_job_t));
	const char* output_source_id;

	mediaplayer_rec_cfg* rec_cfg;

	apr_pool_t* decoding_job_pool;

	//Create thranscode memory pool
	apr_pool_create_ex(&decoding_job_pool,r->server->process->pool,NULL,NULL);

	rv = apr_temp_dir_get(&temp_dir,r->pool);
	if(rv != APR_SUCCESS){

		return -1;
	}






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


	if(file_type == NULL){
		add_error_list(music_query->error_messages,ERROR,"Error with input file","No file type given");
		return -21;
	}



	//Setup decoding job
	status = find_custom_parameter_by_friendly(music_query->query_parameters->query_custom_parameters,"output_type",&output_type_parameter);

	if(status != 0){
		return -99;
	}

	source_id = ((query_where_condition_t*)music_query->query_parameters->query_where_conditions->elts)[0].condition;
	output_file_path = apr_pstrcat(r->server->process->pool, temp_dir,"/", ((query_where_condition_t*)music_query->query_parameters->query_where_conditions->elts)[0].condition,".ogg", NULL);

	apr_cpystrn(decoding_job->song_id, song_id, 255);
	apr_cpystrn(decoding_job->artist_id, artist_id, 255);
	apr_cpystrn(decoding_job->album_id, album_id, 255);
	apr_cpystrn(decoding_job->source_id,source_id,255);

	apr_cpystrn(decoding_job->input_file_path, file_path,255);
	apr_cpystrn(decoding_job->input_file_type, file_type,255);

	apr_cpystrn(decoding_job->output_file_path, output_file_path, 1024);
	apr_cpystrn(decoding_job->output_file_type,output_type_parameter->value, 256);

	decoding_job->progress = 0.0;

	srv_conf = ap_get_module_config(r->server->module_config, &mediaplayer_module);
	//Check if decoding job already exists
	//Check Database
	//Look for matching source_id and output_type
	status = check_db_for_decoding_job(&output_source_id,r->pool,dbd_config,music_query->db_query,song_id,song_id_col,decoding_job->output_file_type,file_type_col,music_query->error_messages);
	if(status > 0){
		apr_cpystrn(decoding_job->new_source_id,output_source_id,256);
		apr_cpystrn(decoding_job->status,"Decoding job already exists in database",256);
		decoding_job->progress = 100.0;
	//Check the working decoding job and the queue
	}else if((status = does_decoding_job_exsits(srv_conf->decoding_queue, &decoding_job)) == 0){

		const char* dec_job_status = apr_psprintf(r->pool,"Added to queue of size %d", popcount_4(srv_conf->decoding_queue->waiting));
		//decoding job does not exsits
		//add it to queue
		status = add_decoding_job_queue(decoding_job,srv_conf->decoding_queue);
		if(status != 0){
			add_error_list(music_query->error_messages,ERROR,"Error adding to queue","It could be full or fucked up");
		}else{
			apr_cpystrn(decoding_job->status,dec_job_status,512);
			if(srv_conf->num_working_threads < 4){
				//Get server config for global queue used for decoding jobs
				transcode_thread_t* transcode_thread = apr_pcalloc(r->server->process->pool,sizeof(transcode_thread_t));
				transcode_thread->pool = r->server->process->pool;

				//
				transcode_thread->num_working_threads = srv_conf->num_working_threads;
				//Global Queue
				transcode_thread->queue = srv_conf->decoding_queue;
				//Database
				transcode_thread->dbd_config = dbd_config;
				//Error Messages
				transcode_thread->error_messages = srv_conf->error_messages;
				rv = apr_thread_create(&trans_thread,NULL,encoder_thread,transcode_thread,r->server->process->pool);
			}
		}
	}


	rec_cfg = ap_get_module_config(r->request_config, &mediaplayer_module);
	//Apply header
	apr_table_add(r->headers_out, "Access-Control-Allow-Origin", "*");
	ap_set_content_type(r, "application/json") ;
	ap_rputs("{\n", r);
	print_error_messages(r, rec_cfg->error_messages);
	ap_rprintf(r,",\n\t\"decoding_job\" :  {\n\t\t \"status\" :  \"%s\",\n\t\t\"input_source_id\" : \"%s\",\n\t\t\"output_type\" : \"%s\",\n\t\t\"progress\" : \"%.2f\",\n\t\t\"output_source_id\" : \"%s\",\n\t\t\"output_file_path\" : \"%s\"\n\t}\n",json_escape_char(r->pool, decoding_job->status),decoding_job->source_id,decoding_job->output_file_type,decoding_job->progress,decoding_job->new_source_id,json_escape_char(r->pool, decoding_job->output_file_path));
	ap_rputs("}\n", r);

	return 0;
}
