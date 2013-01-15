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

#include "apps/app_config.h"

#include "apps/music/dir_sync/dir_sync.h"
#include "apps/music/tag_reader.h"


static void* mediaplayer_config_srv(apr_pool_t* pool, server_rec* s){
	mediaplayer_srv_cfg* srv_conf = apr_pcalloc(pool, sizeof(mediaplayer_srv_cfg));
	srv_conf->dir_sync_shm_file = "/tmp/mp_dir_sync";
	srv_conf->errors_shm_file = "/tmp/mp_errors";
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

int mediaplayer_post_config(apr_pool_t *pconf, apr_pool_t *plog, apr_pool_t* ptemp,server_rec *s){
	apr_status_t rv;
	apr_thread_t* thread_sync_dir;
	mediaplayer_srv_cfg* srv_conf;


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
			apr_status_t rv;

			rv = apr_shm_create(&srv_conf->dir_sync_shm, sizeof(dir_sync_t),srv_conf->dir_sync_shm_file , pconf);
			if (rv == APR_EEXIST) {
				rv = apr_shm_attach(&(srv_conf->dir_sync_shm), srv_conf->dir_sync_shm_file, pconf);
			}
			if(rv != APR_SUCCESS){
				ap_log_error(__FILE__,__LINE__, APLOG_CRIT, rv, s, "Error creating shared memory!!!");
				srv_conf->dir_sync_shm = NULL;
				return HTTP_INTERNAL_SERVER_ERROR;
			}
			rv = apr_shm_create(&srv_conf->errors_shm, sizeof(error_messages_t), srv_conf->errors_shm_file, pconf);
		    if (rv == APR_EEXIST) {
		    	rv =apr_shm_attach(&(srv_conf->errors_shm), srv_conf->errors_shm_file, pconf);
		    }
			if(rv != APR_SUCCESS){
				ap_log_error(__FILE__,__LINE__, APLOG_CRIT, rv, s, "Error creating shared memory!!!");
				srv_conf->errors_shm = NULL;
				return HTTP_INTERNAL_SERVER_ERROR;
			}




			//Create thread to connect to database and synchronize
			dir_sync_t*dir_sync = apr_shm_baseaddr_get(srv_conf->dir_sync_shm);
			srv_conf->error_messages = apr_shm_baseaddr_get(srv_conf->errors_shm);
			srv_conf->error_messages->num_errors = 0;


			srv_conf->apps = apr_pcalloc(pconf,sizeof(app_list_t));
			srv_conf->apps->pool = pconf;

			srv_conf->decoding_queue = apr_pcalloc(pconf,sizeof(queue_t));

			config_app(srv_conf->apps,"music","music",NULL,get_music_query,run_music_query);


			dir_sync->pool = pconf;
			dir_sync->num_files = NULL;
			dir_sync->dir_path = srv_conf->external_directory;
			dir_sync->error_messages = srv_conf->error_messages;
			dir_sync->sync_progress = 0.0;
			dir_sync->app_list = srv_conf->apps;

			rv = apr_thread_create(&thread_sync_dir,NULL, sync_dir, (void*) dir_sync, s->process->pool);
			apr_pool_cleanup_register(pconf, srv_conf->dir_sync_shm,(void*) apr_shm_destroy	, apr_pool_cleanup_null);
			apr_pool_cleanup_register(pconf, srv_conf->errors_shm,(void*) apr_shm_destroy	, apr_pool_cleanup_null);
		}
	}while ((s = s->next) != NULL);


	return OK;
}



