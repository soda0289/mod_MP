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

static void* mediaplayer_config_dir(apr_pool_t* pool, char* x) {
  return apr_pcalloc(pool, sizeof(mediaplayer_dir_cfg)) ;
}

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

	List* file_list;

	music_file* song;

	dir_sync_t* dir_sync = (dir_sync_t*) ptr;

	file_list = apr_pcalloc(dir_sync->pool, sizeof(List));
	dir_sync->num_files = apr_pcalloc(dir_sync->pool, sizeof(int));

	rv = connect_database(dir_sync->pool, &(dir_sync->srv_conf->dbd_config));
	if(rv != APR_SUCCESS){
		apr_strerror(rv, &error_message, (apr_size_t) 255);
		add_error_list(dir_sync->error_messages, "Database error couldn't connect", &error_message);
		return 0;
	}
	status = prepare_database(dir_sync->srv_conf->dbd_config, dir_sync->pool);
	if(status != 0){
		add_error_list(dir_sync->error_messages, "Database error couldn't prepare",apr_dbd_error(dir_sync->srv_conf->dbd_config->dbd_driver,dir_sync->srv_conf-> dbd_config->dbd_handle, status));
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

static int mediaplayer_handler(request_rec* r) {
	dir_sync_t* dir_sync;

  if ( !r->handler || strcmp(r->handler, "mediaplayer") ) {
    return DECLINED ;   /* none of our business */
  }
  if ( r->method_number != M_GET &&  r->method_number != M_POST) {
    return HTTP_METHOD_NOT_ALLOWED ;  /* Reject other methods */
  }

  mediaplayer_srv_cfg* srv_conf = ap_get_module_config(r->server->module_config, &mediaplayer_module) ;

  dir_sync = apr_shm_baseaddr_get(srv_conf->dir_sync_shm);
  error_messages_t* error_messages = apr_shm_baseaddr_get(srv_conf->errors_shm);


  ap_set_content_type(r, "text/html;charset=ascii") ;
  ap_rputs(
	"<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\">\n", r) ;
  ap_rputs(
	"<html><head><title>Reyad's Media Player</title></head>", r) ;
  ap_rputs("<body>", r) ;
  ap_rprintf(r, "Progress %f\n<br \>", dir_sync->sync_progress);
  int i;
  for( i = 0;error_messages->num_errors > i; i++){
	  ap_rprintf(r, "ERROR: %s\n<br />", error_messages->errors[i]);
  }

  ap_rprintf(r, "URI: %s %s<br />", r->unparsed_uri, r->uri);
  ap_rprintf(r, "Filename: %s", r->filename);
  ap_rputs("</body></html>", r) ;

  return OK ;
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
	//ap_hook_child_init(mediaplayer_child_init, NULL, NULL, APR_HOOK_MIDDLE);
	ap_hook_handler(mediaplayer_handler, NULL, NULL, APR_HOOK_MIDDLE) ;
}

module AP_MODULE_DECLARE_DATA mediaplayer_module = {
        STANDARD20_MODULE_STUFF,
        mediaplayer_config_dir,
        NULL,
        mediaplayer_config_srv,
        NULL,
        mediaplayer_cmds,
        mediaplayer_hooks
} ;
