/*
 * mod_mp.c
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
#include "mod_mp.h"
#include "error_messages.h"
#include <util_filter.h>
#include <http_log.h>
#include "unixd.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <time.h>

#include "indexers/indexer.h"
#include "indexers/index_manager.h"

#include "indexers/file/file_indexer.h"

#include "unixd.h"

#include "database/db_config.h"

#include "upload.h"
unixd_config_rec ap_unixd_config;



static void* mp_config_srv(apr_pool_t* pool, server_rec* s){
	mp_srv_cfg* srv_conf = apr_pcalloc(pool, sizeof(mp_srv_cfg));

	srv_conf->errors_shm_file = "/tmp/mp_errors";
	srv_conf->server_rec = s;

	return srv_conf;
}
static const char* mp_set_enable (cmd_parms* cmd, void* cfg, int arg){

	mp_srv_cfg* srv_conf = ap_get_module_config(cmd->server->module_config, &mp_module);

	srv_conf->enable = arg;
	return NULL;
}

static const char* mp_set_db_xml_config_dir (cmd_parms* cmd, void* cfg, const char* arg){

	mp_srv_cfg* srv_conf = ap_get_module_config(cmd->server->module_config, &mp_module);

	srv_conf->db_xml_config_dir = arg;
	return NULL;
}

static const char* mp_set_indexers_xml_config_dir (cmd_parms* cmd, void* cfg, const char* arg){

	mp_srv_cfg* srv_conf = ap_get_module_config(cmd->server->module_config, &mp_module);

	srv_conf->indexer_xml_config_dir = arg;
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




static int init_mod_mp (mp_srv_cfg *srv_conf, apr_pool_t* pool) {
	int status = 0;
	indexer_t* file_indexer;
	const char* file_indexer_xml_config;

	srv_conf->pid = getpid();

	//Initialize error messages
	status = error_messages_init_shared(pool, srv_conf->errors_shm_file, &(srv_conf->error_messages));
	if(status != APR_SUCCESS){
		ap_log_error(APLOG_MARK, APLOG_ERR, status, srv_conf->server_rec, "Failed to initialize Error Messages.");
		srv_conf->enable = 0;
		return -1;
	}

	//Setup Database Parameters from XML files 
	status = db_manager_init(pool, srv_conf->error_messages, srv_conf->db_xml_config_dir, &(srv_conf->db_manager));
	if(status != APR_SUCCESS){
		error_messages_to_ap_log(srv_conf->error_messages, srv_conf->server_rec);
		srv_conf->enable = 0;
		return -1;
	}

	//Initialize index manager
	status = index_manager_init(pool, srv_conf->error_messages, &(srv_conf->index_manager));	
	if(status != APR_SUCCESS){
		error_messages_to_ap_log(srv_conf->error_messages, srv_conf->server_rec);
		srv_conf->enable = 0;
		return -1;
	}

	//Core file and directory based indexer
	file_indexer_xml_config = apr_pstrcat(pool, srv_conf->indexer_xml_config_dir, "/file.xml", NULL);

	indexer_init(pool, srv_conf->error_messages, srv_conf->db_manager, "file", file_indexer_xml_config, &(file_indexer_callbacks), &file_indexer);
	index_manager_add(srv_conf->index_manager, file_indexer);


	/*
	//Configure the Music indexer
	indexer_t* music_indexer;
	music_xml_config = apr_pstrcat(pool, srv_conf->apps_xml_dir, "/music.xml", NULL);
	index_manager_new(srv_conf->index_manager, "music", music_indexer_xml_config,
				music_indexer_callbacks, &music_indexer);
	index_manager_add(srv_conf->index_manager, music_indexer); 
	*/


	return 0;
}

static int mp_post_config(apr_pool_t *pconf, apr_pool_t *plog, apr_pool_t* ptemp,server_rec *s){
	mp_srv_cfg* srv_conf;


	void *data = NULL;
	const char *userdata_key = "mp_post_config";

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
		srv_conf = ap_get_module_config(s->module_config, &mp_module);
		if(srv_conf->enable){
			init_mod_mp (srv_conf, pconf);
		}
	}while ((s = s->next) != NULL);

	return OK;
}

