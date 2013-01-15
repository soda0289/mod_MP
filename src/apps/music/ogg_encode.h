/*
 * ogg_encode.h
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
 *
 *  Code borrowed from oggenc part of the vorbis-tools package. ♡ OS
*/

#ifndef OGG_ENCODE_H_
#define OGG_ENCODE_H_
#include "apps/music/transcoder.h"

int play_song(db_config* dbd_config, request_rec* r, music_query_t* music);
int ogg_encode(apr_pool_t* pool, input_file_t* input_file,const char* output_file_path);
int file_path_query(db_config* dbd_config,query_parameters_t* query_parameters,query_t* db_query,const char** file_path, const char** file_type,error_messages_t* error_messages);

#endif /* OGG_ENCODE_H_ */
