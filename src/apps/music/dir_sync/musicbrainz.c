/*
 * musicbrainz.c
 *
 *  Created on: Feb 7, 2013
 *      Author: Reyad Attiyat
 *      Copyright 2013 Reyad Attiyat
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

#include "musicbrainz5/mb5_c.h"
#include "apps/music/tag_reader.h"
#include "musicbrainz.h"
#include "apr_strings.h"

static inline void set_parameter(mb_parameters_t* mb_parameters, char* parameter_name, char* parameter_value){
	if(mb_parameters->num_parameters < MAX_NUM_PARAMETERS){
		mb_parameters->parameters_name[mb_parameters->num_parameters] = parameter_name;
		mb_parameters->parameters_value[mb_parameters->num_parameters++] = parameter_value;
	}

}

int get_musicbrainz_release_id(apr_pool_t* pool, music_file* song, error_messages_t* error_messages){
	Mb5Query mb_query;
	Mb5Metadata mb_metadata;
	Mb5ReleaseList mb_release_list;
	Mb5Release mb_release;

	tQueryResult mb_result;
	int mb_httpcode = 0;
	int i;
	char error_message[512];

	mb_parameters_t* mb_parameters;


	mb_query = mb5_query_new("mod_mediaplayer",NULL,0);

	if(mb_query == NULL){
		return -1;
	}

	mb_parameters = apr_pcalloc(pool, sizeof(mb_parameters_t));

	mb_parameters->num_parameters = 0;

	set_parameter(mb_parameters, "query", apr_pstrcat(pool,"artist:\"",song->artist,"\" AND release:\"",song->title,"\"",NULL));
	//set_parameter(mb_parameters, "query", apr_pstrcat(pool,"release:",song->title,NULL));



	mb_metadata = mb5_query_query(mb_query,"release",NULL,NULL,mb_parameters->num_parameters, mb_parameters->parameters_name, mb_parameters->parameters_value);

	mb_result = mb5_query_get_lastresult(mb_query);
	if(mb_result != eQuery_Success){
		mb_httpcode = mb5_query_get_lasthttpcode(mb_query);
		if(mb_httpcode != 200){
			return -2;
		}
		mb5_query_get_lasterrormessage(mb_query, error_message, 512);

		add_error_list(error_messages, WARN, "Musicbrainz http status", error_message);
	}

	if(mb_metadata == NULL){
		return 2;
	}

	mb_release_list = mb5_metadata_get_releaselist(mb_metadata);

	for(i = 0; i < mb5_release_list_size(mb_release_list);i++){
		mb_release = mb5_release_list_item(mb_release_list,i);
		//mb_release = mb5_metadata_get_release(mb_metadata);

		if(mb_release == NULL){
			return 3;
		}
		mb5_release_get_id(mb_release,song->mb_id.mb_release_id, 64);
		break;
	}





	mb5_metadata_delete(mb_metadata);
	mb5_query_delete(mb_query);

	return 0;
}
