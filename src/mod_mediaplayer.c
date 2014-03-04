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
#include <http_connection.h>
#include <http_protocol.h>
#include <http_config.h>
#include <http_request.h>

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

#include "database/db_config.h"

#include "upload.h"
unixd_config_rec ap_unixd_config;



static void* mediaplayer_config_srv(apr_pool_t* pool, server_rec* s){
	mediaplayer_srv_cfg* srv_conf = apr_pcalloc(pool, sizeof(mediaplayer_srv_cfg));

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

static const char* mediaplayer_set_db_config_dir (cmd_parms* cmd, void* cfg, const char* arg){

	mediaplayer_srv_cfg* srv_conf = ap_get_module_config(cmd->server->module_config, &mediaplayer_module);

	srv_conf->db_xml_dir = arg;
	return NULL;
}

static const char* mediaplayer_set_app_config_dir (cmd_parms* cmd, void* cfg, const char* arg){

	mediaplayer_srv_cfg* srv_conf = ap_get_module_config(cmd->server->module_config, &mediaplayer_module);

	srv_conf->apps_xml_dir = arg;
	return NULL;
}

int setup_shared_memory(apr_shm_t** shm,apr_size_t size,const char* file_path, apr_pool_t* pool){
	apr_status_t rv;
    apr_uid_t uid = 0;
    apr_gid_t gid = 0;
    key_t shm_key;
    int shm_id;
    struct shmid_ds buf;
    time_t t;

    int status;


    //apr_uid_get(&uid,&gid,"apache", pool);

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
	shm_id = shmget(shm_key,size,IPC_CREAT | 0666);

	status = shmctl(shm_id, IPC_STAT, &buf);
	if(status != 0){
		return -2;
	}
	time(&t);
	buf.shm_ctime = t;
	buf.shm_perm.gid = ap_unixd_config.group_id;
	buf.shm_perm.uid = ap_unixd_config.user_id;
	buf.shm_perm.mode = 0666;
	status = shmctl(shm_id, IPC_SET, &buf);
	if(status != 0){
		return -3;
	}

	status = chmod(file_path, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
	status = chown(file_path,uid,gid);

	return 0;
}


int init_output(apr_pool_t* pool, ap_filter_t* out_filters,output_t** output_ptr){
	output_t* output = *output_ptr = apr_pcalloc(pool, sizeof(output_t));

	//Prepare Output
	output->pool = pool;
	output->filters = out_filters;
	output->headers = apr_table_make(pool, 10);
	output->content_type = apr_pcalloc(pool, sizeof(char) * 255);
	output->bucket_allocator = apr_bucket_alloc_create(pool);

	output->error_messages = apr_pcalloc(pool, sizeof(error_messages_t));
	output->error_messages->num_errors = 0;

	output->bucket_brigade = apr_brigade_create(pool, output->bucket_allocator);
	output->length = 0;
	
	return 0;
}

int finalize_output(output_t* output, apr_table_t* out_headers){
	apr_status_t rv;

	apr_bucket* eos_bucket;
	eos_bucket = apr_bucket_eos_create(output->bucket_allocator);

	APR_BRIGADE_INSERT_TAIL(output->bucket_brigade, eos_bucket);

	apr_table_overlap(out_headers, output->headers, APR_OVERLAP_TABLES_SET);

	rv = apr_brigade_length(output->bucket_brigade,1,&(output->length));

	return rv;
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
			continue;
		}
		if (escape_string[i] == '\\'){
			escape_string[i] = '\0';
			escape_string = apr_pstrcat(pool, &escape_string[0], "\\\\", &escape_string[++i], NULL);
			continue;
		}
	}

	return escape_string;
}

int output_status_json(output_t* output){
	apr_table_add(output->headers,"Access-Control-Allow-Origin", "*");
	apr_cpystrn((char*)output->content_type, "application/json", 255);

	apr_brigade_puts(output->bucket_brigade, NULL,NULL, "{\n");

	print_error_messages(output->pool,output->bucket_brigade, output->error_messages);

	apr_brigade_puts(output->bucket_brigade, NULL,NULL,"\n}\n");

	return 0;
}

int dump_error_messages_to_ap_log(error_messages_t* error_messages, server_rec* server){
	int i = 0;
	
	for (i = 0;i < error_messages->num_errors; i++){
		char* err_header = error_messages->messages[i].header;
		char* err_message = error_messages->messages[i].message;
		ap_log_error(APLOG_MARK, APLOG_ERR, 0, server, "%s: %s", err_header, err_message);
	}

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
		if(srv_conf->enable){
			apr_status_t rv = 0;
			int status = 0;
			const char* music_file_path;

			srv_conf->pid = getpid();


			status = init_error_messages(pconf,&(srv_conf->error_messages), srv_conf->errors_shm_file);
			if(status != APR_SUCCESS){
				ap_log_error(APLOG_MARK, APLOG_ERR, status, s, "Failed to initalize Error Messages.");
				srv_conf->enable = 0;
				continue;
			}

			//Init Database	
			rv = apr_dbd_init(pconf);
			if (rv != APR_SUCCESS){
				ap_log_error(APLOG_MARK, APLOG_ERR, status, s, "Failed to initalize APR DBD.");
			  //Run error function
			  return rv;
			}

			//Setup Database Parameters from XML files 
			status = init_db_array(pconf,srv_conf->db_xml_dir, &(srv_conf->db_array), srv_conf->error_messages);
			if(status != APR_SUCCESS){
				dump_error_messages_to_ap_log(srv_conf->error_messages, s);
				srv_conf->enable = 0;
				continue;
			}

			//Init app manager
			status = init_app_manager(pconf, srv_conf->error_messages, &srv_conf->apps);	
			if(status != APR_SUCCESS){
				dump_error_messages_to_ap_log(srv_conf->error_messages, s);
				srv_conf->enable = 0;
				continue;
			}

			//Config the music app
			music_file_path = apr_pstrcat(pconf, srv_conf->apps_xml_dir, "/music.xml", NULL);
			config_app(srv_conf->apps,"music",music_file_path,init_music_query,reattach_music_query,run_music_query);

			status = init_apps(srv_conf->apps, srv_conf->db_array);
			if(status != APR_SUCCESS){
				dump_error_messages_to_ap_log(srv_conf->error_messages, s);
				srv_conf->enable = 0;
				continue;
			}
		}
	}while ((s = s->next) != NULL);

	return OK;
}



