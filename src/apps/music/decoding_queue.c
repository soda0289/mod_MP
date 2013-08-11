/*
 * decoding_queue.c
 *
 *  Created on: Feb 6, 2013
 *      Author: Reyad Attiyat
 *      Copyright 2013 Reyad Attiyat
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


#include "apr_general.h"
#include "apr_shm.h"


#include "httpd.h"
#include "decoding_queue.h"
#include "unixd.h"
#include "error_handler.h"
#include "mod_mediaplayer.h"
#include "apr_global_mutex.h"
#include "apr_proc_mutex.h"

#include <mpg123.h>

int create_decoding_queue(music_globals_t* music_globals){
	apr_pool_t* pool = music_globals->pool;
	apr_status_t rv;
	int status;
	decoding_queue_t* decoding_queue = music_globals->decoding_queue = apr_pcalloc(pool, sizeof(decoding_queue_t));


	decoding_queue->shm_file = apr_pstrcat(pool,music_globals->tmp_dir,"/mp_decoding_queue",NULL);

	
	status = setup_shared_memory(&(decoding_queue->queue_shm),sizeof(queue_t),decoding_queue->shm_file, pool);
	if(status != 0){
		return status;
	}

	decoding_queue->queue = (queue_t*)apr_shm_baseaddr_get(decoding_queue->queue_shm);

	decoding_queue->queue->head = 0;
	decoding_queue->queue->tail = -1;
	decoding_queue->queue->working = 0ull;
	decoding_queue->queue->waiting = 0ull;


	rv = apr_global_mutex_create(&(decoding_queue->mutex),"/tmp/mp_decoding_queue_lock",APR_LOCK_DEFAULT,pool);
	if(rv != APR_SUCCESS){
		return rv;
	}
	rv = ap_unixd_set_global_mutex_perms(decoding_queue->mutex);
	if(rv != APR_SUCCESS){
		return rv;
	}

	apr_pool_cleanup_register(pool, decoding_queue->mutex,(void*) apr_global_mutex_destroy	, apr_pool_cleanup_null);
	apr_pool_cleanup_register(pool, decoding_queue->queue_shm,(void*) apr_shm_destroy	, apr_pool_cleanup_null);
	return 0;
}


int reattach_decoding_queue(music_globals_t* music_globals){
	apr_status_t rv;
	decoding_queue_t* decoding_queue = music_globals->decoding_queue;

	//Initalize mpg123
	mpg123_init();


	if(decoding_queue->shm_file){
		rv = apr_shm_attach(&(decoding_queue->queue_shm), decoding_queue->shm_file, music_globals->pool);
		if(rv != APR_SUCCESS){
			return rv;
			//ap_log_error(__FILE__,__LINE__,0, APLOG_CRIT, rv, s, "Error reattaching shared memeory Decoding Queue");
		}
	}

	decoding_queue->error_messages = music_globals->error_messages;
	decoding_queue->queue = apr_shm_baseaddr_get(decoding_queue->queue_shm);


	//Decoding queue requires a global lock
	rv = apr_global_mutex_child_init(&(decoding_queue->mutex),"/tmp/mp_decoding_queue_lock",music_globals->pool);
	if(rv != APR_SUCCESS){
			//ap_log_error(__FILE__,__LINE__,0, APLOG_CRIT, rv, s, "Error reattaching to global mutex");
		return rv;
	}

	return 0;
}


decoding_job_t* get_decoding_job_queue(decoding_queue_t* decoding_queue, error_messages_t* error_messages){
	//Lock queue
	decoding_job_t* dec_job = NULL;
	int index;

	apr_status_t rv;
	char error_message[512];

	uint64_t index_flag;

	LOCK_CHECK_ERRORS(decoding_queue->mutex,decoding_queue->error_messages, "Error Locking in get decoding queue");

	if(decoding_queue->queue->waiting){
		//Set dec job to head
		if(decoding_queue->queue->head < 0 || decoding_queue->queue->head > 63 ){
			add_error_list(decoding_queue->error_messages,ERROR,"Errroroororor", "head is less than 0");
		}
		index = decoding_queue->queue->head;
		index_flag = (1ull << (63 - index));
		dec_job = &(decoding_queue->queue->decoding[index]);
		//remove job from waiting queue
		decoding_queue->queue->waiting &= ~(index_flag);
		decoding_queue->queue->head = dec_job->next;
		//Set working (we must do this in the get_decdoing_job function so we don't get both
		//waiting and working set at the same time in other threads or process)
		decoding_queue->queue->working |= index_flag;
	}else{
		//Waiting list empty bail out
		dec_job = NULL;
	}
	//unlock queue
	UNLOCK_CHECK_ERRORS(decoding_queue->mutex,decoding_queue->error_messages, "Error Unlocking in get decoding queue");

	return dec_job;
}

int add_decoding_job_queue(decoding_job_t* job,decoding_queue_t* decoding_queue){
	int empty_index;
	apr_status_t rv;
	char error_message[512];
	uint64_t free;

	//Lock queue
	LOCK_CHECK_ERRORS(decoding_queue->mutex,decoding_queue->error_messages, "Error Locking in add decoding job");


	free = ~(decoding_queue->queue->waiting | decoding_queue->queue->working);
	if(!free){
		add_error_list(decoding_queue->error_messages,ERROR,"Error adding to queue","Queue is FULL!");
		return -1;
	}


	if(free){
		empty_index = __builtin_clzll(free);
	}else{
		//Queue empty
		//reset head and start at 0
		empty_index = 0;
	}
	//if waiting queue is empty
	if(decoding_queue->queue->waiting == 0ull){
		//set head
		decoding_queue->queue->head = empty_index;
	}

	//Add into free index
	decoding_queue->queue->decoding[empty_index] = *job;
	decoding_queue->queue->decoding[empty_index].index = empty_index;
	//Points no where since its last in list
	decoding_queue->queue->decoding[empty_index].next = -1;

	//Mark in waiting
	decoding_queue->queue->waiting |= (1ull << (63 - empty_index));

	//if queue->tail is set(not negative)
	if(decoding_queue->queue->tail >= 0){
		//Update current tail
		decoding_queue->queue->decoding[decoding_queue->queue->tail].next = empty_index;
		//Set tail to new index
	}
	decoding_queue->queue->tail = empty_index;
	//unlock queue
	UNLOCK_CHECK_ERRORS(decoding_queue->mutex,decoding_queue->error_messages, "Error Unlocking in add decoding job");

	return 0;
}

//This is better when most bits in x are 0
//It uses 3 arithmetic operations and one comparison/branch per "1" bit in x.
int popcount_4(uint64_t x) {
    int count;
    for (count=0; x; count++)
        x &= x-1;
    return count;
}


int does_decoding_job_exsits(apr_pool_t* pool,decoding_queue_t* decoding_queue,decoding_job_t** dec_job){
	int i, found = 0;
	uint64_t has_job;

	apr_status_t rv;
	char error_message[512];

	if(*dec_job == NULL){
		return 0;
	}

	LOCK_CHECK_ERRORS(decoding_queue->mutex,decoding_queue->error_messages, "Error locking in does decoding job exsits");

	has_job = decoding_queue->queue->waiting | decoding_queue->queue->working;

	if(decoding_queue->queue->waiting & decoding_queue->queue->working){
		add_error_list(decoding_queue->error_messages,ERROR,"Working and waiting are set WTF MAN",apr_strerror(rv, error_message,512));
	}

	//i - Index is set to the offset, next decoding job in line, or the first one that is being worked on
	//which is equal too the array length minus the number of leading zeros.
	//i is incremented to each value of what is being worked on in the array and then through
	//each element in the queue.

	for(i = __builtin_clzll(has_job);
		has_job;
		i = __builtin_clzll(has_job &= ~(1ull << (63 - i)))){

		//Check if source id and output_type are equal
		//Should also check if encoding options are equal such as quality
		if(apr_strnatcmp(decoding_queue->queue->decoding[i].source_id,(*dec_job)->source_id) == 0 && apr_strnatcmp(decoding_queue->queue->decoding[i].output_file_type,(*dec_job)->output_file_type) == 0){
			const char* dec_job_status;

			*dec_job = &(decoding_queue->queue->decoding[i]);
			//Found decoding job
			dec_job_status = apr_psprintf(pool,"Decoding index: %d Working on",i);
			apr_cpystrn((*dec_job)->status,dec_job_status,512);

			//Job found return 1
			found++;
			//Should break but were debugging so try and find duplicates
		}
	}

	if(found > 1){
		add_error_list(decoding_queue->error_messages, ERROR, "Error found duplicate", "found duplicate in working/waiting queue");
	}

	UNLOCK_CHECK_ERRORS(decoding_queue->mutex,decoding_queue->error_messages, "Error Unlocking in does decoding job exists");

	return found;
}
