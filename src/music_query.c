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
#include "dir_sync.h"

static inline void set_query_parameter(enum parameter_types type, char* parameter_string, music_query* music){
	music->query_parameters[type].query_paramter_string = parameter_string;
	music->query_parameters[type].parameter_value = NULL;
}

int get_music_query(request_rec* r,music_query* music){
	//How many options can we pass in uri
	/*
	 * query_nouns[0] Application (music, video,files)
	 * query_nouns[1]  Query Type
	 * Query Parameters * 2 if every parameter is set
	 */
	char* query_nouns[NUM_QUERY_PARAMETERS * 2 + 2] = {0};

	int i = 0;
	int noun_num, parameter_num;

	char* uri_cpy = apr_pstrdup(r->pool, r->uri);
	char* uri_slash;

	//Add error messages to query
	mediaplayer_rec_cfg* rec_cfg = ap_get_module_config(r->request_config, &mediaplayer_module);
	music->error_messages = rec_cfg->error_messages;


	//Remove leading slash and add trailing slash if one doesn't exsits.
	uri_cpy++;//Remove leading slash
	if (uri_cpy[strlen(uri_cpy) - 1] != '/'){
		uri_cpy = apr_pstrcat(r->pool, uri_cpy, "/", NULL);
	}

	//Fill array query_nouns with uri parts
	while ((uri_slash= strchr(uri_cpy, '/')) != NULL && i <= NUM_QUERY_PARAMETERS){
		 uri_slash[0] = '\0';
		 query_nouns[i] = uri_cpy;
		 uri_cpy = ++uri_slash;
		 i++;
	}
	//Check if we get the minimum amount of data for a query
	if(query_nouns[0] == NULL || query_nouns[1] == NULL){
		return -1;
	}

	//Find type of query
	if (apr_strnatcasecmp(query_nouns[1], "songs") == 0){
		music->type = SONGS;
	}else if (apr_strnatcasecmp(query_nouns[1], "albums") == 0){
		music->type = ALBUMS;
	}else if (apr_strnatcasecmp(query_nouns[1], "artists") == 0){
		music->type = ARTISTS;
	}else if(apr_strnatcasecmp(query_nouns[1], "play") == 0){
		music->type = PLAY;
	}else if(apr_strnatcasecmp(query_nouns[1], "transcode") == 0){
			music->type = TRANSCODE;
	}else{
		return -2;
		//Invlaid type
	}

	if(i < 4){
		return 0;
	}

/*
 * 	SONG_ID =0,
	SONG_NAME,
	ARTIST_ID,
	ARTIST_NAME,
	ALBUM_ID,
	ALBUM_NAME,
	ALBUM_YEAR,
	SOURCE_TYPE,
	SOURCE_ID,
	SORT_BY
 */
	set_query_parameter(SONG_ID, "song_id", music);
	set_query_parameter(SONG_NAME, "song_name", music);
	set_query_parameter(ARTIST_ID, "artist_id", music);
	set_query_parameter(ARTIST_NAME, "artist_name", music);
	set_query_parameter(ALBUM_ID, "album_id", music);
	set_query_parameter(ALBUM_NAME, "album_name", music);
	set_query_parameter(ALBUM_YEAR, "album_year", music);
	set_query_parameter(SOURCE_TYPE, "source_type", music);
	set_query_parameter(SOURCE_ID, "source_id", music);
	set_query_parameter(SORT_BY,"sort_by", music);
	set_query_parameter(ROWCOUNT,"row_count", music);
	set_query_parameter(OFFSET,"offset", music);


	for(noun_num = 2; noun_num+1 < i; noun_num += 2){
		for(parameter_num = 0; parameter_num < NUM_QUERY_PARAMETERS; parameter_num++){
			if(apr_strnatcasecmp(query_nouns[noun_num],music->query_parameters[parameter_num].query_paramter_string) == 0){
				music->query_parameters[parameter_num].parameter_value = query_nouns[noun_num+1];
				music->query_parameters_set |= 1 << parameter_num;
			}
		}
	}
	//Change artist_name/album_name and add percent sign
	if(music->query_parameters_set & (1 <<ALBUM_NAME) ){
		if(music->query_parameters[ALBUM_NAME].parameter_value){
			music->query_parameters[ALBUM_NAME].parameter_value = apr_pstrcat(r->pool, music->query_parameters[ALBUM_NAME].parameter_value,"%",NULL);
		}
	}
	if(music->query_parameters_set & (1 << ARTIST_NAME)){
		if(music->query_parameters[ARTIST_NAME].parameter_value){
			music->query_parameters[ARTIST_NAME].parameter_value = apr_pstrcat(r->pool, music->query_parameters[ARTIST_NAME].parameter_value,"%",NULL);
		}
	}

	//Change sorty_by parameter to SQL command
	if(music->query_parameters_set & (1 << SORT_BY)){

		if (apr_strnatcasecmp(music->query_parameters[SORT_BY].parameter_value, "+titles") == 0){
			music->query_parameters[SORT_BY].parameter_value = "Songs.name ASC";
		}else if (apr_strnatcasecmp(music->query_parameters[SORT_BY].parameter_value, "+albums") == 0){
			music->query_parameters[SORT_BY].parameter_value = "Albums.name ASC";
		}else if (apr_strnatcasecmp(music->query_parameters[SORT_BY].parameter_value, "+artists") == 0){
			music->query_parameters[SORT_BY].parameter_value = "Artists.name ASC";
		}else 	if (apr_strnatcasecmp(music->query_parameters[SORT_BY].parameter_value, "-titles") == 0){
			music->query_parameters[SORT_BY].parameter_value = "Songs.name DESC";
		}else if (apr_strnatcasecmp(music->query_parameters[SORT_BY].parameter_value, "-albums") == 0){
			music->query_parameters[SORT_BY].parameter_value = "Albums.name DESC";
		}else if (apr_strnatcasecmp(music->query_parameters[SORT_BY].parameter_value, "-artists") == 0){
			music->query_parameters[SORT_BY].parameter_value = "Artists.name DESC";
		}else{
			//Unset the SORT_BY
			music->query_parameters_set &= ~(1 << SORT_BY);
			return -3;
			//Invalid sort by
		}
	}




	/*
	//Should we find sorting method
	if (music->types == PLAY){
		music->by_id.id = query_nouns[2];
		music->by_id.id_type = SONGS;
		return 0; //We have a complete query
	}else{
		//Find what to sort on Descending/Ascending
		if (apr_strnatcasecmp(query_nouns[2], "+titles") == 0){
			music->sort_by = ASC_TITLES;
		}else if (apr_strnatcasecmp(query_nouns[2], "+albums") == 0){
			music->sort_by = ASC_ALBUMS;
		}else if (apr_strnatcasecmp(query_nouns[2], "+artists") == 0){
			music->sort_by = ASC_ARTISTS;
		}else 	if (apr_strnatcasecmp(query_nouns[2], "-titles") == 0){
			music->sort_by = DSC_TITLES;
		}else if (apr_strnatcasecmp(query_nouns[2], "-albums") == 0){
			music->sort_by = DSC_ALBUMS;
		}else if (apr_strnatcasecmp(query_nouns[2], "-artists") == 0){
			music->sort_by = DSC_ARTISTS;
		}else{
			return -3;
			//Invlaid sort
		}
*/

	return 0;
}

