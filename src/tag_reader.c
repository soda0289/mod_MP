/*
 * tag_reader.c
 *
 *  Created on: Sep 26, 2012
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
 * See the License for the specific language governing permissions and
*  limitations under the License.
*/

#include <httpd.h>
#include <http_protocol.h>
#include <http_config.h>
#include "apr_file_io.h"
#include "apr_file_info.h"
#include "apr_errno.h"
#include "apr_general.h"
#include "apr_lib.h"
#include "apr_strings.h"
#include "apr_dbd.h"
#include <apr_strmatch.h>
#include <stdlib.h>
#include "FLAC/metadata.h"
#include "FLAC/stream_decoder.h"
#include "tag_reader.h"
#include "dbd.h"
#include "error_handler.h"

void find_vorbis_comment_entry(apr_pool_t*pool, FLAC__StreamMetadata *block, char* entry, char** feild){
	FLAC__StreamMetadata_VorbisComment       *comment;
	FLAC__StreamMetadata_VorbisComment_Entry *comment_entry;
	char* comment_value;
	comment = &block->data.vorbis_comment;
	int comment_length;
	int offset;

	//TITLE
	offset = 0;
	while ( (offset = FLAC__metadata_object_vorbiscomment_find_entry_from(block, offset, entry)) >= 0 ){
		comment_entry = &comment->comments[offset++];
		comment_value = memchr(comment_entry->entry, '=',comment_entry->length);

		if (comment_value){
			//Remove eqal sign form value
			comment_value++;
			if(*feild == NULL){
				comment_length = comment_entry->length - (comment_value - (char*)comment_entry->entry);
				*feild = apr_pstrmemdup(pool, comment_value, comment_length);
			}else{
				//make a comma seperated list
			}
			//free(comment_value);
		}
	}
}

int read_flac_level1(apr_pool_t* pool, music_file* song){
	FLAC__Metadata_SimpleIterator *iter;
	const char* file_path;

	if (song == NULL){
		return 1;
	}
	file_path = song->file_path;
	if (!file_path){
		return 1;
	}
	 iter = FLAC__metadata_simple_iterator_new();

	 if (iter == NULL || !FLAC__metadata_simple_iterator_init(iter, file_path, true, false)){
		 //ap_rprintf(r, "Error creating FLAC interator\n");
		 FLAC__metadata_simple_iterator_delete(iter);
		 return 1;
	 }
	 while (FLAC__metadata_simple_iterator_next(iter)){
		 // Get block of metadata
		 FLAC__StreamMetadata *block = FLAC__metadata_simple_iterator_get_block(iter);

		 //Check what type of block we have Picture, Comment
		 switch ( FLAC__metadata_simple_iterator_get_block_type(iter) ){

		 case FLAC__METADATA_TYPE_VORBIS_COMMENT:{
			 find_vorbis_comment_entry(pool, block, "TITLE", &song->title);
			 find_vorbis_comment_entry(pool, block, "ARTIST", &song->artist);
			 find_vorbis_comment_entry(pool, block, "ALBUM", &song->album);
			 find_vorbis_comment_entry(pool, block, "TRACKNUMBER", &song->track_no);
			 find_vorbis_comment_entry(pool, block, "DISCNUMBER", &song->disc_no);
	         break;
		 }
	     }
		 FLAC__metadata_object_delete(block) ;
	 }
	 FLAC__metadata_simple_iterator_delete(iter);
	return 0;
}
/*
static music_file* read_flac_level0(request_rec* r, char* file_path){
	music_file* song;
	char* strtok_state;

	FLAC__StreamMetadata *metadata;
	FLAC__bool gotit;


	const char* title_pattern = "TITLE";
	const char* album_pattern = "ALBUM";
	const char* artist_pattern = "ARTIST";
	const char* track_pattern = "TRACKNUMBER";
	const char* disc_pattern = "DISCNUMBER";

	song = apr_pcalloc(r->pool, sizeof(music_file));
	song->file_path = file_path;

	if(file_path != NULL){
		gotit = FLAC__metadata_get_tags(file_path , &metadata);
	}else{
		return NULL;
	}
	if (gotit == 1 && metadata != NULL){
		apr_pool_cleanup_register(r->pool, metadata, (void*)FLAC__metadata_object_delete, NULL);
		int i;
		//Read thru each comment and add to song tag struct
		for (i = 0; i < metadata->data.vorbis_comment.num_comments; i++){
			char *comment = apr_palloc(r->pool, metadata->data.vorbis_comment.comments[i].length + 1);
			char *comment_name = apr_palloc(r->pool, metadata->data.vorbis_comment.comments[i].length + 1);
			char *comment_value = apr_palloc(r->pool, metadata->data.vorbis_comment.comments[i].length + 1);

			comment = apr_pstrmemdup(r->pool, (const char*)metadata->data.vorbis_comment.comments[i].entry, metadata->data.vorbis_comment.comments[i].length);
			comment_name = apr_strtok(comment, "=", &strtok_state);
			comment_value = apr_strtok(NULL, "=", &strtok_state);

			if(strcmp(comment_name, title_pattern) == 0){
				song->title = comment_value;
			}else if(strcmp(comment_name, artist_pattern) == 0){
				song->artist = comment_value;
			}else if(strcmp(comment_name, album_pattern) == 0){
				song->album = comment_value;
			}else if(strcmp(comment_name, track_pattern) == 0){
				song->track_no = comment_value;
			}else if(strcmp(comment_name, disc_pattern) == 0){
				song->disc_no = comment_value;
			}
		}
		apr_pool_cleanup_run(r->pool, metadata, (void*)FLAC__metadata_object_delete);
	}else{
		return NULL;
	}
	return song;
}
*/

List* read_dir(apr_pool_t* pool, List* file_list, const char* dir_path, int* count, error_messages_t* error_messages){
	apr_status_t rv;
	char errorbuf [255];

	apr_dir_t *dir;
	apr_finfo_t filef;

	char* file_path;

	  /*rv = apr_dbd_close(dbd_driver, dbd_handle);
	  if (rv != APR_SUCCESS){
		  ap_rputs(apr_strerror(rv, errorbuf, 255), r) ;
	  }
	*/
	rv = apr_dir_open(&dir, dir_path, pool);
	if (rv != 0){
		 apr_strerror(rv, (char *)&errorbuf, 255);
		add_error_list(error_messages, apr_psprintf(pool, "Error opening directory (%s) (%d)", dir_path, rv), errorbuf);
		return NULL;
	}
	  //Read file is directory
	while (apr_dir_read(&filef, APR_FINFO_TYPE | APR_FINFO_NAME | APR_FINFO_MTIME, dir) == 0){
		file_path =  apr_pstrcat(pool, dir_path, "/", filef.name, NULL);
		if (file_path == NULL){
			return NULL;
			///BIG ERROR
		}
		//Is a Directory
		if (filef.filetype == APR_DIR){
			if(filef.name[0] != '.'){
					file_list = read_dir(pool, file_list, file_path, count, error_messages);
			}
		}
		//Is a File
		if (filef.filetype == APR_REG){
			(*count)++;
			List* file_list_new  = apr_pcalloc(pool, sizeof(List));
			file_list->name = file_path;
			file_list->mtime = filef.mtime;
			file_list->next = file_list_new;
			file_list = file_list_new;
		}
	}
	rv = apr_dir_close(dir);
	return file_list;
}

