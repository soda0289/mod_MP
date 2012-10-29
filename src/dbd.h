/*
 * dbd.h

 *
 *  Created on: Sep 20, 2012
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
 * See the License for the specific language governing permissions and
*  limitations under the License.
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
	apr_pool_t* pool;
	apr_dbd_t *dbd_handle;
	const apr_dbd_driver_t *dbd_driver;
	const char* driver_name;
	const char* mysql_parms;
	apr_dbd_transaction_t * transaction;
	int connected;
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
		apr_dbd_prepared_t* select_songs_range[4];
	}statements;

}db_config;

typedef struct{
	int row_count;
	apr_table_t* results;
}results_table_t;


apr_status_t connect_database(apr_pool_t* pool, db_config** dbd_config);
int prepare_database(db_config* dbd_config);
int sync_song(db_config* dbd_config, music_file *song, apr_time_t file_mtime, error_messages_t* error_messages);
int select_db_range(db_config* dbd_config, apr_dbd_prepared_t* select_statment,  char* range_lower, char* range_upper,results_table_t**  results_table);

#endif /* DBD_H_ */
