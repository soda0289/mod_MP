/*
 * musicbrainz.h
 *
 *  Created on: Feb 7, 2013
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

#ifndef MUSICBRAINZ_H_
#define MUSICBRAINZ_H_

#define MAX_NUM_PARAMETERS 8

typedef struct mb_parameters_t_{
	int num_parameters;
	char* parameters_name[MAX_NUM_PARAMETERS];
	char* parameters_value[MAX_NUM_PARAMETERS];
}mb_parameters_t;

int get_musicbrainz_release_id(apr_pool_t* pool, music_file* song, error_messages_t* error_messages);

#endif /* MUSICBRAINZ_H_ */
