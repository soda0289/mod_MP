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

enum query_types{
	SONGS = 0,
	ALBUMS,
	ARTISTS,
	SOURCES,
	TRANSCODE,
	PLAY
};

struct music_query_{
	music_globals_t* music_globals;
	enum query_types type;
	query_t* db_query;
	query_parameters_t* query_parameters;
	apr_bucket_brigade* output_bb;
	apr_table_t* output_headers;
	const char* output_content_type;
	results_table_t* results;
	error_messages_t* error_messages;
};

struct music_globals_{

	apr_shm_t* dir_sync_shm;
	apr_shm_t* decoding_queue_shm;

	const char* dir_sync_shm_file;
	const char* decoding_queue_shm_file;


	int num_songs;
	int num_artits;
	int num_albums;
	int num_sources;

	int num_decoding_threads;
	decoding_queue_t* decoding_queue;

	float* dir_sync_progress;
	float* musicbrainz_progress;
};

int run_music_query(apr_pool_t* pool,apr_pool_t* global_pool, apr_bucket_brigade* output_bb, apr_table_t* output_headers, const char* output_content_type,error_messages_t* error_messages, db_config* dbd_config, query_words_t* query_words, apr_array_header_t* db_queries, const void* global_context);
int output_json(apr_pool_t* pool, apr_bucket_brigade* bb, music_query_t* query);
int init_music_query(apr_pool_t* global_pool, error_messages_t* error_messages, const char* external_directory, const void** global_context);
int reattach_music_query(apr_pool_t* child_pool, error_messages_t* error_messages,const void* global_context);

#endif /* MUSIC_QUERY_H_ */
