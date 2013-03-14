/*
 * dir_sync.h
 *
 *  Created on: Nov 12, 2012
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
 *
 *
 */

#ifndef DIR_SYNC_H_
#define DIR_SYNC_H_

#include "database/dbd.h"
#include "database/db_typedef.h"
#include "apps/app_typedefs.h"
#include "error_handler.h"
#include "apps/music/music_typedefs.h"



typedef struct dir_sync_{
	int* num_files;

	db_config* dbd_config;
	apr_pool_t * pool;
	error_messages_t* error_messages;
	const char* dir_path;
	//This is the only variable that can be accessed by all processes
	float sync_progress;
	int files_scanned;

	app_list_t* app_list;
}dir_sync_t;

void * APR_THREAD_FUNC sync_dir(apr_thread_t* thread, void* ptr);
int output_dirsync_status(music_query_t* music_query,apr_pool_t* pool, apr_bucket_brigade* output_bb,apr_table_t* output_headers, const char* output_content_type,error_messages_t* error_messages);

#endif /* DIR_SYNC_H_ */
