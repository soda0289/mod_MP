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



#define NUM_QUERY_TYPES  6
#define NUM_QUERY_PARAMETERS  3


#include "apr_dbd.h"
#include "error_handler.h"
#include "database/dbd.h"
#include "database/db_typedef.h"
#include "apps/app_config.h"
#include "music_typedefs.h"
#include "decoding_queue.h"
#include "dir_sync/dir_sync.h"
#include "mod_mediaplayer.h"
#include "output.h"

typedef struct dir_sync_ dir_sync_t;

enum query_types{
	SONGS = 0,
	ALBUMS,
	ARTISTS,
	SOURCES,
	TRANSCODE,
	PLAY,
	STATUS
};

struct music_query_{
	music_globals_t* globals;

	enum query_types type;

	db_query_t* db_query;
	query_parameters_t* query_parameters;

	input_t* input;
	output_t* output;

	results_table_t* results;

	apr_pool_t* pool;
	error_messages_t* error_messages;
};

struct music_globals_{
	//This is changed when entering a new proccess
	apr_pool_t* pool;
	error_messages_t* error_messages;
	const char* tmp_dir;

	//Used to queue the songs for decoding
	decoding_queue_t* decoding_queue;
	
	//Directories to search for music files in
	apr_array_header_t* music_dirs;

	//DB Stats
	int num_songs;
	int num_artits;
	int num_albums;
	int num_sources;

	//DB Queries
	apr_array_header_t* db_queries;
};

int run_music_query(input_t* input, output_t* output, const void* global_context);
int init_music_query(apr_pool_t* global_pool, apr_array_header_t* db_queries, apr_xml_elem* xml_params, const void** global_context, error_messages_t* error_messages);
int reattach_music_query(apr_pool_t* child_pool, error_messages_t* error_messages,const void* global_context);

#endif /* MUSIC_QUERY_H_ */
