
#ifndef MANAGER_H
#define MANAGER_H
#include "db_connection.h"

typedef struct db_config_node_ db_config_node_t;
typedef struct db_config_list_ db_config_list_t;


struct db_config_node_ {
	db_config_t* db_config;

	db_config_node_t* next;
	db_config_node_t* prev;
};

struct db_config_list_ {

	db_config_node_t* head;
	db_config_node_t* tail;
};

//Database object
struct db_manager_ {
	//Database connections
	db_config_list_t* db_config_list;

};


int db_manager_init (apr_pool_t* pool, error_messages_t* error_messages, const char* db_xml_config_dir, db_manager_t** db_manager_ptr);
int db_manager_on_fork (apr_pool_t* pool, error_messages_t* child_error_messages, db_manager_t** db_manager_ptr);
int db_manager_find_db (db_manager_t* db_manager, const char* db_id);
#endif
