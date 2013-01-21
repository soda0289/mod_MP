/*
 * ogg_encode.c
 *
 *  Created on: Nov 07, 2012
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
 *  limitations under the License.
 *
 *  Code borrowed from oggenc part of the vorbis-tools package. ♡ OS
*/


#include <httpd.h>
#include <http_protocol.h>
#include <http_config.h>
#include <vorbis/vorbisenc.h>
#include "apps/music/music_query.h"
#include "database/dbd.h"
#include "flac.h"
#include "apps/music/transcoder.h"
#include "database/db_query_config.h"

int play_song(db_config* dbd_config, request_rec* r, music_query_t* music){
	const char* file_path;
	int status;
	apr_status_t rv;

	apr_bucket_brigade* bb;
	apr_bucket* b_header;
	apr_bucket* b_body;
	apr_finfo_t file_info;
	apr_file_t* file_desc;
	apr_size_t total_length = 0;

	column_table_t* file_path_col;
	//create file to save decoded data too

	const char* file_type;


	bb = apr_brigade_create(r->pool, r->connection->bucket_alloc);


	//status = file_path_query(dbd_config,music->query_parameters,music->db_query,&file_path,&file_type,music->error_messages);
	 status = find_select_column_from_query_by_table_id_and_query_id(&file_path_col,music->db_query,"sources","path");
	 if(status != 0){
		 return -11;
	 }
	 status = get_column_results_for_row(music->db_query,music->results,file_path_col,0,&file_path);
	 if(status != 0){
		 return -14;
	 }

	//Determine what kind of file this is
	//Pick right decoder

	//Flac
	if (file_path == NULL){
		ap_rprintf(r,"Error with getting file_path from results");
		return -1;
	}

	rv = apr_file_open(&file_desc, file_path, APR_READ, APR_OS_DEFAULT, r->pool);
	 if (rv != APR_SUCCESS){
		 ap_rprintf(r,"Error opening file (%s)", file_path);
		 return -7;
	 }
	 rv = apr_stat(&file_info,file_path,APR_FINFO_SIZE,r->pool);
	 //TEMPORARY FILE EXSITS USE IT
	//Set headers
	apr_table_add(r->headers_out, "Access-Control-Allow-Origin", "*");

	ap_set_content_type(r, apr_psprintf(r->pool,"audio/%s",file_type)) ;
	ap_set_content_length(r, total_length);
	apr_table_setn(r->headers_out, "Accept-Ranges", "bytes");

	apr_brigade_insert_file(bb, file_desc,0,file_info.size,r->pool);
	 APR_BRIGADE_INSERT_TAIL(bb, apr_bucket_eos_create(bb->bucket_alloc));
	ap_pass_brigade(r->output_filters, bb);

	apr_file_close(file_desc);
	return 0;
}

int ogg_encode(apr_pool_t* pool, input_file_t* input_file,encoding_options_t* enc_opt,const char* output_file_path){
	apr_status_t rv;
	int status;


	apr_file_t* file_desc;
	apr_size_t total_length = 0;

	 rv = apr_file_open(&file_desc, output_file_path, APR_READ | APR_WRITE | APR_CREATE, APR_OS_DEFAULT, pool);
	 if(rv != APR_SUCCESS){
		 return rv;
	 }

	long samples_total;
	long packets_done;
	//OGG Stream
    ogg_stream_state ogg_stream;
    ogg_page ogg_page;
    ogg_packet ogg_pack;

    //OGG header
    vorbis_comment v_comment;
    ogg_packet header_main;
    ogg_packet header_comments;
    ogg_packet header_codebooks;

    //Vorbis encoder
    vorbis_dsp_state vorbis_dsp;
    vorbis_block     vorbis_block;
    vorbis_info      v_info;

    int serial_no;


	vorbis_info_init(&v_info);
	vorbis_encode_init_vbr(&v_info,enc_opt->channels, enc_opt->rate, enc_opt->quality);
    vorbis_encode_setup_init(&v_info);

    //Initialize vorbis analysis
    vorbis_analysis_init(&vorbis_dsp,&v_info);
    vorbis_block_init(&vorbis_dsp,&vorbis_block);

    //Generate random serial number
    srand ( time(NULL) );
    serial_no = rand();

    ogg_stream_init(&ogg_stream, serial_no);

    //Create comments
    vorbis_comment_init(&v_comment);

    //Create Vorbis Header
    vorbis_analysis_headerout(&vorbis_dsp,&v_comment, &header_main,&header_comments,&header_codebooks);

    //Add header to OGG stream
    ogg_stream_packetin(&ogg_stream,&header_main);
    ogg_stream_packetin(&ogg_stream,&header_comments);
    ogg_stream_packetin(&ogg_stream,&header_codebooks);

    int eos = 0;
    packets_done = 0;
    samples_total = 0;
	while(!eos){
    	//Create vorbis buffer of unencoded samples
        float **buffer = vorbis_analysis_buffer(&vorbis_dsp, 1024);
        long samples_read = input_file->process_input_file(input_file->file_struct, buffer, 1024);//Return samples read per channel

        if (samples_read == 0){
        	//Input file is done
        	vorbis_analysis_wrote(&vorbis_dsp,0); //Write 0 bytes to close up vorbis analysis
        }else{
        	samples_total += samples_read;
            if(packets_done>=40){
            	enc_opt->progress = ((double)samples_total / (double)enc_opt->total_samples_per_chanel)*100.0;
            	packets_done = 0;
            }
        	vorbis_analysis_wrote(&vorbis_dsp, samples_read);
        }
        while(vorbis_analysis_blockout(&vorbis_dsp,&vorbis_block) == 1){
        	//A vorbis block is avalilbe time to start encoding

        	//Encode block place in OGG packet and add to stream
            vorbis_analysis(&vorbis_block, &ogg_pack);
            ogg_stream_packetin(&ogg_stream,&ogg_pack);

            packets_done++;

            while(!eos){
            	//Turn ogg packets into pages
                int result = ogg_stream_pageout(&ogg_stream,&ogg_page);
                if(!result) break; //not enough data to create page continue, building ogg packets

            apr_size_t* length_written;
            length_written = apr_pcalloc(pool, sizeof(apr_size_t));
                *length_written = ogg_page.header_len;
            rv = apr_file_write_full(file_desc, (&ogg_page)->header,ogg_page.header_len,length_written);
                if (*length_written != ogg_page.header_len || rv != APR_SUCCESS){
                	//ap_rprintf(r, "Couldn't write all of head");
                	return -1;
                }
                *length_written = ogg_page.body_len;
                apr_file_write_full(file_desc, (&ogg_page)->body,ogg_page.body_len,length_written);
                if (*length_written != ogg_page.body_len || rv != APR_SUCCESS){
                	//ap_rprintf(r,"Couldn't write all of body");
                	return -2;
                }

             total_length += ogg_page.header_len +  ogg_page.body_len;
                //Check if last page
                if(ogg_page_eos(&ogg_page))
                    eos = 1;//Kill loops
            }
        }
    }
	apr_file_close(file_desc);

	//Clean up encoder
	ogg_stream_clear(&ogg_stream);

	vorbis_block_clear(&vorbis_block);
	vorbis_dsp_clear(&vorbis_dsp);
	vorbis_info_clear(&v_info);

	return 0;
}

