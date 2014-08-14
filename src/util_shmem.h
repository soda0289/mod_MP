
#ifndef UTIL_SHMEM_
#define UTIL_SHMEM_
#include <apr.h>
#include <apr_pools.h>

int setup_shared_memory(apr_shm_t** shm,apr_size_t size,const char* file_path, apr_pool_t* pool);

#endif
