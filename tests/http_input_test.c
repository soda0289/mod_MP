#include <check.h>
#include "test_suites.h"
#include "pool_allocator_setup.h"
#include "../src/input.h"

static void setup(void) {
	setup_pool();
}

static void teardown(void) {
	teardown_pool();
}

static int get_apr_table_nelts (apr_table_t* table) {
	apr_array_header_t* array_header;

	array_header = apr_table_elts(table);
	return (array_header != NULL) ? array_header->nelts : -1;
}

START_TEST(test_input_uri_null)
{
	int status = 0;
	input_t* input_test;

	status = input_init(pool, NULL, 0, &input_test);
	ck_assert(!status);
}
END_TEST

START_TEST(test_input_uri_indexer)
{
	int status = 0;
	input_t* input_test;

	status = input_init(pool, "/indexer_test", 0, &input_test);
	ck_assert(!status);

	ck_assert_str_eq(input_test->indexer_string, "indexer_test");
	ck_assert_ptr_eq(input_test->command_string, NULL);
	ck_assert_ptr_eq(input_test->parameter_strings, NULL);
}
END_TEST

START_TEST(test_input_uri_command)
{
	int status = 0;
	input_t* input_test;

	status = input_init(pool, "/indexer_test2/command_test", 0, &input_test);
	ck_assert(!status);

	ck_assert_str_eq(input_test->indexer_string, "indexer_test2");
	ck_assert_str_eq(input_test->command_string, "command_test");
	ck_assert_ptr_eq(input_test->parameter_strings, NULL);

	//input_destroy(input_test);

	status = input_init(pool, "/indexer_test2/command1/command2/command3", 0, &input_test);
	ck_assert(!status);

	ck_assert_str_eq(input_test->indexer_string, "indexer_test2");
	ck_assert_str_eq(input_test->command_string, "command1/command2/command3");
	ck_assert_ptr_eq(input_test->parameter_strings, NULL);
}
END_TEST

START_TEST(test_input_uri_one_parameter)
{
	int status = 0;
	input_t* input_test;

	status = input_init(pool, "/indexer_test3/command_test2?param=value", 0, &input_test);
	ck_assert(!status);

	ck_assert_str_eq(input_test->indexer_string, "indexer_test3");
	ck_assert_str_eq(input_test->command_string, "command_test2");

	ck_assert_ptr_ne(input_test->parameter_strings, NULL);
	ck_assert_str_eq(apr_table_get(input_test->parameter_strings, "param"), "value");
}
END_TEST

START_TEST(test_input_uri_one_parameter_no_value)
{
	int status = 0;
	int num_params;
	input_t* input_test;

	status = input_init(pool, "/indexer_test4/command_test3?one_param", 0, &input_test);
	ck_assert(!status);

	ck_assert_str_eq(input_test->indexer_string, "indexer_test4");
	ck_assert_str_eq(input_test->command_string, "command_test3");

	ck_assert_ptr_ne(input_test->parameter_strings, NULL);
	ck_assert_ptr_ne(apr_table_get(input_test->parameter_strings, "one_param"), NULL);
	ck_assert_str_eq(apr_table_get(input_test->parameter_strings, "one_param"), "");

	num_params = get_apr_table_nelts(input_test->parameter_strings);
	ck_assert_int_eq(num_params, 1);
}
END_TEST

START_TEST(test_input_uri_multi_parameter)
{
	int status = 0;
	int num_params;
	input_t* input_test;


	status = input_init(pool, "/indexer_test5/command_test5?param1=value1&param2=value2&param3&param4=value4&&param5", 0, &input_test);
	ck_assert(!status);

	ck_assert_str_eq(input_test->indexer_string, "indexer_test5");
	ck_assert_str_eq(input_test->command_string, "command_test5");

	ck_assert_ptr_ne(input_test->parameter_strings, NULL);
	ck_assert_str_eq(apr_table_get(input_test->parameter_strings, "param1"), "value1");
	ck_assert_str_eq(apr_table_get(input_test->parameter_strings, "param2"), "value2");
	ck_assert_str_eq(apr_table_get(input_test->parameter_strings, "param3"), "");
	ck_assert_str_eq(apr_table_get(input_test->parameter_strings, "param4"), "value4");
	ck_assert_str_eq(apr_table_get(input_test->parameter_strings, "param5"), "");

	num_params = get_apr_table_nelts(input_test->parameter_strings);
	ck_assert_int_eq(num_params, 5);
}
END_TEST

Suite * http_input_suite (void) {
	Suite *s = suite_create("HTTP URI Input");

	TCase *tc_uri;

	tc_uri = tcase_create("GET request URI");
	tcase_add_checked_fixture(tc_uri, setup, teardown);
	tcase_add_test(tc_uri, test_input_uri_null);
	tcase_add_test(tc_uri, test_input_uri_indexer);
	tcase_add_test(tc_uri, test_input_uri_command);
	tcase_add_test(tc_uri, test_input_uri_one_parameter);
	tcase_add_test(tc_uri, test_input_uri_one_parameter_no_value);
	tcase_add_test(tc_uri, test_input_uri_multi_parameter);

	suite_add_tcase(s, tc_uri);

	return s;
}
