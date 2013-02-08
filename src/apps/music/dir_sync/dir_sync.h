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

typedef struct {
	int* num_files;

	db_config* dbd_config;
	apr_pool_t * pool;
	error_messages_t* error_messages;
	const char* dir_path;
	//This is the only variable that can be accessed by all processes
	float sync_progress;
	app_list_t* app_list;
}dir_sync_t;

void * APR_THREAD_FUNC sync_dir(apr_thread_t* thread, void* ptr);

#endif /* DIR_SYNC_H_ */
