/*
 * mod_mediaplayer.c
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
 *  See the License for the specific language governing permissions and
 *   limitations under the License.
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
#include "mod_mediaplayer.h"
#include "error_handler.h"
#include <util_filter.h>
#include <http_log.h>
#include "unixd.h"


static void* mediaplayer_config_srv(apr_pool_t* pool, server_rec* s){
	mediaplayer_srv_cfg* srv_conf = apr_pcalloc(pool, sizeof(mediaplayer_srv_cfg));

	srv_conf->mutex_name ="/tmp/tmp22";

	return srv_conf;
}
static const char* mediaplayer_set_enable (cmd_parms* cmd, void* cfg, int arg){

	mediaplayer_srv_cfg* srv_conf = ap_get_module_config(cmd->server->module_config, &mediaplayer_module);

	srv_conf->enable = arg;
	return NULL;
}

static const char* mediaplayer_set_external_dir (cmd_parms* cmd, void* cfg, const char* arg){

	mediaplayer_srv_cfg* srv_conf = ap_get_module_config(cmd->server->module_config, &mediaplayer_module);

	srv_conf->external_directory = arg;
	return NULL;
}

static void * APR_THREAD_FUNC sync_dir(apr_thread_t* thread, void* ptr){
	int files_synced = 0;
	int status;
	apr_status_t rv;
	const char* dbd_error;
	char error_message[256];
	List* file_list;

	music_file* song;

	dir_sync_t* dir_sync = (dir_sync_t*) ptr;

	mediaplayer_srv_cfg* srv_conf = ap_get_module_config(dir_sync->s->module_config, &mediaplayer_module);

	file_list = apr_pcalloc(dir_sync->pool, sizeof(List));
	dir_sync->num_files = apr_pcalloc(dir_sync->pool, sizeof(int));

	if(srv_conf->dbd_config->connected != 1){
		add_error_list(srv_conf->error_messages, "Database not connected","ERROR ERROR");
		return 0;
	}
	add_error_list(srv_conf->error_messages, "Thread started sucsefully","ERROR ERROR");



	rv = apr_global_mutex_lock(srv_conf->dbd_mutex);
	if (rv != APR_SUCCESS){
		add_error_list(srv_conf->error_messages, "Couldn't Obtain lock on database", apr_strerror(rv, error_message, sizeof(error_message)));
		return 0;
	}
	read_dir(dir_sync->pool, file_list, srv_conf->external_directory, dir_sync->num_files,  srv_conf->error_messages);

	if ((file_list == NULL || dir_sync->num_files == NULL )&& (*dir_sync->num_files ) > 0 ){
		add_error_list(srv_conf->error_messages, "Killing Sync Thread", "");
		rv = apr_global_mutex_unlock(srv_conf->dbd_mutex);
		if (rv != APR_SUCCESS){
			add_error_list(srv_conf->error_messages, "Couldn't unlock on database 1", apr_strerror(rv, error_message, sizeof(error_message)));
		}
		return 0;
	}
	status = apr_dbd_transaction_start(srv_conf->dbd_config->dbd_driver, dir_sync->pool, srv_conf->dbd_config->dbd_handle,&srv_conf->dbd_config->transaction);
	if(status != 0){
		dbd_error = apr_dbd_error(srv_conf->dbd_config->dbd_driver,srv_conf-> dbd_config->dbd_handle, status);
		add_error_list(srv_conf->error_messages, "Database error start transaction", dbd_error);
		rv = apr_global_mutex_unlock(srv_conf->dbd_mutex);
		if (rv != APR_SUCCESS){
			add_error_list(srv_conf->error_messages, "Couldn't unlock on database 2", apr_strerror(rv, error_message, sizeof(error_message)));
		}
		return 0;
	}
	while(file_list){
		  song = apr_pcalloc(dir_sync->pool, sizeof(music_file));
		  song->file_path = file_list->name;
		  status = read_flac_level1(dir_sync->pool, song);
		  if (status == 0 && song){
			  //Update or Insert song
			  if (srv_conf->dbd_config->connected == 1){
				  status = sync_song(srv_conf->dbd_config, song, file_list->mtime, srv_conf->error_messages);
			  }else{
				  //Lost connection kill threared
				  add_error_list(srv_conf->error_messages, "Failed to sync song:",  "Lost connection to database");
				  rv = apr_global_mutex_unlock(srv_conf->dbd_mutex);
					if (rv != APR_SUCCESS){
						add_error_list(srv_conf->error_messages, "Couldn't unlock on database 3", apr_strerror(rv, error_message, sizeof(error_message)));
					}
				  return 0;
			  }
			  if (status != 0){
				  add_error_list(srv_conf->error_messages, "Failed to sync song:",  apr_psprintf(dir_sync->pool, "Song title: %s Song artist: %s song album:%s song file path: %s",song->title, song->artist, song->album, song->file_path));
			  }
		  }else{
			  //not a flac file
		  }
		  //Calculate the percent of files synchronized
		  dir_sync->sync_progress =(float) files_synced*100 / *(dir_sync->num_files);
		  files_synced++;
		  file_list = file_list->next;
	}
	status = apr_dbd_transaction_end(srv_conf->dbd_config->dbd_driver, dir_sync->pool, srv_conf->dbd_config->transaction);
	if(status != 0){
		dbd_error = apr_dbd_error(srv_conf->dbd_config->dbd_driver,srv_conf-> dbd_config->dbd_handle, status);
		add_error_list(srv_conf->error_messages, "Database error couldn't end transaction",dbd_error);
		rv = apr_global_mutex_unlock(srv_conf->dbd_mutex);
		if (rv != APR_SUCCESS){
			add_error_list(srv_conf->error_messages, "Couldn't unlock on database 4", apr_strerror(rv, error_message, sizeof(error_message)));
		}
		return 0;
	}
	rv = apr_global_mutex_unlock(srv_conf->dbd_mutex);
	if (rv != APR_SUCCESS){
		add_error_list(srv_conf->error_messages, "Couldn't unlock on database 5", apr_strerror(rv, error_message, sizeof(error_message)));
	}
	add_error_list(srv_conf->error_messages, "Thread completed sucsefully","ERROR ERROR");
	return 0;
}

static int mediaplayer_post_config(apr_pool_t *pconf, apr_pool_t *plog, apr_pool_t* ptemp,server_rec *s){
	apr_status_t rv;
	apr_thread_t* thread_sync_dir;
	mediaplayer_srv_cfg* srv_conf;
	int status;
	const char* dbd_error;
	char dbd_error_message[256];

    void *data = NULL;
    const char *userdata_key = "mediaplayer_post_config";


    /* Apache loads DSO modules twice. We want to wait until the second
     * load before setting up our global mutex and shared memory segment.
     * To avoid the first call to the post_config hook, we set some
     * dummy userdata in a pool that lives longer than the first DSO
     * load, and only run if that data is set on subsequent calls to
     * this hook.
     */
	apr_pool_userdata_get(&data, userdata_key, s->process->pool);
	if (data == NULL) {
		apr_pool_userdata_set((const void *)1, userdata_key, apr_pool_cleanup_null, s->process->pool);
		return OK;
    }
	//Scan through every server and determine which one has directories to be synchronized
	do{
		srv_conf = ap_get_module_config(s->module_config, &mediaplayer_module);
		if(srv_conf->enable && srv_conf->external_directory != NULL){

			rv = apr_shm_create(&srv_conf->dir_sync_shm, sizeof(dir_sync_t), NULL, pconf);
			if(rv != APR_SUCCESS){
				ap_log_error(__FILE__,__LINE__, APLOG_CRIT, rv, s, "Error creating shared memory!!!");
				srv_conf->dir_sync_shm = NULL;
				return HTTP_INTERNAL_SERVER_ERROR;
			}
			rv = apr_shm_create(&srv_conf->errors_shm, sizeof(error_messages_t), NULL, pconf);
			if(rv != APR_SUCCESS){
				ap_log_error(__FILE__,__LINE__, APLOG_CRIT, rv, s, "Error creating shared memory!!!");
				srv_conf->errors_shm = NULL;
				return HTTP_INTERNAL_SERVER_ERROR;
			}

			//Create thread to connect to database and synchronize
			dir_sync_t*dir_sync = apr_shm_baseaddr_get(srv_conf->dir_sync_shm);
			srv_conf->error_messages = apr_shm_baseaddr_get(srv_conf->errors_shm);

			rv = apr_global_mutex_create(&srv_conf->dbd_mutex, srv_conf->mutex_name, APR_LOCK_DEFAULT, pconf);
			if(rv != APR_SUCCESS){
				ap_log_error(__FILE__,__LINE__, APLOG_ERR, rv, s, "Error creating mutex %s!!!", srv_conf->mutex_name);
				srv_conf->dbd_mutex = NULL;
				return 0;
			}

			//Set up error_messages
			srv_conf->error_messages->pool = pconf;
			srv_conf->error_messages->error_table = apr_table_make(pconf, 256);


			dir_sync->pool = pconf;
			dir_sync->s =s;

			rv = connect_database(pconf, &(srv_conf->dbd_config));
			if(rv != APR_SUCCESS){
				add_error_list(srv_conf->error_messages, "Database error couldn't connect", apr_strerror(rv, dbd_error_message, sizeof(dbd_error_message)));
				return 0;
			}
			status = prepare_database(srv_conf->dbd_config);
			if(status != 0){
				dbd_error = apr_dbd_error(srv_conf->dbd_config->dbd_driver,srv_conf-> dbd_config->dbd_handle, status);
				add_error_list(srv_conf->error_messages, "Database error couldn't prepare",dbd_error);
				return 0;
			}

			srv_conf->dbd_config->connected = 1;


			rv = apr_thread_create(&thread_sync_dir,NULL, sync_dir, (void*) dir_sync, s->process->pool);
			//apr_pool_cleanup_register(pconf, srv_conf->dbd_mutex,(void*) apr_global_mutex_destroy, apr_pool_cleanup_null);
		}
	}while ((s = s->next) != NULL);


	return OK;
}



