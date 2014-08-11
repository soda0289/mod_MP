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

#include "mod_mp.h"

#include "pull_song.h"

#include "indexers/music/dir_sync/dir_sync.h"

#include "database/db_typedef.h"
#include "database/db.h"
#include "database/db_query.h"
#include "database/db_query_parameters.h"

#include "music_query.h"
#include <inttypes.h>
#include "transcoder.h"

int read_xml_parameters(apr_xml_elem* xml_params, music_globals_t* music_globals){
	int status = 0;
	apr_xml_elem* xml_param;
	apr_xml_elem* xml_dir;
	apr_size_t max_element_size = 255;

	for(xml_param = xml_params->first_child; xml_param != NULL; xml_param = xml_param->next){
		if(apr_strnatcmp(xml_param->name, "music_dirs") == 0){
			music_globals->music_dirs = apr_array_make(music_globals->pool, 3, sizeof(dir_t));
			for(xml_dir = xml_param->first_child; xml_dir != NULL; xml_dir = xml_dir->next){
				dir_t* dir = apr_array_push(music_globals->music_dirs);

				apr_xml_to_text(music_globals->pool,xml_dir,APR_XML_X2T_INNER,NULL,NULL,(const char**)&(dir->path),&max_element_size);
			}
		}
	}

	return status;
}


int music_indexer_init (apr_pool_t* pool, error_messages_t* error_messages, apr_array_header_t* db_objects, apr_array_header_t* settings, const void** global_data){
	int status;
	music_globals_t* music_globals;

	//Init Music Globals
	music_globals = *data = apr_pcalloc(pool, sizeof(music_globals_t));

	music_globals->pool = pool;
	music_globals->error_messages = error_messages;

	music_globals->db_objects = db_objects;

	status = apr_temp_dir_get(&(music_globals->tmp_dir),pool);

	//Create decoding queue
	status = create_decoding_queue(music_globals);

	//Read parameters from xml
	read_xml_parameters(xml_params, music_globals);

	//Start syncing directories with the database
	status = init_dir_sync(music_globals);
	if(status != 0){
		return status;
	}

	return 0;
}

int music_indexer_on_fork(apr_pool_t* child_pool, error_messages_t* error_messages,const void* global_context){
	apr_status_t rv = 0;

	music_globals_t* music_globals = (music_globals_t*) global_context;
	
	//Update Music Globals
	music_globals->pool = child_pool;
	music_globals->error_messages = error_messages;

	//Reattach Directory Sync
	reattach_dir_sync(music_globals);
	
	if(music_globals->decoding_queue != NULL){
		rv = reattach_decoding_queue(music_globals);
	}
	if(rv != 0){
		return rv;
	}
	return 0;
}

