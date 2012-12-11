/*
 * music_query.h
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
 *   limitations under the License.
*/


#ifndef MUSIC_QUERY_H_
#define MUSIC_QUERY_H_
#include "apr_dbd.h"
#include "error_handler.h"

#define NUM_QUERY_TYPES  6
#define NUM_QUERY_PARAMETERS  12

enum query_types{
	SONGS = 0,
	ALBUMS,
	ARTISTS,
	SOURCES,
	TRANSCODE,
	PLAY
};

enum parameter_types {
	SONG_ID =0,
	SONG_NAME,
	ARTIST_ID,
	ARTIST_NAME,
	ALBUM_ID,
	ALBUM_NAME,
	ALBUM_YEAR,
	SOURCE_TYPE,
	SOURCE_ID,
	SORT_BY,
	ROWCOUNT,
	OFFSET
};

typedef struct results_table_t_ results_table_t;

typedef struct{
	char* query_paramter_string;
	char* parameter_value;
}query_parameters_t;

typedef struct{
	enum query_types type;
	//One for each query_paramter type
	//Binary flag for each parameter
	unsigned int query_parameters_set;
	query_parameters_t query_parameters[NUM_QUERY_PARAMETERS];
	results_table_t* results;
	error_messages_t* error_messages;
}music_query;

int run_music_query(request_rec* r, music_query* music);
int get_music_query(request_rec* r,music_query* music);
int output_json(request_rec* r);

#endif /* MUSIC_QUERY_H_ */