static void mediaplayer_child_init(apr_pool_t *child_pool, server_rec *s){
	apr_status_t rv;
	mediaplayer_srv_cfg* srv_conf;

	//Scan through every server and determine which one has directories to be synchronized
	do{
		srv_conf = ap_get_module_config(s->module_config, &mediaplayer_module);
		if(srv_conf->enable){
			rv = apr_global_mutex_child_init(&srv_conf->dbd_mutex,srv_conf->mutex_name, child_pool);
			if(rv != APR_SUCCESS){
				ap_log_error(__FILE__,__LINE__, APLOG_CRIT, rv, s, "Error creating mutex for childs!!!");
				srv_conf->dbd_mutex = NULL;
			}
#ifdef AP_NEED_SET_MUTEX_PERMS
			rv = unixd_set_global_mutex_perms(srv_conf->dbd_mutex);
			if(rv != APR_SUCCESS){
				ap_log_error(__FILE__,__LINE__, APLOG_CRIT, rv, s, "Error creating setting permissions!!");
				srv_conf->dbd_mutex = NULL;
			}
#endif
			//Create new database connection for every fork
			connect_database(child_pool, &srv_conf->dbd_config);
			prepare_database(srv_conf->dbd_config);
		}
	}while ((s = s->next) != NULL);
}

