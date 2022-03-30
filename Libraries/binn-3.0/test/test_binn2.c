#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <math.h>  /* for fabs */
#include <assert.h>
#include "../src/binn.h"

#ifdef _MSC_VER
#define snprintf _snprintf
#endif

int MY_DATE;
int MY_CURRENCY;

char tmp[128];

void * memdup(void *src, int size);

char * i64toa(int64 val, char *buf, int radix);
#ifdef _MSC_VER
#define atoi64 _atoi64
#else
int64  atoi64(char *str);
#endif

BOOL AlmostEqualFloats(float A, float B, int maxUlps);

/*************************************************************************************/

int vint32; unsigned int vuint32;
int64 vint64; uint64 vuint64;
short vint16; unsigned short vuint16;
signed char vint8; unsigned char vuint8;
float vfloat32;
double vfloat64;
BOOL vbool;

/*************************************************************************************/

char * stripchr(char *mainstr, int separator) {
  char *ptr;

  if (mainstr == NULL) return NULL;

  ptr = strchr(mainstr, separator);
  if (ptr == 0) return NULL;
  ptr[0] = '\0';
  ptr++;
  return ptr;

}

/*************************************************************************************/
// MY_DATE
// day:   1-31 -> 2^5 -> 5 bits
// month: 1-12 -> 2^4 -> 4 bits
// year:  16 - 9 bits = 7 bits -> 2^7=128 - from 1900 to 2028

unsigned short str_to_date(char *datestr) {
  unsigned short date;
  int day, month, year;
  char *next;

  if (datestr == NULL) return 0;
  strcpy(tmp, datestr);
  datestr = tmp;

  next = stripchr(datestr, '-');
  year = atoi(datestr) - 1900;

  datestr = next;
  next = stripchr(datestr, '-');
  month = atoi(datestr);

  day = atoi(next);

  date = (day << 11) | (month << 7) | year;
  return date;

}

/*************************************************************************************/

char * date_to_str(unsigned short date) {
  int day, month, year;
  //char *datestr;

  day =   ((date & 0xf800) >> 11);
  month = ((date & 0x0780) >> 7);
  year =   (date & 0x007f);

  sprintf(tmp, "%.4d-%.2d-%.2d", year + 1900 , month, day);

  return tmp;

}

/*************************************************************************************/
// MY_CURRENCY
// 00000000.0000  <- fixed point

#define CURRENCY_DECIMAL_DIGITS   4
#define CURRENCY_DECIMAL_DIGITS_STR  "0000"
#define CURRENCY_FACTOR       10000

int64 str_to_currency(char *str) {
  char *next;
  int size, i;

  if (str == NULL) return 0;
  strcpy(tmp, str);
  str = tmp;

  next = strchr(str, '.');
  if (next) {
    size = strlen(next+1);
    memmove(next, next+1, size+1); // include the null terminator
    if (size <= CURRENCY_DECIMAL_DIGITS) {
      size = CURRENCY_DECIMAL_DIGITS - size;
      for (i=0; i<size; i++) strcat(str, "0");
    } else {
      next[CURRENCY_DECIMAL_DIGITS] = 0;  // puts a null terminator
    }
  } else {
    strcat(str, CURRENCY_DECIMAL_DIGITS_STR);
  }

  return atoi64(str);

}

/*************************************************************************************/

char * currency_to_str(int64 value) {
  char *str, *ptr;
  int size, move, i;

  i64toa(value, tmp, 10);
  str = tmp;

  size = strlen(str);
  if (size > CURRENCY_DECIMAL_DIGITS) {
    ptr = str + size - CURRENCY_DECIMAL_DIGITS;
    memmove(ptr+1, ptr, CURRENCY_DECIMAL_DIGITS+1);  // include the null terminator
    ptr[0] = '.';            // include the point
  } else {
    move = 2 + CURRENCY_DECIMAL_DIGITS - size;
    memmove(str+move, str, size+1);  // include the null terminator
    str[0] = '0';            // include the zero
    str[1] = '.';            // include the point
    for (i=2; i<move; i++) str[i] = '0';
  }

  return str;
}

/*************************************************************************************/

int64 float_to_currency(double value) {
  char buf[128];

  snprintf(buf, 127, "%.4f", value);

  return str_to_currency(buf);

}

/*************************************************************************************/

double currency_to_float(int64 value) {

  currency_to_str(value);

  return atof(tmp);

}

/*************************************************************************************/

int64 mul_currency(int64 value1, int64 value2) {
  return value1 * value2 / CURRENCY_FACTOR;
}

/*************************************************************************************/

int64 div_currency(int64 value1, int64 value2) {
  return value1 * CURRENCY_FACTOR / value2;
}

/*************************************************************************************/
/*
//! this code may not work on processors that does not use the default float standard
//  original name: AlmostEqual2sComplement
BOOL AlmostEqualFloats(float A, float B, int maxUlps) {
  int aInt, bInt, intDiff;
  // Make sure maxUlps is non-negative and small enough that the
  // default NAN won't compare as equal to anything.
  assert(maxUlps > 0 && maxUlps < 4 * 1024 * 1024);
  aInt = *(int*)&A;
  bInt = *(int*)&B;
  // Make aInt lexicographically ordered as a twos-complement int
  if (aInt < 0) aInt = 0x80000000 - aInt;
  if (bInt < 0) bInt = 0x80000000 - bInt;
  intDiff = abs(aInt - bInt);
  if (intDiff <= maxUlps) return TRUE;
  return FALSE;
}
*/
/*************************************************************************************/

char * test_create_object_1(int *psize) {
  binn *obj=INVALID_BINN, *list=INVALID_BINN;

  printf("creating object 1...\n");

  obj = binn_object();
  assert(obj != NULL);

  vint32 = -12345;
  assert(binn_object_set(obj, "int32", BINN_INT32, &vint32, 0) == TRUE);
  vint16 = -258;
  assert(binn_object_set(obj, "int16", BINN_INT16, &vint16, 0) == TRUE);
  vint8 = -120;
  assert(binn_object_set(obj, "int8", BINN_INT8, &vint8, 0) == TRUE);
  vint64 = -1234567890123;
  assert(binn_object_set(obj, "int64", BINN_INT64, &vint64, 0) == TRUE);

  vuint32 = 123456;
  assert(binn_object_set(obj, "uint32", BINN_UINT32, &vuint32, 0) == TRUE);
  vuint16 = 60500;
  assert(binn_object_set(obj, "uint16", BINN_UINT16, &vuint16, 0) == TRUE);
  vuint8 = 250;
  assert(binn_object_set(obj, "uint8", BINN_UINT8, &vuint8, 0) == TRUE);
  vuint64 = 1234567890123;
  assert(binn_object_set(obj, "uint64", BINN_UINT64, &vuint64, 0) == TRUE);

  vfloat32 = -12.345;
  assert(binn_object_set(obj, "float32", BINN_FLOAT32, &vfloat32, 0) == TRUE);
  vfloat32 = -12.345;
  assert(binn_object_set(obj, "single", BINN_SINGLE, &vfloat32, 0) == TRUE);
  vfloat64 = -123456.7895;
  assert(binn_object_set(obj, "float64", BINN_FLOAT64, &vfloat64, 0) == TRUE);
  vfloat64 = -123456.7895;
  assert(binn_object_set(obj, "double", BINN_DOUBLE, &vfloat64, 0) == TRUE);

  assert(binn_object_set(obj, "str", BINN_STRING, "the value", 0) == TRUE);

  vint32 = TRUE;
  assert(binn_object_set(obj, "bool_true", BINN_BOOL, &vint32, 0) == TRUE);
  vint32 = FALSE;
  assert(binn_object_set(obj, "bool_false", BINN_BOOL, &vint32, 0) == TRUE);

  assert(binn_object_set(obj, "null", BINN_NULL, NULL, 0) == TRUE);


  // add a list

  list = binn_list();
  assert(list != NULL);

  assert(binn_list_add(list, BINN_NULL, NULL, 0) == TRUE);
  vint32 = 123;
  assert(binn_list_add(list, BINN_INT32, &vint32, 0) == TRUE);
  assert(binn_list_add(list, BINN_STRING, "this is a string", 0) == TRUE);

  assert(binn_object_set(obj, "list", BINN_LIST, binn_ptr(list), binn_size(list)) == TRUE);

  binn_free(list); list = INVALID_BINN;


  // return the values

  *psize = binn_size(obj);
  return (char *) binn_ptr(obj);

}

/*************************************************************************************/

char * test_create_object_2(int *psize) {
  binn *obj=INVALID_BINN, *list=INVALID_BINN;

  printf("creating object 2...\n");

  obj = binn_object();
  assert(obj != NULL);

  assert(binn_object_set_int32(obj, "int32", -12345) == TRUE);
  assert(binn_object_set_int16(obj, "int16", -258) == TRUE);
  assert(binn_object_set_int8(obj, "int8", -120) == TRUE);
  assert(binn_object_set_int64(obj, "int64", -1234567890123) == TRUE);

  assert(binn_object_set_uint32(obj, "uint32", 123456) == TRUE);
  assert(binn_object_set_int16(obj, "uint16", 60500) == TRUE);
  assert(binn_object_set_int8(obj, "uint8", 250) == TRUE);
  assert(binn_object_set_uint64(obj, "uint64", 1234567890123) == TRUE);

  assert(binn_object_set_float(obj, "float32", -12.345) == TRUE);
  vfloat32 = -12.345;
  assert(binn_object_set(obj, "single", BINN_SINGLE, &vfloat32, 0) == TRUE);
  assert(binn_object_set_double(obj, "float64", -123456.7895) == TRUE);
  vfloat64 = -123456.7895;
  assert(binn_object_set(obj, "double", BINN_DOUBLE, &vfloat64, 0) == TRUE);

  assert(binn_object_set_str(obj, "str", "the value") == TRUE);

  assert(binn_object_set_bool(obj, "bool_true", TRUE) == TRUE);
  assert(binn_object_set_bool(obj, "bool_false", FALSE) == TRUE);

  assert(binn_object_set_null(obj, "null") == TRUE);


  // add a list

  list = binn_list();
  assert(list != NULL);

  assert(binn_list_add_null(list) == TRUE);
  assert(binn_list_add_int32(list, 123) == TRUE);
  assert(binn_list_add_str(list, "this is a string") == TRUE);

  assert(binn_object_set_list(obj, "list", list) == TRUE);

  binn_free(list); list = INVALID_BINN;


  // return the values

  *psize = binn_size(obj);
  return (char *) binn_ptr(obj);

}

/*************************************************************************************/

