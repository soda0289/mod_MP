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
#include "apps/app_config.h"

typedef struct query_t_ query_t;
typedef struct column_table_t_ column_table_t;
typedef struct table_t_ table_t;
typedef struct db_config_ db_config;
typedef char* app_query;
typedef struct query_words_t_ query_words_t;


enum query_types{
	SONGS = 0,
	ALBUMS,
	ARTISTS,
	SOURCES,
	TRANSCODE,
	PLAY
};

typedef struct results_table_t_ results_table_t;
typedef struct query_parameters_t_ query_parameters_t;

typedef struct music_query_{
	enum query_types type;
	query_t* db_query;
	query_parameters_t* query_parameters;

	results_table_t* results;
	error_messages_t* error_messages;
}music_query_t;

int get_music_query(apr_pool_t* pool,error_messages_t*,app_query* query,query_words_t* query_words, apr_array_header_t* db_queries);
int run_music_query(request_rec*, app_query,db_config*, apr_dbd_prepared_t****);

#endif /* MUSIC_QUERY_H_ */