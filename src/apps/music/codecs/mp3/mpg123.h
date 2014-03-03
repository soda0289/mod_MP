/*
 * mpg123.h
 *
 *  Created on: July 30th, 2013
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
*/


#ifndef MPG123_H_


#include <mpg123.h>

#define MPG123_H_
typedef struct mpg3_file_ {
	int channels;
	long rate;
	int encoding;
	long samples;
	long total_samples; //Per channel

	mpg123_handle* mh;
}mp3_file_t;


int read_id3(apr_pool_t* pool, music_file* song);
long process_mp3_file(void *in, float **buffer, int samples);
int read_mp3_file (apr_pool_t* pool, void** mp3_file_ptr, const char* file_path, encoding_options_t* enc_opt);
int close_mp3_file(void* in);


#endif /* MPG123_H_ */
