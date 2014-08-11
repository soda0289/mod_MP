/*
 * tag_reader.c
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
 * See the License for the specific language governing permissions and
*  limitations under the License.
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
#include <apr_strmatch.h>
#include <stdlib.h>


#include "tag_reader.h"
#include "database/db.h"
#include "error_messages.h"


static int get_file_ext(apr_pool_t* pool, const char* file_path){
	int length = strlen(file_path);
	for(; length > 0; length--){
		if(file_path[length] == '.'){
			if(apr_strnatcasecmp((const char*)&file_path[length+1],"flac") == 0){
				return FLAC;
			}else if(apr_strnatcasecmp((const char*)&file_path[length+1],"ogg") == 0){
				return OGG;
			}else if(apr_strnatcasecmp((const char*)&file_path[length+1],"mp3") == 0){
				return MP3;
			}
			break;
		}
	}
	//Not known file extention
	return -1;
}

List* read_dir(apr_pool_t* pool, List* file_list, const char* dir_path, int* count, error_messages_t* error_messages){
	apr_status_t rv;
	char errorbuf [255];

	apr_dir_t *dir;
	apr_finfo_t filef;

	char* file_path;

	rv = apr_dir_open(&dir, dir_path, pool);
	if (rv != 0){
		 apr_strerror(rv, (char *)&errorbuf, 255);
		error_messages_add(error_messages, ERROR,apr_psprintf(pool, "Error opening directory (%s) (%d)", dir_path, rv), errorbuf);
		return NULL;
	}
	  //Read file is directory
	while (apr_dir_read(&filef, APR_FINFO_TYPE | APR_FINFO_NAME | APR_FINFO_MTIME, dir) == 0){
		file_path =  apr_pstrcat(pool, dir_path, "/", filef.name, NULL);
		if (file_path == NULL){
			return NULL;
			///BIG ERROR
		}
		//Is a Directory
		if (filef.filetype == APR_DIR){
			if(filef.name[0] != '.'){
					file_list = read_dir(pool, file_list, file_path, count, error_messages);
			}
		}
		//Is a File
		if (filef.filetype == APR_REG){
			List* file_list_new;
			(*count)++;
			file_list_new  = apr_pcalloc(pool, sizeof(List));
			file_list->file.path = file_path;
			file_list->file.mtime = filef.mtime;
			file_list->file.type  = get_file_ext(pool,file_path);
			file_list->next = file_list_new;
			file_list = file_list_new;
		}
	}
	rv = apr_dir_close(dir);
	return file_list;
}