static int run_get_method(request_rec* r){
	int error_num;
	const char* error_message;
	mediaplayer_srv_cfg* srv_conf = ap_get_module_config(r->server->module_config, &mediaplayer_module) ;
	mediaplayer_rec_cfg* rec_cfg = ap_get_module_config(r->request_config, &mediaplayer_module);
	rec_cfg->query = apr_pcalloc(r->pool, sizeof(music_query));

	if(srv_conf->dbd_config == NULL || srv_conf->dbd_mutex == NULL || srv_conf->dir_sync_shm == NULL || srv_conf->errors_shm == NULL){
		//BIG Error
		ap_rprintf(r, "something is wrong NULL vaue found");
		return OK;
	}

	error_num = get_music_query(r, rec_cfg->query);
	if (error_num != 0){
		add_error_list(rec_cfg->error_messages, "Error with query", "blah");
	}
	if(srv_conf->dbd_config->connected != 1){
		add_error_list(rec_cfg->error_messages, "Database not connected","ERROR ERROR");
	}else{
		error_num = run_music_query(r, rec_cfg->query);
		if (error_num != 0){
			error_message = apr_dbd_error(srv_conf->dbd_config->dbd_driver, srv_conf->dbd_config->dbd_handle, error_num);

			add_error_list(rec_cfg->error_messages, "Error running query", error_message);
		}
	}



	output_json(r);

	return OK;
}

