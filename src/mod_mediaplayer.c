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

	srv_conf->errors_shm_file = "/tmp/mp_errors";

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
			apr_status_t rv = 0;
			int status;

			init_error_messages(pconf,&(srv_conf->error_messages), srv_conf->errors_shm_file);
			srv_conf->pid = getpid();

			srv_conf->apps = apr_pcalloc(pconf,sizeof(app_list_t));
			srv_conf->apps->pool = pconf;

			config_app(srv_conf->apps,"music","music",init_music_query,reattach_music_query,run_music_query);
			init_apps(srv_conf->apps,pconf, srv_conf->error_messages,srv_conf->external_directory);


			rv = apr_dbd_init(pconf);
			if (rv != APR_SUCCESS){
			  //Run error function
			  return rv;
			}
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
			if(/*getpid() != srv_conf->pid*/1){
				//Only when in new proccess do we re attach shared memory
				//If we are in the same proccess we dont need shared memory do we.
				//Reattach shared memory

				reattach_error_messages(child_pool,&(srv_conf->error_messages), srv_conf->errors_shm_file);
				//Were in a new process reattach decoding queue

				reattach_apps(srv_conf->apps,child_pool,srv_conf->error_messages);

				//Setup Database Connection
				rv = connect_database(child_pool, srv_conf->error_messages,&(srv_conf->dbd_config));
				if(rv != APR_SUCCESS){
					add_error_list(srv_conf->error_messages, ERROR, "Database error couldn't connect", apr_strerror(rv, dbd_error_message, sizeof(dbd_error_message)));
				}else{
					status = prepare_database(srv_conf->apps,srv_conf->dbd_config);
					if(status != 0){
						dbd_error = apr_dbd_error(srv_conf->dbd_config->dbd_driver,srv_conf-> dbd_config->dbd_handle, status);
						add_error_list(srv_conf->error_messages, ERROR, "Database error couldn't prepare",dbd_error);
					}
					apr_pool_cleanup_register(child_pool, srv_conf->dbd_config,(void*) close_database	, apr_pool_cleanup_null);
				}

			}else{

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

int output_status_json(apr_pool_t* pool, apr_bucket_brigade* output_bb,apr_table_t* output_headers, const char* output_content_type,error_messages_t* error_messages){
	apr_table_add(output_headers,"Access-Control-Allow-Origin", "*");
	apr_cpystrn(output_content_type, "application/json", 255);

	apr_brigade_puts(output_bb, NULL,NULL, "{\n");

	//Print Status
	apr_brigade_puts(output_bb, NULL,NULL,"\t\"status\" : {\n");
	//apr_brigade_printf(output_bb, NULL,NULL, "\t\t\"Progress\" :  \"%.2f\",\n", dir_sync->sync_progress);
	print_error_messages(pool,output_bb, error_messages);

	apr_brigade_puts(output_bb, NULL,NULL,"\t}\n}\n");
		return 0;
}


int run_get_method(apr_pool_t* req_pool,apr_pool_t* global_pool, apr_bucket_brigade** output_bb, apr_table_t* output_headers, const char* output_content_type,app_list_t* apps,db_config* dbd_config, error_messages_t* error_messages, const char* uri){
	int error_num;
	apr_status_t rv;
	apr_pool_t* pool;

	apr_bucket_alloc_t* output_bucket_allocator;

	apr_bucket* eos_bucket;

	app_t* app;
	const char* error_header  = "Error With Query";

	rv = apr_pool_create(&pool, req_pool);

	output_bucket_allocator = apr_bucket_alloc_create(pool);

	*output_bb = apr_brigade_create(pool, output_bucket_allocator);

	if(dbd_config == NULL || dbd_config->connected != 1){
		add_error_list(error_messages, ERROR,error_header,"Database is not connected.");
		output_status_json(pool,*output_bb,output_headers, output_content_type,error_messages);
	}else if((error_num = app_process_uri(req_pool,uri, apps,&app)) != 0){
		add_error_list(error_messages, ERROR,error_header,"No app found!");
		output_status_json(pool,*output_bb,output_headers, output_content_type,error_messages);
	}else{
		app->run_query(req_pool, global_pool, *output_bb,output_headers, output_content_type, error_messages,dbd_config, &(app->query_words),app->db_queries, app->global_context);
	}

	eos_bucket = apr_bucket_eos_create(output_bucket_allocator);
	APR_BRIGADE_INSERT_TAIL(*output_bb, eos_bucket);



	//output_status_json(r);
	return OK;
}

static int run_post_method(request_rec* r){
	//output_status_json(r);
	return OK;
}

static int mediaplayer_handler(request_rec* r) {
	apr_status_t rv;
	apr_table_t* output_headers;
	const char* output_content_type;
	mediaplayer_rec_cfg* rec_cfg;
	mediaplayer_srv_cfg* srv_conf;
	apr_off_t output_length;

	apr_bucket_brigade* output_bb;


	rec_cfg = apr_pcalloc(r->pool, sizeof(mediaplayer_rec_cfg));
	srv_conf = ap_get_module_config(r->server->module_config, &mediaplayer_module);

	if(srv_conf->enable != 1){
		return DECLINED;
	}
	if(srv_conf->error_messages == NULL){
		//Error no error messages
		return DECLINED;
	}

	copy_error_messages(&(rec_cfg->error_messages), srv_conf->error_messages, r->pool);
	ap_set_module_config(r->request_config, &mediaplayer_module, rec_cfg) ;

	//Prepare Output
	output_headers = apr_table_make(r->pool, 10);
	output_content_type = apr_pcalloc(r->pool, sizeof(char) * 256);

	if ( r->method_number == M_GET){
		run_get_method(r->pool,r->server->process->pool, &output_bb, output_headers, output_content_type,srv_conf->apps, srv_conf->dbd_config,rec_cfg->error_messages,r->uri);
		apr_table_overlap(r->headers_out, output_headers,APR_OVERLAP_TABLES_SET);
		ap_set_content_type(r, output_content_type);
		apr_brigade_length(output_bb,1,&output_length);
		ap_set_content_length(r, output_length);
		rv = ap_pass_brigade(r->output_filters, output_bb);
		return OK;
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
