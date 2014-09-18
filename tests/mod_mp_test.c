#include <check.h>
#include <stdio.h>
#include "test_suites.h"


int main (int argc, char* argv[])
{
	int failed = 0;

	Suite *input = http_input_suite();
	SRunner *sr = srunner_create(input);

	srunner_run_all(sr, CK_NORMAL);
	failed = srunner_ntests_failed(sr);

	srunner_free(sr);
	
	printf("Failed: %d tests\n", failed);

	return (failed == 0) ? 0 : -1;
}
