/*
 * mod_mediaplayer.h
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
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
*/
#ifndef MOD_MEDIAPLAYER_H_
#define MOD_MEDIAPLAYER_H_

#include <httpd.h>
#include <http_protocol.h>
#include <http_config.h>
#include "apr_file_io.h"
#include "apr_file_info.h"
#include "apr_errno.h"
#include "apr_general.h"
#include "apr_lib.h"
#include "apr_buckets.h"
#include "apr_strings.h"
#include "apr_dbd.h"
#include <stdlib.h>
#include "tag_reader.h"

#include "dbd.h"
#include "error_handler.h"
#include "music_query.h"
typedef struct db_config_ db_config;

module AP_MODULE_DECLARE_DATA mediaplayer_module;


typedef struct{
	char* song_id;
	float progress;
}decoding_t;

typedef struct{
	decoding_t decoding[10];
}decoding_quene_t;


typedef struct {
	int enable;
	const char* external_directory;
	db_config* dbd_config;
	apr_shm_t* dir_sync_shm;
	const char* dir_sync_shm_file;
	apr_shm_t* errors_shm;
	const char* errors_shm_file;
	error_messages_t* error_messages;

} mediaplayer_srv_cfg ;

typedef struct{
	error_messages_t* error_messages;
	music_query* query;
}mediaplayer_rec_cfg;

char* json_escape_char(apr_pool_t* pool, const char* string);

#endif
