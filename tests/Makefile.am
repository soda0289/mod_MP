TESTS = check_error_messages check_mod_mp

check_PROGRAMS = check_error_messages \
		 check_mod_mp

check_error_messages_SOURCES = error_messages_test.c \
			       pool_allocator_setup.c 

check_error_messages_CFLAGS = $(APR_CFLAGS) $(CHECK_CLAGS)
check_error_messages_LDFLAGS = -L../src/ $(APR_LDFLAGS) 
check_error_messages_LDADD = -lmp_error_messages -lmp_utils $(CHECK_LIBS)

check_mod_mp_SOURCES = mod_mp_test.c \
		       http_input_test.c \
		       pool_allocator_setup.c 

check_mod_mp_CFLAGS = $(APR_CFLAGS) $(CHECK_CLAGS)
check_mod_mp_LDFLAGS = -L../src/ -L../src/.libs/ $(MODULE_LDFLAGS)
check_mod_mp_LDADD = -lmod_mp $(CHECK_LIBS)
