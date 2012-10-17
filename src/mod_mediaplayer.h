/*
 * error_handler.c
 *
 *  Created on: Sep 26, 2012
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
 * See the License for the specific language governing permissions and
*  limitations under the License.
*/

#include <httpd.h>
#include "dbd.h"
#include "error_handler.h"

module AP_MODULE_DECLARE_DATA mediaplayer_module;

typedef struct {
	int enable;
	const char* external_directory;
	db_config* dbd_config;
	apr_shm_t* dir_sync_shm;
	apr_shm_t* errors_shm;
} mediaplayer_srv_cfg ;


typedef struct {
	int* num_files;
	apr_pool_t * pool;
	float sync_progress;
	mediaplayer_srv_cfg* srv_conf;
	error_messages_t* error_messages;
}dir_sync_t;



typedef struct{
	enum {
		SONGS = 0,
		ALBUMS = 1,
		ARTISTS = 2
	}types;
	enum {
		ASC_TITLES= 0,
		ASC_ALBUMS = 1,
		ASC_ARTISTS = 2,
		DSC_TITLES = 3,
		DSC_ALBUMS = 4,
		DSC_ARTISTS = 5
	}sort_by;
	char* range_lower;
	char* range_upper;
	results_table_t* results;
}music_query;

typedef struct{
	error_messages_t* error_messages;
	music_query* query;
}mediaplayer_rec_cfg;

char* json_escape_char(apr_pool_t* pool, const char* string);
