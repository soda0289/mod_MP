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

#include "mod_mediaplayer.h"
#include "music_query.h"
#define MAX_NUM_COLUMNS 10

typedef	struct {
		const char* columns;
		const char* table_dependcy;
		const char* group_by_columns;
}column_table_t;

typedef struct db_config_{
	apr_pool_t* pool;
	apr_dbd_t *dbd_handle;
	const apr_dbd_driver_t *dbd_driver;
	const char* driver_name;
	const char* mysql_parms;
	apr_dbd_transaction_t * transaction;
	int connected;
	error_messages_t* database_errors;
	struct {
		apr_dbd_prepared_t* select_last_id;
		apr_dbd_prepared_t *add_song;
		apr_dbd_prepared_t *add_artist;
		apr_dbd_prepared_t *add_album;
		apr_dbd_prepared_t* add_source;
		apr_dbd_prepared_t *select_song;
		apr_dbd_prepared_t *select_artist;
		apr_dbd_prepared_t *select_album;
		apr_dbd_prepared_t *select_sources;
		apr_dbd_prepared_t *add_link;
		apr_dbd_prepared_t* select_file_path;
		apr_dbd_prepared_t *select_mtime;
		apr_dbd_prepared_t* update_song;
		//[query_type][query_parameter_combinations]
		apr_dbd_prepared_t* select_by_range[NUM_QUERY_TYPES][(1 << NUM_QUERY_PARAMETERS) - 1];
		/*
		apr_dbd_prepared_t* select_songs_range[4];
		apr_dbd_prepared_t* select_songs_by_artist_id_range[4];
		apr_dbd_prepared_t* select_songs_by_album_id_range[4];
		apr_dbd_prepared_t* select_artists_range[2];
		apr_dbd_prepared_t* select_albums_range[2];
		apr_dbd_prepared_t* select_albums_by_artist_id_range[2];
		*/
	}statements;
	unsigned char num_column_dep[NUM_QUERY_TYPES];
	column_table_t column_table_dep[NUM_QUERY_TYPES][MAX_NUM_COLUMNS];


}db_config;

typedef struct results_table_t_ {
	int song_count;
	apr_table_t* song_results;
	apr_table_t* sources_results;
}results_table_t;


apr_status_t connect_database(apr_pool_t* pool, db_config** dbd_config);
int prepare_database(db_config* dbd_config);
int sync_song(db_config* dbd_config, music_file *song);
int select_db_range(db_config* dbd_config, music_query* query);
int get_file_path(char** file_path, db_config* dbd_config, char* id, apr_dbd_prepared_t* select);



#endif /* DBD_H_ */