void test_binn_read(void *objptr) {
  void *listptr;
  char *ptr;
  binn value={0};

  printf("OK\nreading:\n");

  vint32 = 0;
  assert(binn_object_get(objptr, "int32", BINN_INT32, &vint32, NULL) == TRUE);
  printf("int32: %d\n", vint32);
  assert(vint32 == -12345);

  vint16 = 0;
  assert(binn_object_get(objptr, "int16", BINN_INT16, &vint16, NULL) == TRUE);
  printf("int16: %d\n", vint16);
  assert(vint16 == -258);

  vint8 = 0;
  assert(binn_object_get(objptr, "int8", BINN_INT8, &vint8, NULL) == TRUE);
  printf("int8: %d\n", vint8);
  assert(vint8 == -120);

  vint64 = 0;
  assert(binn_object_get(objptr, "int64", BINN_INT64, &vint64, NULL) == TRUE);
  printf("int64: %" INT64_FORMAT "\n", vint64);
  assert(vint64 == -1234567890123);


  vuint32 = 0;
  assert(binn_object_get(objptr, "uint32", BINN_UINT32, &vuint32, NULL) == TRUE);
  printf("uint32: %d\n", vuint32);
  assert(vuint32 == 123456);

  vuint16 = 0;
  assert(binn_object_get(objptr, "uint16", BINN_UINT16, &vuint16, NULL) == TRUE);
  printf("uint16: %d\n", vuint16);
  assert(vuint16 == 60500);

  vuint8 = 0;
  assert(binn_object_get(objptr, "uint8", BINN_UINT8, &vuint8, NULL) == TRUE);
  printf("uint8: %d\n", vuint8);
  assert(vuint8 == 250);

  vuint64 = 0;
  assert(binn_object_get(objptr, "uint64", BINN_UINT64, &vuint64, NULL) == TRUE);
  printf("uint64: %" UINT64_FORMAT "\n", vuint64);
  assert(vuint64 == 1234567890123);


  vfloat32 = 0;
  assert(binn_object_get(objptr, "float32", BINN_FLOAT32, &vfloat32, NULL) == TRUE);
  printf("float32: %f\n", vfloat32);
  assert(AlmostEqualFloats(vfloat32, -12.345, 2) == TRUE);

  vfloat64 = 0;
  assert(binn_object_get(objptr, "float64", BINN_FLOAT64, &vfloat64, NULL) == TRUE);
  printf("float64: %f\n", vfloat64);
  assert(vfloat64 - -123456.7895 < 0.01);

  vfloat32 = 0;
  assert(binn_object_get(objptr, "single", BINN_SINGLE, &vfloat32, NULL) == TRUE);
  printf("single: %f\n", vfloat32);
  assert(AlmostEqualFloats(vfloat32, -12.345, 2) == TRUE);

  vfloat64 = 0;
  assert(binn_object_get(objptr, "double", BINN_DOUBLE, &vfloat64, NULL) == TRUE);
  printf("double: %f\n", vfloat64);
  assert(vfloat64 - -123456.7895 < 0.01);


  ptr = 0;
  assert(binn_object_get(objptr, "str", BINN_STRING, &ptr, NULL) == TRUE);
  printf("ptr: (%p) '%s'\n", ptr, ptr);
  assert(strcmp(ptr, "the value") == 0);


  vint32 = 999;
  assert(binn_object_get(objptr, "bool_true", BINN_BOOL, &vint32, NULL) == TRUE);
  printf("bool true: %d\n", vint32);
  assert(vint32 == TRUE);

  vint32 = 999;
  assert(binn_object_get(objptr, "bool_false", BINN_BOOL, &vint32, NULL) == TRUE);
  printf("bool false: %d\n", vint32);
  assert(vint32 == FALSE);


  vint32 = 999;
  assert(binn_object_get(objptr, "null", BINN_NULL, &vint32, NULL) == TRUE);
  printf("null: %d\n", vint32);
  //assert(vint32 == 0);
  assert(binn_object_get(objptr, "null", BINN_NULL, NULL, NULL) == TRUE);


  assert(binn_object_get(objptr, "list", BINN_LIST, &listptr, NULL) == TRUE);
  printf("obj ptr: %p  list ptr: %p\n", objptr, listptr);
  assert(listptr != 0);
  assert(listptr > objptr);

  vint32 = 0;
  assert(binn_list_get(listptr, 2, BINN_INT32, &vint32, NULL) == TRUE);
  printf("int32: %d\n", vint32);
  assert(vint32 == 123);

  ptr = 0;
  assert(binn_list_get(listptr, 3, BINN_STRING, &ptr, NULL) == TRUE);
  printf("ptr: (%p) '%s'\n", ptr, ptr);
  assert(strcmp(ptr, "this is a string") == 0);




  // short read functions 1

  vint32 = 0;
  assert(binn_object_get_int32(objptr, "int32", &vint32) == TRUE);
  printf("int32: %d\n", vint32);
  assert(vint32 == -12345);

  vint16 = 0;
  assert(binn_object_get_int16(objptr, "int16", &vint16) == TRUE);
  printf("int16: %d\n", vint16);
  assert(vint16 == -258);

  vint8 = 0;
  assert(binn_object_get_int8(objptr, "int8", &vint8) == TRUE);
  printf("int8: %d\n", vint8);
  assert(vint8 == -120);

  vint64 = 0;
  assert(binn_object_get_int64(objptr, "int64", &vint64) == TRUE);
  printf("int64: %" INT64_FORMAT "\n", vint64);
  assert(vint64 == -1234567890123);


  vuint32 = 0;
  assert(binn_object_get_uint32(objptr, "uint32", &vuint32) == TRUE);
  printf("uint32: %d\n", vuint32);
  assert(vuint32 == 123456);

  vuint16 = 0;
  assert(binn_object_get_uint16(objptr, "uint16", &vuint16) == TRUE);
  printf("uint16: %d\n", vuint16);
  assert(vuint16 == 60500);

  vuint8 = 0;
  assert(binn_object_get_uint8(objptr, "uint8", &vuint8) == TRUE);
  printf("uint8: %d\n", vuint8);
  assert(vuint8 == 250);

  vuint64 = 0;
  assert(binn_object_get_uint64(objptr, "uint64", &vuint64) == TRUE);
  printf("uint64: %" UINT64_FORMAT "\n", vuint64);
  assert(vuint64 == 1234567890123);


  vfloat32 = 0;
  assert(binn_object_get_float(objptr, "float32", &vfloat32) == TRUE);
  printf("float32: %f\n", vfloat32);
  assert(AlmostEqualFloats(vfloat32, -12.345, 2) == TRUE);

  vfloat64 = 0;
  assert(binn_object_get_double(objptr, "float64", &vfloat64) == TRUE);
  printf("float64: %f\n", vfloat64);
  assert(AlmostEqualFloats(vfloat32, -12.345, 2) == TRUE);


  ptr = 0;
  assert(binn_object_get_str(objptr, "str", &ptr) == TRUE);
  printf("ptr: (%p) '%s'\n", ptr, ptr);
  assert(strcmp(ptr, "the value") == 0);


  vint32 = 999;
  assert(binn_object_get_bool(objptr, "bool_true", &vint32) == TRUE);
  printf("bool true: %d\n", vint32);
  assert(vint32 == TRUE);

  vint32 = 999;
  assert(binn_object_get_bool(objptr, "bool_false", &vint32) == TRUE);
  printf("bool false: %d\n", vint32);
  assert(vint32 == FALSE);


  vbool = FALSE;
  assert(binn_object_null(objptr, "null") == TRUE);

  assert(binn_object_null(objptr, "bool_true") == FALSE);


  assert(binn_object_get_list(objptr, "list", &listptr) == TRUE);
  printf("obj ptr: %p  list ptr: %p\n", objptr, listptr);
  assert(listptr != 0);
  assert(listptr > objptr);

  vint32 = 0;
  assert(binn_list_get_int32(listptr, 2, &vint32) == TRUE);
  printf("int32: %d\n", vint32);
  assert(vint32 == 123);

  ptr = 0;
  assert(binn_list_get_str(listptr, 3, &ptr) == TRUE);
  printf("ptr: (%p) '%s'\n", ptr, ptr);
  assert(strcmp(ptr, "this is a string") == 0);




  // short read functions 2

  vint32 = binn_object_int32(objptr, "int32");
  printf("int32: %d\n", vint32);
  assert(vint32 == -12345);

  vint16 = binn_object_int16(objptr, "int16");
  printf("int16: %d\n", vint16);
  assert(vint16 == -258);

  vint8 = binn_object_int8(objptr, "int8");
  printf("int8: %d\n", vint8);
  assert(vint8 == -120);

  vint64 = binn_object_int64(objptr, "int64");
  printf("int64: %" INT64_FORMAT "\n", vint64);
  assert(vint64 == -1234567890123);


  vuint32 = binn_object_uint32(objptr, "uint32");
  printf("uint32: %d\n", vuint32);
  assert(vuint32 == 123456);

  vuint16 = binn_object_uint16(objptr, "uint16");
  printf("uint16: %d\n", vuint16);
  assert(vuint16 == 60500);

  vuint8 = binn_object_uint8(objptr, "uint8");
  printf("uint8: %d\n", vuint8);
  assert(vuint8 == 250);

  vuint64 = binn_object_uint64(objptr, "uint64");
  printf("uint64: %" UINT64_FORMAT "\n", vuint64);
  assert(vuint64 == 1234567890123);


  vfloat32 = binn_object_float(objptr, "float32");
  printf("float32: %f\n", vfloat32);
  assert(AlmostEqualFloats(vfloat32, -12.345, 2) == TRUE);

  vfloat64 = binn_object_double(objptr, "float64");
  printf("float64: %f\n", vfloat64);
  assert(AlmostEqualFloats(vfloat32, -12.345, 2) == TRUE);


  ptr = binn_object_str(objptr, "str");
  printf("ptr: (%p) '%s'\n", ptr, ptr);
  assert(strcmp(ptr, "the value") == 0);


  vint32 = binn_object_bool(objptr, "bool_true");
  printf("bool true: %d\n", vint32);
  assert(vint32 == TRUE);

  vint32 = binn_object_bool(objptr, "bool_false");
  printf("bool false: %d\n", vint32);
  assert(vint32 == FALSE);


  assert(binn_object_null(objptr, "null") == TRUE);
  assert(binn_object_null(objptr, "nonull") == FALSE);


  listptr = binn_object_list(objptr, "list");
  printf("obj ptr: %p  list ptr: %p\n", objptr, listptr);
  assert(listptr != 0);
  assert(listptr > objptr);

  vint32 = binn_list_int32(listptr, 2);
  printf("int32: %d\n", vint32);
  assert(vint32 == 123);

  ptr = binn_list_str(listptr, 3);
  printf("ptr: (%p) '%s'\n", ptr, ptr);
  assert(strcmp(ptr, "this is a string") == 0);




  // read as value / binn

  assert(binn_object_get_value(objptr, "int32", &value) == TRUE);
#ifndef BINN_DISABLE_COMPRESS_INT
  assert(value.type == BINN_INT16);
  assert(value.vint16 == -12345);
#else
  assert(value.type == BINN_INT32);
  assert(value.vint32 == -12345);
#endif

  assert(binn_object_get_value(objptr, "int16", &value) == TRUE);
  assert(value.type == BINN_INT16);
  assert(value.vint16 == -258);

  assert(binn_object_get_value(objptr, "int8", &value) == TRUE);
  assert(value.type == BINN_INT8);
  assert(value.vint8 == -120);

  assert(binn_object_get_value(objptr, "int64", &value) == TRUE);
  assert(value.type == BINN_INT64);
  assert(value.vint64 == -1234567890123);


  assert(binn_object_get_value(objptr, "uint32", &value) == TRUE);
  assert(value.type == BINN_UINT32);
  assert(value.vuint32 == 123456);

  assert(binn_object_get_value(objptr, "uint16", &value) == TRUE);
  assert(value.type == BINN_UINT16);
  assert(value.vuint16 == 60500);

  assert(binn_object_get_value(objptr, "uint8", &value) == TRUE);
  assert(value.type == BINN_UINT8);
  assert(value.vuint8 == 250);

  assert(binn_object_get_value(objptr, "uint64", &value) == TRUE);
  assert(value.type == BINN_UINT64);
  assert(value.vuint64 == 1234567890123);

  puts("reading... OK");

}