int run_music_query(request_rec* r, music_query* music){
	int error_num = 0;
	//We should check if database is connected somewhere

	mediaplayer_srv_cfg* srv_conf = ap_get_module_config(r->server->module_config, &mediaplayer_module);
	error_num = select_db_range(srv_conf->dbd_config, music);

	switch(music->type){
		case SONGS:{
			output_json(r);
			break;
		}
		case ALBUMS:{
			output_json(r);
			break;
		}
		case ARTISTS:{
			output_json(r);
			break;
		}
		case SOURCES:{
			output_json(r);
			break;
		}
		case PLAY:{
			break;
		}
		case TRANSCODE:{
			break;
		}
		default:{
			output_json(r);
						break;
		}
	}
	return OK;
}

char* json_escape_char(apr_pool_t* pool, const char* string){

	int i;
	char* escape_string = apr_pstrdup(pool, string);

	for (i = 0; i < strlen(escape_string); i++){
		if (escape_string[i] == '"'){
			escape_string[i] = '\0';
			escape_string = apr_pstrcat(pool, &escape_string[0], "\\\"", &escape_string[++i], NULL);
		}
	}

	return escape_string;
}

static int print_songs_json(void *rec, const char *key, const char *value){
	request_rec* r = (request_rec*) rec;
	mediaplayer_rec_cfg* rec_cfg = ap_get_module_config(r->request_config, &mediaplayer_module);

	ap_rprintf(r, "{%s}",  value);
	if (--(rec_cfg->query->results->song_count) > 0){
		ap_rprintf(r, ",");
	}
	ap_rprintf(r, "\n");
	return 10;
}

int output_json(request_rec* r){
	mediaplayer_srv_cfg* srv_conf = ap_get_module_config(r->server->module_config, &mediaplayer_module) ;
	dir_sync_t* dir_sync = apr_shm_baseaddr_get(srv_conf->dir_sync_shm);

	mediaplayer_rec_cfg* rec_cfg = ap_get_module_config(r->request_config, &mediaplayer_module);
	//Apply header
	apr_table_add(r->headers_out, "Access-Control-Allow-Origin", "*");
	ap_set_content_type(r, "application/json") ;

	//Print Status
	ap_rputs("{\n\t\"status\" : {", r);
		ap_rprintf(r, "\t\"Progress\" :  \"%.2f\",\n", dir_sync->sync_progress);

		ap_rputs("\"Errors\" : [\n", r);
			//Print Errors
	int i;
	for (i =0;i < rec_cfg->error_messages->num_errors; i++){
		ap_rprintf(r, "\t{\"type\" : %d,\n\t\"header\" : \"%s\",\n\t\"message\" : \"%s\"}\n", rec_cfg->error_messages->messages[i].type, rec_cfg->error_messages->messages[i].header, rec_cfg->error_messages->messages[i].message);
		if (i+1 != rec_cfg->error_messages->num_errors){
			ap_rputs(",",r);
		}
	}
	ap_rputs("]", r);
	ap_rputs("},\n", r);
	//Print query
	switch(rec_cfg->query->type){
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

	if(rec_cfg->query->results != NULL && !apr_is_empty_table(rec_cfg->query->results->song_results)){
		apr_table_do(print_songs_json, r, rec_cfg->query->results->song_results, NULL);
		//apr_table_do(print_songs_json, r, rec_cfg->query->results->song_results, NULL);
	}
	ap_rputs(	"]}",r);
	return 0;
}
