/*
 * error_handler.c
 *
 *  Created on: Sep 26, 2012
 *      Author: Reyad Attiyat
 */

#include <httpd.h>
#include "dbd.h"
#include "error_handler.h"

module AP_MODULE_DECLARE_DATA mediaplayer_module;

typedef struct {
	int enable;
	const char* external_directory;
	db_config* dbd_config;
	apr_shm_t* dir_sync_shm;
	apr_shm_t* errors_shm;
} mediaplayer_srv_cfg ;


typedef struct {
	int* num_files;
	apr_pool_t * pool;
	float sync_progress;
	mediaplayer_srv_cfg* srv_conf;
	error_messages_t* error_messages;
}dir_sync_t;

enum verbs{
	SONGS = 0,
	ALBUMS = 1,
	ARTIST = 2
};