/*************************************************************************************/

void init_udts() {
  binn *obj=INVALID_BINN;
  unsigned short date;
  uint64 value;
  void *ptr;

  puts("testing UDTs...");

  assert(strcmp(date_to_str(str_to_date("1950-08-15")), "1950-08-15") == 0);
  assert(strcmp(date_to_str(str_to_date("1900-12-01")), "1900-12-01") == 0);
  assert(strcmp(date_to_str(str_to_date("2000-10-31")), "2000-10-31") == 0);
  assert(strcmp(date_to_str(str_to_date("2014-03-19")), "2014-03-19") == 0);

  printf("curr=%s\n", currency_to_str(str_to_currency("123.456")) );
  printf("curr=%s\n", currency_to_str(str_to_currency("123.45")) );
  printf("curr=%s\n", currency_to_str(str_to_currency("123.4")) );
  printf("curr=%s\n", currency_to_str(str_to_currency("123.")) );
  printf("curr=%s\n", currency_to_str(str_to_currency("123")) );
  printf("curr=%s\n", currency_to_str(str_to_currency("1.2")) );
  printf("curr=%s\n", currency_to_str(str_to_currency("0.987")) );
  printf("curr=%s\n", currency_to_str(str_to_currency("0.98")) );
  printf("curr=%s\n", currency_to_str(str_to_currency("0.9")) );
  printf("curr=%s\n", currency_to_str(str_to_currency("0.0")) );
  printf("curr=%s\n", currency_to_str(str_to_currency("0")) );
  printf("curr=%s\n", currency_to_str(str_to_currency("123.4567")) );
  printf("curr=%s\n", currency_to_str(str_to_currency("123.45678")) );
  printf("curr=%s\n", currency_to_str(str_to_currency("123.456789")) );
  printf("curr=%s\n", currency_to_str(str_to_currency("0.1234")) );
  printf("curr=%s\n", currency_to_str(str_to_currency(".1234")) );

  assert(float_to_currency(2.5) == 25000);
  assert(float_to_currency(5) == 50000);
  assert(str_to_currency("1.1") == 11000);
  assert(str_to_currency("12") == 120000);
  assert(mul_currency(20000, 20000) == 40000);
  assert(mul_currency(20000, 25000) == 50000);
  assert(mul_currency(30000, 40000) == 120000);
  assert(div_currency(80000, 20000) == 40000);
  assert(div_currency(120000, 40000) == 30000);
  assert(div_currency(100000, 40000) == 25000);

  printf("1.1 * 2.5 = %s\n", currency_to_str(mul_currency(str_to_currency("1.1"), float_to_currency(2.5))) );
  printf("12 / 5 = %s\n", currency_to_str(div_currency(str_to_currency("12"), float_to_currency(5))) );


  //assert(binn_register_type(MY_DATE, BINN_STORAGE_WORD) == TRUE);
  //assert(binn_register_type(MY_CURRENCY, BINN_STORAGE_QWORD) == TRUE);

  MY_DATE = binn_create_type(BINN_STORAGE_WORD, 0x0a);
  MY_CURRENCY = binn_create_type(BINN_STORAGE_QWORD, 0x0a);


  obj = binn_object();
  assert(obj != NULL);

  date = str_to_date("1950-08-15");
  printf(" date 1: %d %s\n", date, date_to_str(date));
  assert(binn_object_set(obj, "date1", MY_DATE, &date, 0) == TRUE);
  assert(binn_object_set(obj, "date1", MY_DATE, &date, 0) == FALSE);

  date = str_to_date("1999-12-31");
  printf(" date 2: %d %s\n", date, date_to_str(date));
  binn_object_set(obj, "date2", MY_DATE, &date, 0);


  value = str_to_currency("123.456");
  printf(" curr 1: %" UINT64_FORMAT " %s\n", value, currency_to_str(value));
  binn_object_set(obj, "curr1", MY_CURRENCY, &value, 0);

  value = str_to_currency("123.45");
  printf(" curr 2: %" UINT64_FORMAT " %s\n", value, currency_to_str(value));
  binn_object_set(obj, "curr2", MY_CURRENCY, &value, 0);

  value = str_to_currency("12.5");
  printf(" curr 3: %" UINT64_FORMAT " %s\n", value, currency_to_str(value));
  binn_object_set(obj, "curr3", MY_CURRENCY, &value, 0);

  value = str_to_currency("5");
  printf(" curr 4: %" UINT64_FORMAT " %s\n", value, currency_to_str(value));
  binn_object_set(obj, "curr4", MY_CURRENCY, &value, 0);

  value = str_to_currency("0.75");
  printf(" curr 5: %" UINT64_FORMAT " %s\n", value, currency_to_str(value));
  binn_object_set(obj, "curr5", MY_CURRENCY, &value, 0);


  ptr = binn_ptr(obj);


  assert(binn_object_get(ptr, "date1", MY_DATE, &date, NULL) == TRUE);
  printf(" date 1: %d %s\n", date, date_to_str(date));

  assert(binn_object_get(ptr, "date2", MY_DATE, &date, NULL) == TRUE);
  printf(" date 2: %d %s\n", date, date_to_str(date));


  assert(binn_object_get(ptr, "curr1", MY_CURRENCY, &value, NULL) == TRUE);
  printf(" curr 1: %" UINT64_FORMAT " %s\n", value, currency_to_str(value));

  assert(binn_object_get(ptr, "curr2", MY_CURRENCY, &value, NULL) == TRUE);
  printf(" curr 2: %" UINT64_FORMAT " %s\n", value, currency_to_str(value));

  assert(binn_object_get(ptr, "curr3", MY_CURRENCY, &value, NULL) == TRUE);
  printf(" curr 3: %" UINT64_FORMAT " %s\n", value, currency_to_str(value));

  assert(binn_object_get(ptr, "curr4", MY_CURRENCY, &value, NULL) == TRUE);
  printf(" curr 4: %" UINT64_FORMAT " %s\n", value, currency_to_str(value));

  assert(binn_object_get(ptr, "curr5", MY_CURRENCY, &value, NULL) == TRUE);
  printf(" curr 5: %" UINT64_FORMAT " %s\n", value, currency_to_str(value));


  binn_free(obj);

  puts("testing UDTs... OK");

}

/*************************************************************************************/

BOOL copy_int_value(void *psource, void *pdest, int source_type, int dest_type);

