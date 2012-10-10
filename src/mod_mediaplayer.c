/*
 * mod_mediaplayer.c
 *
 *  Created on: Sep 26, 2012
 *      Author: Reyad Attiyat
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

static void* mediaplayer_config_srv(apr_pool_t* pool, server_rec* s){
	mediaplayer_srv_cfg* srv_conf = apr_pcalloc(pool, sizeof(mediaplayer_srv_cfg));

	//srv_conf->sync_progress = apr_pcalloc(s->process->pool, sizeof(int));
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
	char error_message[255];
	const char* dbd_error;

	List* file_list;

	music_file* song;

	dir_sync_t* dir_sync = (dir_sync_t*) ptr;

	file_list = apr_pcalloc(dir_sync->pool, sizeof(List));
	dir_sync->num_files = apr_pcalloc(dir_sync->pool, sizeof(int));

	rv = connect_database(dir_sync->pool, &(dir_sync->srv_conf->dbd_config));
	if(rv != APR_SUCCESS){
		apr_strerror(rv, (char*) &error_message[0], (apr_size_t) 255);
		add_error_list(dir_sync->error_messages, "Database error couldn't connect", &error_message[0]);
		return 0;
	}
	status = prepare_database(dir_sync->srv_conf->dbd_config, dir_sync->pool);
	if(status != 0){
		dbd_error = apr_dbd_error(dir_sync->srv_conf->dbd_config->dbd_driver,dir_sync->srv_conf-> dbd_config->dbd_handle, status);
		strncpy(&error_message[0], dbd_error, strlen(dbd_error));
		add_error_list(dir_sync->error_messages, "Database error couldn't prepare",&error_message[0]);
		return 0;
	}
	read_dir(dir_sync->pool, file_list, dir_sync->srv_conf->external_directory, dir_sync->num_files,  dir_sync->error_messages);

	if ((file_list == NULL || dir_sync->num_files == NULL )&& (*dir_sync->num_files ) > 0 ){
		add_error_list(dir_sync->error_messages, "Killing Sync Thread", "");
		return 0;
	}
	apr_dbd_transaction_start(dir_sync->srv_conf->dbd_config->dbd_driver, dir_sync->pool, dir_sync->srv_conf->dbd_config->dbd_handle,&dir_sync->srv_conf->dbd_config->transaction);
	while(file_list){
		  song = apr_pcalloc(dir_sync->pool, sizeof(music_file));
		  song->file_path = file_list->name;
		  status = read_flac_level1(dir_sync->pool, song);
		  if (status == 0&& song){
			  //Update or Insert song
			  status = sync_song(dir_sync->pool, dir_sync->srv_conf->dbd_config, song, file_list->mtime, dir_sync->error_messages);
			  if (status != 0){
				  add_error_list(dir_sync->error_messages, "Failed to sync song:",  apr_psprintf(dir_sync->pool, "Song title: %s <br />Song artist: %s<br />song album:%s <br />song file path: %s",song->title, song->artist, song->album, song->file_path));
			  }
		  }else{
			  //not a flac file
		  }
		  //Calculate the percent of files synchronized
		  dir_sync->sync_progress =(float) files_synced*100 / *(dir_sync->num_files);
		  files_synced++;
		  file_list = file_list->next;
	}
	apr_dbd_transaction_end(dir_sync->srv_conf->dbd_config->dbd_driver, dir_sync->pool, dir_sync->srv_conf->dbd_config->transaction);
	return 0;
}

static int mediaplayer_post_config(apr_pool_t *pconf, apr_pool_t *plog, apr_pool_t* ptemp,server_rec *s){
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
			rv = apr_shm_create(&srv_conf->dir_sync_shm, sizeof(dir_sync_t), NULL, s->process->pool);
			rv = apr_shm_create(&srv_conf->errors_shm, sizeof(error_messages_t), NULL, s->process->pool);
			//Create thread to connect to database and synchronize

			error_messages_t* error_messages = apr_shm_baseaddr_get(srv_conf->errors_shm);
			error_messages->pool = pconf;
			dir_sync_t*dir_sync = apr_shm_baseaddr_get(srv_conf->dir_sync_shm);

			dir_sync->pool = s->process->pool;
			dir_sync->srv_conf = srv_conf ;
			dir_sync->error_messages = error_messages;
			rv = apr_thread_create(&thread_sync_dir,NULL, sync_dir, (void*) dir_sync, s->process->pool);
		}
	}while ((s = s->next) != NULL);
	return OK;
}



static int mediaplayer_child_init(apr_pool_t *child_pool, server_rec *s){

	apr_status_t rv;
	mediaplayer_srv_cfg* srv_conf;

	//Scan through every server and determine which one has directories to be synchronized
	do{
		srv_conf = ap_get_module_config(s->module_config, &mediaplayer_module);
		if(srv_conf->enable && srv_conf->external_directory != NULL){


		}
	}while ((s = s->next) != NULL);
	return OK;
}
static int get_command_uri(request_rec* r){
	int i;
	char *strtok_last = NULL;
	char* filename;
	char* uri;
	char* filename_parts = apr_palloc(r->pool, sizeof(char*) * 25);
	char* uri_parts;
	const char slash = '/';
	//Reverse Search the file name string until first /. Compare that string to the start of uri to delete directory
	//used in directive configuration.
	//for(i =0, uri_parts[i] = apr_strtok(r->uri,&slash, &strtok_last);(uri_parts[i] != NULL && i <25); i++, uri_parts[i] = apr_strtok(NULL,&slash, &strtok_last));
	uri = apr_pcalloc(r->pool, strlen(r->uri));
	apr_cpystrn(uri, r->uri, strlen(r->uri));

	filename = apr_pcalloc(r->pool, strlen(r->filename));
	apr_cpystrn(filename, r->filename, strlen(r->filename));

	uri_parts = apr_pcalloc(r->pool, strlen(r->uri));


	return 0;
}
int static get_verb_noun(request_rec* r,char** verb, char** sortby, char** range){
	int i;
	int len = strlen(r->uri) +1;
	int noun_char=0;
	int noun_char2 = 0;
	char* uri_cpy = apr_pstrdup(r->pool, r->uri);

	//Break apart uri for each /
	for(i=1; i<= len; i++){
		if(uri_cpy[i] == '/' || uri_cpy[i] == '\0'){
			uri_cpy[i] = '\0';
			if(noun_char == 0){
				*verb = &uri_cpy[1];
				noun_char = i + 1;
			}else if(noun_char > 0 && noun_char2 == 0){
				*sortby = &uri_cpy[noun_char];
				noun_char2 = i +1;
			}else if(noun_char2 > 0){
				*range = &uri_cpy[noun_char2];
			}
		}
	}
	ap_rprintf(r, "verb: %s sortby: %s range: %s\n", *verb, *sortby, *range);
	if (apr_strnatcasecmp(*verb, "songs") == 0){
		return 0;
	}else
	if (apr_strnatcasecmp(*verb, "albums") == 0){
		return 1;
	}else
	if (apr_strnatcasecmp(*verb, "artists") == 0){
		return 2;
	}

	return -1;
}
static int run_get_method(request_rec* r){
	int i;
	char* verb = NULL;
	char* sortby = NULL;
	char* range = NULL;
	apr_table_t* results_table = NULL;
	dir_sync_t* dir_sync;
	mediaplayer_srv_cfg* srv_conf = ap_get_module_config(r->server->module_config, &mediaplayer_module) ;
	dir_sync = apr_shm_baseaddr_get(srv_conf->dir_sync_shm);
	error_messages_t* error_messages = apr_shm_baseaddr_get(srv_conf->errors_shm);

	ap_set_content_type(r, "application/json") ;
	ap_rputs("{\n", r);
	ap_rprintf(r, "\t\"Progress\" :  %.2f\n", dir_sync->sync_progress);
	ap_rputs("\t\"Errors\" : {\n", r);
	for( i = 0;error_messages->num_errors > i; i++){
	  ap_rprintf(r, "\t\t\"%s\"", error_messages->errors[i]);
	  if (i > 0){
		  ap_rputs(",",r);
	  }
	  ap_rputs("\n",r);
	}
	ap_rputs("\t}\n", r);


	switch(get_verb_noun(r, &verb, &sortby, &range)){
	case SONGS:
		select_db_range(srv_conf->dbd_config, srv_conf->dbd_config->statements.select_songs_range, sortby, range, &results_table);
		ap_rprintf(r, "verb: %s sortby: %s range: %s\n", verb, sortby, range);
		break;
	case ALBUMS:
		//query_db(srv_conf->dbd_config);
		break;
	case ARTIST:
		//query_db(srv_conf->dbd_config);
		break;
	}

	ap_rputs("}\n", r);

	return OK;
}

static int run_post_method(request_rec* r){

	return OK;
}

static int mediaplayer_handler(request_rec* r) {
	mediaplayer_srv_cfg* srv_conf = ap_get_module_config(r->server->module_config, &mediaplayer_module) ;

	if(srv_conf->enable != 1){
		return DECLINED;
	}
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
