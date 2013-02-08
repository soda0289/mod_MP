/*
 * transcoder.h
 *
 *  Created on: Jan 11, 2013
 *      Author: reyad
 */

#ifndef TRANSCODER_H_
#define TRANSCODER_H_

#include "apr_pools.h"
#include "httpd.h"
#include "database/dbd.h"

#include "mod_mediaplayer.h"

#include "decoding_queue.h"
#include "music_typedefs.h"


typedef struct encoding_options_t_{
	float quality;
	long channels;
	long rate;
	long total_samples_per_chanel;
	float* progress;
}encoding_options_t;

/* The input file struct holds the three decoding functions need to process a input file
 * to the decoder
 *  file_path is the file path on the disk of the file
 *  file_struct is the file details that is passed to each decoding function
 *  open_input_file is a function that opens the file and sets encoding options such as channels and rate
 *  proccess_input_file decodes x number of samples and places them into buffer
 *  close_input_file cleans up the decoder
 *
 */
typedef struct input_file_t_{
	const char* file_path;
	void* file_struct;
	int (*open_input_file)(apr_pool_t* pool,void** file_struct, const char* file_path, encoding_options_t* enc_opt);
	int (*process_input_file)(void *file_struct, float **buffer, int samples);
	int (*close_input_file)(void* file_struct);
}input_file_t;


typedef struct transcode_thread_t_{
	apr_pool_t* pool;
	db_config* dbd_config;
	error_messages_t* error_messages;
	decoding_queue_t* decoding_queue;
	int num_working_threads;
}transcode_thread_t;

int transcode_audio(request_rec* r, db_config* dbd_config,music_query_t* music_query);

#endif /* TRANSCODER_H_ */
