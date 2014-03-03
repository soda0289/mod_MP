
#include "FLAC/metadata.h"
#include "FLAC/stream_decoder.h"

void find_vorbis_comment_entry(apr_pool_t*pool, FLAC__StreamMetadata *block, char* entry, char** feild){
	FLAC__StreamMetadata_VorbisComment       *comment;
	FLAC__StreamMetadata_VorbisComment_Entry *comment_entry;
	char* comment_value;
	int comment_length;
	int offset;

	comment = &block->data.vorbis_comment;


	offset = 0;
	while ( (offset = FLAC__metadata_object_vorbiscomment_find_entry_from(block, offset, entry)) >= 0 ){
		comment_entry = &comment->comments[offset++];
		comment_value = memchr(comment_entry->entry, '=',comment_entry->length);

		if (comment_value){
			//Remove equal sign form value
			comment_value++;
			if(*feild == NULL){
				comment_length = comment_entry->length - (comment_value - (char*)comment_entry->entry);
				*feild = apr_pstrmemdup(pool, comment_value, comment_length);
			}else{
				//make a comma separated list
			}
		}
	}
}

int read_flac_level1(apr_pool_t* pool, music_file* song){
	FLAC__Metadata_SimpleIterator *iter;
	const char* file_path;

	if (song == NULL){
		return 1;
	}
	file_path = song->file->path;
	if (!file_path){
		return 1;
	}
	 iter = FLAC__metadata_simple_iterator_new();

	 if (iter == NULL || !FLAC__metadata_simple_iterator_init(iter, file_path, true, false)){
		 //ap_rprintf(r, "Error creating FLAC interator\n");
		 FLAC__metadata_simple_iterator_delete(iter);
		 return 1;
	 }
	 //Process the current metadata block
	 do{
		 // Get block of metadata
		 FLAC__StreamMetadata *block = FLAC__metadata_simple_iterator_get_block(iter);

		 //Check what type of block we have Picture, Comment
		 switch ( FLAC__metadata_simple_iterator_get_block_type(iter) ){
			 case FLAC__METADATA_TYPE_VORBIS_COMMENT:{
				 find_vorbis_comment_entry(pool, block, "TITLE", &song->title);
				 find_vorbis_comment_entry(pool, block, "ARTIST", &song->artist);
				 find_vorbis_comment_entry(pool, block, "ALBUM", &song->album);
				 find_vorbis_comment_entry(pool, block, "TRACKNUMBER", &song->track_no);
				 find_vorbis_comment_entry(pool, block, "DISCNUMBER", &song->disc_no);
				 find_vorbis_comment_entry(pool, block, "MUSICBRAINZ_ALBUMID", &song->mb_id.mb_release_id);
				 find_vorbis_comment_entry(pool, block, "MUSICBRAINZ_ALBUMARTISTID", &song->mb_id.mb_albumartist_id);
				 find_vorbis_comment_entry(pool, block, "MUSICBRAINZ_ARTISTID", &song->mb_id.mb_artist_id);
				 find_vorbis_comment_entry(pool, block, "MUSICBRAINZ_TRACKID", &song->mb_id.mb_recoriding_id);
			 } break;
			 case FLAC__METADATA_TYPE_STREAMINFO :{
				 FLAC__StreamMetadata_StreamInfo* stream_info;
				 stream_info = (FLAC__StreamMetadata_StreamInfo*)&(block->data);
				 song->length = stream_info->total_samples / stream_info->sample_rate;
			 } break;
			 case FLAC__METADATA_TYPE_UNDEFINED :{
			 } break;
			 case FLAC__METADATA_TYPE_PICTURE :{
			 } break;
			 case FLAC__METADATA_TYPE_PADDING :{
			 } break;
			 case FLAC__METADATA_TYPE_CUESHEET :{
			 } break;
			 case FLAC__METADATA_TYPE_SEEKTABLE :{
			 } break;
			 case FLAC__METADATA_TYPE_APPLICATION :{
			 } break;
	     }

		 FLAC__metadata_object_delete(block);

	 }while (FLAC__metadata_simple_iterator_next(iter)); //Get next block if not null
	 FLAC__metadata_simple_iterator_delete(iter);

	 if (song->title == NULL){
		 return -1;
	 }

	return 0;
}
