TESTS = check_error_messages

check_PROGRAMS = check_error_messages 
check_error_messages_SOURCES = error_messages_test.c
check_error_messages_CFLAGS = $(MODULE_CFLAGS) $(CHECK_CLAGS)
check_error_messages_LDFLAGS = -L../src/ $(MODULE_LDFLAGS) 
check_error_messages_LDADD = -lmp_error_messages -lmp_utils $(CHECK_LIBS)