void test_int_conversion() {

  printf("testing integer conversion...");

  // copy negative value to unsigned integer

  vint8 = -110; vuint8 = 0;
  assert(copy_int_value(&vint8, &vuint8, BINN_INT8, BINN_UINT8) == FALSE);
  assert(vint8 == -110);
  assert(vuint8 == 0);

  vint8 = -110; vuint16 = 0;
  assert(copy_int_value(&vint8, &vuint16, BINN_INT8, BINN_UINT16) == FALSE);
  assert(vint8 == -110);
  assert(vuint16 == 0);

  vint8 = -110; vuint32 = 0;
  assert(copy_int_value(&vint8, &vuint32, BINN_INT8, BINN_UINT32) == FALSE);
  assert(vint8 == -110);
  assert(vuint32 == 0);

  vint8 = -110; vuint64 = 0;
  assert(copy_int_value(&vint8, &vuint64, BINN_INT8, BINN_UINT64) == FALSE);
  assert(vint8 == -110);
  assert(vuint64 == 0);


  vint16 = -123; vuint8 = 0;
  assert(copy_int_value(&vint16, &vuint8, BINN_INT16, BINN_UINT8) == FALSE);
  assert(vint16 == -123);
  assert(vuint8 == 0);

  vint16 = -123; vuint16 = 0;
  assert(copy_int_value(&vint16, &vuint16, BINN_INT16, BINN_UINT16) == FALSE);
  assert(vint16 == -123);
  assert(vuint16 == 0);

  vint16 = -32000; vuint32 = 0;
  assert(copy_int_value(&vint16, &vuint32, BINN_INT16, BINN_UINT32) == FALSE);
  assert(vint16 == -32000);
  assert(vuint32 == 0);

  vint16 = -32000; vuint64 = 0;
  assert(copy_int_value(&vint16, &vuint64, BINN_INT16, BINN_UINT64) == FALSE);
  assert(vint16 == -32000);
  assert(vuint64 == 0);


  vint32 = -123; vuint8 = 0;
  assert(copy_int_value(&vint32, &vuint8, BINN_INT32, BINN_UINT8) == FALSE);
  assert(vint32 == -123);
  assert(vuint8 == 0);

  vint32 = -123; vuint16 = 0;
  assert(copy_int_value(&vint32, &vuint16, BINN_INT32, BINN_UINT16) == FALSE);
  assert(vint32 == -123);
  assert(vuint16 == 0);

  vint32 = -123; vuint32 = 0;
  assert(copy_int_value(&vint32, &vuint32, BINN_INT32, BINN_UINT32) == FALSE);
  assert(vint32 == -123);
  assert(vuint32 == 0);

  vint32 = -123; vuint64 = 0;
  assert(copy_int_value(&vint32, &vuint64, BINN_INT32, BINN_UINT64) == FALSE);
  assert(vint32 == -123);
  assert(vuint64 == 0);


  vint64 = -123; vuint8 = 0;
  assert(copy_int_value(&vint64, &vuint8, BINN_INT64, BINN_UINT8) == FALSE);
  assert(vint64 == -123);
  assert(vuint8 == 0);

  vint64 = -123; vuint16 = 0;
  assert(copy_int_value(&vint64, &vuint16, BINN_INT64, BINN_UINT16) == FALSE);
  assert(vint64 == -123);
  assert(vuint16 == 0);

  vint64 = -123; vuint32 = 0;
  assert(copy_int_value(&vint64, &vuint32, BINN_INT64, BINN_UINT32) == FALSE);
  assert(vint64 == -123);
  assert(vuint32 == 0);

  vint64 = -123; vuint64 = 0;
  assert(copy_int_value(&vint64, &vuint64, BINN_INT64, BINN_UINT64) == FALSE);
  assert(vint64 == -123);
  assert(vuint64 == 0);


  // copy big negative value to small signed integer

  vint16 = -32000; vint8 = 0;
  assert(copy_int_value(&vint16, &vint8, BINN_INT16, BINN_INT8) == FALSE);
  assert(vint16 == -32000);
  assert(vint8 == 0);


  vint32 = -250; vint8 = 0;
  assert(copy_int_value(&vint32, &vint8, BINN_INT32, BINN_INT8) == FALSE);
  assert(vint32 == -250);
  assert(vint8 == 0);

  vint32 = -35000; vint16 = 0;
  assert(copy_int_value(&vint32, &vint16, BINN_INT32, BINN_INT16) == FALSE);
  assert(vint32 == -35000);
  assert(vint16 == 0);


  vint64 = -250; vint8 = 0;
  assert(copy_int_value(&vint64, &vint8, BINN_INT64, BINN_INT8) == FALSE);
  assert(vint64 == -250);
  assert(vint8 == 0);

  vint64 = -35000; vint16 = 0;
  assert(copy_int_value(&vint64, &vint16, BINN_INT64, BINN_INT16) == FALSE);
  assert(vint64 == -35000);
  assert(vint16 == 0);

  vint64 = -25470000000; vint32 = 0;
  assert(copy_int_value(&vint64, &vint32, BINN_INT64, BINN_INT32) == FALSE);
  assert(vint64 == -25470000000);
  assert(vint32 == 0);


  // copy big positive value to small signed integer

  vint16 = 250; vint8 = 0;
  assert(copy_int_value(&vint16, &vint8, BINN_INT16, BINN_INT8) == FALSE);
  assert(vint16 == 250);
  assert(vint8 == 0);


  vint32 = 250; vint8 = 0;
  assert(copy_int_value(&vint32, &vint8, BINN_INT32, BINN_INT8) == FALSE);
  assert(vint32 == 250);
  assert(vint8 == 0);

  vint32 = 35000; vint16 = 0;
  assert(copy_int_value(&vint32, &vint16, BINN_INT32, BINN_INT16) == FALSE);
  assert(vint32 == 35000);
  assert(vint16 == 0);


  vint64 = 250; vint8 = 0;
  assert(copy_int_value(&vint64, &vint8, BINN_INT64, BINN_INT8) == FALSE);
  assert(vint64 == 250);
  assert(vint8 == 0);

  vint64 = 35000; vint16 = 0;
  assert(copy_int_value(&vint64, &vint16, BINN_INT64, BINN_INT16) == FALSE);
  assert(vint64 == 35000);
  assert(vint16 == 0);

  vint64 = 25470000000; vint32 = 0;
  assert(copy_int_value(&vint64, &vint32, BINN_INT64, BINN_INT32) == FALSE);
  assert(vint64 == 25470000000);
  assert(vint32 == 0);



  // copy big positive value to small unsigned integer

  vint16 = 300; vuint8 = 0;
  assert(copy_int_value(&vint16, &vuint8, BINN_INT16, BINN_UINT8) == FALSE);
  assert(vint16 == 300);
  assert(vuint8 == 0);


  vint32 = 300; vuint8 = 0;
  assert(copy_int_value(&vint32, &vuint8, BINN_INT32, BINN_UINT8) == FALSE);
  assert(vint32 == 300);
  assert(vuint8 == 0);

  vint32 = 70000; vuint16 = 0;
  assert(copy_int_value(&vint32, &vuint16, BINN_INT32, BINN_UINT16) == FALSE);
  assert(vint32 == 70000);
  assert(vuint16 == 0);


  vint64 = 300; vuint8 = 0;
  assert(copy_int_value(&vint64, &vuint8, BINN_INT64, BINN_UINT8) == FALSE);
  assert(vint64 == 300);
  assert(vuint8 == 0);

  vint64 = 70000; vuint16 = 0;
  assert(copy_int_value(&vint64, &vuint16, BINN_INT64, BINN_UINT16) == FALSE);
  assert(vint64 == 70000);
  assert(vuint16 == 0);

  vint64 = 25470000000; vuint32 = 0;
  assert(copy_int_value(&vint64, &vuint32, BINN_INT64, BINN_UINT32) == FALSE);
  assert(vint64 == 25470000000);
  assert(vuint32 == 0);




  // valid numbers --------------------
  
  // int8 - copy to signed variable

  vint8 = 123; vint16 = 0;
  assert(copy_int_value(&vint8, &vint16, BINN_INT8, BINN_INT16) == TRUE);
  assert(vint8 == 123);
  assert(vint16 == 123);

  vint8 = -110; vint16 = 0;
  assert(copy_int_value(&vint8, &vint16, BINN_INT8, BINN_INT16) == TRUE);
  assert(vint8 == -110);
  assert(vint16 == -110);

  vint8 = 123; vint32 = 0;
  assert(copy_int_value(&vint8, &vint32, BINN_INT8, BINN_INT32) == TRUE);
  assert(vint8 == 123);
  assert(vint32 == 123);

  vint8 = -110; vint32 = 0;
  assert(copy_int_value(&vint8, &vint32, BINN_INT8, BINN_INT32) == TRUE);
  assert(vint8 == -110);
  assert(vint32 == -110);

  vint8 = 123; vint64 = 0;
  assert(copy_int_value(&vint8, &vint64, BINN_INT8, BINN_INT64) == TRUE);
  assert(vint8 == 123);
  assert(vint64 == 123);

  vint8 = -120; vint64 = 0;
  assert(copy_int_value(&vint8, &vint64, BINN_INT8, BINN_INT64) == TRUE);
  assert(vint8 == -120);
  assert(vint64 == -120);


  // int8 - copy to unsigned variable

  vint8 = 123; vuint16 = 0;
  assert(copy_int_value(&vint8, &vuint16, BINN_INT8, BINN_UINT16) == TRUE);
  assert(vint8 == 123);
  assert(vuint16 == 123);

  vint8 = 123; vuint32 = 0;
  assert(copy_int_value(&vint8, &vuint32, BINN_INT8, BINN_UINT32) == TRUE);
  assert(vint8 == 123);
  assert(vuint32 == 123);

  vint8 = 123; vuint64 = 0;
  assert(copy_int_value(&vint8, &vuint64, BINN_INT8, BINN_UINT64) == TRUE);
  assert(vint8 == 123);
  assert(vuint64 == 123);



  // unsigned int8 - copy to signed variable

  vuint8 = 123; vint16 = 0;
  assert(copy_int_value(&vuint8, &vint16, BINN_UINT8, BINN_INT16) == TRUE);
  assert(vuint8 == 123);
  assert(vint16 == 123);

  vuint8 = 250; vint16 = 0;
  assert(copy_int_value(&vuint8, &vint16, BINN_UINT8, BINN_INT16) == TRUE);
  assert(vuint8 == 250);
  assert(vint16 == 250);

  vuint8 = 123; vint32 = 0;
  assert(copy_int_value(&vuint8, &vint32, BINN_UINT8, BINN_INT32) == TRUE);
  assert(vuint8 == 123);
  assert(vint32 == 123);

  vuint8 = 250; vint32 = 0;
  assert(copy_int_value(&vuint8, &vint32, BINN_UINT8, BINN_INT32) == TRUE);
  assert(vuint8 == 250);
  assert(vint32 == 250);

  vuint8 = 123; vint64 = 0;
  assert(copy_int_value(&vuint8, &vint64, BINN_UINT8, BINN_INT64) == TRUE);
  assert(vuint8 == 123);
  assert(vint64 == 123);

  vuint8 = 250; vint64 = 0;
  assert(copy_int_value(&vuint8, &vint64, BINN_UINT8, BINN_INT64) == TRUE);
  assert(vuint8 == 250);
  assert(vint64 == 250);


  // unsigned int8 - copy to unsigned variable

  vuint8 = 123; vuint16 = 0;
  assert(copy_int_value(&vuint8, &vuint16, BINN_UINT8, BINN_UINT16) == TRUE);
  assert(vuint8 == 123);
  assert(vuint16 == 123);

  vuint8 = 250; vuint16 = 0;
  assert(copy_int_value(&vuint8, &vuint16, BINN_UINT8, BINN_UINT16) == TRUE);
  assert(vuint8 == 250);
  assert(vuint16 == 250);

  vuint8 = 123; vuint32 = 0;
  assert(copy_int_value(&vuint8, &vuint32, BINN_UINT8, BINN_UINT32) == TRUE);
  assert(vuint8 == 123);
  assert(vuint32 == 123);

  vuint8 = 250; vuint32 = 0;
  assert(copy_int_value(&vuint8, &vuint32, BINN_UINT8, BINN_UINT32) == TRUE);
  assert(vuint8 == 250);
  assert(vuint32 == 250);

  vuint8 = 123; vuint64 = 0;
  assert(copy_int_value(&vuint8, &vuint64, BINN_UINT8, BINN_UINT64) == TRUE);
  assert(vuint8 == 123);
  assert(vuint64 == 123);

  vuint8 = 250; vuint64 = 0;
  assert(copy_int_value(&vuint8, &vuint64, BINN_UINT8, BINN_UINT64) == TRUE);
  assert(vuint8 == 250);
  assert(vuint64 == 250);




  vint16 = 250; vuint8 = 0;
  assert(copy_int_value(&vint16, &vuint8, BINN_INT16, BINN_UINT8) == TRUE);
  assert(vint16 == 250);
  assert(vuint8 == 250);


  vint32 = 250; vuint8 = 0;
  assert(copy_int_value(&vint32, &vuint8, BINN_INT32, BINN_UINT8) == TRUE);
  assert(vint32 == 250);
  assert(vuint8 == 250);

  vint32 = 35000; vuint16 = 0;
  assert(copy_int_value(&vint32, &vuint16, BINN_INT32, BINN_UINT16) == TRUE);
  assert(vint32 == 35000);
  assert(vuint16 == 35000);


  vint64 = 250; vuint8 = 0;
  assert(copy_int_value(&vint64, &vuint8, BINN_INT64, BINN_UINT8) == TRUE);
  assert(vint64 == 250);
  assert(vuint8 == 250);

  vint64 = 35000; vuint16 = 0;
  assert(copy_int_value(&vint64, &vuint16, BINN_INT64, BINN_UINT16) == TRUE);
  assert(vint64 == 35000);
  assert(vuint16 == 35000);

  vint64 = 2147000000; vuint32 = 0;
  assert(copy_int_value(&vint64, &vuint32, BINN_INT64, BINN_UINT32) == TRUE);
  assert(vint64 == 2147000000);
  assert(vuint32 == 2147000000);




  // valid negative values


  vint8 = -110; vint16 = 0;
  assert(copy_int_value(&vint8, &vint16, BINN_INT8, BINN_INT16) == TRUE);
  assert(vint8 == -110);
  assert(vint16 == -110);

  vint8 = -110; vint32 = 0;
  assert(copy_int_value(&vint8, &vint32, BINN_INT8, BINN_INT32) == TRUE);
  assert(vint8 == -110);
  assert(vint32 == -110);

  vint8 = -110; vint64 = 0;
  assert(copy_int_value(&vint8, &vint64, BINN_INT8, BINN_INT64) == TRUE);
  assert(vint8 == -110);
  assert(vint64 == -110);


  vint16 = -123; vint8 = 0;
  assert(copy_int_value(&vint16, &vint8, BINN_INT16, BINN_INT8) == TRUE);
  assert(vint16 == -123);
  assert(vint8 == -123);

  vint16 = -32000; vint32 = 0;
  assert(copy_int_value(&vint16, &vint32, BINN_INT16, BINN_INT32) == TRUE);
  assert(vint16 == -32000);
  assert(vint32 == -32000);

  vint16 = -32000; vint64 = 0;
  assert(copy_int_value(&vint16, &vint64, BINN_INT16, BINN_INT64) == TRUE);
  assert(vint16 == -32000);
  assert(vint64 == -32000);


  vint32 = -123; vint8 = 0;
  assert(copy_int_value(&vint32, &vint8, BINN_INT32, BINN_INT8) == TRUE);
  assert(vint32 == -123);
  assert(vint8 == -123);

  vint32 = -123; vint16 = 0;
  assert(copy_int_value(&vint32, &vint16, BINN_INT32, BINN_INT16) == TRUE);
  assert(vint32 == -123);
  assert(vint16 == -123);

  vint32 = -32000; vint16 = 0;
  assert(copy_int_value(&vint32, &vint16, BINN_INT32, BINN_INT16) == TRUE);
  assert(vint32 == -32000);
  assert(vint16 == -32000);

  vint32 = -123; vint64 = 0;
  assert(copy_int_value(&vint32, &vint64, BINN_INT32, BINN_INT64) == TRUE);
  assert(vint32 == -123);
  assert(vint64 == -123);

  vint32 = -2147000000; vint64 = 0;
  assert(copy_int_value(&vint32, &vint64, BINN_INT32, BINN_INT64) == TRUE);
  assert(vint32 == -2147000000);
  assert(vint64 == -2147000000);


  vint64 = -123; vint8 = 0;
  assert(copy_int_value(&vint64, &vint8, BINN_INT64, BINN_INT8) == TRUE);
  assert(vint64 == -123);
  assert(vint8 == -123);

  vint64 = -250; vint16 = 0;
  assert(copy_int_value(&vint64, &vint16, BINN_INT64, BINN_INT16) == TRUE);
  assert(vint64 == -250);
  assert(vint16 == -250);

  vint64 = -35000; vint32 = 0;
  assert(copy_int_value(&vint64, &vint32, BINN_INT64, BINN_INT32) == TRUE);
  assert(vint64 == -35000);
  assert(vint32 == -35000);


  puts("OK");

}

