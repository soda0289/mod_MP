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
#include "tag_reader.h"
#include <stdlib.h>
#include "mod_mp.h"
#include "error_messages.h"
#include "dir_sync.h"

#ifdef WITH_MP3
#include "apps/music/mpg123.h"
#endif

int init_dir_sync(music_globals_t* music_globals){
	int status = 0;
	apr_status_t rv = 0;

	int i = 0;

	db_query_t* db_query;
	db_config_t* db_config;

	char dbd_error_message[255];
	const char* dbd_error;

	apr_thread_t* thread_sync_dir;

#ifdef WITH_MP3
	//Init MP3 support
	mpg123_init();
#endif

	//Find db_params by finding a query we need and connecting to the database that query depends on
	status = indexer_get_db_object (file_indexer, "song_id", &(file_db_obj));
	if(status != 0){
		return -9;
	}

	//For every music directory create a shared struct
	for(i = 0; i < music_globals->music_dirs->nelts; i++){
		dir_sh_stats_t* stats;
		dir_t* dir = &(((dir_t*)music_globals->music_dirs->elts)[i]);
		dir_sync_thread_t* dir_sync_thread = apr_pcalloc(music_globals->pool, sizeof(dir_sync_thread_t));

		//Setup dir_sync_thread
		dir_sync_thread->db_config = db_config;
		dir_sync_thread->pool = music_globals->pool;
		dir_sync_thread->error_messages = music_globals->error_messages;
		dir_sync_thread->dir = dir;

		//Setup Dir
		dir->shm_file = apr_pstrcat(music_globals->pool,music_globals->tmp_dir,"/mp_dir_sync[",apr_itoa(music_globals->pool, i),"]",NULL);
		status = setup_shared_memory(&(dir->shm),sizeof(dir_sh_stats_t),dir->shm_file, music_globals->pool);
		if(status != 0){
			return status;
		}

		
		//Create thread to connect to database and synchronize
		stats = (dir_sh_stats_t*)apr_shm_baseaddr_get(dir->shm);

		dir->stats = stats;

		stats->num_files = NULL;
		stats->sync_progress = 0.0;
		stats->files_scanned = 0;

		rv = apr_thread_create(&thread_sync_dir,NULL, sync_dir, (void*) dir_sync_thread, music_globals->pool);
		if(rv != APR_SUCCESS){
			return rv;
		}

		apr_pool_cleanup_register(music_globals->pool, dir->shm,(void*) apr_shm_destroy	, apr_pool_cleanup_null);
	}
	
	return status;
}

int reattach_dir_sync(music_globals_t* music_globals){
	int i = 0;
	int status = 0;	
	for(i = 0; i < music_globals->music_dirs->nelts; i++){
		dir_t* dir = &(((dir_t*)music_globals->music_dirs->elts)[i]);
		if(dir->shm_file){
			status = apr_shm_attach(&(dir->shm), dir->shm_file, music_globals->pool);
			if(status != APR_SUCCESS){
				return status;
			}
			dir->stats = apr_shm_baseaddr_get(dir->shm);
		}
	}

	return status;
}

