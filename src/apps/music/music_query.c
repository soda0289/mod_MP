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


int get_music_query(apr_pool_t* pool,error_messages_t* error_messages,app_query* query,query_words_t* query_words, apr_array_header_t* db_queries){

	const char** query_nouns = query_words->words;
	int status;
	music_query_t* music_query;

	music_query_t** music_query_ptr = (music_query_t**) query;

	int num_words;
	int table_elem;
	int col_elem;
	int noun_num;
	int num_columns = 0;

	table_t* table;
	column_table_t* column;

	query_t* db_query;

	char cus_par_set = 0;//Flag custom parameters set in db query

	*music_query_ptr = apr_pcalloc(pool,sizeof(music_query_t));
	music_query = *music_query_ptr;
	music_query->error_messages = error_messages;


	num_words = query_words->num_words;

	if(query_nouns == NULL || query_nouns[1] == NULL){
		//Not enough info
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
		return -2;
		//Invlaid type
	}


	//We have valid type lets get DB query if set
	status = find_query_by_id(&(music_query->db_query),db_queries,query_nouns[1]);
	if(status != 0){
		//no query found
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
		return -9;
	}

	//Change sorty_by parameter to SQL command
	if(music_query->query_parameters->query_sql_clauses[ORDER_BY].value != NULL){
		column_table_t* column;
		if(find_column_from_query_by_friendly_name(music_query->db_query,music_query->query_parameters->query_sql_clauses[ORDER_BY].value,&column) == 0){
			music_query->query_parameters->query_sql_clauses[ORDER_BY].value = apr_pstrcat(pool,column->table->name,".",column->name,NULL);
		}else{
			return -20;
		}

	}
	return 0;
}

int output_json(request_rec* r, music_query_t* query){

	mediaplayer_rec_cfg* rec_cfg = ap_get_module_config(r->request_config, &mediaplayer_module);

	//Apply header
	apr_table_add(r->headers_out, "Access-Control-Allow-Origin", "*");
	ap_set_content_type(r, "application/json") ;

	ap_rputs(	"{\n",r);
	print_error_messages(r, rec_cfg->error_messages);
	ap_rprintf(r, ",\n\t\"%s\" :[\n",query->db_query->id);

	if(query->results != NULL && query->results->rows != NULL){
		int row_count = 0;
		int column_index = 0;
		row_t row;
		for(row_count = 0;row_count < query->results->rows->nelts;row_count++){
			row = APR_ARRAY_IDX(query->results->rows,row_count,row_t);
			ap_rputs("\t\t\t{\n",r);
			for(column_index = 0;column_index < query->db_query->select_columns->nelts;column_index++){
				ap_rprintf(r,"\t\t\t\t\"%s\": \"%s\"",APR_ARRAY_IDX(query->db_query->select_columns,column_index,column_table_t*)->freindly_name,json_escape_char(r->pool,row.results[column_index]));
				if(column_index+1 < query->db_query->select_columns->nelts){
					ap_rputs(", ",r);
				}
				ap_rputs("\n",r);
			}
			ap_rputs("\t\t\t}",r);
			if(row_count+1 < query->results->rows->nelts){
				ap_rputs(",",r);
			}
			ap_rputs("\n",r);
		}
	}
	ap_rputs(	"\t]\n}\n",r);
	return 0;
}

int run_music_query(request_rec* r, app_query app_query,db_config* dbd_config, apr_dbd_prepared_t**** select){
	int error_num = 0;
	//We should check if database is connected somewhere

	music_query_t* music =(music_query_t*)app_query;

	if(music->db_query == NULL){
		 output_status_json(r);
		 return OK;
	}
	error_num = select_db_range(r->pool,dbd_config, music->query_parameters,music->db_query,&(music->results),music->error_messages);
	if(error_num != 0){
		add_error_list(music->error_messages, ERROR,"Select DB Range Error", "Couln't run music query");
		output_status_json(r);
		return OK;
	}else{
		switch(music->type){
			case SONGS:{
				output_json(r,music);
				break;
			}
			case ALBUMS:{
				output_json(r,music);
				break;
			}
			case ARTISTS:{
				output_json(r,music);
				break;
			}
			case SOURCES:{
				output_json(r,music);
				break;
			}
			case PLAY:{
				//output_json(r,music);
				play_song(dbd_config,r,music);
				break;
			}
			case TRANSCODE:{
				 transcode_audio(r,dbd_config,music);
				 break;
			}
			default:{
				output_status_json(r);
				 break;
			}
		}
	}
	return OK;
}
