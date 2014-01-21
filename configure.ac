#adjust for the name and version of your 
AC_PREREQ([2.69])
AC_INIT([mod_mediaplayer],[0.0.1])
#adjust for your main source file name
AC_CONFIG_SRCDIR([src/mod_mediaplayer.c])
AC_CONFIG_AUX_DIR([config])
AC_CONFIG_MACRO_DIR([m4])
LT_INIT
AM_MAINTAINER_MODE
AC_CANONICAL_TARGET
AM_INIT_AUTOMAKE()
#adjust here (should be obvious what to change)
AM_CONFIG_HEADER([src/mod_mediaplayer_config.h:config.in])

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
AC_CHECK_FUNCS([memchr memset strchr strerror])




#if you are using a different version of apache, or don't want to require
#the latest, set this to something else.
AP_VERSION="2.2.4"
CHECK_APACHE([1.3],[$AP_VERSION],
    :,:,
    AC_MSG_ERROR([*** Apache version $AP_VERSION not found!])
)

prefix=${AP_PREFIX}

LIBTOOL="`${APR_CONFIG} --apr-libtool`"
AC_SUBST(LIBTOOL)

MODULE_CFLAGS="${APXS_CFLAGS} ${AP_INCLUDES} ${APR_INCLUDES} ${APU_INCLUDES}"
AC_SUBST(MODULE_CFLAGS)

MODULE_LDFLAGS=" `${APR_CONFIG} --link-libtool` `${APU_CONFIG} --link-libtool`"
AC_SUBST(MODULE_LDFLAGS)

BIN_LDFLAGS=" `${APR_CONFIG} --link-libtool` `${APU_CONFIG} --link-libtool` `${APU_CONFIG} --ldflags --libs` `${APR_CONFIG} --ldflags --libs`" 
AC_SUBST(BIN_LDFLAGS)

dnl this should be a list to all of the makefiles you expect to be generated
AC_CONFIG_FILES([Makefile
				src/Makefile
				tools/Makefile])
AC_OUTPUT

dnl whatever you want here, or nothing
echo "---"
echo "Configuration summary for mod_mediaplayer"
echo ""
echo "   * Apache Modules Directory:    $AP_LIBEXECDIR"
echo ""
echo "---"
