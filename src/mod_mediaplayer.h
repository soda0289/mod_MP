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
#include "apps/music/dir_sync/dir_sync.h"


#include "apps/music/tag_reader.h"


#include "error_handler.h"
#include "apps/music/music_query.h"


#include "apps/app_config.h"
#include "apps/music/transcoder.h"
#include "apps/music/decoding_queue.h"
#include "apps/app_typedefs.h"
#include "database/db_typedef.h"

module AP_MODULE_DECLARE_DATA mediaplayer_module;


typedef struct {
	int enable;
	const char* external_directory;
	const char* db_schema_xml;
	db_config* dbd_config;


	pid_t pid;

	apr_shm_t* errors_shm;
	const char* errors_shm_file;
	error_messages_t* error_messages;

	app_list_t* apps;
} mediaplayer_srv_cfg ;

typedef struct{
	error_messages_t* error_messages;
	app_query appquery;
}mediaplayer_rec_cfg;

char* json_escape_char(apr_pool_t* pool, const char* string);
int output_status_json(apr_pool_t* pool, apr_bucket_brigade* output_bb,apr_table_t* output_headers, const char* output_content_type,error_messages_t* error_messages);
int setup_shared_memory(apr_shm_t** shm,apr_size_t size,const char* file_path, apr_pool_t* pool);
int run_get_method(apr_pool_t* req_pool,apr_pool_t* global_pool, apr_bucket_brigade** output_bb, apr_table_t* output_headers, const char* output_content_type,app_list_t* apps,db_config* dbd_config, error_messages_t* error_messages, const char* uri);

#endif
