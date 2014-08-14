#include <check.h>
#include <stdlib.h>
#include "../src/error_messages.h"



static error_messages_t* em_test;
static apr_allocator_t* allocator;
static apr_pool_t* pool;

static int
abort_alloc (int retcode)
{
	return -1;
}

static int
setup_pool (void)
{
	int status;
	status = apr_allocator_create(&allocator);
	if (status != 0) {
		return status;
	}
	status = apr_pool_create_ex (&pool, NULL, abort_alloc, allocator);
	return status;
}

static void
setup (void)
{
	int status;
	setup_pool();

	status = error_messages_init (pool, &em_test);
	if (status != 0){
		em_test = NULL;
	}
}

static void
setup_shmem (void)
{
	int status;
	setup_pool();

	status = error_messages_init_shared (pool, "/tmp/mp_error_messages", &em_test);
	if (status != 0) {
		em_test = NULL;
	}
}

static void
teardown (void)
{
	apr_pool_destroy(pool);
	apr_allocator_destroy(allocator);
}

START_TEST(test_error_messages_add)
{
	int status = 0;
	ck_assert(em_test);
	ck_assert(status == 0);
	
	status = error_messages_add (em_test, ERROR, "Test Header", "Test error message");
	ck_assert(status == 0);

	ck_assert_str_eq ("Test Header", em_test->messages[0].header);
	ck_assert_str_eq ("Test error message", em_test->messages[0].message);
}
END_TEST

START_TEST(test_error_messages_addf)
{
	int status = 0;
	ck_assert(em_test);
	
	status = error_messages_addf (em_test, ERROR, "Test Header", "Test error message format");
	ck_assert(status == 0);

	ck_assert_str_eq ("Test Header", em_test->messages[0].header);
	ck_assert_str_eq ("Test error message format", em_test->messages[0].message);
}
END_TEST

START_TEST(test_error_messages_addf_1_string)
{
	int status = 0;
	ck_assert(em_test);
	
	status = error_messages_addf (em_test, ERROR, "Test Header", "Test error message format %s", "string");
	ck_assert(status == 0);

	ck_assert_str_eq ("Test Header", em_test->messages[0].header);
	ck_assert_str_eq ("Test error message format string", em_test->messages[0].message);
}
END_TEST

static Suite *
error_messages_suite (void) {
	Suite *s = suite_create("Error Messages");

	TCase *tc_add;
	TCase *tc_add_shmem;

	tc_add = tcase_create("Add");
	tcase_add_checked_fixture(tc_add, setup, teardown);
	tcase_add_test(tc_add, test_error_messages_add);
	tcase_add_test(tc_add, test_error_messages_addf);
	tcase_add_test(tc_add, test_error_messages_addf_1_string);
	
	suite_add_tcase(s, tc_add);

	tc_add_shmem = tcase_create("Add Shared Memory");
	tcase_add_checked_fixture(tc_add_shmem, setup_shmem, teardown);
	tcase_add_test(tc_add_shmem, test_error_messages_add);
	tcase_add_test(tc_add_shmem, test_error_messages_addf);
	tcase_add_test(tc_add_shmem, test_error_messages_addf_1_string);

	suite_add_tcase(s, tc_add_shmem);

	return s;
}

int
main (int argc, char* argv[])
{
	int failed = 0;

	Suite *s = error_messages_suite();
	SRunner *sr = srunner_create(s);

	srunner_run_all(sr, CK_NORMAL);

	srunner_free(sr);
	
	printf("Failed: %d tests\n", failed);

	return 0;
}
