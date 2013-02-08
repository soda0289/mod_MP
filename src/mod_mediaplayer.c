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


#include <stdlib.h>
#include "mod_mediaplayer.h"
#include "error_handler.h"
#include <util_filter.h>
#include <http_log.h>
#include "unixd.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <time.h>

#include "apps/app_config.h"

#include "apps/music/dir_sync/dir_sync.h"
#include "apps/music/tag_reader.h"

#include "unixd.h"

#include "apps/music/decoding_queue.h"


static void* mediaplayer_config_srv(apr_pool_t* pool, server_rec* s){
	mediaplayer_srv_cfg* srv_conf = apr_pcalloc(pool, sizeof(mediaplayer_srv_cfg));

	srv_conf->dir_sync_shm_file = "/tmp/mp_dir_sync";
	srv_conf->errors_shm_file = "/tmp/mp_errors";
	srv_conf->queue_shm_file = "/tmp/mp_decoding_queue";
	/*
	srv_conf->dir_sync_shm_file = NULL;
	srv_conf->errors_shm_file = NULL;
	srv_conf->queue_shm_file = NULL;
	*/

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

int setup_shared_memory(apr_shm_t** shm,apr_size_t size,const char* file_path, apr_pool_t* pool){
	apr_status_t rv;
    apr_uid_t uid;
    apr_gid_t gid;
    key_t shm_key;
    int shm_id;
    struct shmid_ds buf;
    time_t t;

    int status;


    apr_uid_get(&uid,&gid,"apache", pool);

	rv = apr_shm_create(shm, size,file_path , pool);
	if(rv == 17){
		//It already exsits lets kill it with fire
		rv = apr_shm_attach(shm,file_path , pool);
		rv = apr_shm_destroy(*shm);
		rv = apr_shm_create(shm, size,file_path , pool);
	}
	if(rv != APR_SUCCESS){

		*shm = NULL;
		return -1;
	}

	shm_key = ftok(file_path,1);
	shm_id = shmget(shm_key,size,0);

	status = shmctl(shm_id, IPC_STAT, &buf);
	if(status != 0){
		return -2;
	}
	time(&t);
	buf.shm_ctime = t;
	buf.shm_perm.gid = gid;
	buf.shm_perm.uid = uid;
	buf.shm_perm.mode = 0664;
	status = shmctl(shm_id, IPC_SET, &buf);
	if(status != 0){
		return -3;
	}

	status = chmod(file_path, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
	status = chown(file_path,uid,gid);

	return 0;
}

static int mediaplayer_post_config(apr_pool_t *pconf, apr_pool_t *plog, apr_pool_t* ptemp,server_rec *s){
	apr_thread_t* thread_sync_dir;
	mediaplayer_srv_cfg* srv_conf;


    void *data = NULL;
	dir_sync_t*dir_sync;
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
			apr_status_t rv = 0;
			int status;
			//Setup Shared Memory
			status = setup_shared_memory(&(srv_conf->dir_sync_shm),sizeof(dir_sync_t),srv_conf->dir_sync_shm_file, pconf);
			if(status != 0){
				ap_log_error(APLOG_MARK, APLOG_CRIT, rv, s, "Error creating shared memory!!!");
			}

			status = setup_shared_memory(&(srv_conf->errors_shm),sizeof(error_messages_t),srv_conf->errors_shm_file, pconf);
			if(status != 0){
				ap_log_error(APLOG_MARK, APLOG_CRIT, rv, s, "Error creating shared memory!!!");
			}

			//Create thread to connect to database and synchronize
			dir_sync = (dir_sync_t*)apr_shm_baseaddr_get(srv_conf->dir_sync_shm);
			srv_conf->error_messages = apr_shm_baseaddr_get(srv_conf->errors_shm);
			srv_conf->pid = getpid();
			srv_conf->error_messages->num_errors = 0;


			srv_conf->apps = apr_pcalloc(pconf,sizeof(app_list_t));
			srv_conf->apps->pool = pconf;
			//Init queue should be moved into app init function

			rv = create_decoding_queue(s->process->pool, srv_conf->queue_shm_file,&(srv_conf->decoding_queue));
			if(rv != 0){
				ap_log_error(APLOG_MARK, APLOG_CRIT, rv, s, "Error creating shared memory!!!");
			}

			config_app(srv_conf->apps,"music","music",NULL,get_music_query,run_music_query);


			rv = apr_dbd_init(pconf);
			if (rv != APR_SUCCESS){
			  //Run error function
			  return rv;
			}



			dir_sync->pool = pconf;
			dir_sync->num_files = NULL;
			dir_sync->dir_path = srv_conf->external_directory;
			dir_sync->error_messages = srv_conf->error_messages;
			dir_sync->sync_progress = 0.0;
			dir_sync->app_list = srv_conf->apps;

			rv = apr_thread_create(&thread_sync_dir,NULL, sync_dir, (void*) dir_sync, s->process->pool);
			if(rv != APR_SUCCESS){
				ap_log_error(__FILE__,__LINE__, 0,APLOG_CRIT, rv, s, "Error starting thread");
			}


			apr_pool_cleanup_register(pconf, srv_conf->dir_sync_shm,(void*) apr_shm_destroy	, apr_pool_cleanup_null);
			apr_pool_cleanup_register(pconf, srv_conf->errors_shm,(void*) apr_shm_destroy	, apr_pool_cleanup_null);
		}
	}while ((s = s->next) != NULL);

	return OK;
}



