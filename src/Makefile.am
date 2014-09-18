
noinst_LTLIBRARIES = libmp_error_messages.la libmp_utils.la

libmp_utils_la_SOURCES = util_json.c \
			util_xml.c \
			util_shmem.c
libmp_utils_la_CFLAGS = $(APR_CFLAGS) -fPIC
libmp_utils_la_LDFLAGS = ${APR_LDFLAGS}
libmp_utils_la_LIBADD = ${APR_LIBS}

libmp_error_messages_la_SOURCES = error_messages.c
libmp_error_messages_la_CFLAGS = $(APR_CFLAGS) -fPIC
libmp_error_messages_la_LDFLAGS = ${APR_LDFLAGS}


lib_LTLIBRARIES = libmod_mp.la

libmod_mp_la_SOURCES = libmod_mp.c \
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

INDEXER_LIBS = indexers/file/libmp_indexer_file.la


libmod_mp_la_CFLAGS = ${APR_CFLAGS}
libmod_mp_la_LDFLAGS = ${APR_LDFLAGS}
libmod_mp_la_LIBADD = libmp_utils.la libmp_error_messages.la ${INDEXER_LIBS}


SUBDIRS = indexers/file 

install: install-am
	rm -f $(DESTDIR)${AP_LIBEXECDIR}/mod_mp.a
	rm -f $(DESTDIR)${AP_LIBEXECDIR}/mod_mp.la