void mediaplayer_child_init(apr_pool_t *child_pool, server_rec *s){
	apr_status_t rv;
	mediaplayer_srv_cfg* srv_conf;
	int status;
	const char* dbd_error;
	char dbd_error_message[256];
	//Scan through every server and determine which one has directories to be synchronized
	do{
		srv_conf = ap_get_module_config(s->module_config, &mediaplayer_module);
		if(srv_conf->enable){
			//Create shared memory

			//Reattach shared memory
			apr_shm_attach(&(srv_conf->dir_sync_shm), srv_conf->dir_sync_shm_file, child_pool);
			apr_shm_attach(&(srv_conf->errors_shm), srv_conf->errors_shm_file, child_pool);
			//Create new database connection for every fork
			rv = connect_database(child_pool, srv_conf->error_messages,&(srv_conf->dbd_config));
			if(rv != APR_SUCCESS){
				add_error_list(srv_conf->error_messages, ERROR, "Database error couldn't connect", apr_strerror(rv, dbd_error_message, sizeof(dbd_error_message)));
			}else{
				status = prepare_database(srv_conf->apps,srv_conf->dbd_config);
				if(status != 0){
					dbd_error = apr_dbd_error(srv_conf->dbd_config->dbd_driver,srv_conf-> dbd_config->dbd_handle, status);
					add_error_list(srv_conf->error_messages, ERROR, "Database error couldn't prepare",dbd_error);
				}
			}
		}
	}while ((s = s->next) != NULL);

}

char* json_escape_char(apr_pool_t* pool, const char* string){

	int i;
	if(string == NULL){
		return NULL;
	}
	char* escape_string = apr_pstrdup(pool, string);

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
	apr_table_add(r->headers_out, "Access-Control-Allow-Origin", "*");
	ap_set_content_type(r, "application/json") ;

	//Print Status
		ap_rputs("{\n\t\"status\" : {", r);
			ap_rprintf(r, "\t\"Progress\" :  \"%.2f\",\n", dir_sync->sync_progress);

			ap_rputs("\"Errors\" : [\n", r);
				//Print Errors
		int i;
		for (i =0;i < rec_cfg->error_messages->num_errors; i++){
			ap_rprintf(r, "\t{\"type\" : %d,\n\t\"header\" : \"%s\",\n\t\"message\" : \"%s\"}\n", rec_cfg->error_messages->messages[i].type,json_escape_char(r->pool,rec_cfg->error_messages->messages[i].header), json_escape_char(r->pool,rec_cfg->error_messages->messages[i].message));
			if (i+1 != rec_cfg->error_messages->num_errors){
				ap_rputs(",",r);
			}
		}
		ap_rputs("]", r);
		ap_rputs("},\n", r);
		return 0;
}

int run_get_method(request_rec* r){
	int error_num;

	app_t* app;

	mediaplayer_srv_cfg* srv_conf = ap_get_module_config(r->server->module_config, &mediaplayer_module) ;
	mediaplayer_rec_cfg* rec_cfg = ap_get_module_config(r->request_config, &mediaplayer_module);

	if(srv_conf->dbd_config == NULL || srv_conf->dir_sync_shm == NULL || srv_conf->errors_shm == NULL){
		//BIG Error
		ap_rprintf(r, "something is wrong NULL value found");
		return OK;
	}

	if(srv_conf->dbd_config->connected != 1){
		add_error_list(rec_cfg->error_messages, ERROR,"Database Error","Database is not connected.");
		return output_status_json(r);
	}else{//Everything is OK
		error_num = app_process_uri(r->pool,r->uri, srv_conf->apps,&app);
		if (error_num != 0){
			//Query Failed
			add_error_list(rec_cfg->error_messages, ERROR,"Error no app found", "Problem with URI");
			//Print status and die
			return output_status_json(r);
		}
		error_num = app->get_query(r->pool,rec_cfg->error_messages,&(app->query),&(app->query_words),app->db_queries);
		//Check if Query is ok
		if (error_num != 0){
			//Query Failed
			add_error_list(rec_cfg->error_messages, ERROR,"Error with query", "Problem with URI");
			//Print status and die
			return output_status_json(r);
		}

		//Query OK run app and die
		app->run_query(r,app->query,srv_conf->dbd_config,NULL);
	}
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
	int i = 0;
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