static void mediaplayer_child_init(apr_pool_t *child_pool, server_rec *s){

	mediaplayer_srv_cfg* srv_conf;

	//Scan through every server and determine which one has directories to be synchronized
	do{
		srv_conf = ap_get_module_config(s->module_config, &mediaplayer_module);
		if(srv_conf->enable){

			//Reattach shared memory
			if(getpid() != srv_conf->pid){
				//Only when in new proccess do we re attach shared memory
				//If we are in the same proccess we dont need shared memory do we.
				
				//Reattach shared memory
				reattach_error_messages(child_pool,&(srv_conf->error_messages), srv_conf->errors_shm_file);

				//Were in a new process reattach apps 
				reattach_apps(srv_conf->apps,child_pool,srv_conf->error_messages);

			}else{
				//WE ARE IN DEBUG MODE
				//THERE WILL BE NO NEW PROCCESS


			}
		}
	}while ((s = s->next) != NULL);

}

int mediaplayer_pre_connection (conn_rec *c, void *csd){
	//ap_add_input_filter("upload-filter", NULL, NULL, c);
	return 0;

}

int mediaplayer_fixups(request_rec* r){
	int status = 0;
	
	input_t* input;

	init_input(r->pool, r->uri, r->method_number, &input);

	ap_set_module_config(r->request_config, &mediaplayer_module, input);

	ap_add_input_filter("upload-filter", NULL, r, r->connection);


	return status;
}
//This is handled by the app
/*
int run_get_method(apr_bucket_brigade** output_bb, apr_table_t* output_headers, const char* output_content_type,app_list_t* apps,db_config_t* dbd_config, error_messages_t* error_messages, const char* uri){
	int error_num;
	apr_status_t rv;
	apr_pool_t* pool;


	return OK;
}

static int run_post_method(request_rec* r){
	//output_status_json(r);
	return OK;
}
*/


static int mediaplayer_handler(request_rec* r) {
	int status = 0;	
	mediaplayer_srv_cfg* srv_conf;

	app_t* app = NULL;
	const char* error_header  = "Error With Query";
	
	input_t* input = ap_get_module_config(r->request_config, &mediaplayer_module);
	output_t* output;


	srv_conf = ap_get_module_config(r->server->module_config, &mediaplayer_module);

	//Check if we got enough set
	if(srv_conf->enable != 1 || srv_conf->error_messages == NULL){
		//Error no error messages
		return DECLINED;
	}
	
	if(input == NULL){
		init_input(r->pool, r->uri, r->method_number, &input);
	}
	//Prepare struct that are passed to app query handler
	init_output(r->pool, r->output_filters, &output);
	
	//Copy server initalition error messages
	copy_error_messages(output->error_messages, srv_conf->error_messages, r->pool);

	//Procces input for request
	status = app_process_uri(input, srv_conf->apps,&app);
	if(status != 0){
		add_error_list(output->error_messages, ERROR,error_header,"No app found!");
		output_status_json(output);
	}else if(status == 0 && app != NULL){
		app->run_query(input, output, app->global_context);
	}

	//Prepare and send output
	status = finalize_output(output, r->headers_out);
	if(status != 0){
		return DECLINED;
	}
	ap_set_content_length(r, output->length);
	ap_pass_brigade(output->filters, output->bucket_brigade);

  return OK;
}

/*Define Configuration file parameters*/
static const command_rec mediaplayer_cmds[] = {
	AP_INIT_FLAG("Mediaplayer", mediaplayer_set_enable, NULL, RSRC_CONF, "Enable mod_mediaplayer on server(On/Off)"),
	AP_INIT_TAKE1("Music_Dir", mediaplayer_set_external_dir, NULL, RSRC_CONF, "Directory containing music files") ,
	AP_INIT_TAKE1("DB_Config_Dir", mediaplayer_set_db_config_dir, NULL, RSRC_CONF, "The Directory of the database config xml file(s)") ,
	AP_INIT_TAKE1("App_Config_Dir", mediaplayer_set_app_config_dir, NULL, RSRC_CONF, "The directory of app config xml files") ,
	{ NULL }
};

/* Hook our handler into Apache at startup */
static void mediaplayer_hooks(apr_pool_t* pool) {
	//Filters
	ap_register_input_filter("upload-filter", upload_filter, NULL, AP_FTYPE_RESOURCE);

	ap_hook_fixups(mediaplayer_fixups, NULL, NULL, APR_HOOK_MIDDLE);
	ap_hook_post_config(mediaplayer_post_config, NULL, NULL, APR_HOOK_MIDDLE);
	ap_hook_child_init(mediaplayer_child_init, NULL, NULL, APR_HOOK_MIDDLE);
	ap_hook_handler(mediaplayer_handler, NULL, NULL, APR_HOOK_MIDDLE) ;
}

/*Define apache module*/
module AP_MODULE_DECLARE_DATA mediaplayer_module = {
	STANDARD20_MODULE_STUFF,
	NULL,
	NULL,
	mediaplayer_config_srv,
	NULL,
	mediaplayer_cmds,
	mediaplayer_hooks
};
