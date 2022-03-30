#include "../../src/binn.h"
#include "jansson.h"

char * APIENTRY binn_to_javascript(void *binn);

char * APIENTRY binn_to_json(void *binn);
binn * APIENTRY json_to_binn(char *json);

json_t * APIENTRY binn_to_json_obj(void *binn);
binn   * APIENTRY json_obj_to_binn(json_t *base);
