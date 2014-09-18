#adjust for the name and version of your 
AC_PREREQ([2.69])
AC_INIT([mod_MP],[0.0.1])
#adjust for your main source file name
AC_CONFIG_SRCDIR([src/libmod_mp.c])
AC_CONFIG_AUX_DIR([config])
AC_CONFIG_MACRO_DIR([m4])

LT_INIT
AM_MAINTAINER_MODE
AC_CANONICAL_TARGET
AM_INIT_AUTOMAKE()
#adjust here (should be obvious what to change)
AM_CONFIG_HEADER([src/mod_mp_config.h:config.in])

AC_PROG_CC
AC_PROG_CXX
LT_PATH_LD
AC_PROG_INSTALL



# Checks for header files.
AC_CHECK_HEADERS([inttypes.h stdlib.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_UINT64_T

# Checks for library functions.
AC_FUNC_CHOWN
AC_FUNC_MALLOC
AC_CHECK_FUNCS([memchr memset strchr strerror strstr])

PKG_CHECK_MODULES([CHECK], [check >= 0.9.4])
PKG_CHECK_MODULES([APR], [apr-2])
#Check for MusicBrainz
AC_ARG_WITH([musicbrainz], AS_HELP_STRING([--with-musicbrainz], ["Build with musicbrainz support"]))
AS_IF(test "x$with_musicbrainz" = "xyes"],[
	PKG_CHECK_MODULES([MUSICBRAINZ], 
					   [musicbrainz], 
					   WITH_MUSICBRAINZ = true \
					   AC_DEFINE([WITH_MUSICBRAINZ],1,["Compile with MusicBrainz support"])
					   PLUGINS_LDFLAGS = MUSICBRAINZ_LDFLAGS
					 )
	]
)
AM_CONDITIONAL([WITH_MUSICBRAINZ], $WITH_MUSICBRAINZ)

#Check for FLAC
AC_ARG_WITH([flac], AS_HELP_STRING([--with-flac], ["Build with the FLAC support"]))
AS_IF(test "x$with_flac" = "xyes"],[
	PKG_CHECK_MODULES([FLAC], 
					   [flac], 
					   WITH_FLAC = true \
					   AC_DEFINE([WITH_FLAC],1,["Compile with FLAC support"])
					   CODECS_LDFLAGS = FLAC_LDFLAGS
					 )
	]
)
AM_CONDITIONAL([WITH_FLAC], [test "xwith_flac" = "xyes"])

#Check for OGG/Vorbis
AC_ARG_WITH([ogg], AS_HELP_STRING([--with-ogg], ["Build with the OGG/Vorbis support"]))
AS_IF(test "x$with_ogg" = "xyes"],[
	PKG_CHECK_MODULES([OGG], 
					   [ogg vorbis], 
					   WITH_OGG = true \
					   AC_DEFINE([WITH_OGG],1,["Compile with OGG/Vorbis support"])
					   CODECS_LDFLAGS = OGG_LDFLAGS
					 )
	]
)
AM_CONDITIONAL([WITH_OGG], [test "xwith_ogg" = "xyes"])



#Check for MP3 (mpg123) 
AC_ARG_WITH([mp3], AS_HELP_STRING([--with-mp3], ["Build with the MP3 support"]))
AS_IF(test "x$with_mp3" = "xyes"],[
	PKG_CHECK_MODULES([MP3], 
					   [mpg123], 
					   WITH_MP3 = true \
					   AC_DEFINE([WITH_MP3],1,["Compile with MP3 support"])
					   CODECS_LDFLAGS = MP3_LDFLAGS
					 )
	]
)
AM_CONDITIONAL([WITH_MP3], [test "xwith_mp3" = "xyes"])


#if you are using a different version of apache, or don't want to require
#the latest, set this to something else.
AP_VERSION="2.2.4"
CHECK_APACHE(,[$AP_VERSION],
    :,:,
    AC_MSG_ERROR([*** Apache version $AP_VERSION not found!])
)
prefix=/usr
libdir=/usr/lib64
LIBTOOL="`${APR_CONFIG} --apr-libtool`"
AC_SUBST(LIBTOOL)


AP_MODULE_CFLAGS="${APXS_CFLAGS} ${AP_INCLUDES} -Werror -fno-inline"
AC_SUBST(AP_MODULE_CFLAGS)

AP_MODULE_LDFLAGS=" `${APR_CONFIG} --link-libtool` $APU_LDFLAGS"
AC_SUBST(AP_MODULE_LDFLAGS)

AP_BIN_LDFLAGS=" `${APR_CONFIG} --link-libtool`  `${APR_CONFIG} --ldflags --libs` $APU_LDFLAGS" 
AC_SUBST(AP_BIN_LDFLAGS)

#this should be a list to all of the makefiles you expect to be generated
AC_CONFIG_FILES([Makefile
		tests/Makefile
		src/Makefile
		src/indexers/file/Makefile
		src/indexers/music/Makefile
		src/indexers/music/codecs/flac/Makefile
		src/web_server/apache_httpd/Makefile
		])
AC_OUTPUT

#whatever you want here, or nothing
echo "---"
echo "Configuration summary for mod_MP"
echo ""
echo "   * Apache Modules Directory:    $AP_LIBEXECDIR"
echo ""
echo "---"
