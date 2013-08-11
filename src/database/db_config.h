/*
 * db_config.h
 *
 *  Created on: Aug 5, 2013 
 *     Author: Reyad Attiyat
 *		Copyright 2013 Reyad Attiyat
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

#ifndef DB_CONFIG_H_
#define DB_CONFIG_H_

#include "db_typedef.h"
#include "error_handler.h"

int init_db_array(apr_pool_t* pool, const char* xml_db_dir, apr_array_header_t** db_array_ptr,error_messages_t* error_messages);
int find_db_by_id(apr_array_header_t* db_arrays, const char* db_id, db_params_t** db_params_ptr);

#endif /* DB_CONFIG_H_ */