int output_dirsync_status(music_query_t* music_query){
	int i = 0;


	apr_bucket_brigade* output_bb = music_query->output->bucket_brigade;

	apr_table_add(music_query->output->headers,"Access-Control-Allow-Origin", "*");
	apr_cpystrn((char*)music_query->output->content_type, "application/json", 255);



	apr_brigade_puts(music_query->output->bucket_brigade, NULL,NULL, "{\n");

	//Print Status
	if(music_query->globals->music_dirs != NULL){
		apr_brigade_puts(output_bb, NULL,NULL,"\t\"dir_sync_status\" : {\n");
			for(i = 0; i < music_query->globals->music_dirs->nelts; i++){
				dir_t* dir = &(((dir_t*)music_query->globals->music_dirs->elts)[i]);
				apr_brigade_printf(output_bb, NULL, NULL, "\t\t\"%s\" : {\n",dir->path);	
				apr_brigade_printf(output_bb, NULL,NULL, "\t\t\"Progress\" :  \"%.2f\",\n",dir->stats->sync_progress);
				apr_brigade_printf(output_bb, NULL,NULL, "\t\t\"Files Scanned\" :  \"%d\"\n", dir->stats->files_scanned);
				apr_brigade_printf(output_bb, NULL, NULL, "\t\t}");
				if(i < (music_query->globals->music_dirs->nelts - 1)){
					apr_brigade_printf(output_bb, NULL, NULL, ",");
				}
			}

		apr_brigade_puts(output_bb, NULL,NULL,"\t},\n");
	}

	apr_brigade_puts(output_bb, NULL,NULL,"\t\"db_status\" : ");
	output_db_result_json(music_query->results,music_query->db_query,music_query->output);
	apr_brigade_puts(output_bb, NULL,NULL,"\n,");

	print_error_messages(music_query->pool,output_bb, music_query->error_messages);

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

	dir_sync_thread_t* dir_sync = (dir_sync_thread_t*) ptr;
	apr_pool_t* pool;
	
	db_config_t* db_config = dir_sync->db_config;
	char error_message[256];

	//Create sub pool used for sync
	rv = apr_pool_create_ex(&pool, dir_sync->pool, NULL, NULL);
	if(rv != APR_SUCCESS){
		error_messages_add(dir_sync->error_messages, ERROR ,"Memory Pool Error", apr_strerror(rv, error_message, sizeof(error_message)));
		return 0;
	}

	file_list = apr_pcalloc(pool, sizeof(List));
	dir_sync->dir->stats->num_files = apr_pcalloc(pool, sizeof(int));

	if(db_config->connected != 1){
		error_messages_add(dir_sync->error_messages, ERROR, "Database not connected","ERROR ERROR");
		return 0;
	}

	count_table_rows();


	read_dir(pool, file_list, dir_sync->dir->path, dir_sync->dir->stats->num_files,  dir_sync->error_messages);

	if ((file_list == NULL || dir_sync->dir->stats->num_files == NULL )&& (*dir_sync->dir->stats->num_files ) > 0 ){
		error_messages_add(dir_sync->error_messages, ERROR, "Killing Sync Thread", "");
		return 0;
	}

	status = apr_dbd_transaction_start(db_config->dbd_driver, pool, db_config->dbd_handle,&(db_config->transaction));
	if(status != 0){
		dbd_error = apr_dbd_error(db_config->dbd_driver, db_config->dbd_handle, status);
		error_messages_add(dir_sync->error_messages,ERROR, "Database error start transaction", dbd_error);
		return 0;
	}
	while(file_list->file.path){
		  song = apr_pcalloc(pool, sizeof(music_file));
		  song->file = &(file_list->file);
		  switch(song->file->type){
#ifdef WITH_FLAC
		  	  case FLAC:{
		  		status = read_flac_level1(pool, song);
		  		if (status == 0){
		  			song->file->type_string = "flac";
		  		}
		  		break;
		  	  }
#endif
#ifdef WITH_OGG
		  	  case OGG:{
		  		status = read_ogg(pool, song);
		  		if (status == 0){
		  			song->file->type_string = "ogg";
		  		}
		  		break;
		  	  }
#endif
#ifdef WITH_MP3
		  	  case MP3:{
		  		  status = read_id3(pool,song);
			  	if (status == 0){
			  		song->file->type_string = "mp3";
			  	}
			  	break;
		  	  }
#endif
		  	 default:{
		  		  status = -1;
		  		  break;
		  	  }
		  }



		  if (status == 0 && song){
			  //We have song get musicbrainz ids
			  //status = get_musicbrainz_release_id(pool, song, dir_sync->error_messages);
			  //Update or Insert song
			  if (db_config->connected == 1){
				  status = sync_song(pool, db_config, song);
				  if (status != 0){
					error_messages_add(dir_sync->error_messages, ERROR, apr_psprintf(pool,"Failed to sync song:"),  apr_psprintf(pool, "(%d) Song title: %s Song artist: %s song album:%s song file path: %s",status, song->title, song->artist, song->album, song->file->path));
				  }
			  }else{
				  //Lost connection kill thread
				  error_messages_add(dir_sync->error_messages, ERROR, "Failed to sync song:",  "Lost connection to database");
				  return 0;
			  }
		  }
		  //Calculate the percent of files synchronized
		  dir_sync->dir->stats->sync_progress =(float) files_synced*100 / (*(dir_sync->dir->stats->num_files) - 1);
		  dir_sync->dir->stats->files_scanned = files_synced;
		  files_synced++;
		  file_list = file_list->next;
	}
	status = apr_dbd_transaction_end(db_config->dbd_driver, pool, db_config->transaction);
	if(status != 0){
		dbd_error = apr_dbd_error(db_config->dbd_driver, db_config->dbd_handle, status);
		error_messages_add(dir_sync->error_messages, ERROR, "Database error couldn't end transaction",dbd_error);
		return 0;
	}

	apr_pool_clear(pool);
	return 0;
}
