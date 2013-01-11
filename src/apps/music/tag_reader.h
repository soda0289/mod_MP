/*
 * tag_reader.h
 *
 *  Created on: Sep 18, 2012
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


#ifndef TAG_READER_H_
#define TAG_READER_H_

#include "error_handler.h"

enum FILE_TYPE{
	MP3 = 0,
	FLAC = 1,
	OGG = 2
};

typedef struct song_tags{
	const char* tag_name;
	char** tag_dest;
}song_tags;

typedef struct{
	char* path;
	enum FILE_TYPE type;
	char* type_string;
	int quality;
	apr_time_t mtime;
}file_t;

typedef struct {
	char* title;
	char* artist;
	char* album;
	char* track_no;
	char* disc_no;
	int   length;
	//char** feature;

	file_t file;
} music_file;

struct List{
	file_t file;
	struct List* next;
};

typedef struct List List;

typedef struct{
	char* name;
	struct List* next;
}List_Head;

List* read_dir(apr_pool_t* pool, List* file_list, const char* dir_path, int* count, error_messages_t* error_messages);
int read_flac_level1(apr_pool_t* pool, music_file* song);
int read_ogg(apr_pool_t* pool, music_file* song);

#endif /* TAG_READER_H_ */
