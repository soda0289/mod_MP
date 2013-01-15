/*
 * flac.h
 *
 *  Created on: Nov 07, 2012
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

#ifndef PLAY_FLAC_H_
#define PLAY_FLAC_H_

#include "FLAC/metadata.h"
#include "FLAC/stream_decoder.h"
#include "apr.h"
#include "apr_general.h"
#include "apps/music/transcoder.h"

typedef struct {
	FLAC__StreamDecoder* stream_decoder;

	//Header Info
    short channels;
    int rate;
    int samples;
    long total_samples; // per channel

    int eos; //End of stream

    //Sample Buffer 2 dimensional Channels and Sample
    float **buf;
    int buffer_sample_len;
    int buffer_sample_seek;
    int buffer_sample_count;

}flac_file;

int read_flac_file (apr_pool_t* pool, flac_file** flac, const char* file_path, encoding_options_t* enc_opt);
long process_flac_file(void *in, float **buffer, int samples);
void close_flac(flac_file* flac);

#endif /* PLAY_SONG_H_ */
