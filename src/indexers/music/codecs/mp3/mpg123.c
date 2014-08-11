/*
 * mpg123.c
 *
 *  Created on: July 26th, 2013 
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
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *  Code borrowed from oggenc part of the vorbis-tools package.
*/



#include "apps/music/transcoder.h"
#include "apps/music/mpg123.h" 
#include <mpg123.h>

#include "apps/music/id3v2.h"

int find_id3_tag_id(apr_pool_t* pool, mpg123_id3v2* id3v2, id3v2_ids_t* id3v2_ids){
	int i = 0;
	int k = 0;

	int* ids_key = NULL;
	int* id_tag_id = NULL;

	for(i = 0; i < id3v2_ids->num; i++){
		for(k = 0; k < id3v2->texts; k++){
			//So since an int is equal to 4 char we just make that happen and compare.
			ids_key = (int*)&(id3v2_ids->ids[i].id);
			id_tag_id = (int*)&(id3v2->text[k].id);

			if(*(ids_key) == *(id_tag_id)){
				*(id3v2_ids->ids[i].texts) = apr_pstrdup(pool,id3v2->text[k].text.p);
				break;
			}

		}
	}
	return 0;
}


int read_id3(apr_pool_t* pool, music_file* song){
	int i = 0;

	int status = 0;
	int length = 9;

	int sample_count = 0;
	long sample_rate = 0;

	int meta = 0;

	mpg123_handle* m;
	mpg123_id3v2* id3v2;

	

	m = mpg123_new(NULL, NULL);
	
	if(m == NULL){
		return -1;
	}
	//Open mp3 file for scanning
	status = mpg123_open(m, song->file->path);	

	//Scan entire file to find all ID3 tags and accurate length
	status = mpg123_scan(m);
	if(status != MPG123_OK){
		return -3;
	}

	//Check if we found any ID3 tags
	meta = mpg123_meta_check(m);
	if(meta & MPG123_ID3){
		status = mpg123_id3(m, NULL, &id3v2);
		if(status != MPG123_OK){
			return -5;
		}
		//metadata is good fill song info
		id3v2_id_t ids[8];
		id3v2_ids_t tags_request;
		tags_request.ids = ids;
		tags_request.num = 8;

		memcpy(ids[0].id, "TIT2", 4);
		ids[0].texts = &(song->title); 

		memcpy(ids[1].id, "TPE1", 4);
		ids[1].texts = &(song->artist); 

		memcpy(ids[2].id, "TALB", 4);
		ids[2].texts = &(song->album);

		memcpy(ids[3].id, "TRCK", 4);
		ids[3].texts = &(song->track_no);

		memcpy(ids[4].id, "TPOS", 4);
		ids[4].texts = &(song->disc_no);

		memcpy(ids[5].id, "TYER", 4);
		ids[5].texts = &(song->year);

		memcpy(ids[6].id, "TDAT", 4);
		ids[6].texts = &(song->date);

		memcpy(ids[7].id, "TDRC", 4);
		ids[7].texts = &(song->date);

	
		find_id3_tag_id(pool, id3v2, &tags_request);

		//Check for '/' in track and disc no
		
		if(song->track_no != NULL){
			for(i = 0; i < strlen(song->track_no); i++){
				if(song->track_no[i] == '/'){
					song->track_no[i] = '\0';
					song->total_tracks = &(song->track_no[i+1]);	
					break;
				}
			}
		}


		if(song->disc_no != NULL){
			for(i = 0; i < strlen(song->disc_no); i++){
				if(song->disc_no[i] == '/'){
					song->disc_no[i] = '\0';
					song->discs_in_set = &(song->disc_no[i+1]);	
					break;
				}
			}
		}
	}else{
		return -6;
	}

	status = mpg123_getformat(m, &sample_rate,NULL, NULL);

	sample_count = mpg123_length(m);
	length = sample_count/sample_rate;
	if(length >= 0){
		song->length = length;
	}



	mpg123_close(m);
	mpg123_delete(m);
	
	return 0;
}

