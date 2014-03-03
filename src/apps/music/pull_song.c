#include "music_query.h"

int pull_song(music_query_t* music){
	apr_pool_t* pool = music->pool;


	const char* file_path;
	int status;
	apr_status_t rv;

	const char* error_header = "Error Playing Song";
	apr_finfo_t file_info;
	apr_file_t* file_desc;
	//apr_size_t total_length = 0;

	column_table_t* file_path_col;
	column_table_t* file_type_col;
	//create file to save decoded data too

	const char* file_type;


	//status = file_path_query(dbd_config,music->query_parameters,music->db_query,&file_path,&file_type,music->error_messages);
	 status = find_select_column_from_query_by_table_id_and_query_id(&file_path_col,music->db_query,"sources","path");
	 if(status != 0){
		 return -11;
	 }
	 status = find_select_column_from_query_by_table_id_and_query_id(&file_type_col,music->db_query,"sources","type");
	 if(status != 0){
		 return -11;
	 }
	 status = get_column_results_for_row(music->db_query,music->results,file_path_col,0,&file_path);
	 if(status != 0){
		 return -14;
	 }
	 status = get_column_results_for_row(music->db_query,music->results,file_type_col,0,&file_type);
	 	 if(status != 0){
	 		 return -14;
	 	 }

	//Determine what kind of file this is
	//Pick right decoder

	//Flac
	if (file_path == NULL){
		add_error_list(music->error_messages,ERROR, error_header,"Error with getting file_path from results");
		return -1;
	}

	rv = apr_file_open(&file_desc, file_path, APR_READ, APR_OS_DEFAULT, pool);
	 if (rv != APR_SUCCESS){
		add_error_list(music->error_messages,ERROR, error_header, apr_psprintf(pool,"Error opening file (%s)", file_path));
		 return -7;
	 }
	 rv = apr_stat(&file_info,file_path,APR_FINFO_SIZE,pool);
	 //TEMPORARY FILE EXSITS USE IT

	apr_cpystrn((char*)music->output->content_type ,apr_psprintf(pool,"audio/%s",file_type),255) ;

	apr_table_add(music->output->headers, "Accept-Ranges", "bytes");

	apr_brigade_insert_file(music->output->bucket_brigade, file_desc,0,file_info.size,pool);
	//apr_file_close(file_desc);
	return 0;
}
