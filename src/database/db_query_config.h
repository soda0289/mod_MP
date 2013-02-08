/*
 * db_query_config.h
 *
 *  Created on: Dec 18, 2012
 *     Author: Reyad Attiyat
 *		Copyright 2012 Reyad Attiyat
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
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#ifndef DB_QUERY_CONFIG_H_
#define DB_QUERY_CONFIG_H_

#include "db_typedef.h"
#include "apps/app_typedefs.h"

#define SUCCESS 0

int init_db_schema(app_list_t* app_list,char* file_path, db_config* dbd_config);
int find_query_by_id(query_t** element, apr_array_header_t*array, const char* id);
int find_column_from_query_by_friendly_name(query_t* query,const char* friendly_name, column_table_t** column);
int find_select_column_from_query_by_table_id_and_query_id(column_table_t** select_column,query_t* query, const char* table_id, const char* column_id);
#endif /* DB_QUERY_CONFIG_H_ */
