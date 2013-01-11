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

#include "database/db_query_config.h"

static void setup_sql_clause(query_sql_clauses_t** clauses,sql_clauses type,const char* fname){
	(*clauses)[type].freindly_name = fname;
}

static void set_query_parameters(query_parameters_t* query_parameters,parameter_type type, char index){

	switch(type){
		case(WHERE):{
			query_parameters->parameters_set |= (1 << index);
			break;
		}
		case(SQL):{
			query_parameters->parameters_set |= (1 << (index + query_parameters->num_columns));
			break;
		}
		case(CUSTOM):{
			query_parameters->parameters_set |= (1 << (index + query_parameters->num_columns + NUM_SQL_CLAUSES));
			break;
		}
	}
}

static int get_query_parameters(query_parameters_t* query_parameters,parameter_type type, char index){

	switch(type){
		case(WHERE):{
			return query_parameters->parameters_set & (1 << index);
			break;
		}
		case(SQL):{
			return query_parameters->parameters_set & (1 << (index + query_parameters->num_columns));
			break;
		}
		case(CUSTOM):{
			return query_parameters->parameters_set & (1 << (index + query_parameters->num_columns + NUM_SQL_CLAUSES));
			break;
		}
	}
	return -1;
}

int get_music_query(apr_pool_t* pool,error_messages_t* error_messages,app_query* query,query_words_t* query_words, apr_array_header_t* db_queries){

	const char** query_nouns = query_words->words;
	int status;

	music_query_t** music_query_ptr = (music_query_t**) query;
	*music_query_ptr = apr_pcalloc(pool,sizeof(music_query_t));
	music_query_t* music_query = *music_query_ptr;
	music_query->error_messages = error_messages;

	int num_words = query_words->num_words;
	int table_elem;
	int col_elem;
	int noun_num;
	int num_columns = 0;

	table_t* table;
	column_table_t* column;
	query_where_condition_t* query_where_condition;
	query_t* db_query;

	char cus_par_set = 0;//Flag custom parameters set in db query

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

	//INITALIZE query parameters
	//should really be moved to a separate file
	music_query->query_parameters = apr_pcalloc(pool,sizeof(query_parameters_t));

	music_query->query_parameters->query_sql_clauses = apr_pcalloc(pool,sizeof(query_sql_clauses_t )* NUM_SQL_CLAUSES);
	setup_sql_clause(&music_query->query_parameters->query_sql_clauses,LIMIT,"limit");
	setup_sql_clause(&music_query->query_parameters->query_sql_clauses,OFFSET,"offset");
	//Group by has no friendly name because it cannot be accessed by client
	setup_sql_clause(&music_query->query_parameters->query_sql_clauses,GROUP_BY,NULL);
	setup_sql_clause(&music_query->query_parameters->query_sql_clauses,ORDER_BY,"sort_by");

	//Copy array
	if(music_query->db_query->custom_parameters){
		cus_par_set = 1;
	music_query->query_parameters->query_custom_parameters = apr_array_copy(pool,music_query->db_query->custom_parameters);
	}
	music_query->query_parameters->query_where_conditions = apr_array_make(pool,10,sizeof(query_where_condition_t));
	//Look for query parameters; every second uri part
	for(noun_num = 2; noun_num+1 < num_words; noun_num += 2){
		int sql_clause_found = 0;
		//Look for WHERE columns
		if(find_column_from_query_by_friendly_name(music_query->db_query,query_nouns[noun_num],&column) == 0){
			//Column found that matches freidnly name
			//Add column to list of where conditions
			query_where_condition = apr_array_push(music_query->query_parameters->query_where_conditions);
			query_where_condition->column = column;
			if(strchr(query_nouns[noun_num+1],'*') == NULL){
				query_where_condition->operator = EQUAL;
			}else{
				//Query word contains asterisk
				//Replace them all with % sign for MySQL
				char* asterisk = query_nouns[noun_num+1];
				while((asterisk = strchr(asterisk, '*')) != NULL){
					asterisk[0] = '%';
				}
				//Since query word contains % change to operator to LIKE
				query_where_condition->operator = LIKE;
			}
			query_where_condition->condition = query_nouns[noun_num+1];
			//set_query_parameters(music_query->query_parameters,WHERE,music_query->query_parameters->num_columns);
			//found a match continue for loop
			continue;
		}
		//Look for query parameter (row_count, offset, sort by)
		int sql_clauses;

		for(sql_clauses =0;sql_clauses < NUM_SQL_CLAUSES;sql_clauses++){
			if(music_query->query_parameters->query_sql_clauses[sql_clauses].freindly_name == NULL){
				//Skip null friendly names
				continue;
			}
			if(apr_strnatcasecmp(query_nouns[noun_num],music_query->query_parameters->query_sql_clauses[sql_clauses].freindly_name) == 0){
				music_query->query_parameters->query_sql_clauses[sql_clauses].value = query_nouns[noun_num+1];
				//set_query_parameters(music_query->query_parameters,SQL,sql_clauses);
				sql_clause_found = 1;
				continue;
			}
		}
		if(sql_clause_found == 1){
			//found a match continue for loop
			continue;
		}

		if(cus_par_set == 0){
			//custom parameters not set continue for loop
			continue;
		}

		custom_parameter_t* custom_parameter;
		//Look for custom parameters
		if(find_custom_parameter_by_friendly(music_query->query_parameters->query_custom_parameters,query_nouns[noun_num],&custom_parameter) == 0){
			//Change default value
			custom_parameter->value = apr_pstrdup(pool,query_nouns[noun_num+1]);
			continue;
		}

		//Query noun is not valid abort
		return -9;
	}

	//Change sorty_by parameter to SQL command
	if(get_query_parameters(music_query->query_parameters,SQL,ORDER_BY)){
		for(table_elem = 0;table_elem < music_query->db_query->tables->nelts;table_elem++){
			table = APR_ARRAY_IDX(music_query->db_query->tables,table_elem,table_t*);
			//table = &(((table_t*)music->db_query->tables->elts)[table_elem]);
			for(col_elem = 0;col_elem < table->columns->nelts;col_elem++){
				//For each column in query check if freindly names math
				column = &(APR_ARRAY_IDX(table->columns,col_elem,column_table_t));
				//Add to out column count used to calculate parameter set binary flag
				(num_columns)++;
				//column =(((column_table_t*)table.columns->elts)[col_elem]);
				if(apr_strnatcasecmp(music_query->query_parameters->query_sql_clauses[ORDER_BY].value,column->freindly_name) == 0){
					music_query->query_parameters->query_sql_clauses[ORDER_BY].value = apr_pstrcat(pool,column->table->name,".",column->name,NULL);
				}
			}
		}
	}
	return 0;
}

