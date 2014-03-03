moddir=${AP_LIBEXECDIR}
mod_LTLIBRARIES = mod_mediaplayer.la

mod_mediaplayer_la_SOURCES = mod_mediaplayer.c \
							 error_handler.c \
							 upload.c \
							 input.c \
							 database/dbd.c \
							 database/db_config.c \
							 database/db_query_config.c \
							 database/db_query_parameters.c \
							 apps/app_config.c 

MUSIC_APP_LIB = apps/music/libmp_music.la


mod_mediaplayer_la_CFLAGS = -Wall ${MODULE_CFLAGS}
mod_mediaplayer_la_LDFLAGS = -lpthread -lm -lrt -lssl -lcrypto -ldl -rpath ${AP_LIBEXECDIR} -module -avoid-version ${MODULE_LDFLAGS}
mod_mediaplayer_la_LIBADD = ${MUSIC_APP_LIB}


SUBDIRS = apps/music

install: install-am
	rm -f $(DESTDIR)${AP_LIBEXECDIR}/mod_mediaplayer.a
	rm -f $(DESTDIR)${AP_LIBEXECDIR}/mod_mediaplayer.la
