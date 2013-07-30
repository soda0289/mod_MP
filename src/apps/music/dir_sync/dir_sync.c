/*
 * dir_sync.c
 *
 *  Created on: Nov 12, 2012
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
 *
 */

#include <httpd.h>
#include <http_protocol.h>
#include <http_config.h>
#include "apr_file_io.h"
#include "apr_file_info.h"
#include "apr_errno.h"
#include "apr_general.h"
#include "apr_lib.h"
#include "apr_buckets.h"
#include "apps/music/tag_reader.h"
#include <stdlib.h>
#include "mod_mediaplayer.h"
#include "error_handler.h"
#include "dir_sync.h"
#include "apps/music/mpg123.h"

int output_dirsync_status(music_query_t* music_query,apr_pool_t* pool, apr_bucket_brigade* output_bb,apr_table_t* output_headers, const char* output_content_type,error_messages_t* error_messages){
	apr_table_add(output_headers,"Access-Control-Allow-Origin", "*");
	apr_cpystrn((char*)output_content_type, "application/json", 255);

	apr_brigade_puts(output_bb, NULL,NULL, "{\n");

	//Print Status

	if(music_query->music_globals->dir_sync){
		apr_brigade_puts(output_bb, NULL,NULL,"\t\"dirsync_status\" : {\n");
		apr_brigade_printf(output_bb, NULL,NULL, "\t\t\"Progress\" :  \"%.2f\",\n",music_query->music_globals->dir_sync->sync_progress);
		apr_brigade_printf(output_bb, NULL,NULL, "\t\t\"Files Scanned\" :  \"%d\"\n", music_query->music_globals->dir_sync->files_scanned);
		apr_brigade_puts(output_bb, NULL,NULL,"\t},\n");
	}
	apr_brigade_puts(output_bb, NULL,NULL,"\t\"db_status\" : ");
	output_db_result_json(music_query->results,music_query->db_query,pool,output_bb);
	apr_brigade_puts(output_bb, NULL,NULL,"\n,");

	print_error_messages(pool,output_bb, error_messages);

	apr_brigade_puts(output_bb, NULL,NULL,"\n}\n");
	return 0;
}

int count_table_rows(){


	return 0;
}

void * APR_THREAD_FUNC sync_dir(apr_thread_t* thread, void* ptr){
	int files_synced = 0;
	int status;
	apr_status_t rv;
	const char* dbd_error;
	List* file_list;

	music_file* song;

	dir_sync_t* dir_sync = (dir_sync_t*) ptr;
	apr_pool_t* pool;

	char error_message[256];

	//Create sub pool used for sync
	rv = apr_pool_create_ex(&pool, dir_sync->pool, NULL, NULL);
	if(rv != APR_SUCCESS){
		add_error_list(dir_sync->error_messages, ERROR ,"Memory Pool Error", apr_strerror(rv, error_message, sizeof(error_message)));
		return 0;
	}

	//Init MP3 support
	mpg123_init();

	file_list = apr_pcalloc(pool, sizeof(List));
	dir_sync->num_files = apr_pcalloc(pool, sizeof(int));

	if(dir_sync->dbd_config->connected != 1){
		add_error_list(dir_sync->error_messages, ERROR, "Database not connected","ERROR ERROR");
		return 0;
	}

	count_table_rows();


	read_dir(pool, file_list, dir_sync->dir_path, dir_sync->num_files,  dir_sync->error_messages);

	if ((file_list == NULL || dir_sync->num_files == NULL )&& (*dir_sync->num_files ) > 0 ){
		add_error_list(dir_sync->error_messages, ERROR, "Killing Sync Thread", "");
		return 0;
	}

	status = apr_dbd_transaction_start(dir_sync->dbd_config->dbd_driver, pool, dir_sync->dbd_config->dbd_handle,&(dir_sync->dbd_config->transaction));
	if(status != 0){
		dbd_error = apr_dbd_error(dir_sync->dbd_config->dbd_driver, dir_sync->dbd_config->dbd_handle, status);
		add_error_list(dir_sync->error_messages,ERROR, "Database error start transaction", dbd_error);
		return 0;
	}
	while(file_list->file.path){
		  song = apr_pcalloc(pool, sizeof(music_file));
		  song->file = &(file_list->file);
		  switch(song->file->type){
		  	  case FLAC:{
		  		status = read_flac_level1(pool, song);
		  		if (status == 0){
		  			song->file->type_string = "flac";
		  		}
		  		break;
		  	  }
		  	  case OGG:{
		  		status = read_ogg(pool, song);
		  		if (status == 0){
		  			song->file->type_string = "ogg";
		  		}
		  		break;
		  	  }
		  	  case MP3:{
		  		  status = read_id3(pool,song);
			  	if (status == 0){
			  		song->file->type_string = "mp3";
			  	}
			  	break;
		  	  }
		  	 default:{
		  		  status = -1;
		  		  break;
		  	  }
		  }



		  if (status == 0 && song){
			  //We have song get musicbrainz ids
			  //status = get_musicbrainz_release_id(pool, song, dir_sync->error_messages);
			  //Update or Insert song
			  if (dir_sync->dbd_config->connected == 1){
				  status = sync_song(pool, dir_sync->dbd_config, song);
				  if (status != 0){
					add_error_list(dir_sync->error_messages, ERROR, apr_psprintf(pool,"Failed to sync song:"),  apr_psprintf(pool, "(%d) Song title: %s Song artist: %s song album:%s song file path: %s",status, song->title, song->artist, song->album, song->file->path));
				  }
			  }else{
				  //Lost connection kill thread
				  add_error_list(dir_sync->error_messages, ERROR, "Failed to sync song:",  "Lost connection to database");
				  return 0;
			  }
		  }
		  //Calculate the percent of files synchronized
		  dir_sync->sync_progress =(float) files_synced*100 / (*(dir_sync->num_files) - 1);
		  dir_sync->files_scanned = files_synced;
		  files_synced++;
		  file_list = file_list->next;
	}
	status = apr_dbd_transaction_end(dir_sync->dbd_config->dbd_driver, pool, dir_sync->dbd_config->transaction);
	if(status != 0){
		dbd_error = apr_dbd_error(dir_sync->dbd_config->dbd_driver, dir_sync->dbd_config->dbd_handle, status);
		add_error_list(dir_sync->error_messages, ERROR, "Database error couldn't end transaction",dbd_error);
		return 0;
	}


	apr_dbd_close(dir_sync->dbd_config->dbd_driver,dir_sync->dbd_config->dbd_handle);
	apr_pool_clear(pool);
	return 0;
}
