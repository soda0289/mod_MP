#include "libmod_mp.h"
//getpid()
#include <sys/types.h>
#include <unistd.h> 

#include "indexers/index_manager.h"
#include "error_messages.h"

#include "indexers/file/file_indexer.h"
void mod_mp_set_data(mp_srv_cfg *srv_conf, void* data){
	srv_conf->web_server_data  = data;
}

int mod_mp_init (mp_srv_cfg *srv_conf, apr_pool_t* pool) {
	int status = 0;
	indexer_t* file_indexer;
	const char* file_indexer_xml_config;

	srv_conf->pid = getpid();

	//Initialize error messages
	status = error_messages_init_shared(pool, srv_conf->errors_shm_file, &(srv_conf->error_messages));
	if(status != APR_SUCCESS)
		goto error_out;

	//Setup Database Parameters from XML files 
	status = db_manager_init(pool, srv_conf->error_messages, srv_conf->db_xml_config_dir, &(srv_conf->db_manager));
	if(status != APR_SUCCESS)
		goto error_out;

	//Initialize index manager
	status = index_manager_init(pool, srv_conf->error_messages, &(srv_conf->index_manager));	
	if(status != APR_SUCCESS)
		goto error_out;

	//Core file and directory based indexer
	file_indexer_xml_config = apr_pstrcat(pool, srv_conf->indexer_xml_config_dir, "/file.xml", NULL);

	indexer_init(pool, srv_conf->error_messages, srv_conf->db_manager, "file", file_indexer_xml_config, &(file_indexer_callbacks), &file_indexer);
	index_manager_add(srv_conf->index_manager, file_indexer);

	/*
	//Configure the Music indexer
	indexer_t* music_indexer;
	music_xml_config = apr_pstrcat(pool, srv_conf->apps_xml_dir, "/music.xml", NULL);
	index_manager_new(srv_conf->index_manager, "music", music_indexer_xml_config,
				music_indexer_callbacks, &music_indexer);
	index_manager_add(srv_conf->index_manager, music_indexer); 
	*/


	return 0;
	error_out:
		srv_conf->enable = 0;
		return -1;
}

