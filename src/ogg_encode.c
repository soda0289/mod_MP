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


	if(music->by_id.id_type != SONGS){
		return -1;
	}
	vorbis_info_init(&v_info);

	status = get_file_path(&file_path, dbd_config, music->by_id.id, dbd_config->statements.select_song);

	//Determine what kind of file this is
	//Pick right decoder

	//Flac
	if (status == 0){
		read_flac_file(&flac, file_path);
		//ap_rprintf(r, "%s",file_path);
	}else{
		//ap_rprintf(r,"Error with get_file_path");
	}

	ap_set_content_type(r, "audio/ogg") ;

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
        	//A block is avalibe time to start encoding

        	//Encode block place in OGG packet and add to stream
            vorbis_analysis(&vorbis_block, &ogg_pack);
            ogg_stream_packetin(&ogg_stream,&ogg_pack);

            while(!eos){
            	//Turn ogg packets into pages
                int result = ogg_stream_pageout(&ogg_stream,&ogg_page);
                if(!result) break; //not enough data to create page contiune, building ogg packets

                //Write pages
                ap_rwrite((&ogg_page)->header,(&ogg_page)->header_len ,r);
                ap_rwrite((&ogg_page)->body,(&ogg_page)->body_len ,r);

                //Check if last page
                if(ogg_page_eos(&ogg_page))
                    eos = 1;//Kill loops
            }
        }
    }

	//Clean up enocder
	ogg_stream_clear(&ogg_stream);

	vorbis_block_clear(&vorbis_block);
	vorbis_dsp_clear(&vorbis_dsp);
	vorbis_info_clear(&v_info);



	return 0;
}