int output_json(request_rec* r, music_query_t* query){

	output_status_json(r);

	//Print query
	switch(query->type){
		case SONGS:
			ap_rputs("\"songs\" : [", r);
			break;
		case ARTISTS:
			ap_rputs("\"artists\" : [", r);
			break;
		case ALBUMS:
			ap_rputs("\"albums\" : [", r);
			break;
	}

	if(query->results != NULL && query->results->rows != NULL){
		int row_count = 0;
		int column_index = 0;
		row_t row;
		for(row_count = 0;row_count < query->results->rows->nelts;row_count++){
			row = APR_ARRAY_IDX(query->results->rows,row_count,row_t);
			ap_rputs("{",r);
			for(column_index = 0;column_index < query->db_query->select_columns->nelts;column_index++){
				ap_rprintf(r,"\"%s\": \"%s\" ",APR_ARRAY_IDX(query->db_query->select_columns,column_index,column_table_t*)->freindly_name,json_escape_char(r->pool,row.results[column_index]));
				if(column_index+1 < query->db_query->select_columns->nelts){
					ap_rputs(", ",r);
				}
			}
			ap_rputs("}",r);
			if(row_count+1 < query->results->rows->nelts){
				ap_rputs(",\n",r);
			}
		}
	}
	ap_rputs(	"]}",r);
	return 0;
}

int run_music_query(request_rec* r, app_query app_query,db_config* dbd_config, apr_dbd_prepared_t**** select){
	int error_num = 0;
	//We should check if database is connected somewhere

	music_query_t* music =(music_query_t*)app_query;

	error_num = select_db_range(dbd_config, select,music);

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
			ogg_encode(r,dbd_config,music);
			break;
		}
		default:{
			output_json(r,music);
						break;
		}
	}
	return OK;
}
