mod_mediaplayer_la_SOURCES = mod_mediaplayer.c \
							 apps/music/tag_reader.c \
							 database/dbd.c \
							 database/db_query_config.c \
							 error_handler.c \
							 apps/music/music_query.c \
							 apps/music/flac.c \
							 apps/music/ogg_encode.c \
							 apps/music/dir_sync/dir_sync.c \
							 apps/app_config.c \
							 apps/music/transcoder.c \
							 database/db_query_parameters.c \
							 apps/music/decoding_queue.c \
							 apps/music/dir_sync/musicbrainz.c \
							 apps/music/mpg123.c \
							 database/db_config.c \
							 upload.c \
							 input.c

mod_mediaplayer_la_CFLAGS = -Wall ${MODULE_CFLAGS}
mod_mediaplayer_la_LDFLAGS = -lmusicbrainz5 -lpthread -lm -lrt -lssl -lcrypto -ldl -lFLAC -logg -lvorbisenc -lvorbis -lvorbisfile -lmpg123 -rpath ${AP_LIBEXECDIR} -module -avoid-version ${MODULE_LDFLAGS}

mod_LTLIBRARIES = mod_mediaplayer.la
moddir=${AP_LIBEXECDIR}

install: install-am
	rm -f $(DESTDIR)${AP_LIBEXECDIR}/mod_mediaplayer.a
	rm -f $(DESTDIR)${AP_LIBEXECDIR}/mod_mediaplayer.la