int get_music_query(music_query_t* music_query){
	apr_pool_t* pool = music_query->pool;
	error_messages_t* error_messages = music_query->error_messages;

	const char** query_nouns = music_query->input->query_words->words;
	int status = 0;
	const char* error_header = "Error With Music Query";

	int num_words = music_query->input->query_words->num_words;

	int noun_num;

	column_table_t* column;

	char cus_par_set = 0;//Flag custom parameters set in db query



	if(query_nouns == NULL || query_nouns[1] == NULL){
		//Not enough info
		error_messages_add(music_query->error_messages, ERROR, error_header, "Query doesn't contain enough words.");
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
	}else if(apr_strnatcasecmp(query_nouns[1], "pull") == 0){
		music_query->type = PLAY;
	}else if(apr_strnatcasecmp(query_nouns[1], "transcode") == 0){
		music_query->type = TRANSCODE;
	}else if(apr_strnatcasecmp(query_nouns[1], "status") == 0){
		music_query->type = STATUS;
	}else{
		//Invlaid type
		error_messages_add(music_query->error_messages, ERROR, error_header, apr_pstrcat(music_query->pool,"Query type ",query_nouns[1]," is not supported by API.",NULL));
		return -2;

	}


	//We have valid type lets get DB query if set
	status = find_query_by_id(&(music_query->db_query),music_query->globals->db_queries,query_nouns[1]);
	if(status != 0){
		error_messages_add(music_query->error_messages, ERROR, error_header, apr_pstrcat(music_query->pool, "Query type ",query_nouns[1]," is not supported by Database.", NULL ));
		return -1;
	}

	if(num_words < 4){

		return 0;
	}

	db_query_parameters_init (music_query->pool, &(music_query->db_query_parameters));

	/*
	//Copy array
	if(music_query->db_query->custom_parameters){
		cus_par_set = 1;
		music_query->query_parameters->custom_parameters = apr_array_copy(pool,music_query->db_query->custom_parameters);
	}
	*/

	//Look for query parameters; every second uri part
	for(noun_num = 2; noun_num+1 < num_words; noun_num += 2){
		int sql_clauses;
		int sql_clause_found = 0;
		//Look for WHERE columns
		if(find_column_from_query_by_friendly_name(music_query->db_query,query_nouns[noun_num],&column) == 0){
			//Column found that matches friendly name
			//Add column to list of where conditions
			status = db_query_parameters_add_where(music_query->db_query_parameters, column, query_nouns[noun_num+1]);
			if(status != 0){
				error_messages_add(error_messages, ERROR,"Error with query", "There was an error adding the where query parameter");
			}
			//found a match continue for loop
			continue;
		}


		//Look for query parameter (row_count, offset, sort by)
		for(sql_clauses =0;sql_clauses < NUM_SQL_CLAUSES;sql_clauses++){
			query_sql_clauses_t* query_clause = &(music_query->db_query_parameters->sql_clauses[sql_clauses]);


			if(query_clause->freindly_name == NULL){
				//Skip null friendly names
				continue;
			}
			if(apr_strnatcasecmp(query_nouns[noun_num],query_clause->freindly_name) == 0){
				apr_size_t max_size = 255; 
				char* sort_col_list = apr_pstrndup(pool, query_nouns[noun_num+1], max_size);
				
				//Check if friendly name is sort by
				if(sql_clauses == ORDER_BY){
					char* last; //Last state of strtol
					char* col_fname;
					

					//break value entered into segments check for +/- modifier and
					//convert column friendlt name into real table dot column name
					for(col_fname = apr_strtok(sort_col_list, ",", &last); col_fname != NULL; col_fname = apr_strtok(NULL, ",", &last)){
						char a_or_d[6] = "";
						column_table_t* column;
						
						if(col_fname[0] == '+'){
							//Split string
							strncpy(a_or_d, " ASC",6);
							col_fname++;
						}else if(col_fname[0] == '-'){
							//Split string
							strncpy(a_or_d, " DESC", 6);
							col_fname++;
						}
							
						//Check each comma sperated item and make sure it is valid column friendly name
						//If it is replace witha actual table name dotted with acutal column name
						if(find_column_from_query_by_friendly_name(music_query->db_query,col_fname,&column) == 0){
							
							if(query_clause->value == NULL){
								query_clause->value = apr_pstrcat(pool, column->table->name,".",column->name, a_or_d ,NULL);

							}else{
								query_clause->value = apr_pstrcat(pool, query_clause->value, ",",column->table->name,".",column->name, a_or_d ,NULL);
							}

						}else{
							error_messages_add(error_messages, ERROR, "Error with query", apr_pstrcat(pool, "Column ",col_fname," doesn't exists in the database schema.", NULL ));
						}

							
					}				
				}else{
					query_clause->value = query_nouns[noun_num+1];
				}
				
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
			if(db_query_parameters_find_custom_by_friendly (music_query->db_query_parameters, query_nouns[noun_num], &custom_parameter) == 0){
				//Change default value
				custom_parameter->value = apr_pstrdup(pool,query_nouns[noun_num+1]);
				continue;
			}
		}

		//Query noun is not valid abort
		error_messages_add(error_messages,ERROR,error_header,apr_pstrcat(pool,"Query word ", query_nouns[noun_num], " is not supported by server.", NULL));
		return -9;
	}

	//Change sorty_by parameter to SQL command
	/*
	if(music_query->query_parameters->query_sql_clauses[ORDER_BY].value != NULL){
		

	}
	*/

	return 0;
}


int output_db_query_json(music_query_t* query){
	apr_pool_t* pool = query->output->pool;
	apr_bucket_brigade* bb = query->output->bucket_brigade;
	int print_query_results = 0;

	apr_table_add(query->output->headers,"Access-Control-Allow-Origin", "*");
	apr_cpystrn((char*)query->output->content_type, "application/json", 255);

	if(query->db_query == NULL || query->db_query->id == NULL || query->results == NULL || query->results->rows_array == NULL){
		//Heavy debug
		error_messages_add(query->error_messages, ERROR, "Error printing query","URI: %s, query->db_query: %d, query->results: %d, query->results->rows: %d");
	}else{
		print_query_results = 1;
	}

	error_messages_duplicate(query->error_messages, query->db_query->db_config->error_messages, query->pool);

	apr_brigade_puts(bb, NULL,NULL, "{\n");

	error_messages_print_json_bb (query->error_messages, pool,bb);
	if(print_query_results){
		apr_brigade_printf(bb, NULL,NULL, ",\n\t\"%s\" :[\n", query->db_query->id);

		output_db_result_json(query->results, query->db_query, query->output);

		apr_brigade_puts(bb, NULL,NULL, "\n\t]");
	}
	apr_brigade_puts(bb, NULL,NULL, "\n}\n");
	return 0;
}

int music_indexer_query (input_t* input, output_t* output, const void* global_context){
	int error_num = 0;
	int status;

	//Setup music query
	music_query_t* music_query = apr_pcalloc(output->pool, sizeof(music_query_t));
	music_query->globals = (music_globals_t*) global_context;
	music_query->input = input;
	music_query->output = output;
	music_query->pool = output->pool;
	music_query->error_messages = output->error_messages;

	if(music_query->globals->db_queries == NULL){
		output_status_json(output);
		return OK;
	}

	status = get_music_query(music_query);
	if(status != 0){
		output_status_json(output);
		return OK;
	}

	if(music_query->db_query == NULL){
		output_status_json(output);
		return OK;
	}

	error_num = db_query_run(music_query->db_query, music_query->db_query_parameters, &(music_query->results));

	if(error_num != 0 && music_query->results != NULL){
		error_messages_add(music_query->error_messages, ERROR,"Select DB Range Error", "Couln't run music query");
		output_status_json(output);
		return OK;
	}else{
		switch(music_query->type){
			case SONGS:
			case ALBUMS:
			case ARTISTS:
			case SOURCES:{
				error_num = output_db_query_json(music_query);
				break;
			}
			case PLAY:{
				error_num = pull_song(music_query);
				 if(error_num != 0){
					 error_messages_add(music_query->error_messages, ERROR, "Error playing audio", apr_itoa(music_query->pool, error_num));
					 output_status_json(output);
				 }
				break;
			}
			case TRANSCODE:{
				apr_table_add(output->headers,"Access-Control-Allow-Origin", "*");
				apr_cpystrn((char*)output->content_type, "application/json", 255);
				error_num = transcode_audio(music_query);
				if(error_num != 0){
					error_messages_add(music_query->error_messages, ERROR, "Error transcoding audio", apr_itoa(music_query->pool, error_num));
					output_status_json(output);
				}
				break;
			}
			case STATUS:{
				output_dirsync_status(music_query);
				break;
			}
			default:{
				error_num = output_status_json(output);
				 break;
			}
		}
	}
	return OK;
}
