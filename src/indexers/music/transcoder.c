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

#include "music_query.h"

#ifdef WITH_OGG
#include "codecs/ogg/ogg_encode.h"
#endif

#ifdef WITH_FLAC
#include "codecs/flac/flac.h"
#endif

#ifdef WITH_MP3
#include "codecs/mp3/mpg123.h"
#endif

#include "database/db.h"
#include <stdio.h>
#include <stdlib.h>

#include "decoding_queue.h"
#include "music_typedefs.h"

/* 
static int add_new_source_db(apr_pool_t* pool, db_config_t* dbd_config,decoding_job_t* decoding_job,error_messages_t* error_messages){
	apr_status_t rv;
	int error_num = 0;
	const char* args[4];
	const char* links_args[7];

	char* source_id = NULL;

	char error_message[512];

	//add row to soruce table
	rv = apr_thread_mutex_lock(dbd_config->mutex);
	if(rv != APR_SUCCESS){
		error_messages_add(error_messages,ERROR,"Error locking dbd mutex",apr_strerror(rv, error_message,512));
	}

	//`type`, `path`, `quality`,`mtime`
	args[0] = (decoding_job)->output_file_type;
	args[1] = (decoding_job)->output_file_path;
	args[2] = apr_itoa(pool, 100);
	args[3] = apr_ltoa(pool,1337);
	error_num = insert_db(&source_id, dbd_config, dbd_config->statements.add_source, args);
	//Check if insert db worked
	if (error_num != 0 || source_id == NULL){
		error_messages_add(error_messages, ERROR, "DBD insert_source error(Encoder Thread)", apr_dbd_error(dbd_config->dbd_driver, dbd_config->dbd_handle, error_num));
		error_num = -1;
	}else{//ADD LINK
		//artistid, albumid, songid, sourceid, feature, track_no, disc_no
		links_args[0] = (decoding_job)->artist_id;
		links_args[1] =(decoding_job)->album_id;
		links_args[2] =(decoding_job)->song_id;
		links_args[3] = source_id;
		links_args[4] = "0";
		links_args[5] = "0";
		links_args[6] = "0";

		error_num = insert_db(NULL, dbd_config, dbd_config->statements.add_link, links_args);
		if (error_num != 0){
			error_messages_add(error_messages, ERROR, "DBD insert_link error:", apr_dbd_error(dbd_config->dbd_driver,dbd_config->dbd_handle, error_num));
			error_num = -2;
		}
	}
	rv = apr_thread_mutex_unlock(dbd_config->mutex);
	if(rv != APR_SUCCESS){
		error_messages_add(error_messages,ERROR,"Error unlocking dbd mutex",apr_strerror(rv, error_message,512));
	}

	return error_num;
}



void * APR_THREAD_FUNC encoder_thread(apr_thread_t* thread, void* ptr){
	int status;
	transcode_thread_t* transcode_thread = (transcode_thread_t*)ptr;
	apr_pool_t* pool = transcode_thread->pool;

	encoding_options_t enc_opt = {0};
	enc_opt.quality = 2;
	decoding_job_t* decoding_job = NULL;

	apr_status_t rv;

	char error_message[512];


	//Lock queue
	LOCK_CHECK_ERRORS(transcode_thread->decoding_queue->mutex,transcode_thread->decoding_queue->error_messages, "Error Locking encoding thread");

	++transcode_thread->num_working_threads;
	++transcode_thread->decoding_queue->queue->num_working_threads;

	UNLOCK_CHECK_ERRORS(transcode_thread->decoding_queue->mutex, transcode_thread->decoding_queue->error_messages,"Error Unlocking encoding thread");

	while((decoding_job = get_decoding_job_queue(transcode_thread->decoding_queue, transcode_thread->decoding_queue->error_messages)) != NULL){
		input_file_t input_file;

		int (*encoder_function)(apr_pool_t*, input_file_t*,encoding_options_t*,const char*) = NULL;

		uint64_t index_flag = (1ull << (63 - decoding_job->index));

		//Determine proper decoder
		//TODO
		//Should really load these dynamicly somewhere and set a dynamic varible
		//instead of ugly ifdef
		
		#ifdef WITH_FLAC
		if(apr_strnatcmp(decoding_job->input_file_type,"flac") == 0){
			input_file.open_input_file = read_flac_file;
			input_file.close_input_file = close_flac;
			input_file.process_input_file = process_flac_file;
		}else 
		#endif

		#ifdef WITH_MP3
		if(apr_strnatcmp(decoding_job->input_file_type,"mp3") == 0){
			input_file.open_input_file = read_mp3_file;
			input_file.close_input_file = close_mp3_file;
			input_file.process_input_file = process_mp3_file;

		}else
		#endif


		{

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
			error_messages_add(transcode_thread->error_messages,ERROR,apr_pstrcat(pool,"Error opening input file", decoding_job->input_file_path,NULL),error_message);
			goto remove_from_queue;
		}
		enc_opt.progress = &((decoding_job)->progress);

		#ifdef WITH_OGG
		if(apr_strnatcmp(decoding_job->output_file_type,"ogg") == 0){
			encoder_function = ogg_encode;
		}
		#endif

		if(encoder_function == NULL){
			error_messages_add(transcode_thread->error_messages,ERROR,apr_pstrcat(pool,"Error encoding input file", decoding_job->input_file_path,NULL),"No encoder found");
			goto remove_from_queue;
		}

		status = encoder_function(pool,&input_file,&enc_opt,decoding_job->output_file_path);
		if(status != 0){
			apr_strerror(status,error_message,512);
			error_messages_add(transcode_thread->error_messages,ERROR,apr_pstrcat(pool,"Error encoding input file", decoding_job->input_file_path,NULL),error_message);
			goto remove_from_queue;
		}
		status = input_file.close_input_file(input_file.file_struct);
		if(status != 0){
			error_messages_add(transcode_thread->error_messages,ERROR,"Error closing file",apr_itoa(pool,status));
			goto remove_from_queue;
		}
		//Write to database with new source

		if(decoding_job == NULL){
			error_messages_add(transcode_thread->error_messages,ERROR,"WTF decoding job is now NULL",apr_itoa(pool,status));
			goto remove_from_queue;
		}

		status = add_new_source_db(pool, transcode_thread->dbd_config, decoding_job, transcode_thread->decoding_queue->error_messages);
		if(status != 0){
			error_messages_add(transcode_thread->error_messages,ERROR,"Failed to add decoding job to database",apr_itoa(pool,status));
		}
		remove_from_queue:

		LOCK_CHECK_ERRORS(transcode_thread->decoding_queue->mutex, transcode_thread->decoding_queue->error_messages,"Error Locking encoding thread");
		//Unset flag in working queue
		transcode_thread->decoding_queue->queue->working &= ~index_flag;

		UNLOCK_CHECK_ERRORS(transcode_thread->decoding_queue->mutex, transcode_thread->decoding_queue->error_messages,"Error Unlocking encoding thread");
	}

	LOCK_CHECK_ERRORS(transcode_thread->decoding_queue->mutex,transcode_thread->decoding_queue->error_messages, "Error Locking encoding thread");

	--transcode_thread->num_working_threads;
	--transcode_thread->decoding_queue->queue->num_working_threads;

	UNLOCK_CHECK_ERRORS(transcode_thread->decoding_queue->mutex, transcode_thread->decoding_queue->error_messages,"Error unlocking encoding thread");

	return 0;
}

static int check_db_for_decoding_job(const char** output_source_id, apr_pool_t* pool,db_config_t* dbd_config,db_query_t* db_query,const char* song_id, column_table_t* song_id_col,const char* output_type,column_table_t* type,error_messages_t* error_messages){
	//Check database
	results_table_t* results_table;
	query_parameters_t* query_parameters;
	int status = 0;
	column_table_t* source_id_col;

	init_query_parameters(pool,&query_parameters);

	add_where_query_parameter(pool, query_parameters,song_id_col,song_id);
	add_where_query_parameter(pool, query_parameters,type,output_type);
	status = run_query(db_query,query_parameters,&results_table);
	if(status != 0){
		 return -11;
	}

	status = find_select_column_from_query_by_table_id_and_query_id(&source_id_col,db_query,"links","sourceid");
	if(status != 0){
		return -13;
	}

	 if(results_table->rows_array->nelts > 0){
		 status = get_column_results_for_row(db_query,results_table,source_id_col,0,output_source_id);
		 if(status != 0){
			 return -14;
		 }
	 }else{
		 return -1;
	 }

	return results_table->rows_array->nelts;

}

int transcode_audio(music_query_t* music_query){
	apr_pool_t* pool = music_query->pool;
	apr_pool_t* proccess_pool = music_query->globals->pool;
	db_config_t* db_config = music_query->db_query->db_config;
	
	int status;
	apr_status_t rv;
	const char* error_header = "Error Setting Up Transcode Audio";

	const char* file_path;
	const char* file_type;

	const char* song_id;
	const char* album_id;
	const char* artist_id;
	const char* source_id;

	const char* output_file_path;
//	const char* input_file_path;

	const char* temp_dir;

	custom_parameter_t* output_type_parameter;


	apr_thread_t* trans_thread;
	column_table_t* file_path_col;
	column_table_t* file_type_col;
	column_table_t* song_id_col;
	column_table_t* album_id_col;
	column_table_t* artist_id_col;

	decoding_job_t* decoding_job = apr_pcalloc(proccess_pool,sizeof(decoding_job_t));
	const char* output_source_id;

	apr_pool_t* decoding_job_pool;

	//Create thranscode memory pool
	apr_pool_create_ex(&decoding_job_pool,proccess_pool,NULL,NULL);

	rv = apr_temp_dir_get(&temp_dir,pool);
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
		 return -23;
	 }
	 status = find_select_column_from_query_by_table_id_and_query_id(&album_id_col,music_query->db_query,"links","albumid");
	 if(status != 0){
		 return -33;
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
		 return -25;
	 }
	 status = get_column_results_for_row(music_query->db_query,music_query->results,artist_id_col,0,&artist_id);
	 if(status != 0){
		 return -35;
	 }
	 status = get_column_results_for_row(music_query->db_query,music_query->results,album_id_col,0,&album_id);
	 if(status != 0){
		 return -45;
	 }


	if(file_type == NULL){
		error_messages_add(music_query->error_messages,ERROR,error_header,"Database query failed to return results");
		return -21;
	}

	if(music_query->query_parameters == NULL){
		error_messages_add(music_query->error_messages,ERROR,error_header,"No parameters given in query");
		return -98;
	}

	//Setup decoding job
	status = db_query_find_custom_parameter_by_friendly(music_query->db_query_parameters, "output_type", &output_type_parameter);
	if(status != 0){
		error_messages_add(music_query->error_messages,ERROR,error_header,"No output file type given");
		return -99;
	}

	source_id = ((query_where_condition_t*)music_query->query_parameters->query_where_conditions->elts)[0].condition;
	output_file_path = apr_pstrcat(pool, temp_dir,"/", ((query_where_condition_t*)music_query->query_parameters->query_where_conditions->elts)[0].condition,".ogg", NULL);

	apr_cpystrn(decoding_job->song_id, song_id, 255);
	apr_cpystrn(decoding_job->artist_id, artist_id, 255);
	apr_cpystrn(decoding_job->album_id, album_id, 255);
	apr_cpystrn(decoding_job->source_id,source_id,255);

	apr_cpystrn(decoding_job->input_file_path, file_path,255);
	apr_cpystrn(decoding_job->input_file_type, file_type,255);

	apr_cpystrn(decoding_job->output_file_path, output_file_path, 1024);
	apr_cpystrn(decoding_job->output_file_type,output_type_parameter->value, 256);
	apr_cpystrn(decoding_job->new_source_id,"0", 256);
	apr_cpystrn(decoding_job->status,"Created decoding_job", 256);

	decoding_job->progress = 0.0;

	//Check if decoding job already exists
	//Check Database
	//Look for matching source_id and output_type
	status = check_db_for_decoding_job(&output_source_id,pool,db_config,music_query->db_query,song_id,song_id_col,decoding_job->output_file_type,file_type_col,music_query->error_messages);
	if(status > 0){
		apr_cpystrn(decoding_job->new_source_id,output_source_id,256);
		apr_cpystrn(decoding_job->status,"Decoding job already exists in database",256);
		decoding_job->progress = 100.0;
	//Check the working decoding job and the queue
	}else if((status = does_decoding_job_exsits(pool,music_query->globals->decoding_queue, &decoding_job)) == 0){

		const char* dec_job_status = apr_psprintf(pool,"Added to queue of size %d", popcount_4(music_query->globals->decoding_queue->queue->waiting));
		//decoding job does not exsits
		//add it to queue
		status = add_decoding_job_queue(decoding_job,music_query->globals->decoding_queue);

		if(status != 0){
			error_messages_add(music_query->error_messages,ERROR,"Error adding to queue","It could be full or fucked up");
		}else{
			apr_cpystrn(decoding_job->status,dec_job_status,512);
			if(music_query->globals->decoding_queue->num_decoding_threads < 4){
				//Get server config for global queue used for decoding jobs
				transcode_thread_t* transcode_thread = apr_pcalloc(proccess_pool,sizeof(transcode_thread_t));
				transcode_thread->pool = proccess_pool;

				//
				transcode_thread->num_working_threads = music_query->globals->decoding_queue->num_decoding_threads;

				//Global Queue
				transcode_thread->decoding_queue = music_query->globals->decoding_queue;
				//Database
				transcode_thread->dbd_config = db_config;
				//Error Messages
				transcode_thread->error_messages = music_query->error_messages;
				rv = apr_thread_create(&trans_thread,NULL,encoder_thread,transcode_thread,proccess_pool);
			}
		}
	}



	apr_brigade_puts(music_query->output->bucket_brigade, NULL, NULL,"{\n");
	print_error_messages(pool,music_query->output->bucket_brigade, music_query->error_messages);
	apr_brigade_printf(music_query->output->bucket_brigade, NULL, NULL,",\n\t\"decoding_job\" :  {\n\t\t \"status\" :  \"%s\",\n\t\t\"input_source_id\" : \"%s\",\n\t\t\"output_type\" : \"%s\",\n\t\t\"progress\" : \"%.2f\",\n\t\t\"output_source_id\" : \"%s\",\n\t\t\"output_file_path\" : \"%s\"\n\t}\n",json_escape_char(pool, decoding_job->status),decoding_job->source_id,decoding_job->output_file_type,decoding_job->progress,decoding_job->new_source_id,json_escape_char(pool, decoding_job->output_file_path));
	apr_brigade_puts(music_query->output->bucket_brigade, NULL, NULL,"}\n");

	return 0;
}

*/
