noinst_LTLIBRARIES = libmp_music.la

libmp_music_la_SOURCES = /music_query.c \
  			 tag_reader.c \
			 transcoder.c \
			 decoding_queue.c \
			 pull_song.c \
			 dir_sync/dir_sync.c

if WITH_FLAC
FLAC_DIRS = codecs/flac
FLAC_LIB = codecs/flac/libmp_codec_flash.la
endif

if WITH_OGG

endif

if WITH_MP3

endif

libmp_music_la_CFLAGS = -Wall ${MODULE_CFLAGS}
libmp_music_la_LDFLAGS = ${MODULE_LDFLAGS} 
libmp_music_la_LIBADD = ${FLAC_LIB}

if WITH_MUSICBRAINZ
EXTRA_libmp_music_la_SOURCES = dir_sync/musicbrainz.c
endif

SUBDIRS = ${FLAC_DIRS}
