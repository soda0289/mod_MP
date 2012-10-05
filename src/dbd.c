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

static int insert_db_artist(char** id, apr_pool_t * pool, db_config* dbd_config, apr_dbd_prepared_t* query, music_file* song){
	int num_errors = 0;
	int nrows = 1;

	const char* error_message = NULL;

	//Create new title
	num_errors = apr_dbd_pvquery(dbd_config->dbd_driver, pool, dbd_config->dbd_handle, &nrows, query, song->artist);
	if (num_errors != 0){
		*id = get_insert_last_id(pool, dbd_config);
	}
	return num_errors;
}

static int insert_db_album(char** id, apr_pool_t * pool, db_config* dbd_config, apr_dbd_prepared_t* query, music_file* song){
	int num_errors = 0;
	int nrows = 1;

	//Create new title
	num_errors = apr_dbd_pvquery(dbd_config->dbd_driver, pool, dbd_config->dbd_handle, &nrows, query, song->album);
	if (num_errors != 0){
		*id = get_insert_last_id(pool, dbd_config);
	}
	return num_errors;
}

static int insert_db_song(char** id, apr_pool_t * pool, db_config* dbd_config, apr_dbd_prepared_t* query, music_file* song, apr_time_t mtime){
	int num_errors;
	int nrows = 1;
	//Create new title
	num_errors = apr_dbd_pvquery(dbd_config->dbd_driver, pool, dbd_config->dbd_handle, &nrows, query, song->title, "aaa", song->file_path,apr_itoa(pool, song->length),  apr_ltoa(pool,mtime), apr_itoa(pool, 0));
	if (num_errors == 0){
		*id = get_insert_last_id(pool, dbd_config);
	}
	return num_errors;
}

static int insert_db_links(apr_pool_t * pool, db_config* dbd_config, apr_dbd_prepared_t* query, music_file* song, char* artist_id, char* album_id, char* song_id){
	int num_errors = 0;
	int nrows = 1;
	int feature = 0;

	if (song->track_no == NULL){
		song->track_no = apr_itoa(pool, 0);
	}
	if (song->disc_no == NULL){
		song->disc_no = apr_itoa(pool, 0);
	}
	num_errors = apr_dbd_pvquery(dbd_config->dbd_driver, pool, dbd_config->dbd_handle, &nrows, dbd_config->statements.add_link,artist_id,album_id,song_id,apr_itoa(pool,  feature),song->track_no,song->disc_no);
	return num_errors;
}

static int update_song(apr_pool_t * pool, db_config* dbd_config, music_file* song, apr_time_t mtime){
	int num_errors = 0;
	int nrows = 0;
	int feature = 0;

	num_errors = apr_dbd_pvquery(dbd_config->dbd_driver, pool, dbd_config->dbd_handle, &nrows, dbd_config->statements.update_song,song->album,song->title, apr_itoa(pool,  song->length), pool, song->track_no, song->disc_no, apr_ltoa(pool,(long) mtime));
					if (num_errors != 0){
						return num_errors;
					}
	return 0;
}



static int get_id(char** id, apr_pool_t * pool, db_config* dbd_config, char* title, apr_dbd_prepared_t* select){
	int num_errors = 0;
	int row_count;
	apr_dbd_results_t* results = NULL;
	apr_dbd_row_t *row = NULL;

	//*id = apr_pcalloc(pool, 255);
		num_errors = apr_dbd_pvselect(dbd_config->dbd_driver, pool, dbd_config->dbd_handle,  &results, select, 0, title);
		if (num_errors != 0){
			return num_errors;
		}
		//Cylce throgh all of them to clear cursor
		for (row_count = 0, num_errors = apr_dbd_get_row(dbd_config->dbd_driver, pool, results, &row, -1);
				num_errors != -1;
				row_count++, num_errors = apr_dbd_get_row(dbd_config->dbd_driver, pool, results, &row, -1)) {
				//only get first result
						*id = apr_pstrdup(pool,apr_dbd_get_entry (dbd_config->dbd_driver,  row, 0));
						return num_errors ;
			}
			if (*id == NULL || strcmp(*id, "") == 0){
				return -2;
			}
	return num_errors;
}