/*
 * This function takes in the mp3_file struct a buffer to hold the samples. Samples
 * indicates how many samples per channel to get
 */
long process_mp3_file(void *in, float **buffer, int samples){
	int status = 0;
	int j = 0, k = 0;

	mp3_file_t* mp3_file = (mp3_file_t*)in;
	size_t done = 0;
	size_t size_of_sample = mpg123_encsize(MPG123_ENC_FLOAT_32);

	size_t bytes_ch = mp3_file->channels * size_of_sample; 

	//Buffer size is equal to number of channels * number of samples per channel
	size_t buffer_size = samples * bytes_ch;
	unsigned char* mp3_out_buff = malloc(buffer_size);
	memset(mp3_out_buff,0,buffer_size);
	
	//Buffer is equal to  buffer[c] where c is channel index
	//                    buffer[c][s] where s is sample index
	status = mpg123_read (mp3_file->mh, mp3_out_buff, buffer_size, &done);
	if(status != MPG123_OK){
		if(status == MPG123_NEED_MORE){
			return 0;
		}else if (status == MPG123_DONE){
			
		}else if(status == MPG123_NEW_FORMAT){
			return 0;
		}
	}
	
	//Check if 32bit Float or 64bit Double
	if(size_of_sample == sizeof(float)){
		//Transfer from one array to next
		for(j = 0; j < done/bytes_ch; j ++ ){
			for(k = 0; k < mp3_file->channels; k++){
				//buffer[k][(j/mp3_file->channels) + k] = ((float*)mp3_out_buff)[j + k];
				buffer[k][j] = ((float*)mp3_out_buff)[j*mp3_file->channels + k];
			}
		}
	}
	
	free(mp3_out_buff);
		
	return (done/bytes_ch);
	

}

int read_mp3_file (apr_pool_t* pool, void** mp3_file_ptr, const char* file_path, encoding_options_t* enc_opt){
	int status = 0;
	mp3_file_t* mp3_file = *mp3_file_ptr = (mp3_file_t*)apr_pcalloc(pool, sizeof(mp3_file_t));




	mp3_file->mh = mpg123_new(NULL, &status);
	if(mp3_file->mh == NULL){
		return -1;
	}


	//Force Float
	mpg123_param(mp3_file->mh, MPG123_ADD_FLAGS, MPG123_FORCE_FLOAT, 0);

	//Open mp3 file
	status = mpg123_open(mp3_file->mh, file_path);
	if(status != MPG123_OK){
		//Error couldn't open file
		return -1;
	}

	//Get basic data about mp3 file sample rate, channles, and encoding
	status = mpg123_getformat(mp3_file->mh, &(mp3_file->rate), (int*)&(mp3_file->channels), &(mp3_file->encoding));
	if(status != MPG123_OK){
		return -2;
	}

	//Set encoding options to whatever the current mp3 file is
	enc_opt->channels = mp3_file->channels;
	enc_opt->rate = mp3_file->rate;
	enc_opt->total_samples_per_channel = mpg123_length(mp3_file->mh);
	enc_opt->samples_to_request = (mpg123_safe_buffer()/sizeof(float))/mp3_file->channels;

	status = mpg123_format_none(mp3_file->mh);
	//Set encoding to 32bit float as that is what vorbis will use
	status = mpg123_format(mp3_file->mh, enc_opt->rate, enc_opt->channels, MPG123_ENC_FLOAT_32);
	if(status != MPG123_OK){
		//Error setting encoder to float
		return -3;
	}


	return 0;
}

int close_mp3_file(void* in){
	mp3_file_t* mp3_file = (mp3_file_t*)in;

	mpg123_close(mp3_file->mh);
	mpg123_delete(mp3_file->mh);

	return 0;
}