/*************************************************************************************/

void test_binn_int_conversion() {
  binn *obj=INVALID_BINN;
  void *ptr;

  printf("testing binn integer read conversion... ");

  obj = binn_object();
  assert(obj != NULL);

  assert(binn_object_set_int8(obj, "int8", -8) == TRUE);
  assert(binn_object_set_int16(obj, "int16", -16) == TRUE);
  assert(binn_object_set_int32(obj, "int32", -32) == TRUE);
  assert(binn_object_set_int64(obj, "int64", -64) == TRUE);

  assert(binn_object_set_uint8(obj, "uint8", 111) == TRUE);
  assert(binn_object_set_uint16(obj, "uint16", 112) == TRUE);
  assert(binn_object_set_uint32(obj, "uint32", 113) == TRUE);
  assert(binn_object_set_uint64(obj, "uint64", 114) == TRUE);

  ptr = binn_ptr(obj);

  assert(binn_object_int8(ptr, "int8") == -8);
  assert(binn_object_int8(ptr, "int16") == -16);
  assert(binn_object_int8(ptr, "int32") == -32);
  assert(binn_object_int8(ptr, "int64") == -64);

  assert(binn_object_int16(ptr, "int8") == -8);
  assert(binn_object_int16(ptr, "int16") == -16);
  assert(binn_object_int16(ptr, "int32") == -32);
  assert(binn_object_int16(ptr, "int64") == -64);

  assert(binn_object_int32(ptr, "int8") == -8);
  assert(binn_object_int32(ptr, "int16") == -16);
  assert(binn_object_int32(ptr, "int32") == -32);
  assert(binn_object_int32(ptr, "int64") == -64);

  assert(binn_object_int64(ptr, "int8") == -8);
  assert(binn_object_int64(ptr, "int16") == -16);
  assert(binn_object_int64(ptr, "int32") == -32);
  assert(binn_object_int64(ptr, "int64") == -64);


  assert(binn_object_int8(ptr, "uint8") == 111);
  assert(binn_object_int8(ptr, "uint16") == 112);
  assert(binn_object_int8(ptr, "uint32") == 113);
  assert(binn_object_int8(ptr, "uint64") == 114);

  assert(binn_object_int16(ptr, "uint8") == 111);
  assert(binn_object_int16(ptr, "uint16") == 112);
  assert(binn_object_int16(ptr, "uint32") == 113);
  assert(binn_object_int16(ptr, "uint64") == 114);

  assert(binn_object_int32(ptr, "uint8") == 111);
  assert(binn_object_int32(ptr, "uint16") == 112);
  assert(binn_object_int32(ptr, "uint32") == 113);
  assert(binn_object_int32(ptr, "uint64") == 114);

  assert(binn_object_int64(ptr, "uint8") == 111);
  assert(binn_object_int64(ptr, "uint16") == 112);
  assert(binn_object_int64(ptr, "uint32") == 113);
  assert(binn_object_int64(ptr, "uint64") == 114);


  binn_free(obj);

  puts("OK");

}

/*************************************************************************************/