static void mediaplayer_child_init(apr_pool_t *child_pool, server_rec *s){
	apr_status_t rv = 0;
	mediaplayer_srv_cfg* srv_conf;
	int status;
	const char* dbd_error;
	char dbd_error_message[256];
	//Scan through every server and determine which one has directories to be synchronized
	do{
		srv_conf = ap_get_module_config(s->module_config, &mediaplayer_module);
		if(srv_conf->enable){

			//Reattach shared memory
			if(getpid() != srv_conf->pid){
				//Only when in new proccess do we re attach shared memory
				//If we are in the same proccess we dont need shared memory do we.
				//Reattach shared memory

				if(srv_conf->dir_sync_shm_file){
					rv = apr_shm_attach(&(srv_conf->dir_sync_shm), srv_conf->dir_sync_shm_file, child_pool);
					if(rv != APR_SUCCESS){
						ap_log_error(APLOG_MARK, APLOG_CRIT, rv, s, "Error reattaching shared memeory Directory Sync");
					}
				}
				if(srv_conf->errors_shm_file){
					rv = apr_shm_attach(&(srv_conf->errors_shm), srv_conf->errors_shm_file, child_pool);
					if(rv != APR_SUCCESS){
							ap_log_error(__FILE__,__LINE__, 0,APLOG_CRIT, rv, s, "Error reattaching shared memeory Error Messages");
					}
				}


				//Setup srv_conf
				srv_conf->error_messages = apr_shm_baseaddr_get(srv_conf->errors_shm);
				//Were in a new process reattach decoding queue
				rv = reattach_decoding_queue(s->process->pool, srv_conf->decoding_queue, srv_conf->queue_shm_file, srv_conf->error_messages);
				if(rv != 0){
					ap_log_error(APLOG_MARK, APLOG_CRIT, rv, s, "Error recreating shared memory!!!");
				}

				//Setup Database Connection
				rv = connect_database(s->process->pconf, srv_conf->error_messages,&(srv_conf->dbd_config));
				if(rv != APR_SUCCESS){
					add_error_list(srv_conf->error_messages, ERROR, "Database error couldn't connect", apr_strerror(rv, dbd_error_message, sizeof(dbd_error_message)));
				}else{
					status = prepare_database(srv_conf->apps,srv_conf->dbd_config);
					if(status != 0){
						dbd_error = apr_dbd_error(srv_conf->dbd_config->dbd_driver,srv_conf-> dbd_config->dbd_handle, status);
						add_error_list(srv_conf->error_messages, ERROR, "Database error couldn't prepare",dbd_error);
					}
					//apr_pool_cleanup_register(child_pool, srv_conf->queue_shm,(void*) close_database	, apr_pool_cleanup_null);
				}

			}else{

				//THIS SHOULD ONLY HAPPEN IN DEBUG MODE
				//You cannot copy pointers in shared memory
				dir_sync_t* dir_sync;
				dir_sync = (dir_sync_t*)apr_shm_baseaddr_get(srv_conf->dir_sync_shm);
				srv_conf->dbd_config = dir_sync->dbd_config;
			}
		}
	}while ((s = s->next) != NULL);

}