static int run_post_method(request_rec* r){

	return OK;
}

static int mediaplayer_handler(request_rec* r) {
	apr_status_t rv;
	mediaplayer_srv_cfg* srv_conf = ap_get_module_config(r->server->module_config, &mediaplayer_module) ;
	if(srv_conf->enable != 1){
		return DECLINED;
	}
	error_messages_t* error_messages = apr_shm_baseaddr_get(srv_conf->errors_shm);
	mediaplayer_rec_cfg* rec_cfg = apr_pcalloc(r->pool, sizeof(mediaplayer_rec_cfg));


	//Copy error messages from shared memory
	//Create request error table
	rec_cfg->error_messages =apr_pcalloc(r->pool, sizeof(error_messages_t));
	rec_cfg->error_messages->error_table = apr_table_clone(r->pool, error_messages->error_table);
	rec_cfg->error_messages->num_errors = error_messages->num_errors;
	rec_cfg->error_messages->pool = r->pool;
	ap_set_module_config(r->request_config, &mediaplayer_module, rec_cfg) ;






	rv = apr_global_mutex_trylock(srv_conf->dbd_mutex);
	if( APR_STATUS_IS_EBUSY(rv)){
		ap_rprintf(r, "Locked Up!");
		return OK;
	}
	apr_global_mutex_lock(srv_conf->dbd_mutex);
	apr_pool_cleanup_register(r->pool, srv_conf->dbd_mutex, (void*) apr_global_mutex_unlock, apr_pool_cleanup_null);
	//Allow cross site xmlHTTPRequest
	apr_table_add(r->headers_out, "Access-Control-Allow-Origin", "*");
	if ( r->method_number == M_GET){
		return run_get_method(r);
	}
	if (r->method_number == M_POST) {
		return run_post_method(r);
	}
  return DECLINED;
}

/*Define Configuration file parameters*/
static const command_rec mediaplayer_cmds[] = {
		AP_INIT_FLAG("Mediaplayer", mediaplayer_set_enable, NULL, RSRC_CONF, "Enable mod_mediaplayer on server(On/Off"),
		AP_INIT_TAKE1("Music_Dir", mediaplayer_set_external_dir, NULL, RSRC_CONF, "Directory containing media files") ,
  { NULL }
};

/* Hook our handler into Apache at startup */
static void mediaplayer_hooks(apr_pool_t* pool) {
	ap_hook_post_config(mediaplayer_post_config, NULL, NULL, APR_HOOK_MIDDLE);
	ap_hook_child_init(mediaplayer_child_init, NULL, NULL, APR_HOOK_MIDDLE);
	ap_hook_handler(mediaplayer_handler, NULL, NULL, APR_HOOK_MIDDLE) ;
}

module AP_MODULE_DECLARE_DATA mediaplayer_module = {
        STANDARD20_MODULE_STUFF,
        NULL,
        NULL,
        mediaplayer_config_srv,
        NULL,
        mediaplayer_cmds,
        mediaplayer_hooks
} ;
