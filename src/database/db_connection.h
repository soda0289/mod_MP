
#ifndef DB_CONNECTION_H
#define DB_CONNECTION_H

typedef struct db_conn_node_ db_conn_node_t;
typedef struct db_conn_list_ db_conn_list_t;

struct db_conn_node_ {
	db_connection_t* db_conn;

	db_conn_node_t* next;
	db_conn_node_t* prev;
};

struct db_conn_list_ {

	db_conn_node_t* head;
	db_conn_node_t* tail;
};

struct db_connection_ {

	apr_dbd_t *dbd_handle;

	db_config_t* db_config;

	//Thread lock - database can only send one query per proccess
	apr_thread_mutex_t* mutex;
};

int db_connection_init (db_config_t* db_config, const char* connection_params, db_connection_t** db_connection_ptr);
void db_connection_close(db_connection_t* db_conn);
#endif
