/*
 * dbd.h

 *
 *  Created on: Sep 20, 2012
 *      Author: Reyad Attiyat
 */


#ifndef DBD_H_
#define DBD_H_
#include <httpd.h>
#include <http_protocol.h>
#include <http_config.h>
#include "apr_file_io.h"
#include "apr_file_info.h"
#include "apr_errno.h"
#include "apr_general.h"
#include "apr_lib.h"
#include "apr_strings.h"
#include "apr_dbd.h"
#include "tag_reader.h"
#include "error_handler.h"

typedef struct {
	apr_dbd_t *dbd_handle;
	const apr_dbd_driver_t *dbd_driver;
	const char* driver_name;
	const char* mysql_parms;
	apr_dbd_transaction_t * transaction;
	struct {
		apr_dbd_prepared_t* select_last_id;
		apr_dbd_prepared_t *add_song;
		apr_dbd_prepared_t *add_artist;
		apr_dbd_prepared_t *add_album;
		apr_dbd_prepared_t *select_song;
		apr_dbd_prepared_t *select_artist;
		apr_dbd_prepared_t *select_album;
		apr_dbd_prepared_t *add_link;
		apr_dbd_prepared_t* select_file_path;
		apr_dbd_prepared_t* update_song;
	}statements;

}db_config;

apr_status_t connect_database(apr_pool_t* pool, db_config** dbd_config);
int prepare_database(db_config* dbd_config, apr_pool_t* pool);
int sync_song(apr_pool_t * pool, db_config* dbd_config, music_file *song, apr_time_t file_mtime, error_messages_t* error_messages);

#endif /* DBD_H_ */
