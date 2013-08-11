/*
 * app_config.c
 *
 *  Created on: Dec 29, 2012
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
 *  See the License for the specific language governing permissions and
 *   limitations under the License.
*/

/*
 * config_app
 * app pointer
 * app type
 * freindly_name (safe to search)
 * app id
 * init app pointer function
 * get query pointer function
 * run query pointer function
 *
*/

#include <stdlib.h>
#include "apr_general.h"
#include "apr_lib.h"
#include "apr_strings.h"
#include <apr_xml.h>
#include "apps/app_config.h"

#include "database/db_query_config.h"
#include "database/dbd.h"

int init_app_manager(apr_pool_t* pool, error_messages_t* error_messages, app_list_t** apps_ptr){
	int status = 0;
	app_list_t* apps;
	//Setup the app manager
	apps = *apps_ptr = apr_pcalloc(pool,sizeof(app_list_t));
	apps->pool = pool;
	apps->error_messages = error_messages;


	return status;
}

int add_to_app_list(app_list_t* app_list, app_t** app){
	int status = 0;

	app_node_t* app_node = NULL;
	
	app_node = apr_pcalloc(app_list->pool,sizeof(app_node_t));
	app_node->next = NULL;

	//Add to linked list
	if(app_list->first_node == NULL){
		//No apps in list
		app_list->first_node = app_node;
	}else{
		app_node->next = app_list->first_node;
		app_list->first_node = app_node;
	}

	(app_list->count)++;

	app_node->app = *app =  apr_pcalloc(app_list->pool,sizeof(app_t));

	return status;
}



int read_app_xml_config(apr_pool_t* pool, app_t* app,apr_array_header_t* db_array, error_messages_t* error_messages){
	apr_xml_parser* xml_parser;
	apr_xml_doc* xml_doc;
	apr_xml_elem* xml_elem;

	apr_file_t* xml_file;

	apr_status_t status;

	int error_num;
	apr_size_t max_element_size = 255;

	app->db_queries = apr_array_make(pool, 2, sizeof(struct db_queries_));
	
	status = apr_file_open(&xml_file, app->xml_config_file_path,APR_READ,APR_OS_DEFAULT ,pool);
	if(status != APR_SUCCESS){
		 	add_error_list(error_messages,ERROR,"Error opening DB XML file","ERRRORROROOROR");
		 	return -1;
	}
	xml_parser = apr_xml_parser_create(pool);

	status = apr_xml_parse_file(pool, &xml_parser,&xml_doc,xml_file,20480);
	if(status != APR_SUCCESS){
		 add_error_list(error_messages,ERROR,"Error parsing DB XML file","ERRRORROROOROR");
		 return -1;
	}

	if(apr_strnatcmp(xml_doc->root->name, "app") != 0){
		 add_error_list(error_messages,ERROR,"Invalid XML FILE","ERRRORROROOROR");
		 return -1;
	}

	for(xml_elem = xml_doc->root->first_child;xml_elem != NULL;xml_elem = xml_elem->next){
		const char* db_id;
		db_params_t* db_params;
		//Generate queries for app
		if(apr_strnatcmp(xml_elem->name,"parameters") ==0){
			app->xml_parameters = xml_elem;
		}else if(apr_strnatcmp(xml_elem->name,"queries") ==0){
			db_queries_t* db_queries;
			get_xml_attr(pool,xml_elem, "db_id", &db_id);
			//Find database by id and pass tables to the query generator
			status = find_db_by_id(db_array, db_id, &db_params);
			if(status != 0){
				continue;
				//Error db not found
			}
		
			db_queries = apr_array_push(app->db_queries);
				
			db_queries->db_params = db_params;
			db_queries->queries = apr_array_make(pool,10,sizeof(db_query_t));

			error_num = generate_app_queries(pool, db_queries, xml_elem, db_params, error_messages); 
			if(error_num != 0){
				 return error_num;
			}
		}	
		if(apr_strnatcmp(xml_elem->name,"fname") ==0){
			apr_xml_to_text(pool,xml_elem,APR_XML_X2T_INNER,NULL,NULL,&(app->friendly_name),&max_element_size);
			if(error_num != 0){
				 return error_num;
			}
		}
	}

	apr_file_close(xml_file);
	return 0;
}


int config_app(app_list_t* app_list, const char* app_id, const char* xml_config_path, init_app_fnt init_app, reattach_app_fnt reattach_app, run_query_fnt run_query){
	app_t* app;

	add_to_app_list(app_list, &app);

	app->xml_config_file_path = apr_pstrdup(app_list->pool, xml_config_path);

	app->global_context = NULL;
	app->id = apr_pstrdup(app_list->pool,app_id);


	//Set callback functions
	app->init_app = init_app;
	app->reattach_app = reattach_app;
	app->run_query = run_query;
	return 0;
}

void reattach_apps(app_list_t* app_list,apr_pool_t* child_pool, error_messages_t* error_messages){
	app_node_t* app_node;

	for(app_node = app_list->first_node;app_node != NULL;app_node = app_node->next){
		app_node->app->reattach_app(child_pool,error_messages,app_node->app->global_context);
	}

}

int init_apps(app_list_t* app_list, apr_array_header_t* db_array){
	app_node_t* app_node;

	for(app_node = app_list->first_node;app_node != NULL;app_node = app_node->next){
		app_t* app = app_node->app;
		read_app_xml_config(app_list->pool, app, db_array, app_list->error_messages);

		app_node->app->init_app(app_list->pool, app->db_queries, app->xml_parameters, &(app->global_context),app_list->error_messages);
	}


	return 0;
}

int app_process_uri(input_t* input, app_list_t* app_list, app_t** app){
	app_node_t* app_node;
	const char* app_fname;

	int i =0;
	char* uri_cpy = apr_pstrdup(input->pool, input->uri);
	char* uri_slash;

	const char** query_nouns = apr_pcalloc(input->pool,sizeof(char) * 15 * 255);

	//Remove leading slash and add trailing slash if one doesn't exsits.
	uri_cpy++;//Remove leading slash
	if (uri_cpy[strlen(uri_cpy) - 1] != '/'){
		uri_cpy = apr_pstrcat(input->pool, uri_cpy, "/", NULL);
	}

	//Fill array query_nouns with uri parts
	while ((uri_slash= strchr(uri_cpy, '/')) != NULL && i <= 15){
		 uri_slash[0] = '\0';
		 query_nouns[i] = uri_cpy;
		 uri_cpy = ++uri_slash;
		 i++;
	}
	//Check if we get the minimum amount of data for a query
	if(query_nouns[0] == NULL){
		return -2;
	}

	app_fname = query_nouns[0];

	for(app_node = app_list->first_node;app_node != NULL;app_node = app_node->next){
		if(app_node->app->friendly_name != NULL && apr_strnatcmp(app_node->app->friendly_name,app_fname) == 0){
			*app = app_node->app;
			input->query_words = apr_pcalloc(input->pool, sizeof(query_words_t));
			input->query_words->words = query_nouns;
			input->query_words->num_words = i;
			return 0;
		}
	}


	return -1;
}
