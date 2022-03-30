/*
This file shows the basics of using binn objects. It does not show lists and maps.
For more examples check https://github.com/liteserver/binn/blob/master/usage.md
*/

#include <binn.h>

/*
** option 1: use the raw buffer pointer
*/

void read_example_1(void *buf) {
  int id;
  char *name;
  double price;

  id = binn_object_int32(buf, "id");
  name = binn_object_str(buf, "name");
  price = binn_object_double(buf, "price");

}

/*
** option 2: load the raw buffer to a binn pointer
*/

void read_example_2(void *buf) {
  int id;
  char *name;
  double price;
  binn *obj;

  obj = binn_open(buf);
  if (obj == 0) return;

  id = binn_object_int32(obj, "id");
  name = binn_object_str(obj, "name");
  price = binn_object_double(obj, "price");

  binn_free(obj);  /* releases the binn pointer but NOT the received buf */

}

/*
** option 3: use the current created object
**
** this is used when both the writer and the reader are in the same app
*/

void read_example_3a(binn *obj) {
  int id;
  char *name;
  double price;

  id = binn_object_int32(obj, "id");
  name = binn_object_str(obj, "name");
  price = binn_object_double(obj, "price");

}

/* almost the same but in this case the binn pointer is returned from another function */

void read_example_3b() {
  int id;
  char *name;
  double price;
  binn *obj;

  obj = some_function();  /* this function should return a binn pointer */

  id = binn_object_int32(obj, "id");
  name = binn_object_str(obj, "name");
  price = binn_object_double(obj, "price");

  binn_free(obj);

}
