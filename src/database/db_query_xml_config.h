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

#include <apr_xml.h>
#include "indexers/indexer_typedefs.h"
#include "error_messages.h"


#define SUCCESS 0


int get_xml_attr(apr_pool_t* pool,apr_xml_elem* elem,const char* attr_name,const char** attr_value);
int find_query_by_id(db_query_t** element, apr_array_header_t*array, const char* id);
int find_column_from_query_by_friendly_name(db_query_t* query,const char* friendly_name, db_table_column_t** column);
int find_select_column_from_query_by_table_id_and_query_id(db_table_column_t** select_column,db_query_t* query, const char* table_id, const char* column_id);


#endif /* DB_QUERY_CONFIG_H_ */
