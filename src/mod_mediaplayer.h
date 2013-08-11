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

	//Output directory
	const char* external_directory;

	//Database array holds database configurations
	//stored in XML
	apr_array_header_t* db_array;
	//Directroy of XML database configuration files
	const char* db_xml_dir;

	//Proccess iD
	pid_t pid;

	//Error manager shared memory and file
	apr_shm_t* errors_shm;
	const char* errors_shm_file;
	error_messages_t* error_messages;

	//Variables for app manager
	app_list_t* apps;
	const char* apps_xml_dir;
} mediaplayer_srv_cfg ;

typedef struct{
	error_messages_t* error_messages;
	app_query appquery;
}mediaplayer_rec_cfg;

int output_status_json(output_t* output);
char* json_escape_char(apr_pool_t* pool, const char* string);
int setup_shared_memory(apr_shm_t** shm,apr_size_t size,const char* file_path, apr_pool_t* pool);

#endif
