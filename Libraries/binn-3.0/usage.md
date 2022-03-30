Binn Interface & Usage
======================

### Containers

There are 3 types of containers:

* Lists
* Maps
* Objects

Lists
-----

The values are stored in a sequential order:

> [2, 5, 9.1, "value", true]

#### Writing

```c
binn *list;

// create a new list
list = binn_list();

// add values to it
binn_list_add_int32(list, 123);
binn_list_add_double(list, 2.55);
binn_list_add_str(list, "testing");

// send over the network or save to a file...
send(sock, binn_ptr(list), binn_size(list));

// release the buffer
binn_free(list);
```

#### Reading by position

```c
int id;
double rate;
char *name;

id = binn_list_int32(list, 1);
rate = binn_list_double(list, 2);
name = binn_list_str(list, 3);
```

#### Reading values of the same type

```c
int i, count;
double note;

count = binn_count(list);
for(i=1; i<=count; i++) {
  note = binn_list_double(list, i);
}
```

#### Reading using for each

```c
binn_iter iter;
binn value;

binn_list_foreach(list, value) {
  do_something(&value);
}
```

Maps
----

The values are stored with integer keys:

> {2: "test", 5: 2.5, 10: true}

You can define the integer keys in a header file shared between the applications that will use the map:

```c
#define USER_ID    11
#define USER_NAME  12
#define USER_VALUE 13
```

#### Writing

```c
binn *map;

// create a new map
map = binn_map();

// add values to it
binn_map_set_int32(map, USER_ID, 123);
binn_map_set_str(map, USER_NAME, "John");
binn_map_set_double(map, USER_VALUE, 2.55);

// send over the network or save to a file...
send(sock, binn_ptr(map), binn_size(map));

// release the buffer
binn_free(map);
```

#### Reading by key

```c
int id;
char *name;
double note;

id = binn_map_int32(map, USER_ID);
name = binn_map_str(map, USER_NAME);
note = binn_map_double(map, USER_VALUE);
```

#### Reading sequentially

```c
binn_iter iter;
binn value;
int id;

binn_map_foreach(map, id, value) {
  do_something(id, &value);
}
```

Objects
-------

The values are stored with string keys:

> {"name": "John", "grade": 8.5, "active": true}

#### Writing

```c
binn *obj;

// create a new object
obj = binn_object();

// add values to it
binn_object_set_int32(obj, "id", 123);
binn_object_set_str(obj, "name", "John");
binn_object_set_double(obj, "total", 2.55);

// send over the network or save to a file...
send(sock, binn_ptr(obj), binn_size(obj));

// release the buffer
binn_free(obj);
```

#### Reading by key

```c
int id;
char *name;
double total;

id = binn_object_int32(obj, "id");
name = binn_object_str(obj, "name");
total = binn_object_double(obj, "total");
```

#### Reading sequentially

```c
binn_iter iter;
binn value;
char key[256];

binn_object_foreach(obj, key, value) {
  do_something(key, &value);
}
```

---

## Nested Structures

We can put containers inside others (a list inside of an object, a list of objects, a list of maps...)

### Example 1: A list inside of an object

> {id: 123, name: "John", values: [2.5, 7.35, 9.15]}

#### Writing

```c
binn *obj, *list;

// create a new object
obj = binn_object();

// add values to it
binn_object_set_int32(obj, "id", 123);
binn_object_set_str(obj, "name", "John");

// create a new list
list = binn_list();
binn_list_add_double(list, 2.50);
binn_list_add_double(list, 7.35);
binn_list_add_double(list, 9.15);
binn_object_set_list(obj, "values", list);
binn_free(list);

// send over the network or save to a file...
send(sock, binn_ptr(obj), binn_size(obj));

// release the memory
binn_free(obj);
```

#### Reading

```c
int id, i, count;
char *name;
void *list;
double grade;

id = binn_object_int32(obj, "id");
name = binn_object_str(obj, "name");
list = binn_object_list(obj, "values");

count = binn_count(list);
for(i=1; i<=count; i++) {
  grade = binn_list_double(list, i);
}
```

### Example 2: A list of objects

> [ {name: "John", email: "john@gmail.com"} , {name: "Eric", email: "eric@gmail.com"} ]

#### Writing

```c
binn *list, *obj;

// create a new list
list = binn_list();

// create the first object
obj = binn_object();
binn_object_set_str(obj, "name", "John");
binn_object_set_str(obj, "email", "john@gmail.com");
// add to the list and discard
binn_list_add_object(list, obj);
binn_free(obj);

// create the second object
obj = binn_object();
binn_object_set_str(obj, "name", "Eric");
binn_object_set_str(obj, "email", "eric@gmail.com");
// add to the list and discard
binn_list_add_object(list, obj);
binn_free(obj);

// send over the network or save to a file...
send(sock, binn_ptr(list), binn_size(list));

// release the memory
binn_free(list);
```

#### Reading

```c
int i, count;
void *obj;
char *name, *email;

count = binn_count(list);
for(i=1; i<=count; i++) {
  obj = binn_list_object(list, i);
  name  = binn_object_str(obj, "name");
  email = binn_object_str(obj, "email");
}
```

#### Note

When read, the internal containers are returned as static pointers that should not be "freed".

---

### Binn Values

Some functions return a structure called binn value. Here is an example of dealing with them:

```c
void print_value(binn *value) {
  switch (value->type) {
  case BINN_INT32:
    printf("integer: %d\n", value->vint32);
    break;
  case BINN_STRING:
    printf("string: %s\n", value->ptr);
    break;
  ...
  }
}
```
