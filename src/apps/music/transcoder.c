/*
 * transcoder.c
 *
 *  Created on: Jan 11, 2013
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


int transcode_audio(){
	rv = apr_temp_dir_get(&temp_dir,r->pool);
	if(rv != APR_SUCCESS){
		ap_rprintf(r, "Couldn't get temp dir");
		return -1;
	}

	status = file_path_query(dbd_config,music_query,&file_path,&file_type);
	if(status == 0){
		ap_rprintf(r,"The output type is:%s\nThe file path is %s",((custom_parameter_t*)music_query->query_parameters->query_custom_parameters->elts)[0].value,file_path);
		return 0;
	}

	tmp_file_path = apr_pstrcat(r->pool, temp_dir,"/", ((query_where_condition_t*)music_query->query_parameters->query_where_conditions->elts)[0].condition,".ogg", NULL);
	return 0;
}
