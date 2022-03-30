/*
This file shows the basics of using binn objects. It does not show lists and maps.
For more examples check https://github.com/liteserver/binn/blob/master/usage.md
*/

#include <binn.h>

void create_and_pass_buffer() {
  binn *obj;

  // create a new object
  obj = binn_object();

  // add values to it
  binn_object_set_int32(obj, "id", 123);
  binn_object_set_str(obj, "name", "Samsung Galaxy");
  binn_object_set_double(obj, "price", 299.90);

  // pass the buffer to another function
  // send over the network or save to a file...
  another_function(binn_ptr(obj), binn_size(obj));

  // release the object
  binn_free(obj);

}

void create_and_pass_binn() {
  binn *obj;

  // create a new object
  obj = binn_object();

  // add values to it
  binn_object_set_int32(obj, "id", 123);
  binn_object_set_str(obj, "name", "Samsung Galaxy");
  binn_object_set_double(obj, "price", 299.90);

  // pass the binn pointer to another function
  another_function(obj);

  // release the object
  binn_free(obj);

}

binn * create_and_return_binn() {
  binn *obj;

  // create a new object
  obj = binn_object();

  // add values to it
  binn_object_set_int32(obj, "id", 123);
  binn_object_set_str(obj, "name", "Samsung Galaxy");
  binn_object_set_double(obj, "price", 299.90);

  // return the object
  return obj;

}

void * create_and_return_buffer() {
  binn *obj;

  // create a new object
  obj = binn_object();

  // add values to it
  binn_object_set_int32(obj, "id", 123);
  binn_object_set_str(obj, "name", "Samsung Galaxy");
  binn_object_set_double(obj, "price", 299.90);

  // release the binn structure but keeps the binn buffer allocated
  // returns the pointer to the buffer
  // must be released later with free() or the registered alloc/free function
  return binn_release(obj);

}
