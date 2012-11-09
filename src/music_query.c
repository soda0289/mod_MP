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


int get_music_query(request_rec* r,music_query* music){
	char* query_nouns[6] = {0};
	int i = 0;
	char* uri_cpy = apr_pstrdup(r->pool, r->uri);
	char* uri_slash;

	//Remove leading slash and add trailing slash if one doesn't exsits.
	uri_cpy++;//Remove leading slash
	if (uri_cpy[strlen(uri_cpy) - 1] != '/'){
		uri_cpy = apr_pstrcat(r->pool, uri_cpy, "/", NULL);
	}

	//Fill array query_nouns with uri parts
	while ((uri_slash= strchr(uri_cpy, '/')) != NULL && i <= (sizeof(query_nouns) / sizeof(char*))){
		 uri_slash[0] = '\0';
		 query_nouns[i] = uri_cpy;
		 uri_cpy = ++uri_slash;
		 i++;
	}
	//Check if we get the minimum amount of data for a query
	if(query_nouns[0] == NULL || query_nouns[1] == NULL || query_nouns[2] == NULL){
		return -1;
	}

	//Find what table to get
	if (apr_strnatcasecmp(query_nouns[1], "songs") == 0){
		music->types = SONGS;
	}else if (apr_strnatcasecmp(query_nouns[1], "albums") == 0){
		music->types = ALBUMS;
	}else if (apr_strnatcasecmp(query_nouns[1], "artists") == 0){
		music->types = ARTISTS;
	}else if(apr_strnatcasecmp(query_nouns[1], "play") == 0){
		music->types = PLAY;
	}else{
		return -2;
		//Invlaid type
	}

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

		//Find what range to display Upper: where to start Lower: how many rows to display
		if((music->range_upper = strchr(query_nouns[3], '-')) != NULL) {
			music->range_upper[0] = '\0';
			music->range_upper++;
			music->range_lower = &(query_nouns[3][0]);
		}else{
			return -4;
			//Invalid range
		}
	}
	//Find which id to restrict on
	if (query_nouns[4] != NULL && query_nouns[5] != NULL){
	if (apr_strnatcasecmp(query_nouns[4], "artist_id") == 0){
		music->by_id.id_type = ARTISTS;
		music->by_id.id = query_nouns[5];
	}else
	if (apr_strnatcasecmp(query_nouns[4], "album_id") == 0){
		music->by_id.id_type = ALBUMS;
		music->by_id.id  = query_nouns[5];
	}else{
		return -5;
		//Invalid id
	}
	}


	return 0;
}

int run_music_query(request_rec* r, music_query* music){
	int error_num = 0;
	//We should check if database is connected somewhere

	mediaplayer_srv_cfg* srv_conf = ap_get_module_config(r->server->module_config, &mediaplayer_module);

	//set correct prepared statement
	switch(music->types){
	    case PLAY:
	    	play_song(srv_conf->dbd_config, r, music);
	    	break;
		case SONGS:
			if (music->by_id.id_type == SONGS){
				music->statement = srv_conf->dbd_config->statements.select_songs_range[music->sort_by];
			}else if (music->by_id.id_type == ALBUMS){
				music->statement = srv_conf->dbd_config->statements.select_songs_by_album_id_range[music->sort_by];
			}else if (music->by_id.id_type == ARTISTS){
				music->statement = srv_conf->dbd_config->statements.select_songs_by_artist_id_range[music->sort_by];
			}
			break;
		case ARTISTS:
			if (music->sort_by == ASC_ARTISTS){
				music->statement = srv_conf->dbd_config->statements.select_artists_range[0];
			}else if  (music->sort_by == DSC_ARTISTS){
				music->statement = srv_conf->dbd_config->statements.select_artists_range[1];
			}
			break;
		case ALBUMS:
			if (music->by_id.id_type == ARTISTS){
				if (music->sort_by == ASC_ALBUMS){
					music->statement = srv_conf->dbd_config->statements.select_albums_by_artist_id_range[0];
				}else if  (music->sort_by == DSC_ALBUMS){
					music->statement = srv_conf->dbd_config->statements.select_albums_by_artist_id_range[1];
				}
			}else{
				if (music->sort_by == ASC_ALBUMS){
					music->statement  = srv_conf->dbd_config->statements.select_albums_range[0];
				}else if  (music->sort_by == DSC_ALBUMS){
					music->statement  = srv_conf->dbd_config->statements.select_albums_range[1];
				}
			}
			break;
	}
	if (music->statement != NULL){
		error_num = select_db_range(srv_conf->dbd_config, music);
	}else{
		//No statement
		error_num = -1;
	}
	return error_num;
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
static int print_error_json(void *rec, const char *key, const char *value){
	request_rec* r = (request_rec*) rec;
	mediaplayer_rec_cfg* rec_cfg = ap_get_module_config(r->request_config, &mediaplayer_module);

	ap_rprintf(r, "{\n%s\n }", value);
	if (atoi(key) == (rec_cfg->error_messages->num_errors - 1)){
		ap_rprintf(r, "");
	}else{
		ap_rprintf(r, ",\n");
	}
	return 10;
}

static int print_songs_json(void *rec, const char *key, const char *value){
	request_rec* r = (request_rec*) rec;
	mediaplayer_rec_cfg* rec_cfg = ap_get_module_config(r->request_config, &mediaplayer_module);

	ap_rprintf(r, "{\n%s\n }",  value);
	if (atoi(key) == (rec_cfg->query->results->row_count - 1)){
		ap_rprintf(r, "");
	}else{
		ap_rprintf(r, ",\n");
	}
	return 10;
}

int output_json(request_rec* r){
	mediaplayer_srv_cfg* srv_conf = ap_get_module_config(r->server->module_config, &mediaplayer_module) ;
	dir_sync_t* dir_sync = apr_shm_baseaddr_get(srv_conf->dir_sync_shm);

	mediaplayer_rec_cfg* rec_cfg = ap_get_module_config(r->request_config, &mediaplayer_module);
	//Apply header
	ap_set_content_type(r, "application/json") ;
	//Print Status
	ap_rputs("{\n\t\"status\" : {", r);
		ap_rprintf(r, "\t\"Progress\" :  \"%.2f\",\n", dir_sync->sync_progress);

		ap_rputs("\"Errors\" : [", r);
		if(!apr_is_empty_table(rec_cfg->error_messages->error_table)){
			apr_table_do(print_error_json, r, rec_cfg->error_messages->error_table, NULL);
		}
	ap_rputs("]", r);
	ap_rputs("},\n", r);
	//Print query
	switch(rec_cfg->query->types){
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

	if(rec_cfg->query->results != NULL && !apr_is_empty_table(rec_cfg->query->results->results)){
		apr_table_do(print_songs_json, r, rec_cfg->query->results->results, NULL);
	}
	ap_rputs(	"]}",r);
	return 0;
}
