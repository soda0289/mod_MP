#include <apr.h>
#include <apr_strings.h>
#include "util_json.h"

char* json_escape_char(apr_pool_t* pool, const char* string){

	int i;
	char* escape_string;
	if(string == NULL){
		return NULL;
	}

	escape_string = apr_pstrdup(pool, string);

	for (i = 0; i < strlen(escape_string); i++){
		if (escape_string[i] == '"'){
			escape_string[i] = '\0';
			escape_string = apr_pstrcat(pool, &escape_string[0], "\\\"", &escape_string[++i], NULL);
			continue;
		}
		if (escape_string[i] == '\\'){
			escape_string[i] = '\0';
			escape_string = apr_pstrcat(pool, &escape_string[0], "\\\\", &escape_string[++i], NULL);
			continue;
		}
	}

	return escape_string;
}
