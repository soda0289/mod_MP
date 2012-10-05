/*
 * tag_reader.h
 *
 *  Created on: Sep 18, 2012
 *      Author: Reyad Attiyat
 */
#include "error_handler.h"

#ifndef TAG_READER_H_
#define TAG_READER_H_

typedef struct {
	char* file_path;
	char* title;
	char* artist;
	char* album;
	char* track_no;
	char* disc_no;
	int   length;
	//char** feature;

} music_file;

struct List{
	char* name;
	apr_time_t mtime;
	struct List* next;
};

typedef struct List List;

typedef struct{
	char* name;
	struct List* next;
}List_Head;

List* read_dir(apr_pool_t* pool, List* file_list, const char* dir_path, int* count, error_messages_t* error_messages);
int read_flac_level1(apr_pool_t* pool, music_file* song);

#endif /* TAG_READER_H_ */
