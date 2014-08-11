/*
 * upload.h
 *
 *  Created on: Aug 13, 2013
 *      Author: Reyad Attiyat
 *      Copyright 2013 Reyad Attiyat
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

#ifndef UPLOAD_H_
#define UPLOAD_H_
#define MAX_ERROR_SIZE 1024
apr_status_t upload_filter(ap_filter_t *filter, apr_bucket_brigade *bbout, ap_input_mode_t mode, apr_read_type_e block, apr_off_t nbytes);

struct file_ {
	const char* path;
	const char* type_string;
};

typedef struct file_ file_t;


#endif
