/*
 * music_query.h
 *
 *  Created on: Oct 30, 2012
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
 *   limitations under the License.
*/


#ifndef MUSIC_QUERY_H_
#define MUSIC_QUERY_H_
#include "apr_dbd.h"

typedef struct results_table_t_ results_table_t;

typedef struct{
	enum {
		SONGS = 0,
		ALBUMS,
		ARTISTS,
		PLAY
	}types;
	enum {
		ASC_TITLES= 0,
		ASC_ALBUMS,
		ASC_ARTISTS,
		DSC_TITLES,
		DSC_ALBUMS,
		DSC_ARTISTS
	}sort_by;
	struct{
		int id_type; //Use types enum
		char* id;
	}by_id;
	char* range_lower;
	char* range_upper;
	results_table_t* results;
	apr_dbd_prepared_t* statement;
}music_query;

int run_music_query(request_rec* r, music_query* music);
int get_music_query(request_rec* r,music_query* music);
int output_json(request_rec* r);

#endif /* MUSIC_QUERY_H_ */
