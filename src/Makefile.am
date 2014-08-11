moddir=${AP_LIBEXECDIR}
mod_LTLIBRARIES = mod_mp.la

mod_mp_la_SOURCES = mod_mp.c \
	util_json.c \
	util_xml.c \
	error_messages.c \
	input.c \
	output.c \
	upload.c \
	database/db_manager.c \
	database/db_config.c \
	database/db_connection.c \
	database/db_object.c \
	database/db_config_xml.c \
	database/db_query.c \
	database/db_query_parameters.c \
	database/db_query_xml_config.c \
	indexers/indexer.c \
	indexers/index_manager.c

MUSIC_APP_LIB = indexers/file/libmp_indexer_file.la


mod_mp_la_CFLAGS = ${MODULE_CFLAGS}
mod_mp_la_LDFLAGS = -lpthread -lm -lrt -lssl -lcrypto -ldl -rpath ${AP_LIBEXECDIR} -module -avoid-version ${MODULE_LDFLAGS}
mod_mp_la_LIBADD = ${MUSIC_APP_LIB}


SUBDIRS = indexers/file 

install: install-am
	rm -f $(DESTDIR)${AP_LIBEXECDIR}/mod_mp.a
	rm -f $(DESTDIR)${AP_LIBEXECDIR}/mod_mp.la
