/*
 * dbd.c
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
#include "apr_strings.h"
#include "apr_dbd.h"
#include "dbd.h"
#include "tag_reader.h"
#include "mod_mediaplayer.h"

apr_status_t connect_database(apr_pool_t* pool, db_config** dbd_config){
	apr_status_t rv;

	*dbd_config = apr_pcalloc(pool, sizeof(db_config));

	rv = apr_dbd_init(pool);
	if (rv != APR_SUCCESS){
	  //Run error function
	  return rv;
	}
	(*dbd_config)->driver_name = "mysql";
	(*dbd_config)->mysql_parms = "host=127.0.0.1,user=root";
	(*dbd_config)->pool = pool;

	rv = apr_dbd_get_driver(pool, (*dbd_config)->driver_name, &((*dbd_config)->dbd_driver));
	if (rv != APR_SUCCESS){
		//Run error function
		return rv;
	}

	rv = apr_dbd_open((*dbd_config)->dbd_driver, pool, (*dbd_config)->mysql_parms, &((*dbd_config)->dbd_handle));
	if (rv != APR_SUCCESS){
		//Run error function
		return rv;
	}
	return rv;
}

int prepare_database(db_config* dbd_config, apr_pool_t* pool){
	int error_num;

	error_num = apr_dbd_set_dbname(dbd_config->dbd_driver, pool, dbd_config->dbd_handle, "mediaplayer");
	if (error_num != 0){
		return error_num;
	}
	error_num = apr_dbd_prepare(dbd_config->dbd_driver, pool, dbd_config->dbd_handle, "SELECT LAST_INSERT_ID();",NULL, &(dbd_config->statements.select_last_id));
	if (error_num != 0){
		return error_num;
	}
	error_num = apr_dbd_prepare(dbd_config->dbd_driver, pool, dbd_config->dbd_handle, "SELECT id FROM Artists WHERE name=%s;",NULL, &(dbd_config->statements.select_artist));
	if (error_num != 0){
		return error_num;
	}
	error_num = apr_dbd_prepare(dbd_config->dbd_driver, pool, dbd_config->dbd_handle, "SELECT id FROM Albums WHERE name=%s;",NULL, &(dbd_config->statements.select_album));
	if (error_num != 0){
		return error_num;
	}
	error_num = apr_dbd_prepare(dbd_config->dbd_driver, pool, dbd_config->dbd_handle, "UPDATE Albums, Artists, Songs, links "
			"SET Albums.name = %s, Artists.name=%s, Songs.name=%s, Songs.length = %d, links.track_no = %d, links.disc_no = %d, Songs.mtime = %ld "
			"WHERE Songs.file_path = %s  AND links.artistid = Artists.id AND links.albumid = Albums.id AND links.songid = Songs.id;" ,NULL, &(dbd_config->statements.update_song));
	if (error_num != 0){
		return error_num;
	}
	error_num = apr_dbd_prepare(dbd_config->dbd_driver, pool, dbd_config->dbd_handle, "SELECT mtime FROM Songs WHERE file_path=%s", NULL, &(dbd_config->statements.select_file_path));
	if (error_num != 0){
		return error_num;
	}
	error_num = apr_dbd_prepare(dbd_config->dbd_driver, pool, dbd_config->dbd_handle, "INSERT INTO `Artists` (name) VALUE (%s)",NULL, &(dbd_config->statements.add_artist));
	if (error_num != 0){
		return error_num;
	}
	error_num = apr_dbd_prepare(dbd_config->dbd_driver, pool, dbd_config->dbd_handle, "INSERT INTO `Albums` (name) VALUE (%s);",NULL, &(dbd_config->statements.add_album));
	if (error_num != 0){
		return error_num;
	}
	error_num = apr_dbd_prepare(dbd_config->dbd_driver, pool, dbd_config->dbd_handle, "INSERT INTO `Songs` (`name`, `musicbrainz_id`, `file_path`, `length`,`mtime`, `play_count`) VALUE (%s, %s, %s, %d,%lld,%d);",NULL, &(dbd_config->statements.add_song));
	if (error_num != 0){
		return error_num;
	}
	error_num = apr_dbd_prepare(dbd_config->dbd_driver, pool, dbd_config->dbd_handle, "INSERT INTO `links` (artistid, albumid, songid, feature, track_no, disc_no) VALUES (%s, %s, %s, %s, %s, %s);",NULL, &(dbd_config->statements.add_link));
	if (error_num != 0){
		return error_num;
	}
	int i;
	const char* Sort_By_Table_Names[] = {"Songs.name", "Albums.name", "Artists.name"};
	for (i = 0; i < 3; i++){
		const char* statement_string = apr_pstrcat(dbd_config->pool, "SELECT Songs.file_path, Songs.name, Artists.name, Albums.name, Songs.length, links.track_no, links.disc_no FROM links LEFT JOIN Songs ON links.songid = Songs.id LEFT JOIN Artists ON links.artistid = Artists.id LEFT JOIN Albums ON links.albumid = Albums.id ORDER BY ", Sort_By_Table_Names[i]," LIMIT %d,%d;", NULL);
		error_num = apr_dbd_prepare(dbd_config->dbd_driver, pool, dbd_config->dbd_handle, statement_string,NULL, &(dbd_config->statements.select_songs_range[i]));
		if (error_num != 0){
			return error_num;
		}
	}


	return error_num;
}
static char* get_insert_last_id (apr_pool_t * pool, db_config* dbd_config){
	int no_errors = 0;
	int row_count;

	char* id = NULL;
	const char* error_message = NULL;

	apr_dbd_results_t* results = NULL;
	apr_dbd_row_t *row = NULL;

	//Get id of new title
		no_errors = apr_dbd_pvselect(dbd_config->dbd_driver, pool, dbd_config->dbd_handle,  &results, dbd_config->statements.select_last_id, 0);
		if (no_errors != 0){
			error_message = apr_dbd_error(dbd_config->dbd_driver, dbd_config->dbd_handle, no_errors);
			//ap_rputs(error_message,r);
		}
		//If it does cylce throgh all of them to clear cursor
		for (row_count = 0, no_errors = apr_dbd_get_row(dbd_config->dbd_driver, pool, results, &row, -1);
				no_errors != -1;
				row_count++,no_errors = apr_dbd_get_row(dbd_config->dbd_driver, pool, results, &row, -1)) {
				//only get first result
				if(row_count == 0){
					id = apr_pstrdup(pool,apr_dbd_get_entry (dbd_config->dbd_driver,  row, 0 ));
				}
		}
		return id;
}

int select_db_range(db_config* dbd_config, apr_dbd_prepared_t* select_statment,  char* range_lower, char* range_upper,results_table_t**  results_table){

	const char* Atributes[] = {"file_path", "title", "Artist", "Album", "length","track_no", "disc_no"};
	int error_num;

	apr_dbd_results_t* results = NULL;
	apr_dbd_row_t *row = NULL;

	*results_table = apr_pcalloc(dbd_config->pool, sizeof(results_table_t));

	error_num = apr_dbd_pvselect(dbd_config->dbd_driver, dbd_config->pool, dbd_config->dbd_handle,  &results, select_statment, 0, range_lower, range_upper);
	if (error_num != 0){
		return error_num;
	}
	(*results_table)->results = apr_table_make(dbd_config->pool, 6);
	//Cycle through all of them to clear cursor
	for ((*results_table)->row_count = 0, error_num = apr_dbd_get_row(dbd_config->dbd_driver, dbd_config->pool, results, &row, -1);
			error_num != -1;
			((*results_table)->row_count)++,error_num = apr_dbd_get_row(dbd_config->dbd_driver, dbd_config->pool, results, &row, -1)) {
			//only get first result
					const char* key =  apr_itoa(dbd_config->pool, (*results_table)->row_count);
					int i;
					for(i = 0; i < apr_dbd_num_cols(dbd_config->dbd_driver, results); i++){
						const char* value = apr_psprintf(dbd_config->pool, "\"%s\": \"%s\"", Atributes[i ],apr_dbd_get_entry (dbd_config->dbd_driver,  row, i));
						apr_table_merge((*results_table)->results, key, value);
					}
					//return error_num;
		}

	return 0;
}

static int insert_db_artist(char** id, apr_pool_t * pool, db_config* dbd_config, apr_dbd_prepared_t* query, music_file* song){
	int error_num = 0;
	int nrows = 1;

	//Create new title
	error_num = apr_dbd_pvquery(dbd_config->dbd_driver, pool, dbd_config->dbd_handle, &nrows, query, song->artist);
	if (error_num != 0){
		*id = get_insert_last_id(pool, dbd_config);
	}
	return error_num;
}

static int insert_db_album(char** id, apr_pool_t * pool, db_config* dbd_config, apr_dbd_prepared_t* query, music_file* song){
	int error_num = 0;
	int nrows = 1;

	//Create new title
	error_num = apr_dbd_pvquery(dbd_config->dbd_driver, pool, dbd_config->dbd_handle, &nrows, query, song->album);
	if (error_num != 0){
		*id = get_insert_last_id(pool, dbd_config);
	}
	return error_num;
}

static int insert_db_song(char** id, apr_pool_t * pool, db_config* dbd_config, apr_dbd_prepared_t* query, music_file* song, apr_time_t mtime){
	int error_num;
	int nrows = 1;
	//Create new title
	error_num = apr_dbd_pvquery(dbd_config->dbd_driver, pool, dbd_config->dbd_handle, &nrows, query, song->title, "aaa", song->file_path,apr_itoa(pool, song->length),  apr_ltoa(pool,mtime), apr_itoa(pool, 0));
	if (error_num == 0){
		*id = get_insert_last_id(pool, dbd_config);
	}
	return error_num;
}

static int insert_db_links(apr_pool_t * pool, db_config* dbd_config, apr_dbd_prepared_t* query, music_file* song, char* artist_id, char* album_id, char* song_id){
	int error_num = 0;
	int nrows = 1;
	int feature = 0;

	if (song->track_no == NULL){
		song->track_no = apr_itoa(pool, 0);
	}
	if (song->disc_no == NULL){
		song->disc_no = apr_itoa(pool, 0);
	}
	error_num = apr_dbd_pvquery(dbd_config->dbd_driver, pool, dbd_config->dbd_handle, &nrows, dbd_config->statements.add_link,artist_id,album_id,song_id,apr_itoa(pool,  feature),song->track_no,song->disc_no);
	return error_num;
}

static int update_song(apr_pool_t * pool, db_config* dbd_config, music_file* song, apr_time_t mtime){
	int error_num = 0;
	int nrows = 0;
	int feature = 0;

	error_num = apr_dbd_pvquery(dbd_config->dbd_driver, pool, dbd_config->dbd_handle, &nrows, dbd_config->statements.update_song,song->album,song->title, apr_itoa(pool,  song->length), pool, song->track_no, song->disc_no, apr_ltoa(pool,(long) mtime));
					if (error_num != 0){
						return error_num;
					}
	return 0;
}



static int get_id(char** id, apr_pool_t * pool, db_config* dbd_config, char* title, apr_dbd_prepared_t* select){
	int error_num = 0;
	int row_count;
	apr_dbd_results_t* results = NULL;
	apr_dbd_row_t *row = NULL;

	//*id = apr_pcalloc(pool, 255);
		error_num = apr_dbd_pvselect(dbd_config->dbd_driver, pool, dbd_config->dbd_handle,  &results, select, 0, title);
		if (error_num != 0){
			return error_num;
		}
		//Cylce throgh all of them to clear cursor
		for (row_count = 0, error_num = apr_dbd_get_row(dbd_config->dbd_driver, pool, results, &row, -1);
				error_num != -1;
				row_count++, error_num = apr_dbd_get_row(dbd_config->dbd_driver, pool, results, &row, -1)) {
				//only get first result
						*id = apr_pstrdup(pool,apr_dbd_get_entry (dbd_config->dbd_driver,  row, 0));
						return error_num ;
			}
			if (*id == NULL || strcmp(*id, "") == 0){
				return -2;
			}
	return error_num;
}

static int get_mtime(apr_time_t* mtime, apr_pool_t * pool, db_config* dbd_config, char* file_path, apr_dbd_prepared_t* select){
	int error_num = 0;
	int row_count;

	apr_dbd_results_t* results = NULL;
	apr_dbd_row_t *row = NULL;

	if (file_path != NULL){
		error_num = apr_dbd_pvselect(dbd_config->dbd_driver, pool, dbd_config->dbd_handle,  &results, select, 0, file_path);
		if (error_num != 0){
			return error_num;
		}
		//Cylce throgh all of them to clear cursor
		for (row_count = 0, error_num = apr_dbd_get_row(dbd_config->dbd_driver, pool, results, &row, -1);
				error_num != -1;
				row_count++,error_num = apr_dbd_get_row(dbd_config->dbd_driver, pool, results, &row, -1)) {
				//only get first result
						*mtime = (apr_time_t) apr_atoi64(apr_dbd_get_entry (dbd_config->dbd_driver,  row, 0));
						return error_num;
			}
			if (*mtime == 0 || row_count == 0){
				return -2;
			}
		}else{
			return -3;
		}
	return 0;
}

int sync_song(apr_pool_t * pool, db_config* dbd_config, music_file *song, apr_time_t file_mtime, error_messages_t* error_messages){
	char* artist_id = NULL;
	char* album_id = NULL;
	char* song_id = NULL;
	apr_time_t db_mtime = 0;
	int error_num = 0;
	const char* error_message = NULL;

	//Check if song file path already exsits in database
	error_num = get_mtime(&db_mtime,pool, dbd_config, song->file_path, dbd_config->statements.select_file_path);
	if (error_num == 0){
		if(file_mtime > db_mtime){
				//Update song
				error_num = update_song(pool, dbd_config, song, file_mtime);
				if (error_num == 0){
					return 0;
				}else{
					//Report error
					error_message = apr_dbd_error(dbd_config->dbd_driver, dbd_config->dbd_handle, error_num);
					//add_error_list(error_messages, apr_psprintf(pool,"DBD update  error(%d): %s %lld %lld", error_num,(char *) error_message, (long long int) file_mtime, (long long int)db_mtime), error_message);
					add_error_list(error_messages, apr_psprintf(pool,"DBD update  error(%d): %s", error_num,(char *) error_message), error_message);
					return 1;
				}
			}else if(file_mtime == db_mtime){
				//File is up to date
				return 0;
			}
	}else if(error_num == -2){
		//No file_path found so add new song
		error_num = insert_db_song(&song_id, pool, dbd_config, dbd_config->statements.add_song, song, file_mtime);
		if (error_num != 0){
			add_error_list(error_messages, "DBD insert_song error:", apr_dbd_error(dbd_config->dbd_driver, dbd_config->dbd_handle, error_num));
			return -1;
		}
		error_num = get_id(&artist_id, pool, dbd_config, song->artist, dbd_config->statements.select_artist);
		if (error_num  == -2){
			error_num = insert_db_artist(&artist_id, pool, dbd_config, dbd_config->statements.add_artist, song);
			if (error_num != 0){
				add_error_list(error_messages,   apr_psprintf(pool,"DBD insert artist error(%d):", error_num), apr_dbd_error(dbd_config->dbd_driver, dbd_config->dbd_handle, error_num));
				return -1;
			}
		}else if(error_num != 0){
			add_error_list(error_messages,  apr_psprintf(pool,"DBD get artist id error(%d):", error_num), apr_dbd_error(dbd_config->dbd_driver, dbd_config->dbd_handle, error_num));
			return -1;
		}
		error_num = get_id(&album_id, pool, dbd_config, song->album, dbd_config->statements.select_album);
		if(error_num == -2){
			error_num=  insert_db_album(&album_id , pool, dbd_config, dbd_config->statements.add_album, song);
			if (error_num != 0){
				add_error_list(error_messages, "DBD insert_album error:", apr_dbd_error(dbd_config->dbd_driver, dbd_config->dbd_handle, error_num));
				return -1;
			}
		}else if (error_num != 0){
			add_error_list(error_messages, "DBD album get id error:", apr_dbd_error(dbd_config->dbd_driver, dbd_config->dbd_handle, error_num));
			return -1;
		}

			if (artist_id != NULL && album_id != NULL && song_id != NULL){
				error_num = insert_db_links(pool, dbd_config, dbd_config->statements.add_link, song, artist_id, album_id, song_id);
				if (error_num != 0){
					//error_message = apr_dbd_error(dbd_config->dbd_driver, dbd_config->dbd_handle, error_num);
					add_error_list(error_messages, "DBD insert_song error:", apr_dbd_error(dbd_config->dbd_driver, dbd_config->dbd_handle, error_num));
					return -1;
				}
			}else{
				//add_error_list(error_messages, "DBD link error:", apr_psprintf(pool, "artist_id: %s album_id=%s song_id=%s", artist_id, album_id, song_id));
				return -1;
			}
	}else{
		add_error_list(error_messages, "DBD get mtime error", apr_psprintf(pool, "(%d) (%s)",error_num, song->file_path));
		return -1;
	}
	return 0;
}

