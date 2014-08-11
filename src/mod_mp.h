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

#include "indexers/indexer.h"
#include "indexers/indexer_typedefs.h"

#include "error_messages.h"

#include "database/db_typedefs.h"
#include "database/db_manager.h"

module AP_MODULE_DECLARE_DATA mp_module;

typedef struct {
	int enable;

	//Process ID
	pid_t pid;

	server_rec* server_rec;

	//Error manager shared memory and file
	const char* errors_shm_file;

	error_messages_t* error_messages;

	//Output directory
	const char* external_directory;

	//Directory of XML database configuration files
	const char* db_xml_config_dir;

	db_manager_t* db_manager;

	//Directory of the Indexers XML configuration files
	const char* indexer_xml_config_dir;
	
	index_manager_t* index_manager;
} mp_srv_cfg ;

typedef struct{
	error_messages_t* error_messages;
	//app_query appquery;
}mp_rec_cfg;

int setup_shared_memory(apr_shm_t** shm, apr_size_t size, const char* file_path, apr_pool_t* pool);

#endif
