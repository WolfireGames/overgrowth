#include <stdio.h>
#include <stdlib.h>

#include "../conversions/json/binn_json.h"

void main() {
	char *json;
	binn *obj;
	void *list;

	//json = "{id: 1, name: 'testing this', value: 2.55, active: true, list: [12, 23, 34]}";
	json = "{'id': 1, 'name': 'testing this', 'value': 2.55, 'active': true, 'list': [12, 23, 34]}";
	json = "{\"id\": 1, \"name\": \"testing this\", \"value\": 2.55, \"active\": true, \"list\": [12, 23, 34]}";

	obj = json_to_binn(json);
	if (obj == INVALID_BINN) {
		puts("invalid obj returned");
		exit(1);
	}

	puts("successful conversion!");

	puts("reading...");

	printf("id: %d\n", binn_object_int32(obj, "id"));
	printf("name: %s\n", binn_object_str(obj, "name"));
	printf("value: %g\n", binn_object_double(obj, "value"));
	printf("active: %d\n", binn_object_bool(obj, "active"));

    list = binn_object_list(obj, "list");
	if (list == NULL) {
		puts("invalid list returned");
		exit(1);
	}

	puts("list items:");
	printf("\t1: %d\n", binn_list_int32(list, 1));
	printf("\t2: %d\n", binn_list_int32(list, 2));
	printf("\t3: %d\n", binn_list_int32(list, 3));

}