static void fork_mod_mp (apr_pool_t* child_pool, mp_srv_cfg* srv_conf) {
	//Only when in new proccess do we re attach shared memory
	//If we are in the same proccess we dont need shared memory do we.
	
	//Reattach shared memory
	error_messages_on_fork(&(srv_conf->error_messages), child_pool);

	//Reattach all the databases
	db_manager_on_fork(child_pool, srv_conf->error_messages, &(srv_conf->db_manager));

	//Were in a new process reattach apps 
	index_manager_on_fork(srv_conf->index_manager, child_pool, srv_conf->error_messages);

}


static void mp_child_init(apr_pool_t *child_pool, server_rec *s){

	mp_srv_cfg* srv_conf;

	//Scan through every server and determine which one has directories to be synchronized
	do{
		srv_conf = ap_get_module_config(s->module_config, &mp_module);
		if(srv_conf->enable){

			//Reattach shared memory
			if(getpid() != srv_conf->pid){
				fork_mod_mp(child_pool, srv_conf);
			}else{
				//WE ARE IN DEBUG MODE
				//THERE WILL BE NO NEW PROCCESS


			}
		}
	}while ((s = s->next) != NULL);

}

static int mp_fixups(request_rec* r){
	int status = 0;
	
	input_t* input;

	input_init(r->pool, r->uri, r->method_number, &input);

	ap_set_module_config(r->request_config, &mp_module, input);

	//ap_add_input_filter("upload-filter", NULL, r, r->connection);


	return status;
}

static int mp_handler(request_rec* r) {
	int status = 0;	
	mp_srv_cfg* srv_conf;

	indexer_t* indexer = NULL;
	const char* error_header  = "Error With Query";
	
	input_t* input = ap_get_module_config(r->request_config, &mp_module);
	output_t* output;


	srv_conf = ap_get_module_config(r->server->module_config, &mp_module);

	//Check if we got enough set
	if(srv_conf->enable != 1 || srv_conf->error_messages == NULL){
		//Error no error messages
		return DECLINED;
	}
	
	if(input == NULL){
		input_init(r->pool, r->uri, r->method_number, &input);
	}
	//Prepare struct that are passed to app query handler
	output_init_bb(r->pool, r->output_filters, &output);
	
	//Copy server initalition error messages
	error_messages_duplicate(output->error_messages, srv_conf->error_messages, r->pool);

	//Procces input for request
	status = index_manager_find_indexer (srv_conf->index_manager, input->indexer_string, &indexer);
	if(status != 0){
		error_messages_add(output->error_messages, ERROR,error_header,"No indexer found!");
		output_status_json(output);
	}else if(status == 0 && indexer != NULL){
		indexer->callbacks->indexer_query_cb(indexer, input, output);
	}


	//Prepare and send output
	status = output_finalize_bb(output, r->headers_out);
	if(status != 0){
		return DECLINED;
	}

	ap_set_content_type(r, output->content_type);
	ap_set_content_length(r, output->length);
	ap_pass_brigade(output->filters, output->bucket_brigade);

  return OK;
}

/*Define Configuration file parameters*/
static const command_rec mp_cmds[] = {
	AP_INIT_FLAG("MP_Server_Enabled", mp_set_enable, NULL, RSRC_CONF, "Enable mod_MP on server this server (On/Off)"),
	AP_INIT_TAKE1("DB_Config_Dir", mp_set_db_xml_config_dir, NULL, RSRC_CONF, "The Directory of the database configuration XML file(s)") ,
	AP_INIT_TAKE1("Indexer_Config_Dir", mp_set_indexers_xml_config_dir, NULL, RSRC_CONF, "The directory of the indexer configuration XML files") ,
	{ NULL }
};

/* Hook our handler into Apache at startup */
static void mp_hooks(apr_pool_t* pool) {
	//Filters
	//ap_register_input_filter("upload-filter", upload_filter, NULL, AP_FTYPE_RESOURCE);

	ap_hook_fixups(mp_fixups, NULL, NULL, APR_HOOK_MIDDLE);
	ap_hook_post_config(mp_post_config, NULL, NULL, APR_HOOK_MIDDLE);
	ap_hook_child_init(mp_child_init, NULL, NULL, APR_HOOK_MIDDLE);
	ap_hook_handler(mp_handler, NULL, NULL, APR_HOOK_MIDDLE) ;
}

/*Define apache module*/
module AP_MODULE_DECLARE_DATA mp_module = {
	STANDARD20_MODULE_STUFF,
	NULL,
	NULL,
	mp_config_srv,
	NULL,
	mp_cmds,
	mp_hooks
};