void test_value_conversion() {
  binn *value;
  char *ptr, blob[64] = "test blob";
  void *pblob;
  int size, vint32;
  int64 vint64;
  double vdouble;
  BOOL vbool;

  printf("testing binn value conversion... ");

  /* test string values */

  ptr = "static string";
  value = binn_string(ptr, BINN_STATIC);
  assert(value != NULL);
  assert(value->type == BINN_STRING);
  assert(value->ptr != NULL);
  assert(value->ptr == ptr);
  assert(value->freefn == NULL);
  binn_free(value);

  ptr = "transient string";
  value = binn_string(ptr, BINN_TRANSIENT);
  assert(value != NULL);
  assert(value->type == BINN_STRING);
  assert(value->ptr != NULL);
  assert(value->ptr != ptr);
  assert(strcmp((char*)value->ptr, ptr) == 0);
  assert(value->freefn != NULL);
  binn_free(value);

  ptr = strdup("dynamic allocated string");
  value = binn_string(ptr, free);
  assert(value != NULL);
  assert(value->type == BINN_STRING);
  assert(value->ptr != NULL);
  assert(value->ptr == ptr);
  assert(value->freefn != NULL);
  assert(value->freefn == &free);
  binn_free(value);

  /* test blob values */

  size = 64;
  pblob = blob;
  value = binn_blob(pblob, size, BINN_STATIC);
  assert(value != NULL);
  assert(value->type == BINN_BLOB);
  assert(value->ptr != NULL);
  assert(value->ptr == pblob);
  assert(value->freefn == NULL);
  binn_free(value);

  size = 64;
  pblob = blob;
  value = binn_blob(pblob, size, BINN_TRANSIENT);
  assert(value != NULL);
  assert(value->type == BINN_BLOB);
  assert(value->ptr != NULL);
  assert(value->ptr != pblob);
  assert(memcmp(value->ptr, pblob, size) == 0);
  assert(value->freefn != NULL);
  binn_free(value);

  size = 64;
  pblob = memdup(blob, size);
  value = binn_blob(pblob, size, free);
  assert(value != NULL);
  assert(value->type == BINN_BLOB);
  assert(value->ptr != NULL);
  assert(value->ptr == pblob);
  assert(value->freefn == &free);
  binn_free(value);


  /* test conversions */

  ptr = "123";
  value = binn_string(ptr, BINN_STATIC);
  assert(value != NULL);
  assert(value->type == BINN_STRING);
  assert(value->ptr != NULL);
  assert(value->ptr == ptr);
  assert(value->freefn == NULL);
  //
  assert(binn_get_str(value) == ptr);
  assert(binn_get_int32(value, &vint32) == TRUE);
  assert(vint32 == 123);
  assert(binn_get_int64(value, &vint64) == TRUE);
  assert(vint64 == 123);
  assert(binn_get_double(value, &vdouble) == TRUE);
  assert(AlmostEqualFloats(vdouble, 123, 4) == TRUE);
  assert(binn_get_bool(value, &vbool) == TRUE);
  assert(vbool == TRUE);
  // check that the type is the same
  assert(value->type == BINN_STRING);
  assert(value->ptr != NULL);
  assert(value->ptr == ptr);
  assert(value->freefn == NULL);
  //
  binn_free(value);


  ptr = "-456";
  value = binn_string(ptr, BINN_STATIC);
  assert(value != NULL);
  assert(value->type == BINN_STRING);
  assert(value->ptr != NULL);
  assert(value->ptr == ptr);
  assert(value->freefn == NULL);
  //
  assert(binn_get_str(value) == ptr);
  assert(binn_get_int32(value, &vint32) == TRUE);
  assert(vint32 == -456);
  assert(binn_get_int64(value, &vint64) == TRUE);
  assert(vint64 == -456);
  assert(binn_get_double(value, &vdouble) == TRUE);
  assert(AlmostEqualFloats(vdouble, -456, 4) == TRUE);
  assert(binn_get_bool(value, &vbool) == TRUE);
  assert(vbool == TRUE);
  // check that the type is the same
  assert(value->type == BINN_STRING);
  assert(value->ptr != NULL);
  assert(value->ptr == ptr);
  assert(value->freefn == NULL);
  //
  binn_free(value);


  ptr = "-4.56";
  value = binn_string(ptr, BINN_STATIC);
  assert(value != NULL);
  assert(value->type == BINN_STRING);
  assert(value->ptr != NULL);
  assert(value->ptr == ptr);
  assert(value->freefn == NULL);
  //
  assert(binn_get_str(value) == ptr);
  assert(binn_get_int32(value, &vint32) == TRUE);
  assert(vint32 == -4);
  assert(binn_get_int64(value, &vint64) == TRUE);
  assert(vint64 == -4);
  assert(binn_get_double(value, &vdouble) == TRUE);
  assert(AlmostEqualFloats(vdouble, -4.56, 4) == TRUE);
  assert(binn_get_bool(value, &vbool) == TRUE);
  assert(vbool == TRUE);
  // check that the type is the same
  assert(value->type == BINN_STRING);
  assert(value->ptr != NULL);
  assert(value->ptr == ptr);
  assert(value->freefn == NULL);
  //
  binn_free(value);


  // to boolean

  ptr = "yes";
  value = binn_string(ptr, BINN_STATIC);
  assert(value != NULL);
  assert(binn_get_str(value) == ptr);
  assert(binn_get_bool(value, &vbool) == TRUE);
  assert(vbool == TRUE);
  binn_free(value);

  ptr = "no";
  value = binn_string(ptr, BINN_STATIC);
  assert(value != NULL);
  assert(binn_get_bool(value, &vbool) == TRUE);
  assert(vbool == FALSE);
  binn_free(value);

  ptr = "on";
  value = binn_string(ptr, BINN_STATIC);
  assert(value != NULL);
  assert(binn_get_bool(value, &vbool) == TRUE);
  assert(vbool == TRUE);
  binn_free(value);

  ptr = "off";
  value = binn_string(ptr, BINN_STATIC);
  assert(value != NULL);
  assert(binn_get_bool(value, &vbool) == TRUE);
  assert(vbool == FALSE);
  binn_free(value);

  ptr = "true";
  value = binn_string(ptr, BINN_STATIC);
  assert(value != NULL);
  assert(binn_get_bool(value, &vbool) == TRUE);
  assert(vbool == TRUE);
  binn_free(value);

  ptr = "false";
  value = binn_string(ptr, BINN_STATIC);
  assert(value != NULL);
  assert(binn_get_bool(value, &vbool) == TRUE);
  assert(vbool == FALSE);
  binn_free(value);

  ptr = "1";
  value = binn_string(ptr, BINN_STATIC);
  assert(value != NULL);
  assert(binn_get_bool(value, &vbool) == TRUE);
  assert(vbool == TRUE);
  binn_free(value);

  ptr = "0";
  value = binn_string(ptr, BINN_STATIC);
  assert(value != NULL);
  assert(binn_get_bool(value, &vbool) == TRUE);
  assert(vbool == FALSE);
  binn_free(value);


  // from int32

  value = binn_int32(-345);
  assert(value != NULL);
  assert(value->type == BINN_INT32);
  assert(value->vint32 == -345);
  assert(value->freefn == NULL);
  //
  assert(binn_get_int32(value, &vint32) == TRUE);
  assert(vint32 == -345);
  assert(binn_get_int64(value, &vint64) == TRUE);
  assert(vint64 == -345);
  assert(binn_get_double(value, &vdouble) == TRUE);
  assert(AlmostEqualFloats(vdouble, -345, 4) == TRUE);
  assert(binn_get_bool(value, &vbool) == TRUE);
  assert(vbool == TRUE);
  // check that the type is the same
  assert(value->type == BINN_INT32);
  assert(value->vint32 == -345);
  assert(value->freefn == NULL);
  // convert the value to string
  ptr = binn_get_str(value);
  assert(ptr != NULL);
  assert(strcmp(ptr, "-345") == 0);
  assert(value->type == BINN_STRING);
  assert(value->ptr != NULL);
  assert(value->ptr == ptr);
  assert(value->freefn != NULL);
  //
  binn_free(value);


  value = binn_int32(0);
  assert(value != NULL);
  assert(value->type == BINN_INT32);
  assert(value->vint32 == 0);
  assert(value->freefn == NULL);
  //
  assert(binn_get_int32(value, &vint32) == TRUE);
  assert(vint32 == 0);
  assert(binn_get_int64(value, &vint64) == TRUE);
  assert(vint64 == 0);
  assert(binn_get_double(value, &vdouble) == TRUE);
  assert(AlmostEqualFloats(vdouble, 0, 4) == TRUE);
  assert(binn_get_bool(value, &vbool) == TRUE);
  assert(vbool == FALSE);
  // check that the type is the same
  assert(value->type == BINN_INT32);
  assert(value->vint32 == 0);
  assert(value->freefn == NULL);
  // convert the value to string
  ptr = binn_get_str(value);
  assert(ptr != NULL);
  assert(strcmp(ptr, "0") == 0);
  assert(value->type == BINN_STRING);
  assert(value->ptr != NULL);
  assert(value->ptr == ptr);
  assert(value->freefn != NULL);
  //
  binn_free(value);


  // from int64

  value = binn_int64(-345678);
  assert(value != NULL);
  assert(value->type == BINN_INT64);
  assert(value->vint64 == -345678);
  assert(value->freefn == NULL);
  //
  assert(binn_get_int32(value, &vint32) == TRUE);
  assert(vint32 == -345678);
  assert(binn_get_int64(value, &vint64) == TRUE);
  assert(vint64 == -345678);
  assert(binn_get_double(value, &vdouble) == TRUE);
  assert(AlmostEqualFloats(vdouble, -345678, 4) == TRUE);
  assert(binn_get_bool(value, &vbool) == TRUE);
  assert(vbool == TRUE);
  // check that the type is the same
  assert(value->type == BINN_INT64);
  assert(value->vint64 == -345678);
  assert(value->freefn == NULL);
  // convert the value to string
  ptr = binn_get_str(value);
  assert(ptr != NULL);
  assert(strcmp(ptr, "-345678") == 0);
  assert(value->type == BINN_STRING);
  assert(value->ptr != NULL);
  assert(value->ptr == ptr);
  assert(value->freefn != NULL);
  //
  binn_free(value);


  // from double

  value = binn_double(-345.678);
  assert(value != NULL);
  assert(value->type == BINN_DOUBLE);
  assert(value->vdouble == -345.678);
  assert(value->freefn == NULL);
  //
  assert(binn_get_int32(value, &vint32) == TRUE);
  assert(vint32 == -345);
  assert(binn_get_int64(value, &vint64) == TRUE);
  assert(vint64 == -345);
  assert(binn_get_double(value, &vdouble) == TRUE);
  assert(AlmostEqualFloats(vdouble, -345.678, 4) == TRUE);
  assert(binn_get_bool(value, &vbool) == TRUE);
  assert(vbool == TRUE);
  // check that the type is the same
  assert(value->type == BINN_DOUBLE);
  assert(value->vdouble == -345.678);
  assert(value->freefn == NULL);
  // convert the value to string
  ptr = binn_get_str(value);
  assert(ptr != NULL);
  assert(strcmp(ptr, "-345.678") == 0);
  assert(value->type == BINN_STRING);
  assert(value->ptr != NULL);
  assert(value->ptr == ptr);
  assert(value->freefn != NULL);
  //
  binn_free(value);


  value = binn_double(0.0);
  assert(value != NULL);
  assert(value->type == BINN_DOUBLE);
  assert(value->vdouble == 0.0);
  assert(value->freefn == NULL);
  //
  assert(binn_get_int32(value, &vint32) == TRUE);
  assert(vint32 == 0);
  assert(binn_get_int64(value, &vint64) == TRUE);
  assert(vint64 == 0);
  assert(binn_get_double(value, &vdouble) == TRUE);
  assert(AlmostEqualFloats(vdouble, 0, 4) == TRUE);
  assert(binn_get_bool(value, &vbool) == TRUE);
  assert(vbool == FALSE);
  // check that the type is the same
  assert(value->type == BINN_DOUBLE);
  assert(value->vdouble == 0.0);
  assert(value->freefn == NULL);
  // convert the value to string
  ptr = binn_get_str(value);
  assert(ptr != NULL);
  assert(strcmp(ptr, "0") == 0);
  assert(value->type == BINN_STRING);
  assert(value->ptr != NULL);
  assert(value->ptr == ptr);
  assert(value->freefn != NULL);
  //
  binn_free(value);


  // from bool

  value = binn_bool(FALSE);
  assert(value != NULL);
  assert(value->type == BINN_BOOL);
  assert(value->vbool == FALSE);
  assert(value->freefn == NULL);
  //
  assert(binn_get_int32(value, &vint32) == TRUE);
  assert(vint32 == 0);
  assert(binn_get_int64(value, &vint64) == TRUE);
  assert(vint64 == 0);
  assert(binn_get_double(value, &vdouble) == TRUE);
  assert(AlmostEqualFloats(vdouble, 0, 4) == TRUE);
  assert(binn_get_bool(value, &vbool) == TRUE);
  assert(vbool == FALSE);
  // check that the type is the same
  assert(value->type == BINN_BOOL);
  assert(value->vbool == FALSE);
  assert(value->freefn == NULL);
  // convert the value to string
  ptr = binn_get_str(value);
  assert(ptr != NULL);
  assert(strcmp(ptr, "false") == 0);
  assert(value->type == BINN_STRING);
  assert(value->ptr != NULL);
  assert(value->ptr == ptr);
  assert(value->freefn != NULL);
  //
  binn_free(value);


  value = binn_bool(TRUE);
  assert(value != NULL);
  assert(value->type == BINN_BOOL);
  assert(value->vbool == TRUE);
  assert(value->freefn == NULL);
  //
  assert(binn_get_int32(value, &vint32) == TRUE);
  assert(vint32 == 1);
  assert(binn_get_int64(value, &vint64) == TRUE);
  assert(vint64 == 1);
  assert(binn_get_double(value, &vdouble) == TRUE);
  assert(AlmostEqualFloats(vdouble, 1, 4) == TRUE);
  assert(binn_get_bool(value, &vbool) == TRUE);
  assert(vbool == TRUE);
  // check that the type is the same
  assert(value->type == BINN_BOOL);
  assert(value->vbool == TRUE);
  assert(value->freefn == NULL);
  // convert the value to string
  ptr = binn_get_str(value);
  assert(ptr != NULL);
  assert(strcmp(ptr, "true") == 0);
  assert(value->type == BINN_STRING);
  assert(value->ptr != NULL);
  assert(value->ptr == ptr);
  assert(value->freefn != NULL);
  //
  binn_free(value);


  puts("OK");

}

/*************************************************************************************/

void test_value_copy() {

  printf("testing binn value copy... ");

  //TODO

  puts("TODO!!!");

}

/*************************************************************************************/

void test_virtual_types() {
  binn *list=INVALID_BINN;
  void *ptr;
  int storage_type, extra_type;
  BOOL value;

  printf("testing binn virtual types... ");

  assert(binn_get_type_info(BINN_BOOL, &storage_type, &extra_type) == TRUE);
  assert(storage_type == BINN_STORAGE_DWORD);
  assert(extra_type == 1);

  list = binn_list();
  assert(list != NULL);

  assert(binn_list_add_bool(list, TRUE) == TRUE);
  assert(binn_list_add_bool(list, FALSE) == TRUE);
  assert(binn_list_add_null(list) == TRUE);

  ptr = binn_ptr(list);
  assert(ptr != 0);

  assert(binn_list_get_bool(ptr, 1, &value) == TRUE);
  assert(value == TRUE);

  assert(binn_list_get_bool(ptr, 2, &value) == TRUE);
  assert(value == FALSE);

  assert(binn_list_null(ptr, 3) == TRUE);

  // invalid values
  assert(binn_list_null(ptr, 1) == FALSE);
  assert(binn_list_null(ptr, 2) == FALSE);
  assert(binn_list_get_bool(ptr, 3, &value) == FALSE);

  binn_free(list);

  puts("OK");
}

/*************************************************************************************/