char* json_escape_char(apr_pool_t* pool, const char* string){

	int i;
	char* escape_string;
	if(string == NULL){
		return NULL;
	}

	escape_string = apr_pstrdup(pool, string);

	for (i = 0; i < strlen(escape_string); i++){
		if (escape_string[i] == '"'){
			escape_string[i] = '\0';
			escape_string = apr_pstrcat(pool, &escape_string[0], "\\\"", &escape_string[++i], NULL);
		}
	}

	return escape_string;
}

int output_status_json(request_rec* r){
	mediaplayer_srv_cfg* srv_conf = ap_get_module_config(r->server->module_config, &mediaplayer_module) ;
	dir_sync_t* dir_sync = apr_shm_baseaddr_get(srv_conf->dir_sync_shm);

	mediaplayer_rec_cfg* rec_cfg = ap_get_module_config(r->request_config, &mediaplayer_module);

	//Apply header
	apr_table_set(r->headers_out, "Access-Control-Allow-Origin", "*");
	ap_set_content_type(r, "application/json") ;
	ap_rputs("{\n", r);

	//Print Status
		ap_rputs("\t\"status\" : {\n", r);
		ap_rprintf(r, "\t\t\"Progress\" :  \"%.2f\",\n", dir_sync->sync_progress);
		print_error_messages(r, rec_cfg->error_messages);

		ap_rputs("\t}\n}\n",r);
		return 0;
}


int run_get_method(request_rec* r){
	int error_num;

	app_t* app;

	mediaplayer_srv_cfg* srv_conf = ap_get_module_config(r->server->module_config, &mediaplayer_module) ;
	mediaplayer_rec_cfg* rec_cfg = ap_get_module_config(r->request_config, &mediaplayer_module);

	if(srv_conf->dbd_config == NULL){
		//Try and copy dbd from dir_sync
		//WE SHOULD ONLY DO THIS FOR DEBUG
		if(srv_conf->dbd_config == NULL){
			dir_sync_t* dir_sync;
			dir_sync = (dir_sync_t*)apr_shm_baseaddr_get(srv_conf->dir_sync_shm);
			srv_conf->dbd_config = dir_sync->dbd_config;
		}
	}

	if(srv_conf->dbd_config->connected != 1){
		add_error_list(rec_cfg->error_messages, ERROR,"Database Error","Database is not connected. Skipping app!");
	}else if((error_num = app_process_uri(r->pool,r->uri, srv_conf->apps,&app)) != 0){
		add_error_list(rec_cfg->error_messages, ERROR,"Processing URI","No app found!");
	}else{
		error_num = app->get_query(r->pool,rec_cfg->error_messages,&(app->query),&(app->query_words),app->db_queries);
		//Query OK run app and die
		app->run_query(r,app->query,srv_conf->dbd_config,NULL);
		return OK;
	}
	//ERROR
	output_status_json(r);
	return OK;
}

static int run_post_method(request_rec* r){
	output_status_json(r);
	return OK;
}

static int mediaplayer_handler(request_rec* r) {
	mediaplayer_rec_cfg* rec_cfg;
	mediaplayer_srv_cfg* srv_conf;
	error_messages_t* error_messages;

	int i;

	rec_cfg = apr_pcalloc(r->pool, sizeof(mediaplayer_rec_cfg));
	srv_conf = ap_get_module_config(r->server->module_config, &mediaplayer_module);

	if(srv_conf->enable != 1){
		return DECLINED;
	}

	if(srv_conf->errors_shm == NULL || srv_conf->dir_sync_shm == NULL){
		//BIG Error
		add_error_list(srv_conf->error_messages,ERROR,"Error shared memory is null","This should not be able to print without shared memory");
		output_status_json(r);
		return OK;
	}
	error_messages = apr_shm_baseaddr_get(srv_conf->errors_shm);

	//Copy error messages from shared memory
	i = 0;
	rec_cfg->error_messages =apr_pcalloc(r->pool, sizeof(error_messages_t));
	rec_cfg->error_messages->num_errors = error_messages->num_errors;
	for(i = 0; i < error_messages->num_errors; i++){
		rec_cfg->error_messages->messages[i] = error_messages->messages[i];
	}
	ap_set_module_config(r->request_config, &mediaplayer_module, rec_cfg) ;

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
