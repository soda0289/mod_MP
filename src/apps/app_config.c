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
#include "apps/app_config.h"



int config_app(app_list_t* app_list,const char* freindly_name, const char* app_id,init_app_fnt init_app, reattach_app_fnt reattach_app, run_query_fnt run_query){
	app_node_t* app_node = NULL;
	app_node_t* app_node_previous = NULL;
	app_t** app;
	if(app_list->first_node == NULL){
		//No apps in list
		app_list->first_node = apr_pcalloc(app_list->pool,sizeof(app_node_t));
		app_list->first_node->next = NULL;
		app = &(app_list->first_node->app);
	}else{
		for(app_node = app_list->first_node;app_node != NULL;app_node = app_node->next){
			app_node_previous = app_node;
		}
		app_node_previous->next = apr_pcalloc(app_list->pool,sizeof(app_node_t));
		app_node_previous->next->next = NULL;
		(app_list->count)++;
		app = &(app_node_previous->next->app);
	}
	*app =  apr_pcalloc(app_list->pool,sizeof(app_t));

	(*app)->friendly_name = apr_pstrdup(app_list->pool,freindly_name);
	(*app)->global_context = NULL;
	(*app)->id = apr_pstrdup(app_list->pool,app_id);
	(*app)->init_app = init_app;
	(*app)->reattach_app = reattach_app;
	(*app)->run_query = run_query;
	return 0;
}

int reattach_apps(app_list_t* app_list,apr_pool_t* child_pool, error_messages_t* error_messages){
	app_node_t* app_node;

	for(app_node = app_list->first_node;app_node != NULL;app_node = app_node->next){
		app_node->app->reattach_app(child_pool,error_messages,app_node->app->global_context);
	}

	return 0;
}

int init_apps(app_list_t* app_list,apr_pool_t* global_pool, error_messages_t* error_messages, const char* media_directory){
	app_node_t* app_node;

	for(app_node = app_list->first_node;app_node != NULL;app_node = app_node->next){
		app_node->app->init_app(global_pool,error_messages, media_directory, &(app_node->app->global_context));
	}


	return 0;
}

int app_process_uri(apr_pool_t* pool, const char* uri, app_list_t* app_list, app_t** app){
	app_node_t* app_node;
	const char* app_fname;
	int i =0;
	char* uri_cpy = apr_pstrdup(pool, uri);
	char* uri_slash;

	const char** query_nouns = apr_pcalloc(pool,sizeof(char) * 15 * 255);

	//Remove leading slash and add trailing slash if one doesn't exsits.
	uri_cpy++;//Remove leading slash
	if (uri_cpy[strlen(uri_cpy) - 1] != '/'){
		uri_cpy = apr_pstrcat(pool, uri_cpy, "/", NULL);
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
		if(apr_strnatcmp(app_node->app->friendly_name,app_fname) == 0){
			*app = app_node->app;
			(*app)->query_words.words = query_nouns;
			(*app)->query_words.num_words = i;
			return 0;
		}
	}
	return -1;
}
