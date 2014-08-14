#include <apr.h>
#include <apr_pools.h>
#include <apr_shm.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include <time.h>

#include "util_shmem.h"

int setup_shared_memory(apr_shm_t** shm,apr_size_t size,const char* file_path, apr_pool_t* pool){
	apr_status_t rv;
	apr_uid_t uid = 0;
	apr_gid_t gid = 0;
	key_t shm_key;
	int shm_id;
	struct shmid_ds buf;
	time_t t;

	int status;


	apr_uid_get(&uid,&gid,"apache", pool);

	rv = apr_shm_create(shm, size,file_path , pool);
	if(rv == 17){
		//It already exsits lets kill it with fire
		rv = apr_shm_attach(shm,file_path , pool);
		rv = apr_shm_destroy(*shm);
		rv = apr_shm_create(shm, size,file_path , pool);
	}
	if(rv != APR_SUCCESS){

		*shm = NULL;
		return -1;
	}

	shm_key = ftok(file_path,1);
	shm_id = shmget(shm_key,size,IPC_CREAT | 0666);

	status = shmctl(shm_id, IPC_STAT, &buf);
	if(status != 0){
		return -2;
	}
	time(&t);
	buf.shm_ctime = t;
	buf.shm_perm.gid = uid;
	buf.shm_perm.uid = gid; 
	buf.shm_perm.mode = 0666;
	status = shmctl(shm_id, IPC_SET, &buf);
	if(status != 0){
		return -3;
	}

	status = chmod(file_path, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
	status = chown(file_path,uid,gid);

	return 0;
}
