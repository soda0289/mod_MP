/*
 * dir_sync.c
 *
 *  Created on: Nov 12, 2012
 *      Author: reyad
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

void * APR_THREAD_FUNC sync_dir(apr_thread_t* thread, void* ptr){
	int files_synced = 0;
	int status;
	apr_status_t rv;
	const char* dbd_error;
	List* file_list;

	music_file* song;

	dir_sync_t* dir_sync = (dir_sync_t*) ptr;
	db_config* dbd_config;

	char dbd_error_message[256];

	rv = connect_database(dir_sync->pool, dir_sync->error_messages,&(dbd_config));
	if(rv != APR_SUCCESS){
		add_error_list(dir_sync->error_messages, ERROR ,"Database error couldn't connect", apr_strerror(rv, dbd_error_message, sizeof(dbd_error_message)));
		return 0;
	}
	status = prepare_database(dir_sync->app_list,dbd_config);
	if(status != 0){
		dbd_error = apr_dbd_error(dbd_config->dbd_driver,dbd_config->dbd_handle, status);
		add_error_list(dir_sync->error_messages, ERROR, "Database error couldn't prepare",dbd_error);
		return 0;
	}

	dbd_config->connected = 1;
	dbd_config->database_errors = dir_sync->error_messages;

	file_list = apr_pcalloc(dir_sync->pool, sizeof(List));
	dir_sync->num_files = apr_pcalloc(dir_sync->pool, sizeof(int));

	if(dbd_config->connected != 1){
		add_error_list(dir_sync->error_messages, ERROR, "Database not connected","ERROR ERROR");
		return 0;
	}


	read_dir(dir_sync->pool, file_list, dir_sync->dir_path, dir_sync->num_files,  dir_sync->error_messages);

	if ((file_list == NULL || dir_sync->num_files == NULL )&& (*dir_sync->num_files ) > 0 ){
		add_error_list(dir_sync->error_messages, ERROR, "Killing Sync Thread", "");
		return 0;
	}

	status = apr_dbd_transaction_start(dbd_config->dbd_driver, dir_sync->pool, dbd_config->dbd_handle,&dbd_config->transaction);
	if(status != 0){
		dbd_error = apr_dbd_error(dbd_config->dbd_driver, dbd_config->dbd_handle, status);
		add_error_list(dir_sync->error_messages,ERROR, "Database error start transaction", dbd_error);
		return 0;
	}
	while(file_list->file.path){
		  song = apr_pcalloc(dir_sync->pool, sizeof(music_file));
		  song->file = file_list->file;
		  switch(song->file.type){
		  	  case FLAC:{
		  		status = read_flac_level1(dir_sync->pool, song);
		  		if (status == 0){
		  			song->file.type_string = "flac";
		  		}
		  		break;
		  	  }
		  	  case OGG:{
		  		status = read_ogg(dir_sync->pool, song);
		  		if (status == 0){
		  			song->file.type_string = "ogg";
		  		}
		  		break;
		  	  }
		  	  case MP3:{
		  		  //not supported yet
		  		  //status = read_id3(dir_sync->pool,song);
		  		  status = 5;
			  		if (status == 0){
			  			song->file.type_string = "mp3";
			  		}
			  	break;
		  	  }
		  	 default:{
		  		  status = -1;
		  		  break;
		  	  }
		  }

		  if (status == 0 && song){
			  //Update or Insert song
			  if (dbd_config->connected == 1){
				  status = sync_song(dbd_config, song);
				  if (status != 0){
					  add_error_list(dir_sync->error_messages, ERROR, apr_psprintf(dir_sync->pool,"Failed to sync song:"),  apr_psprintf(dir_sync->pool, "(%d) Song title: %s Song artist: %s song album:%s song file path: %s",status, song->title, song->artist, song->album, song->file.path));
				  }
			  }else{
				  //Lost connection kill thread
				  add_error_list(dir_sync->error_messages, ERROR, "Failed to sync song:",  "Lost connection to database");
				  return 0;
			  }
		  }
		  //Calculate the percent of files synchronized
		  dir_sync->sync_progress =(float) files_synced*100 / (*(dir_sync->num_files) - 1);
		  files_synced++;
		  file_list = file_list->next;
	}
	status = apr_dbd_transaction_end(dbd_config->dbd_driver, dir_sync->pool, dbd_config->transaction);
	if(status != 0){
		dbd_error = apr_dbd_error(dbd_config->dbd_driver, dbd_config->dbd_handle, status);
		add_error_list(dir_sync->error_messages, ERROR, "Database error couldn't end transaction",dbd_error);
		return 0;
	}
	add_error_list(dir_sync->error_messages, DEBUG, "Thread DIR Sync completed successfully",":)");
	return 0;
}
