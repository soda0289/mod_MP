
#ifndef DB_OBJECT_H
#define DB_OBJECT_H

enum db_obj_value_type {
	VALUE_STRING,
	VAULE_FLOAT,
	VALUE_INT,
	VALUE_REF_OBJECT,
	VALUE_NUM
};

struct db_obj_value_ {
	enum db_obj_value_type type;
	void* data;
	int multi;
};

struct db_object_ {
	const char* name;

	apr_array_header_t* value_array;
	
};

#endif
