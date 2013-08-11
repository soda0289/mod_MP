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


//Directory sync stats is stored in shared memory
typedef struct dir_shared_{
	int* num_files;
	float sync_progress;
	int files_scanned;
}dir_sh_stats_t;

typedef struct dir_{
	//Shared memory
	apr_shm_t* shm;
	const char* shm_file;

	dir_sh_stats_t* stats;

	char* path;
}dir_t;

typedef struct dir_sync_thread_{
	db_params_t* db_params;
	apr_pool_t * pool;
	error_messages_t* error_messages;

	dir_t* dir;
}dir_sync_thread_t;

void * APR_THREAD_FUNC sync_dir(apr_thread_t* thread, void* ptr);
int output_dirsync_status(music_query_t* music_query);
int init_dir_sync(music_globals_t* music_globals);
int reattach_dir_sync(music_globals_t* music_globals);


#endif /* DIR_SYNC_H_ */
