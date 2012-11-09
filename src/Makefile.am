mod_mediaplayer_la_SOURCES = mod_mediaplayer.c mod_mediaplayer.h tag_reader.c tag_reader.h dbd.c dbd.h error_handler.c error_handler.h music_query.c music_query.h flac.c flac.h ogg_encode.c ogg_encode.h
mod_mediaplayer_la_CFLAGS = -Wall ${MODULE_CFLAGS}
mod_mediaplayer_la_LDFLAGS = -lpthread -lm -lrt -lssl -lcrypto -ldl -lFLAC -logg -lvorbisenc -lvorbis -rpath ${AP_LIBEXECDIR} -module -avoid-version ${MODULE_LDFLAGS}

mod_LTLIBRARIES = mod_mediaplayer.la
moddir=${AP_LIBEXECDIR}

install: install-am
	rm -f $(DESTDIR)${AP_LIBEXECDIR}/mod_mediaplayer.a
	rm -f $(DESTDIR)${AP_LIBEXECDIR}/mod_mediaplayer.la
