/*
 * music_query.c
 *
 *  Created on: Oct 30, 2012
 *      Author: Reyad Attiyat
 *      Copyright 2012 Reyad Attiyat
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

#include "mod_mediaplayer.h"
#include "ogg_encode.h"
#include "apps/music/dir_sync/dir_sync.h"
#include "database/dbd.h"
#include "music_query.h"
#include <inttypes.h>
#include "transcoder.h"
#include "database/db_query_config.h"
#include "database/db_query_parameters.h"


int init_music_query(apr_pool_t* global_pool, error_messages_t* error_messages, const char* external_directory, const void** global_context){
	apr_status_t rv;
	int status;
	dir_sync_t* dir_sync;
	apr_thread_t* thread_sync_dir;
	music_globals_t* music_globals;
	const char* shared_directory;
	const char* dbd_error;

	char dbd_error_message[256];


	*global_context = music_globals = apr_pcalloc(global_pool, sizeof(music_globals_t));

	rv = apr_temp_dir_get(&shared_directory,global_pool);

	music_globals->dir_sync_shm_file = apr_pstrcat(global_pool,shared_directory,"/mp_dir_sync",NULL);
	music_globals->decoding_queue_shm_file = apr_pstrcat(global_pool,shared_directory,"/mp_decoding_queue",NULL);


	status = setup_shared_memory(&(music_globals->dir_sync_shm),sizeof(dir_sync_t),music_globals->dir_sync_shm_file, global_pool);
	if(status != 0){
		return status;
	}

	//Create thread to connect to database and synchronize
	dir_sync = (dir_sync_t*)apr_shm_baseaddr_get(music_globals->dir_sync_shm);

	dir_sync->pool = global_pool;
	dir_sync->num_files = NULL;
	dir_sync->dir_path = external_directory;
	dir_sync->error_messages = error_messages;
	dir_sync->sync_progress = 0.0;
	dir_sync->dbd_config = NULL;

	//We must connect to the database in the main thread as we have to wait for dynamic module loading
	// to unlock before forking.
	rv = connect_database(global_pool, dir_sync->error_messages,&(dir_sync->dbd_config));
	if(rv != APR_SUCCESS){
		add_error_list(error_messages, ERROR ,"Database error couldn't connect", apr_strerror(rv, dbd_error_message, sizeof(dbd_error_message)));
		return -5;
	}

	status = prepare_database(NULL,dir_sync->dbd_config, NULL);
	if(status != 0){
		dbd_error = apr_dbd_error(dir_sync->dbd_config->dbd_driver,dir_sync->dbd_config->dbd_handle, status);
		add_error_list(dir_sync->error_messages, ERROR, "Database error couldn't prepare",dbd_error);
		return -6;
	}

	rv = apr_thread_create(&thread_sync_dir,NULL, sync_dir, (void*) dir_sync, global_pool);
	if(rv != APR_SUCCESS){
		return rv;
	}

	apr_pool_cleanup_register(global_pool, music_globals->dir_sync_shm,(void*) apr_shm_destroy	, apr_pool_cleanup_null);

	rv = create_decoding_queue(global_pool, music_globals->decoding_queue_shm_file,&(music_globals->decoding_queue));
	if(rv != 0){
		return rv;
	}

	return 0;
}

int reattach_music_query(apr_pool_t* child_pool, error_messages_t* error_messages,const void* global_context){
	apr_status_t rv;
	music_globals_t* music_globals = (music_globals_t*) global_context;

	if(music_globals->dir_sync_shm_file){
		rv = apr_shm_attach(&(music_globals->dir_sync_shm), music_globals->dir_sync_shm_file, child_pool);
		if(rv != APR_SUCCESS){
			return rv;
		}
		music_globals->dir_sync_progress = &(((dir_sync_t*) apr_shm_baseaddr_get(music_globals->dir_sync_shm))->sync_progress);
	}

	rv = reattach_decoding_queue(child_pool, music_globals->decoding_queue, music_globals->decoding_queue_shm_file, error_messages);
	if(rv != 0){
		return rv;
	}
	return 0;
}

static int get_music_query(apr_pool_t* pool,error_messages_t* error_messages,music_query_t* music_query,query_words_t* query_words, apr_array_header_t* db_queries){

	const char** query_nouns = query_words->words;
	int status;
	const char* error_header = "Error With Music Query";

	int num_words;

	int noun_num;

	column_table_t* column;

	char cus_par_set = 0;//Flag custom parameters set in db query

	music_query->error_messages = error_messages;


	num_words = query_words->num_words;

	if(query_nouns == NULL || query_nouns[1] == NULL){
		//Not enough info
		add_error_list(error_messages, ERROR, error_header, "Query doesn't contain enough words.");
		return -3;
	}


	//Find type of query
	if (apr_strnatcasecmp(query_nouns[1], "songs") == 0){
		music_query->type = SONGS;
	}else if (apr_strnatcasecmp(query_nouns[1], "albums") == 0){
		music_query->type = ALBUMS;
	}else if (apr_strnatcasecmp(query_nouns[1], "artists") == 0){
		music_query->type = ARTISTS;
	}else if (apr_strnatcasecmp(query_nouns[1], "sources") == 0){
		music_query->type = SOURCES;
	}else if(apr_strnatcasecmp(query_nouns[1], "play") == 0){
		music_query->type = PLAY;
	}else if(apr_strnatcasecmp(query_nouns[1], "transcode") == 0){
		music_query->type = TRANSCODE;
	}else{
		//Invlaid type
		add_error_list(error_messages, ERROR, error_header, apr_pstrcat(pool,"Query type ",query_nouns[1]," is not supported by API.",NULL));
		return -2;

	}


	//We have valid type lets get DB query if set
	status = find_query_by_id(&(music_query->db_query),db_queries,query_nouns[1]);
	if(status != 0){
		add_error_list(error_messages, ERROR, error_header, apr_pstrcat(pool, "Query type ",query_nouns[1],"is not supported by Database.", NULL ));
		return -1;
	}

	if(num_words < 4){

		return 0;
	}

	init_query_parameters(pool,&(music_query->query_parameters));
	//Copy array
	if(music_query->db_query->custom_parameters){
		cus_par_set = 1;
		music_query->query_parameters->query_custom_parameters = apr_array_copy(pool,music_query->db_query->custom_parameters);
	}

	//Look for query parameters; every second uri part
	for(noun_num = 2; noun_num+1 < num_words; noun_num += 2){
		int sql_clauses;
		int sql_clause_found = 0;
		//Look for WHERE columns
		if(find_column_from_query_by_friendly_name(music_query->db_query,query_nouns[noun_num],&column) == 0){
			//Column found that matches freidnly name
			//Add column to list of where conditions
			add_where_query_parameter(music_query->query_parameters,column,query_nouns[noun_num+1]);
			//found a match continue for loop
			continue;
		}


		//Look for query parameter (row_count, offset, sort by)
		for(sql_clauses =0;sql_clauses < NUM_SQL_CLAUSES;sql_clauses++){
			if(music_query->query_parameters->query_sql_clauses[sql_clauses].freindly_name == NULL){
				//Skip null friendly names
				continue;
			}
			if(apr_strnatcasecmp(query_nouns[noun_num],music_query->query_parameters->query_sql_clauses[sql_clauses].freindly_name) == 0){
				music_query->query_parameters->query_sql_clauses[sql_clauses].value = query_nouns[noun_num+1];
				//set_query_parameters(music_query->query_parameters,SQL,sql_clauses);
				sql_clause_found = 1;
				break;
			}
		}
		if(sql_clause_found == 1){
			//found a match continue for loop
			continue;
		}

		if(cus_par_set == 1){

		custom_parameter_t* custom_parameter;
		//Look for custom parameters
		if(find_custom_parameter_by_friendly(music_query->query_parameters->query_custom_parameters,query_nouns[noun_num],&custom_parameter) == 0){
			//Change default value
			custom_parameter->value = apr_pstrdup(pool,query_nouns[noun_num+1]);
			continue;
		}

		}

		//Query noun is not valid abort
		add_error_list(error_messages,ERROR,error_header,apr_pstrcat(pool,"Query word ", query_nouns[noun_num], " is not supported by server.", NULL));
		return -9;
	}

	//Change sorty_by parameter to SQL command
	if(music_query->query_parameters->query_sql_clauses[ORDER_BY].value != NULL){
		column_table_t* column;
		if(find_column_from_query_by_friendly_name(music_query->db_query,music_query->query_parameters->query_sql_clauses[ORDER_BY].value,&column) == 0){
			music_query->query_parameters->query_sql_clauses[ORDER_BY].value = apr_pstrcat(pool,column->table->name,".",column->name,NULL);
		}else{
			add_error_list(error_messages, ERROR, "Error with query", apr_pstrcat(pool, "Column ",music_query->query_parameters->query_sql_clauses[ORDER_BY].value,"doesn't exists in the database schema.", NULL ));
			return -20;
		}

	}
	return 0;
}

int output_json(apr_pool_t* pool, apr_bucket_brigade* bb, music_query_t* query){
	int print_query_results = 0;

	if(query->db_query == NULL || query->db_query->id == NULL || query->results == NULL || query->results->rows == NULL){
		//Heavy debug
		add_error_list(query->error_messages, ERROR, "Error printing query","URI: %s, query->db_query: %d, query->results: %d, query->results->rows: %d");
	}else{
		print_query_results = 1;
	}

	apr_brigade_puts(bb, NULL,NULL, "{\n");

	print_error_messages(pool,bb,query->error_messages);
	if(print_query_results){
		int row_count = 0;
		int column_index = 0;
		row_t row;

		apr_brigade_printf(bb, NULL,NULL, ",\n\t\"%s\" :[\n",query->db_query->id);

		for(row_count = 0;row_count < query->results->rows->nelts;row_count++){
			row = APR_ARRAY_IDX(query->results->rows,row_count,row_t);
			apr_brigade_puts(bb, NULL,NULL, "\t\t\t{\n");
			for(column_index = 0;column_index < query->db_query->select_columns->nelts;column_index++){
				apr_brigade_printf(bb, NULL,NULL, "\t\t\t\t\"%s\": \"%s\"",APR_ARRAY_IDX(query->db_query->select_columns,column_index,column_table_t*)->freindly_name,json_escape_char(pool,row.results[column_index]));
				if(column_index+1 < query->db_query->select_columns->nelts){
					apr_brigade_puts(bb, NULL,NULL, ", ");
				}
				apr_brigade_puts(bb, NULL,NULL, "\n");
			}
			apr_brigade_puts(bb, NULL,NULL, "\t\t\t}");
			if(row_count+1 < query->results->rows->nelts){
				apr_brigade_puts(bb, NULL,NULL, ",");
			}
		}
		apr_brigade_puts(bb, NULL,NULL, "\n\t]");
	}
	apr_brigade_puts(bb, NULL,NULL, "\n}\n");
	return 0;
}

int run_music_query(apr_pool_t* pool,apr_pool_t* global_pool, apr_bucket_brigade* output_bb, apr_table_t* output_headers, const char* output_content_type,error_messages_t* error_messages, db_config* dbd_config, query_words_t* query_words, apr_array_header_t* db_queries,const void* global_context){
	int error_num = 0;
	int status;
	//We should check if database is connected somewhere

	music_query_t* music_query = apr_pcalloc(pool, sizeof(music_query_t));

	if(db_queries == NULL){
		output_status_json(pool,output_bb,output_headers, output_content_type,error_messages);
		return OK;
	}

	status = get_music_query(pool,error_messages, music_query, query_words, db_queries);
	if(status != 0){
		output_status_json(pool,output_bb,output_headers, output_content_type,error_messages);
		return OK;
	}
	music_query->music_globals = (music_globals_t*) global_context;
	music_query->output_bb = output_bb;
	music_query->output_headers = output_headers;
	music_query->output_content_type = output_content_type;


	if(music_query->db_query == NULL){
		output_status_json(pool,output_bb,output_headers, output_content_type,error_messages);
		return OK;
	}
	error_num = select_db_range(pool,dbd_config, music_query->query_parameters,music_query->db_query,&(music_query->results),music_query->error_messages);
	if(error_num != 0 && music_query->results != NULL){
		add_error_list(music_query->error_messages, ERROR,"Select DB Range Error", "Couln't run music query");
		output_status_json(pool,output_bb,output_headers, output_content_type,error_messages);
		return OK;
	}else{
		switch(music_query->type){
			case SONGS:
			case ALBUMS:
			case ARTISTS:
			case SOURCES:{
				apr_table_add(output_headers,"Access-Control-Allow-Origin", "*");
				apr_cpystrn(output_content_type, "application/json", 255);
				error_num = output_json(pool,output_bb,music_query);
				break;
			}
			case PLAY:{
				error_num = play_song(pool,dbd_config,music_query);
				 if(error_num != 0){
					 add_error_list(music_query->error_messages, ERROR, "Error playing audio", apr_itoa(pool, error_num));
					 output_status_json(pool,output_bb,output_headers, output_content_type,error_messages);
				 }
				break;
			}
			case TRANSCODE:{
				apr_table_add(output_headers,"Access-Control-Allow-Origin", "*");
				apr_cpystrn(output_content_type, "application/json", 255);
				 error_num = transcode_audio(pool, global_pool, dbd_config,music_query);
				 if(error_num != 0){
					 add_error_list(music_query->error_messages, ERROR, "Error transcoding audio", apr_itoa(pool, error_num));
					 output_status_json(pool,output_bb,output_headers, output_content_type,error_messages);
				 }
				 break;
			}
			default:{
				error_num = output_status_json(pool,output_bb,output_headers, output_content_type,error_messages);
				 break;
			}
		}
	}
	return OK;
}
