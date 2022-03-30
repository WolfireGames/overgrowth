#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>

#ifdef WIN32
#include <windows.h>
#endif

#include "binn_json.h"

/*************************************************************************************/

static const char hexdigits[] = {
  '0', '1', '2', '3', '4', '5', '6', '7',
  '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
};

void to_hex(char *source, int size, char *dest){
  char *end = source + size;
  for(; source<end; source++){
    unsigned char c = *source;
    *(dest++) = hexdigits[(c>>4)&0xf];
    *(dest++) = hexdigits[c&0xf];
  }
  *dest = 0;
}

/*************************************************************************************/

binn * APIENTRY json_obj_to_binn(json_t *base) {
  size_t  i, count;
  json_t  *value;
  const char  *key;
  binn  *obj, *list;

  switch (json_typeof(base)) {
  case JSON_OBJECT:
    obj = binn_object();
    json_object_foreach(base, key, value) {
      if (binn_object_set_new(obj, (char*)key, json_obj_to_binn(value)) == FALSE) { binn_free(obj); return NULL; }
    }
    return obj;

  case JSON_ARRAY:
    list = binn_list();
    count = json_array_size(base);
    for (i = 0; i < count; i++) {
      value = json_array_get(base, i);
      if (binn_list_add_new(list, json_obj_to_binn(value)) == FALSE) { binn_free(list); return NULL; }
    }
    return list;

  case JSON_STRING:
    return binn_string((char*)json_string_value(base), BINN_TRANSIENT);

  case JSON_INTEGER:
    return binn_int64(json_integer_value(base));

  case JSON_REAL:
    return binn_double(json_real_value(base));

  case JSON_TRUE:
    return binn_bool(TRUE);

  case JSON_FALSE:
    return binn_bool(FALSE);

  case JSON_NULL:
    return binn_null();

  default:
    return NULL;
  }

}

/*************************************************************************************/

BINN_PRIVATE json_t * binn_to_json_obj2(binn *base) {
  json_t    *value;
  json_int_t intvalue;
  binn_iter  iter;
  binn   binn_value={0};
  int    id, size;
  char   key[256], *ptr;
  double floatvalue;

  if (base == NULL) return NULL;

  switch (base->type) {
  case BINN_STRING:
  case BINN_DATE:
  case BINN_TIME:
  case BINN_DATETIME:
  case BINN_DECIMAL:
  case BINN_XML:
  case BINN_HTML:
  case BINN_CSS:
  case BINN_JSON:
  case BINN_JAVASCRIPT:
    value = json_string((char *)base->ptr);
    break;

  case BINN_BLOB:
    size = (base->size * 2) + 1;
    ptr = malloc(size);
    if (!ptr) return NULL;
    to_hex(base->ptr, base->size, ptr);
    value = json_string(ptr);
    free(ptr);
    break;

  case BINN_INT8:
    intvalue = base->vint8;
    goto loc_integer;
  case BINN_UINT8:
    intvalue = base->vuint8;
    goto loc_integer;
  case BINN_INT16:
    intvalue = base->vint16;
    goto loc_integer;
  case BINN_UINT16:
    intvalue = base->vuint16;
    goto loc_integer;
  case BINN_INT32:
    intvalue = base->vint32;
    goto loc_integer;
  case BINN_UINT32:
    intvalue = base->vuint32;
    goto loc_integer;
  case BINN_INT64:
    intvalue = base->vint64;
    goto loc_integer;
  case BINN_UINT64:
    intvalue = base->vuint64;
loc_integer:
    value = json_integer(intvalue);
    break;

  case BINN_BOOL:
    if (base->vbool)
      value = json_true();
    else
      value = json_false();
    break;

  case BINN_FLOAT:
    value = json_real(base->vfloat);
    break;
  case BINN_DOUBLE:
    value = json_real(base->vdouble);
    break;

  case BINN_CURRENCYSTR:
    ptr = (char *)base->ptr;
    floatvalue = atof(ptr);
    value = json_real(floatvalue);
    break;

  case BINN_OBJECT:
    value = json_object();
    binn_object_foreach(base, key, binn_value) {
      json_object_set_new(value, key, binn_to_json_obj2(&binn_value));
    }
    break;

  case BINN_MAP:
    value = json_object();
    binn_map_foreach(base, id, binn_value) {
#ifdef _MSC_VER
      itoa(id, key, 10);
#else
      snprintf(key, sizeof(key), "%d", id);
#endif
      json_object_set_new(value, key, binn_to_json_obj2(&binn_value));
    }
    break;

  case BINN_LIST:
    value = json_array();
    binn_list_foreach(base, binn_value) {
      json_array_append_new(value, binn_to_json_obj2(&binn_value));
    }
    break;

  case BINN_NULL:
    value = json_null();
    break;

  default:
    value = NULL;
    break;
  }

  return value;

}

/*************************************************************************************/

json_t * APIENTRY binn_to_json_obj(void *base) {
  binn item;

  if (binn_is_struct(base))
    return binn_to_json_obj2((binn*)base);

  binn_load(base, &item);
  return binn_to_json_obj2(&item);

}

/*************************************************************************************/

binn * APIENTRY json_to_binn(char *json_str) {
  json_t *base;
  //json_error_t error;
  binn *item;

  base = json_loads(json_str, 0, NULL);
  if (base == NULL) return FALSE;

  item = json_obj_to_binn(base);

  json_decref(base);
  return item;
}

/*************************************************************************************/

char * APIENTRY binn_to_json(void *base) {
  json_t *json; char *ptr;

  json = binn_to_json_obj(base);
  ptr = json_dumps(json, JSON_PRESERVE_ORDER);
  json_decref(json);

  return ptr;
}

/*************************************************************************************/

#ifdef JSON_JAVASCRIPT

char * APIENTRY binn_to_javascript(void *base) {
  json_t *json; char *ptr;

  json = binn_to_json_obj(base);
  ptr = json_dumps(json, JSON_PRESERVE_ORDER | JSON_JAVASCRIPT);
  json_decref(json);

  return ptr;
}

#endif

/*************************************************************************************/
