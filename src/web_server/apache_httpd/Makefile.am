
moddir=${AP_LIBEXECDIR}

mod_LTLIBRARIES = mod_mp.la

mod_mp_la_SOURCES = mod_mp.c

mod_mp_la_CFLAGS = ${MODULE_CFLAGS}
mod_mp_la_LDFLAGS = -L../../ -lpthread -lm -lrt -lssl -lcrypto -ldl -rpath ${AP_LIBEXECDIR} -module -avoid-version ${MODULE_LDFLAGS}
mod_mp_la_LIBADD = ../../.libs/libmod_mp.so
