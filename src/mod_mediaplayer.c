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
				  add_error_list(dir_sync->error_messages, "Failed to sync song:",  apr_psprintf(dir_sync->pool, "Song title: %s Song artist: %s song album:%s song file path: %s",song->title, song->artist, song->album, song->file_path));
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
			error_messages->error_table = apr_table_make(error_messages->pool, 256);

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

int get_music_query(request_rec* r,music_query* music){
	char* query_nouns[4];
	int i = 0;
	char* uri_cpy = apr_pstrdup(r->pool, r->uri);
	char* uri_slash;

	//Remove leading slash and add trailing slash if one doesn't exsits.
	uri_cpy++;//Remove leading slash
	if (uri_cpy[strlen(uri_cpy) - 1] != '/'){
		uri_cpy = apr_pstrcat(r->pool, uri_cpy, "/", NULL);
	}
	while ((uri_slash= strchr(uri_cpy, '/')) != NULL && i <=4){
		 uri_slash[0] = '\0';
		 query_nouns[i] = uri_cpy;
		 uri_cpy = ++uri_slash;
		 i++;
	}
	if (apr_strnatcasecmp(query_nouns[1], "songs") == 0){
		music->types = SONGS;
	}else
	if (apr_strnatcasecmp(query_nouns[1], "albums") == 0){
		music->types = ALBUMS;
	}else
	if (apr_strnatcasecmp(query_nouns[1], "artists") == 0){
		music->types = ARTISTS;
	}else{
		return -2;
		//Invlaid type
	}

	if (apr_strnatcasecmp(query_nouns[2], "+titles") == 0){
		music->sort_by = ASC_TITLES;
	}else
	if (apr_strnatcasecmp(query_nouns[2], "+albums") == 0){
		music->sort_by = ASC_ALBUMS;
	}else
	if (apr_strnatcasecmp(query_nouns[2], "+artists") == 0){
		music->sort_by = ASC_ARTISTS;
	}else{
		return -3;
		//Invlaid sort
	}

	if((music->range_upper = strchr(query_nouns[3], '-')) != NULL) {
		music->range_upper[0] = '\0';
		music->range_upper++;
		music->range_lower = &(query_nouns[3][0]);
	}else{
		return -4;
		//Invalid range
	}


	return 0;
}

static int run_music_query(request_rec* r, music_query* music){
	int error_num;
	//We should check if database is connected somewhere
	mediaplayer_srv_cfg* srv_conf = ap_get_module_config(r->server->module_config, &mediaplayer_module) ;
	error_num = select_db_range(srv_conf->dbd_config, srv_conf->dbd_config->statements.select_songs_range[music->sort_by], music->range_lower, music->range_upper, &(music->results));

	return error_num;
}

char* json_escape_char(apr_pool_t* pool, const char* string){
	int i;
	char* temp_string = apr_pstrdup(pool, string);
	char* escaped_string = NULL;

	for (i = 0; i < strlen(temp_string); i++){
		if (temp_string[i] == '"'){
			temp_string[i] = '\0';
			escaped_string = apr_pstrcat(pool, string[0], "	&quot;", string[i+1], NULL);
		}
	}
	if (escaped_string == NULL){
		return temp_string;
	}else{
		return escaped_string;
	}
}
static int print_error_json(void *rec, const char *key, const char *value){
	request_rec* r = (request_rec*) rec;
	mediaplayer_rec_cfg* rec_cfg = ap_get_module_config(r->request_config, &mediaplayer_module);

	ap_rprintf(r, "%s", value);
	if (atoi(key) == (rec_cfg->error_messages->num_errors - 1)){
		ap_rprintf(r, "\n");
	}else{
		ap_rprintf(r, ",\n");
	}
	return 10;
}

static int print_songs_json(void *rec, const char *key, const char *value){
	request_rec* r = (request_rec*) rec;
	mediaplayer_rec_cfg* rec_cfg = ap_get_module_config(r->request_config, &mediaplayer_module);

	ap_rprintf(r, "{\n%s\n }",  value);
	if (atoi(key) == (rec_cfg->query->results->row_count - 1)){
		ap_rprintf(r, "\n");
	}else{
		ap_rprintf(r, ",\n");
	}
	return 10;
}

int output_json(request_rec* r){
	mediaplayer_srv_cfg* srv_conf = ap_get_module_config(r->server->module_config, &mediaplayer_module) ;
	dir_sync_t* dir_sync;
	dir_sync = apr_shm_baseaddr_get(srv_conf->dir_sync_shm);

	int i;
	mediaplayer_rec_cfg* rec_cfg = ap_get_module_config(r->request_config, &mediaplayer_module);

	ap_set_content_type(r, "application/json") ;
	ap_rputs("{\n\t\"status\" : {", r);
		ap_rprintf(r, "\t\"Progress\" :  \"%.2f\",\n", dir_sync->sync_progress);

		ap_rputs("\"Errors\" : {", r);
		if(!apr_is_empty_table(rec_cfg->error_messages->error_table)){
			apr_table_do(print_error_json, r, rec_cfg->error_messages->error_table, NULL);
		}
		ap_rputs("\t}\n", r);
	ap_rputs("\t}\n", r);
	ap_rputs("\"songs\" : [", r);
	if(!apr_is_empty_table(rec_cfg->query->results->results)){
		apr_table_do(print_songs_json, r, rec_cfg->query->results->results, NULL);
	}
	ap_rputs(	"]}",r);
	return 0;
}

static int run_get_method(request_rec* r){
	int error_num;

	mediaplayer_rec_cfg* rec_cfg = ap_get_module_config(r->request_config, &mediaplayer_module);
	rec_cfg->query = apr_pcalloc(r->pool, sizeof(music_query));

	error_num = get_music_query(r, rec_cfg->query);
	if (error_num != 0){
		add_error_list(rec_cfg->error_messages, "Error with query", "blah");
		return OK;
	}

	run_music_query(r, rec_cfg->query);

	if (error_num != 0){
		add_error_list(rec_cfg->error_messages, "Error running query", "blah");
			return OK;
	}



	output_json(r);

	return OK;
}

static int run_post_method(request_rec* r){

	return OK;
}

static int mediaplayer_handler(request_rec* r) {
	mediaplayer_srv_cfg* srv_conf = ap_get_module_config(r->server->module_config, &mediaplayer_module) ;
	error_messages_t* error_messages = apr_shm_baseaddr_get(srv_conf->errors_shm);
	mediaplayer_rec_cfg* rec_cfg = apr_pcalloc(r->pool, sizeof(mediaplayer_rec_cfg));

	//Copy error messages from shared memory
	rec_cfg->error_messages =apr_pcalloc(r->pool, sizeof(error_messages_t));
	rec_cfg->error_messages->error_table = apr_table_clone(r->pool, error_messages->error_table);
	rec_cfg->error_messages->num_errors = error_messages->num_errors;
	ap_set_module_config(r->request_config, &mediaplayer_module, rec_cfg) ;



	if(srv_conf->enable != 1){
		return DECLINED;
	}
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
