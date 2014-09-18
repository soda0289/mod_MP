#include <apr_pools.h>
#include "pool_allocator_setup.h"

//Globals used for test cases
apr_allocator_t* allocator;
apr_pool_t* pool;

static int
abort_alloc (int retcode)
{
	return -1;
}

int setup_pool (void)
{
	int status;
	status = apr_allocator_create(&allocator);
	if (status != 0) {
		return status;
	}
	status = apr_pool_create_ex (&pool, NULL, abort_alloc, allocator);

	return status;
}

void teardown_pool (void)
{
	apr_pool_destroy(pool);
	apr_allocator_destroy(allocator);
}
