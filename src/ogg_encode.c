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
#include "music_query.h"
#include "dbd.h"
#include "flac.h"

int play_song(db_config* dbd_config, request_rec* r, music_query* music){
	char* file_path;
	int status;
	long samples_total;
	apr_status_t rv;

	apr_bucket_brigade* bb;
	apr_bucket* b_header;
	apr_bucket* b_body;

	apr_file_t* file_desc;
	apr_size_t total_length = 0;
	//create file to save decoded data too
	char* tmp_file_path;
	const char* temp_dir;

	bb = apr_brigade_create(r->pool, r->connection->bucket_alloc);

	rv = apr_temp_dir_get(&temp_dir,r->pool);
	if(rv != APR_SUCCESS){
		ap_rprintf(r, "Couldn't get temp dir");
		return -1;
	}

	tmp_file_path = apr_pstrcat(r->pool, temp_dir,"/", music->query_parameters[SONG_ID].parameter_value,".tran", NULL);
	 rv = apr_file_open(&file_desc, tmp_file_path, APR_READ | APR_WRITE | APR_CREATE | APR_EXCL, APR_OS_DEFAULT, r->pool);
	 if (rv != APR_SUCCESS){
		 	 if(rv == 17){
		 		apr_finfo_t finfo ;
		 		  if ( apr_stat(&finfo, tmp_file_path, APR_FINFO_SIZE, r->pool) != APR_SUCCESS ) {
		 			  ap_rprintf(r,"COulnt read size of caches file");
		 		    return -1 ;
		 		  }
		 		 rv = apr_file_open(&file_desc, tmp_file_path, APR_READ | APR_WRITE | APR_CREATE, APR_OS_DEFAULT, r->pool);
		 		 //TEMPORARY IF FILE EXSITS USE IT
		 		//Set headers
		 		apr_table_add(r->headers_out, "Access-Control-Allow-Origin", "*");
		 		ap_set_content_type(r, "audio/ogg") ;
		 		ap_set_content_length(r, total_length);
		 		apr_table_setn(r->headers_out, "Accept-Ranges", "bytes");

		 		apr_brigade_insert_file(bb, file_desc,0,finfo.size,r->pool);
		 		 APR_BRIGADE_INSERT_TAIL(bb, apr_bucket_eos_create(bb->bucket_alloc));
		 		ap_pass_brigade(r->output_filters, bb);

		 		apr_file_close(file_desc);
		 		return 0;
		 	 }
			ap_rprintf(r,"Error(%d) creating temp file %s",rv, tmp_file_path);
			return -1;
	 }










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


    flac_file* flac;
    int serial_no;


	if(music->query_parameters_set & 1 << SONG_ID){
		return -1;
	}
	vorbis_info_init(&v_info);

	status = get_file_path(&file_path, dbd_config, music->query_parameters[SONG_ID].parameter_value, dbd_config->statements.select_file_path);

	//Determine what kind of file this is
	//Pick right decoder

	//Flac
	if (status != 0){
		ap_rprintf(r,"Error with get_file_path");
		return -1;
	}
	status = read_flac_file(&flac, file_path,r->pool);
	if (status != 0){
		ap_rprintf(r,"Error with read_file_path");
		close_flac(flac);
		return -1;
	}

	vorbis_encode_init_vbr(&v_info,flac->channels, flac->rate, 0);

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
	while(!eos){
    	//Create vorbis buffer of unencoded samples
        float **buffer = vorbis_analysis_buffer(&vorbis_dsp, 1024);
        long samples_read = process_flac_file(flac, buffer, 1024);//Return samples read per channel

        if (samples_read == 0){
        	vorbis_analysis_wrote(&vorbis_dsp,0); //Write 0 bytes to close up vorbis analysis
        }else{
        	samples_total += samples_read;
        	vorbis_analysis_wrote(&vorbis_dsp, samples_read);
        }
        while(vorbis_analysis_blockout(&vorbis_dsp,&vorbis_block) == 1){
        	//A vorbis block is avalilbe time to start encoding

        	//Encode block place in OGG packet and add to stream
            vorbis_analysis(&vorbis_block, &ogg_pack);
            ogg_stream_packetin(&ogg_stream,&ogg_pack);

            while(!eos){
            	//Turn ogg packets into pages
                int result = ogg_stream_pageout(&ogg_stream,&ogg_page);
                if(!result) break; //not enough data to create page continue, building ogg packets

                apr_size_t* length_written;
                length_written = apr_pcalloc(r->pool, sizeof(apr_size_t));
                *length_written = ogg_page.header_len;
                rv = apr_file_write_full(file_desc, (&ogg_page)->header,ogg_page.header_len,length_written);
                if (*length_written != ogg_page.header_len || rv != APR_SUCCESS){
                	ap_rprintf(r, "Couldn't write all of head");
                	return -1;
                }
                *length_written = ogg_page.body_len;
                apr_file_write_full(file_desc, (&ogg_page)->body,ogg_page.body_len,length_written);
                if (*length_written != ogg_page.body_len || rv != APR_SUCCESS){
                	ap_rprintf(r,"Couldn't write all of body");
                	return -1;
                }

                total_length += ogg_page.header_len +  ogg_page.body_len;
                /*
                b_header = apr_bucket_immortal_create((char *)((&ogg_page)->header),(&ogg_page)->header_len, bb->bucket_alloc);
                b_body = apr_bucket_immortal_create((char *)((&ogg_page)->body),(&ogg_page)->body_len, bb->bucket_alloc);

                APR_BRIGADE_INSERT_TAIL(bb, b_header);
                APR_BRIGADE_INSERT_TAIL(bb, b_body);


                //Write pages
                ap_rwrite((&ogg_page)->header,(&ogg_page)->header_len ,r);
                ap_rwrite((&ogg_page)->body,(&ogg_page)->body_len ,r);
				*/

                //Check if last page
                if(ogg_page_eos(&ogg_page))
                    eos = 1;//Kill loops
            }
        }
    }

	/*
	//Set headers
	apr_table_add(r->headers_out, "Access-Control-Allow-Origin", "*");
	ap_set_content_type(r, "audio/ogg") ;
	ap_set_content_length(r, total_length);
	//apr_table_setn(r->headers_out, "Accept-Ranges", "bytes");
	apr_table_setn(r->headers_out, "Accept-Ranges", "none");
	apr_brigade_insert_file(bb, file_desc,0,total_length,r->pool);
	 APR_BRIGADE_INSERT_TAIL(bb, apr_bucket_eos_create(bb->bucket_alloc));
	ap_pass_brigade(r->output_filters, bb);

	apr_file_close(file_desc);
	*/
	//Clean up encoder
	ogg_stream_clear(&ogg_stream);

	vorbis_block_clear(&vorbis_block);
	vorbis_dsp_clear(&vorbis_dsp);
	vorbis_info_clear(&v_info);

	close_flac(flac);
	ap_rprintf(r,"Successfully converted song");

	return 0;
}

