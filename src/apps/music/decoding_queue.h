/*
 * decoding_queue.h
 *
 *  Created on: Feb 6, 2013
 *      Author: reyad
 */

#ifndef DECODING_QUEUE_H_
#define DECODING_QUEUE_H_

#include "error_handler.h"
#include "apr_global_mutex.h"
#include "apr_proc_mutex.h"
#include "music_typedefs.h"

#define MAX_NUM_WORKERS 4

struct decoding_job_{
	//Index in array
	int index;
	//INPUT
	char song_id[256];
	char artist_id[256];
	char album_id[256];
	char source_id[256];
	char input_file_path[1024];
	char input_file_type[1024];
	//OUTPUT
	char output_file_type[256];
	char output_file_path[1024];
	//Decdoing job generates
	char new_source_id[256];
	//Status messages as it moves in queue
	char status[512];
	//Progress of encoding
	float progress;


	int next;
};


struct queue_{
	int head;
	int tail;

	uint64_t working;
	uint64_t waiting;
	//uint64_t free;
	decoding_job_t decoding[64];

	int num_working_threads;
};


struct decoding_queue_{
	apr_shm_t* queue_shm;

	//These pointers must be set in child proccess
	error_messages_t* error_messages;
	apr_global_mutex_t* mutex;
	queue_t* queue;
};



#define LOCK_CHECK_ERRORS(lock, error_messages, error_header) if((rv = apr_global_mutex_lock(lock) != APR_SUCCESS))add_error_list(error_messages,ERROR,error_header,apr_strerror(rv, error_message,512))
#define UNLOCK_CHECK_ERRORS(lock, error_messages, error_header) if((rv = apr_global_mutex_unlock(lock) != APR_SUCCESS))add_error_list(error_messages,ERROR,error_header,apr_strerror(rv, error_message,512))


int create_decoding_queue(apr_pool_t* pool, const char* queue_shm_file,decoding_queue_t** decoding_queue);
int reattach_decoding_queue(apr_pool_t* pool,decoding_queue_t* decoding_queue, const char* queue_shm_file,error_messages_t* error_messages);

decoding_job_t* get_decoding_job_queue(decoding_queue_t* decoding_queue, error_messages_t* error_messages);
int add_decoding_job_queue(decoding_job_t* job,decoding_queue_t* decoding_queue);
int popcount_4(uint64_t x);
int does_decoding_job_exsits(apr_pool_t* pool,decoding_queue_t* decoding_queue,decoding_job_t** dec_job);


#endif /* DECODING_QUEUE_H_ */
