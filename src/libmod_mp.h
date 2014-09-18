/*
 * libmod_mp.h
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
#ifndef LIBMOD_MP_H_
#define LIBMOD_MP_H_

#include "indexers/indexer.h"
#include "indexers/indexer_typedefs.h"

#include "error_messages.h"

#include "database/db_typedefs.h"
#include "database/db_manager.h"

typedef struct {
	int enable;

	//Process ID
	pid_t pid;

	void* web_server_data;

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


void mod_mp_set_data(mp_srv_cfg *srv_conf, void* data);
int mod_mp_init (mp_srv_cfg *srv_conf, apr_pool_t* pool);

#endif
