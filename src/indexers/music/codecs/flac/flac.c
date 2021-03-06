/*
 * flac.c
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
 *  Code borrowed from oggenc part of the vorbis-tools package.
*/

#include "apps/music/transcoder.h"
#include "FLAC/metadata.h"
#include "FLAC/stream_decoder.h"
#include "flac.h"

/* FLAC follows the WAV channel ordering pattern; we must permute to
   put things in Vorbis channel order */
static int wav_permute_matrix[8][8] =
{
  {0},              /* 1.0 mono   */
  {0,1},            /* 2.0 stereo */
  {0,2,1},          /* 3.0 channel ('wide') stereo */
  {0,1,2,3},        /* 4.0 discrete quadraphonic */
  {0,2,1,3,4},      /* 5.0 surround */
  {0,2,1,4,5,3},    /* 5.1 surround */
  {0,2,1,4,5,6,3},  /* 6.1 surround */
  {0,2,1,6,7,4,5,3} /* 7.1 surround (classic theater 8-track) */
};


long process_flac_file(void* in, float** buffer, int samples){
    flac_file_t *flac = (flac_file_t *)in;
    long realsamples = 0;
	FLAC__bool ret;
	int i,j;
	while (realsamples < samples){
    	if (flac->buffer_sample_count > 0){
			int copy = (flac->buffer_sample_count< (samples - realsamples)) ? flac->buffer_sample_count : (samples - realsamples);

			for (i = 0; i < flac->channels; i++){

				int permute = wav_permute_matrix[flac->channels-1][i];
				for (j = 0; j < copy; j++){
					buffer[i][j+realsamples] = flac->buf[permute][j+flac->buffer_sample_seek];
				}
			}
			flac->buffer_sample_seek += copy;
			flac->buffer_sample_count -= copy;
			realsamples += copy;
        }else if (!flac->eos){
        	ret = FLAC__stream_decoder_process_single(flac->stream_decoder);
        	if (!ret || FLAC__stream_decoder_get_state(flac->stream_decoder) == FLAC__STREAM_DECODER_END_OF_STREAM){
                flac->eos = 1;  /* Bail out! */
			}
        }else{
            break;
		}
    }

	return realsamples;
}


void resize_buffer(flac_file_t* flac, int newchannels, int newsamples){
    int i;

    if (newchannels == flac->channels && newsamples == flac->buffer_sample_len)
    {
        flac->buffer_sample_seek= 0;
        flac->buffer_sample_count = 0;
        return;
    }


    /* Not the most efficient approach, but it is easy to follow */
    if(newchannels != flac->channels)
    {
        /* Deallocate all of the sample vectors */
        for (i = 0; i < flac->channels; i++)
            free(flac->buf[i]);

        if(flac->buf != NULL){
        	free(flac->buf);
        }

        flac->buf = malloc(sizeof(float*) * newchannels);
        flac->channels = newchannels;

    }

    for (i = 0; i < newchannels; i++)
        flac->buf[i] = malloc(sizeof(float) * newsamples);

    flac->buffer_sample_len = newsamples;
    flac->buffer_sample_seek = 0;
    flac->buffer_sample_count = 0;
}

void flac_errors(const FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data){

}

void flac_metadata(const FLAC__StreamDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data){
	flac_file_t* flac = (flac_file_t*) client_data;
    switch (metadata->type)
    {
    case FLAC__METADATA_TYPE_STREAMINFO:
        flac->total_samples = metadata->data.stream_info.total_samples;
        flac->rate = metadata->data.stream_info.sample_rate;
        break;

    case FLAC__METADATA_TYPE_VORBIS_COMMENT:
        //flac->comments = FLAC__metadata_object_clone(metadata);
        break;
    default:
        break;
    }

}

FLAC__StreamDecoderWriteStatus flac_write(const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 * const buffer[], void *client_data){

	flac_file_t* flac = (flac_file_t*)client_data;

    int samples = frame->header.blocksize;
    int channels = frame->header.channels;
    int bits_per_sample = frame->header.bits_per_sample;
    int i, j;

    //Create a buffer to hold the entire frame
    resize_buffer(flac, channels, samples);

    for (i = 0; i < channels; i++)
        for (j = 0; j < samples; j++)
        	//Convert interger to float using fixed-point arithmetic where the scaller is equal to the 2 to the power of the bits per sample
            flac->buf[i][j] = buffer[i][j] / (float) (1 << (bits_per_sample - 1));

    flac->buffer_sample_count = samples;

	return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}


int read_flac_file (apr_pool_t* pool,void** file_struct, const char* file_path, encoding_options_t* enc_opt){
	FLAC__StreamDecoderInitStatus status;
	FLAC__bool status2;
	flac_file_t* flac_file = *file_struct = (flac_file_t*) apr_pcalloc(pool, sizeof(flac_file_t));

	flac_file->stream_decoder = FLAC__stream_decoder_new();
	flac_file->channels = 0;
	flac_file->samples = 0;
	flac_file->total_samples = 0;
	flac_file->rate = 0;
	flac_file->buf = NULL;

	status = FLAC__stream_decoder_init_file(flac_file->stream_decoder, file_path, flac_write, flac_metadata, flac_errors, flac_file);
	if (status != FLAC__STREAM_DECODER_INIT_STATUS_OK){
		return -1;
	}

	status2 = FLAC__stream_decoder_process_until_end_of_metadata(flac_file->stream_decoder);
	if (status2 != true){
		return -2;
	}
	FLAC__stream_decoder_process_single(flac_file->stream_decoder);
	if (status2 != true){
			return -2;
	}
	enc_opt->channels = flac_file->channels;
	enc_opt->rate = flac_file->rate;
	enc_opt->total_samples_per_channel = flac_file->total_samples;
	enc_opt->samples_to_request = 1024;

	return 0;
}

int close_flac(void* in){
	flac_file_t* flac_file = (flac_file_t*)in;
	int i;

    for (i = 0; i < flac_file->channels; i++){
        free(flac_file->buf[i]);
    }
    free(flac_file->buf);

    FLAC__stream_decoder_finish(flac_file->stream_decoder);
    FLAC__stream_decoder_delete(flac_file->stream_decoder);
    //free(flac);
    return 0;
}