static int get_mtime(apr_time_t* mtime, apr_pool_t * pool, db_config* dbd_config, char* file_path, apr_dbd_prepared_t* select){
	int num_errors = 0;
	int row_count;

	apr_dbd_results_t* results = NULL;
	apr_dbd_row_t *row = NULL;

	if (file_path != NULL){
		num_errors = apr_dbd_pvselect(dbd_config->dbd_driver, pool, dbd_config->dbd_handle,  &results, select, 0, file_path);
		if (num_errors != 0){
			return num_errors;
		}
		//Cylce throgh all of them to clear cursor
		for (row_count = 0, num_errors = apr_dbd_get_row(dbd_config->dbd_driver, pool, results, &row, -1);
				num_errors != -1;
				row_count++,num_errors = apr_dbd_get_row(dbd_config->dbd_driver, pool, results, &row, -1)) {
				//only get first result
						*mtime = (apr_time_t) apr_atoi64(apr_dbd_get_entry (dbd_config->dbd_driver,  row, 0));
						return num_errors;
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
	int num_errors = 0;
	const char* error_message = NULL;

	//Check if song file path already exsits in database
	num_errors = get_mtime(&db_mtime,pool, dbd_config, song->file_path, dbd_config->statements.select_file_path);
	if (num_errors == 0){
		if(file_mtime > db_mtime){
				//Update song
				num_errors = update_song(pool, dbd_config, song, file_mtime);
				if (num_errors == 0){
					return 0;
				}else{
					//Report error
					error_message = apr_dbd_error(dbd_config->dbd_driver, dbd_config->dbd_handle, num_errors);
					//add_error_list(error_messages, apr_psprintf(pool,"DBD update  error(%d): %s %lld %lld", num_errors,(char *) error_message, (long long int) file_mtime, (long long int)db_mtime), error_message);
					add_error_list(error_messages, apr_psprintf(pool,"DBD update  error(%d): %s", num_errors,(char *) error_message), error_message);
					return 1;
				}
			}else if(file_mtime == db_mtime){
				//File is up to date
				return 0;
			}
	}else if(num_errors=-2){
		//No file_path found so add new song
		num_errors = insert_db_song(&song_id, pool, dbd_config, dbd_config->statements.add_song, song, file_mtime);
		if (num_errors != 0){
			add_error_list(error_messages, "DBD insert_song error:", apr_dbd_error(dbd_config->dbd_driver, dbd_config->dbd_handle, num_errors));
			return -1;
		}
		num_errors = get_id(&artist_id, pool, dbd_config, song->artist, dbd_config->statements.select_artist);
		if (num_errors  == -2){
			num_errors = insert_db_artist(&artist_id, pool, dbd_config, dbd_config->statements.add_artist, song);
			if (num_errors != 0){
				add_error_list(error_messages,   apr_psprintf(pool,"DBD insert artist error(%d):", num_errors), apr_dbd_error(dbd_config->dbd_driver, dbd_config->dbd_handle, num_errors));
				return -1;
			}
		}else if(num_errors != 0){
			add_error_list(error_messages,  apr_psprintf(pool,"DBD get artist id error(%d):", num_errors), apr_dbd_error(dbd_config->dbd_driver, dbd_config->dbd_handle, num_errors));
			return -1;
		}
		num_errors = get_id(&album_id, pool, dbd_config, song->album, dbd_config->statements.select_album);
		if(num_errors == -2){
			num_errors=  insert_db_album(&album_id , pool, dbd_config, dbd_config->statements.add_album, song);
			if (num_errors != 0){
				add_error_list(error_messages, "DBD insert_album error:", apr_dbd_error(dbd_config->dbd_driver, dbd_config->dbd_handle, num_errors));
				return -1;
			}
		}else if (num_errors != 0){
			add_error_list(error_messages, "DBD album get id error:", apr_dbd_error(dbd_config->dbd_driver, dbd_config->dbd_handle, num_errors));
			return -1;
		}

			if (artist_id != NULL && album_id != NULL && song_id != NULL){
				num_errors = insert_db_links(pool, dbd_config, dbd_config->statements.add_link, song, artist_id, album_id, song_id);
				if (num_errors != 0){
					//error_message = apr_dbd_error(dbd_config->dbd_driver, dbd_config->dbd_handle, num_errors);
					add_error_list(error_messages, "DBD insert_song error:", apr_dbd_error(dbd_config->dbd_driver, dbd_config->dbd_handle, num_errors));
					return -1;
				}
			}else{
				//add_error_list(error_messages, "DBD link error:", apr_psprintf(pool, "artist_id: %s album_id=%s song_id=%s", artist_id, album_id, song_id));
				return -1;
			}
	}else{
		add_error_list(error_messages, "DBD get file path error", apr_psprintf(pool, "(%s)",song->file_path));
		return -1;
	}
	return 0;
}

