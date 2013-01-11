/*
 * dir_sync.h
 *
 *  Created on: Nov 12, 2012
 *      Author: reyad
 */

#ifndef DIR_SYNC_H_
#define DIR_SYNC_H_

typedef struct {
	int* num_files;
	apr_pool_t * pool;
	error_messages_t* error_messages;
	const char* dir_path;
	//This is the only variable that can be accessed by all processes
	float sync_progress;
	app_list_t* app_list;
}dir_sync_t;

void * APR_THREAD_FUNC sync_dir(apr_thread_t* thread, void* ptr);

#endif /* DIR_SYNC_H_ */