void test_binn_iter(BOOL use_int_compression) {
  binn *list, *map, *obj;
  binn *list2, *copy=NULL;
  binn_iter iter, iter2;
  binn value, value2;
  int  blob_size, id, id2, list2size;
  void *ptr, *blob_ptr;
  char key[256], key2[256];

  blob_ptr = "key\0value\0\0";
  blob_size = 11;

  printf("testing binn sequential read (use_int_compression = %d)... ", use_int_compression);

  // create the 

  list = binn_list();
  list2 = binn_list();
  map = binn_map();
  obj = binn_object();

  assert(list != NULL);
  assert(list2 != NULL);
  assert(map != NULL);
  assert(obj != NULL);

  if (use_int_compression == FALSE) {
    list->disable_int_compression = TRUE;
    map->disable_int_compression = TRUE;
    obj->disable_int_compression = TRUE;
  }

  assert(binn_list_add_int32(list2, 250) == TRUE);
  assert(binn_list_add_null(list2) == TRUE);
  assert(binn_list_add_str(list2, "l1st2") == TRUE);
  assert(binn_list_add_bool(list2, TRUE) == TRUE);

  list2size = binn_size(list2);

  assert(binn_list_add_int8(list, 111) == TRUE);
  assert(binn_list_add_int32(list, 123456789) == TRUE);
  assert(binn_list_add_int16(list, -123) == TRUE);
  assert(binn_list_add_int64(list, 9876543210) == TRUE);
  assert(binn_list_add_float(list, 1.25) == TRUE);
  assert(binn_list_add_double(list, 25.987654321) == TRUE);
  assert(binn_list_add_bool(list, TRUE) == TRUE);
  assert(binn_list_add_bool(list, FALSE) == TRUE);
  assert(binn_list_add_null(list) == TRUE);
  assert(binn_list_add_str(list, "testing...") == TRUE);
  assert(binn_list_add_blob(list, (char *)blob_ptr, blob_size) == TRUE);
  assert(binn_list_add_list(list, list2) == TRUE);

  assert(binn_object_set_int8(obj, "a", 111) == TRUE);
  assert(binn_object_set_int32(obj, "b", 123456789) == TRUE);
  assert(binn_object_set_int16(obj, "c", -123) == TRUE);
  assert(binn_object_set_int64(obj, "d", 9876543210) == TRUE);
  assert(binn_object_set_float(obj, "e", 1.25) == TRUE);
  assert(binn_object_set_double(obj, "f", 25.987654321) == TRUE);
  assert(binn_object_set_bool(obj, "g", TRUE) == TRUE);
  assert(binn_object_set_bool(obj, "h", FALSE) == TRUE);
  assert(binn_object_set_null(obj, "i") == TRUE);
  assert(binn_object_set_str(obj, "j", "testing...") == TRUE);
  assert(binn_object_set_blob(obj, "k", (char *)blob_ptr, blob_size) == TRUE);
  assert(binn_object_set_list(obj, "l", list2) == TRUE);

  assert(binn_map_set_int8(map, 55010, 111) == TRUE);
  assert(binn_map_set_int32(map, 55020, 123456789) == TRUE);
  assert(binn_map_set_int16(map, 55030, -123) == TRUE);
  assert(binn_map_set_int64(map, 55040, 9876543210) == TRUE);
  assert(binn_map_set_float(map, 55050, 1.25) == TRUE);
  assert(binn_map_set_double(map, 55060, 25.987654321) == TRUE);
  assert(binn_map_set_bool(map, 55070, TRUE) == TRUE);
  assert(binn_map_set_bool(map, 55080, FALSE) == TRUE);
  assert(binn_map_set_null(map, 55090) == TRUE);
  assert(binn_map_set_str(map, 55100, "testing...") == TRUE);
  assert(binn_map_set_blob(map, 55110, (char *)blob_ptr, blob_size) == TRUE);
  assert(binn_map_set_list(map, 55120, list2) == TRUE);



  // read list sequentially - using value

  /*
  assert(binn_iter_init(&iter, binn_ptr(list), BINN_LIST));

  while (binn_list_next(&iter, &value)) {
    ...
  }
  */

  ptr = binn_ptr(list);
  assert(ptr != 0);
  assert(binn_iter_init(&iter, ptr, BINN_LIST));
  assert(iter.pnext > ptr);
  assert(iter.plimit > ptr);
  assert(iter.count == 12);
  assert(iter.current == 0);

  assert(binn_list_next(&iter, &value) == TRUE);
  assert(iter.pnext > ptr);
  assert(iter.plimit > ptr);
  assert(iter.count == 12);
  assert(iter.current == 1);
  assert(value.type == BINN_INT8);
  assert(value.vint8 == 111);

  assert(binn_list_next(&iter, &value) == TRUE);
  assert(iter.current == 2);
  if (use_int_compression) {
    assert(value.type == BINN_UINT32);
  } else {
    assert(value.type == BINN_INT32);
  }
  assert(value.vint32 == 123456789);

  assert(binn_list_next(&iter, &value) == TRUE);
  assert(iter.current == 3);
  if (use_int_compression) {
    assert(value.type == BINN_INT8);
    assert(value.vint8 == -123);
  } else {
    assert(value.type == BINN_INT16);
    assert(value.vint16 == -123);
  }

  assert(binn_list_next(&iter, &value) == TRUE);
  assert(iter.current == 4);
  assert(value.type == BINN_INT64);
  assert(value.vint64 == 9876543210);

  assert(binn_list_next(&iter, &value) == TRUE);
  assert(iter.current == 5);
  assert(value.type == BINN_FLOAT32);
  assert(AlmostEqualFloats(value.vfloat, 1.25, 2));

  assert(binn_list_next(&iter, &value) == TRUE);
  assert(iter.current == 6);
  assert(value.type == BINN_FLOAT64);
  assert(value.vdouble - 25.987654321 < 0.00000001);

  assert(binn_list_next(&iter, &value) == TRUE);
  assert(iter.current == 7);
  assert(value.type == BINN_BOOL);
  assert(value.vbool == TRUE);

  assert(binn_list_next(&iter, &value) == TRUE);
  assert(iter.current == 8);
  assert(value.type == BINN_BOOL);
  assert(value.vbool == FALSE);

  assert(binn_list_next(&iter, &value) == TRUE);
  assert(iter.current == 9);
  assert(value.type == BINN_NULL);

  assert(binn_list_next(&iter, &value) == TRUE);
  assert(iter.current == 10);
  assert(value.type == BINN_STRING);
  assert(strcmp((char *)value.ptr, "testing...") == 0);

  assert(binn_list_next(&iter, &value) == TRUE);
  assert(iter.current == 11);
  assert(value.type == BINN_BLOB);
  assert(memcmp(value.ptr, blob_ptr, blob_size) == 0);

  assert(binn_list_next(&iter, &value) == TRUE);
  assert(iter.current == 12);
  assert(value.type == BINN_LIST);
  assert(value.size == list2size);
  assert(value.count == 4);
  assert(value.ptr != 0);
  assert(binn_list_int32(value.ptr, 1) == 250);
  assert(binn_list_null(value.ptr, 2) == TRUE);
  ptr = binn_list_str(value.ptr, 3);
  assert(ptr != 0);
  assert(strcmp((char *)ptr, "l1st2") == 0);
  assert(binn_list_bool(value.ptr, 4) == TRUE);

  assert(binn_list_next(&iter, &value) == FALSE);
  //assert(iter.current == 13);
  //assert(iter.count == 12);

  assert(binn_list_next(&iter, &value) == FALSE);
  //assert(iter.current == 13);  // must keep the same position
  //assert(iter.count == 12);



  // read object sequentially - using value

  ptr = binn_ptr(obj);
  assert(ptr != 0);
  assert(binn_iter_init(&iter, ptr, BINN_OBJECT));
  assert(iter.pnext > ptr);
  assert(iter.plimit > ptr);
  assert(iter.count == 12);
  assert(iter.current == 0);

  assert(binn_object_next(&iter, key, &value) == TRUE);
  assert(iter.pnext > ptr);
  assert(iter.plimit > ptr);
  assert(iter.count == 12);
  assert(iter.current == 1);
  assert(value.type == BINN_INT8);
  assert(value.vint8 == 111);
  //printf("%s ", key);
  assert(strcmp(key, "a") == 0);

  assert(binn_object_next(&iter, key, &value) == TRUE);
  assert(iter.current == 2);
  if (use_int_compression) {
    assert(value.type == BINN_UINT32);
  } else {
    assert(value.type == BINN_INT32);
  }
  assert(value.vint32 == 123456789);
  //printf("%s ", key);
  assert(strcmp(key, "b") == 0);

  assert(binn_object_next(&iter, key, &value) == TRUE);
  assert(iter.current == 3);
  if (use_int_compression) {
    assert(value.type == BINN_INT8);
    assert(value.vint8 == -123);
  } else {
    assert(value.type == BINN_INT16);
    assert(value.vint16 == -123);
  }
  //printf("%s ", key);
  assert(strcmp(key, "c") == 0);

  assert(binn_object_next(&iter, key, &value) == TRUE);
  assert(iter.current == 4);
  assert(value.type == BINN_INT64);
  assert(value.vint64 == 9876543210);
  //printf("%s ", key);
  assert(strcmp(key, "d") == 0);

  assert(binn_object_next(&iter, key, &value) == TRUE);
  assert(iter.current == 5);
  assert(value.type == BINN_FLOAT32);
  assert(AlmostEqualFloats(value.vfloat, 1.25, 2));
  //printf("%s ", key);
  assert(strcmp(key, "e") == 0);

  assert(binn_object_next(&iter, key, &value) == TRUE);
  assert(iter.current == 6);
  assert(value.type == BINN_FLOAT64);
  assert(value.vdouble - 25.987654321 < 0.00000001);
  //printf("%s ", key);
  assert(strcmp(key, "f") == 0);

  assert(binn_object_next(&iter, key, &value) == TRUE);
  assert(iter.current == 7);
  assert(value.type == BINN_BOOL);
  assert(value.vbool == TRUE);
  //printf("%s ", key);
  assert(strcmp(key, "g") == 0);

  assert(binn_object_next(&iter, key, &value) == TRUE);
  assert(iter.current == 8);
  assert(value.type == BINN_BOOL);
  assert(value.vbool == FALSE);
  //printf("%s ", key);
  assert(strcmp(key, "h") == 0);

  assert(binn_object_next(&iter, key, &value) == TRUE);
  assert(iter.current == 9);
  assert(value.type == BINN_NULL);
  //printf("%s ", key);
  assert(strcmp(key, "i") == 0);

  assert(binn_object_next(&iter, key, &value) == TRUE);
  assert(iter.current == 10);
  assert(value.type == BINN_STRING);
  assert(strcmp((char *)value.ptr, "testing...") == 0);
  //printf("%s ", key);
  assert(strcmp(key, "j") == 0);

  assert(binn_object_next(&iter, key, &value) == TRUE);
  assert(iter.current == 11);
  assert(value.type == BINN_BLOB);
  assert(memcmp(value.ptr, blob_ptr, blob_size) == 0);
  //printf("%s ", key);
  assert(strcmp(key, "k") == 0);

  assert(binn_object_next(&iter, key, &value) == TRUE);
  assert(iter.current == 12);
  assert(value.type == BINN_LIST);
  assert(value.size == list2size);
  assert(value.count == 4);
  assert(value.ptr != 0);
  assert(binn_list_int32(value.ptr, 1) == 250);
  assert(binn_list_null(value.ptr, 2) == TRUE);
  ptr = binn_list_str(value.ptr, 3);
  assert(ptr != 0);
  assert(strcmp((char *)ptr, "l1st2") == 0);
  assert(binn_list_bool(value.ptr, 4) == TRUE);
  //printf("%s ", key);
  assert(strcmp(key, "l") == 0);

  assert(binn_object_next(&iter, key, &value) == FALSE);
  //assert(iter.current == 13);
  //assert(iter.count == 12);

  assert(binn_object_next(&iter, key, &value) == FALSE);
  //assert(iter.current == 13);  // must keep the same position
  //assert(iter.count == 12);




  // read map sequentially - using value

  ptr = binn_ptr(map);
  assert(ptr != 0);
  assert(binn_iter_init(&iter, ptr, BINN_MAP));
  assert(iter.pnext > ptr);
  assert(iter.plimit > ptr);
  assert(iter.count == 12);
  assert(iter.current == 0);

  assert(binn_map_next(&iter, &id, &value) == TRUE);
  assert(iter.pnext > ptr);
  assert(iter.plimit > ptr);
  assert(iter.count == 12);
  assert(iter.current == 1);
  assert(value.type == BINN_INT8);
  assert(value.vint8 == 111);
  assert(id == 55010);

  assert(binn_map_next(&iter, &id, &value) == TRUE);
  assert(iter.current == 2);
  if (use_int_compression) {
    assert(value.type == BINN_UINT32);
  } else {
    assert(value.type == BINN_INT32);
  }
  assert(value.vint32 == 123456789);
  assert(id == 55020);

  assert(binn_map_next(&iter, &id, &value) == TRUE);
  assert(iter.current == 3);
  if (use_int_compression) {
    assert(value.type == BINN_INT8);
    assert(value.vint8 == -123);
  } else {
    assert(value.type == BINN_INT16);
    assert(value.vint16 == -123);
  }
  assert(id == 55030);

  assert(binn_map_next(&iter, &id, &value) == TRUE);
  assert(iter.current == 4);
  assert(value.type == BINN_INT64);
  assert(value.vint64 == 9876543210);
  assert(id == 55040);

  assert(binn_map_next(&iter, &id, &value) == TRUE);
  assert(iter.current == 5);
  assert(value.type == BINN_FLOAT32);
  assert(AlmostEqualFloats(value.vfloat, 1.25, 2));
  assert(id == 55050);

  assert(binn_map_next(&iter, &id, &value) == TRUE);
  assert(iter.current == 6);
  assert(value.type == BINN_FLOAT64);
  assert(value.vdouble - 25.987654321 < 0.00000001);
  assert(id == 55060);

  assert(binn_map_next(&iter, &id, &value) == TRUE);
  assert(iter.current == 7);
  assert(value.type == BINN_BOOL);
  assert(value.vbool == TRUE);
  assert(id == 55070);

  assert(binn_map_next(&iter, &id, &value) == TRUE);
  assert(iter.current == 8);
  assert(value.type == BINN_BOOL);
  assert(value.vbool == FALSE);
  assert(id == 55080);

  assert(binn_map_next(&iter, &id, &value) == TRUE);
  assert(iter.current == 9);
  assert(value.type == BINN_NULL);
  assert(id == 55090);

  assert(binn_map_next(&iter, &id, &value) == TRUE);
  assert(iter.current == 10);
  assert(value.type == BINN_STRING);
  assert(strcmp((char *)value.ptr, "testing...") == 0);
  assert(id == 55100);

  assert(binn_map_next(&iter, &id, &value) == TRUE);
  assert(iter.current == 11);
  assert(value.type == BINN_BLOB);
  assert(memcmp(value.ptr, blob_ptr, blob_size) == 0);
  assert(id == 55110);

  assert(binn_map_next(&iter, &id, &value) == TRUE);
  assert(iter.current == 12);
  assert(value.type == BINN_LIST);
  assert(value.size == list2size);
  assert(value.count == 4);
  assert(value.ptr != 0);
  assert(binn_list_int32(value.ptr, 1) == 250);
  assert(binn_list_null(value.ptr, 2) == TRUE);
  ptr = binn_list_str(value.ptr, 3);
  assert(ptr != 0);
  assert(strcmp((char *)ptr, "l1st2") == 0);
  assert(binn_list_bool(value.ptr, 4) == TRUE);
  assert(id == 55120);

  assert(binn_map_next(&iter, &id, &value) == FALSE);
  //assert(iter.current == 13);
  //assert(iter.count == 12);

  assert(binn_map_next(&iter, &id, &value) == FALSE);
  //assert(iter.current == 13);  // must keep the same position
  //assert(iter.count == 12);


  // test binn copy

  copy = binn_copy(list);
  assert(copy!=NULL);
  assert(binn_type(copy) == binn_type(list));
  assert(binn_count(copy) == binn_count(list));
  assert(binn_size(copy) == binn_size(list));
  assert(binn_iter_init(&iter, list, BINN_LIST));
  assert(binn_iter_init(&iter2, copy, BINN_LIST));
  while( binn_list_next(&iter, &value) ){
    assert(binn_list_next(&iter2, &value2) == TRUE);
    assert(value.type == value2.type);
    //assert(value.vint32 == value.vint32);
  }
  assert(binn_list_add_str(copy, "testing...") == TRUE);
  assert(binn_type(copy) == binn_type(list));
  assert(binn_count(copy) == binn_count(list)+1);
  assert(binn_size(copy) > binn_size(list));
  binn_free(copy);

  copy = binn_copy(map);
  assert(copy!=NULL);
  assert(binn_type(copy) == binn_type(map));
  assert(binn_count(copy) == binn_count(map));
  assert(binn_size(copy) == binn_size(map));
  assert(binn_iter_init(&iter, map, BINN_MAP));
  assert(binn_iter_init(&iter2, copy, BINN_MAP));
  while( binn_map_next(&iter, &id, &value) ){
    assert(binn_map_next(&iter2, &id2, &value2) == TRUE);
    assert(id == id2);
    assert(value.type == value2.type);
    //assert(value.vint32 == value.vint32);
  }
  assert(binn_map_set_int32(copy, 5600, 123) == TRUE);
  assert(binn_type(copy) == binn_type(map));
  assert(binn_count(copy) == binn_count(map)+1);
  assert(binn_size(copy) > binn_size(map));
  binn_free(copy);

  copy = binn_copy(obj);
  assert(copy!=NULL);
  assert(binn_type(copy) == binn_type(obj));
  assert(binn_count(copy) == binn_count(obj));
  assert(binn_size(copy) == binn_size(obj));
  assert(binn_iter_init(&iter, obj, BINN_OBJECT));
  assert(binn_iter_init(&iter2, copy, BINN_OBJECT));
  while( binn_object_next(&iter, key, &value) ){
    assert(binn_object_next(&iter2, key2, &value2) == TRUE);
    assert(strcmp(key,key2)==0);
    assert(value.type == value2.type);
    //assert(value.vint32 == value.vint32);
  }
  assert(binn_object_set_int32(copy, "another", 123) == TRUE);
  assert(binn_type(copy) == binn_type(obj));
  assert(binn_count(copy) == binn_count(obj)+1);
  assert(binn_size(copy) > binn_size(obj));
  binn_free(copy);


  // test binn copy reading from buffer

  ptr = binn_ptr(list);
  copy = binn_copy(ptr);
  assert(copy!=NULL);
  assert(binn_type(copy) == binn_type(list));
  assert(binn_count(copy) == binn_count(list));
  assert(binn_size(copy) == binn_size(list));
  assert(binn_iter_init(&iter, ptr, BINN_LIST));
  assert(binn_iter_init(&iter2, copy, BINN_LIST));
  while( binn_list_next(&iter, &value) ){
    assert(binn_list_next(&iter2, &value2) == TRUE);
    assert(value.type == value2.type);
    //assert(value.vint32 == value.vint32);
  }
  assert(binn_list_add_str(copy, "testing...") == TRUE);
  assert(binn_type(copy) == binn_type(list));
  assert(binn_count(copy) == binn_count(list)+1);
  assert(binn_size(copy) > binn_size(list));
  binn_free(copy);

  ptr = binn_ptr(map);
  copy = binn_copy(ptr);
  assert(copy!=NULL);
  assert(binn_type(copy) == binn_type(map));
  assert(binn_count(copy) == binn_count(map));
  assert(binn_size(copy) == binn_size(map));
  assert(binn_iter_init(&iter, ptr, BINN_MAP));
  assert(binn_iter_init(&iter2, copy, BINN_MAP));
  while( binn_map_next(&iter, &id, &value) ){
    assert(binn_map_next(&iter2, &id2, &value2) == TRUE);
    assert(id == id2);
    assert(value.type == value2.type);
    //assert(value.vint32 == value.vint32);
  }
  assert(binn_map_set_int32(copy, 5600, 123) == TRUE);
  assert(binn_type(copy) == binn_type(map));
  assert(binn_count(copy) == binn_count(map)+1);
  assert(binn_size(copy) > binn_size(map));
  binn_free(copy);

  ptr = binn_ptr(obj);
  copy = binn_copy(ptr);
  assert(copy!=NULL);
  assert(binn_type(copy) == binn_type(obj));
  assert(binn_count(copy) == binn_count(obj));
  assert(binn_size(copy) == binn_size(obj));
  assert(binn_iter_init(&iter, ptr, BINN_OBJECT));
  assert(binn_iter_init(&iter2, copy, BINN_OBJECT));
  while( binn_object_next(&iter, key, &value) ){
    assert(binn_object_next(&iter2, key2, &value2) == TRUE);
    assert(strcmp(key,key2)==0);
    assert(value.type == value2.type);
    //assert(value.vint32 == value.vint32);
  }
  assert(binn_object_set_int32(copy, "another", 123) == TRUE);
  assert(binn_type(copy) == binn_type(obj));
  assert(binn_count(copy) == binn_count(obj)+1);
  assert(binn_size(copy) > binn_size(obj));
  binn_free(copy);


  binn_free(list);
  binn_free(list2);
  binn_free(map);
  binn_free(obj);

  puts("OK");

}

/*************************************************************************************/

void test_binn2() {
  char *obj1ptr, *obj2ptr;
  int  obj1size, obj2size;

  test_virtual_types();

  test_int_conversion();
  test_binn_int_conversion();
  test_value_conversion();
  test_value_copy();

  init_udts();

  obj1ptr = test_create_object_1(&obj1size);
  obj2ptr = test_create_object_2(&obj2size);

  assert(obj1ptr != NULL);
  assert(obj2ptr != NULL);

  printf("obj1size=%d obj2size=%d\n", obj1size, obj2size);
  assert(obj1size == obj2size);

  test_binn_read(obj1ptr);

  test_binn_iter(FALSE);

  test_binn_iter(TRUE);

}

/*************************************************************************************/
/*
void main() {

  test_set_timeout();

  //test_binn2();

}
*/