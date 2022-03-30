#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <math.h>  /* for fabs */
#include <assert.h>
#include "../src/binn.h"

#define BINN_MAGIC            0x1F22B11F

#define MAX_BINN_HEADER       9  // [1:type][4:size][4:count]
#define MIN_BINN_SIZE         3  // [1:type][1:size][1:count]
#define CHUNK_SIZE            256  // 1024

typedef unsigned short int     u16;
typedef unsigned int           u32;
typedef unsigned long long int u64;

extern void* (*malloc_fn)(int len);
extern void* (*realloc_fn)(void *ptr, int len);
extern void  (*free_fn)(void *ptr);

/*************************************************************************************/

void test_binn_version() {
  char *version = binn_version();
  assert(version);
  assert(strcmp(version,"3.0.0")==0);
}

/*************************************************************************************/

BINN_PRIVATE void copy_be16(u16 *pdest, u16 *psource);
BINN_PRIVATE void copy_be32(u32 *pdest, u32 *psource);
BINN_PRIVATE void copy_be64(u64 *pdest, u64 *psource);

void test_endianess() {
  u16 vshort1, vshort2, vshort3;
  u32 vint1, vint2, vint3;
  u64 value1, value2, value3;

  printf("testing endianess... ");

  /* tobe16 */
  vshort1 = 0x1122;
  copy_be16(&vshort2, &vshort1);
#if __BYTE_ORDER == __LITTLE_ENDIAN
  assert(vshort2 == 0x2211);
#else
  assert(vshort2 == 0x1122);
#endif
  copy_be16(&vshort3, &vshort2);
  assert(vshort3 == vshort1);

  vshort1 = 0xF123;
  copy_be16(&vshort2, &vshort1);
#if __BYTE_ORDER == __LITTLE_ENDIAN
  assert(vshort2 == 0x23F1);
#else
  assert(vshort2 == 0xF123);
#endif
  copy_be16(&vshort3, &vshort2);
  assert(vshort3 == vshort1);

  vshort1 = 0x0123;
  copy_be16(&vshort2, &vshort1);
#if __BYTE_ORDER == __LITTLE_ENDIAN
  assert(vshort2 == 0x2301);
#else
  assert(vshort2 == 0x0123);
#endif
  copy_be16(&vshort3, &vshort2);
  assert(vshort3 == vshort1);

  /* tobe32 */
  vint1 = 0x11223344;
  copy_be32(&vint2, &vint1);
#if __BYTE_ORDER == __LITTLE_ENDIAN
  assert(vint2 == 0x44332211);
#else
  assert(vint2 == 0x11223344);
#endif
  copy_be32(&vint3, &vint2);
  assert(vint3 == vint1);

  vint1 = 0xF1234580;
  copy_be32(&vint2, &vint1);
#if __BYTE_ORDER == __LITTLE_ENDIAN
  assert(vint2 == 0x804523F1);
#else
  assert(vint2 == 0xF1234580);
#endif
  copy_be32(&vint3, &vint2);
  assert(vint3 == vint1);

  vint1 = 0x00112233;
  copy_be32(&vint2, &vint1);
#if __BYTE_ORDER == __LITTLE_ENDIAN
  assert(vint2 == 0x33221100);
#else
  assert(vint2 == 0x00112233);
#endif
  copy_be32(&vint3, &vint2);
  assert(vint3 == vint1);

  /* tobe64 */
  value1 = 0x1122334455667788;
  copy_be64(&value2, &value1);
  //printf("v1: %llx\n", value1);
  //printf("v2: %llx\n", value2);
#if __BYTE_ORDER == __LITTLE_ENDIAN
  assert(value2 == 0x8877665544332211);
#else
  assert(value2 == 0x1122334455667788);
#endif
  copy_be64(&value3, &value2);
  assert(value3 == value1);

  printf("OK\n");

}

/***************************************************************************/

void * memdup(void *src, int size) {
  void *dest;

  if (src == NULL || size <= 0) return NULL;
  dest = malloc(size);
  if (dest == NULL) return NULL;
  memcpy(dest, src, size);
  return dest;

}

/***************************************************************************/

char * i64toa(int64 value, char *buf, int radix) {
#ifdef _MSC_VER
  return _i64toa(value, buf, radix);
#else
  switch (radix) {
  case 10:
    snprintf(buf, 64, "%" INT64_FORMAT, value);
    break;
  case 16:
    snprintf(buf, 64, "%" INT64_HEX_FORMAT, value);
    break;
  default:
    buf[0] = 0;
  }
  return buf;
#endif
}

/*************************************************************************************/

void pass_int64(int64 a) {

  assert(a == 9223372036854775807);
  assert(a > 9223372036854775806);

}

int64 return_int64() {

  return 9223372036854775807;

}

int64 return_passed_int64(int64 a) {

  return a;

}

/*************************************************************************************/

void test_int64() {
  int64 i64;
  //uint64 b;
  //long long int b;  -- did not work!
  char buf[64];

  printf("testing int64... ");

  pass_int64(9223372036854775807);

  i64 = return_int64();
  assert(i64 == 9223372036854775807);

  /*  do not worked!
  b = 9223372036854775807;
  printf("value of b1=%" G_GINT64_FORMAT "\n", b);
  snprintf(64, buf, "%" G_GINT64_FORMAT, b);
  printf(" value of b2=%s\n", buf);

  ltoa(i64, buf, 10);
  printf(" value of i64=%s\n", buf);
  */

  i64toa(i64, buf, 10);
  //printf(" value of i64=%s\n", buf);
  assert(strcmp(buf, "9223372036854775807") == 0);

  i64 = return_passed_int64(-987654321987654321);
  assert(i64 == -987654321987654321);

  //snprintf(64, buf, "%" G_GINT64_FORMAT, i64);
  i64toa(i64, buf, 10);
  assert(strcmp(buf, "-987654321987654321") == 0);

  printf("OK\n");

}

/*************************************************************************************/

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

/*************************************************************************************/

#define VERYSMALL  (1.0E-150)
#define EPSILON    (1.0E-8)

#ifndef max
#define max(a,b)   (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b)   (((a) < (b)) ? (a) : (b))
#endif

BOOL AlmostEqualDoubles(double a, double b) {
    double absDiff, maxAbs, absA, absB;

    absDiff = fabs(a - b);
    if (absDiff < VERYSMALL) return TRUE;

    absA = fabs(a);
    absB = fabs(b);
    maxAbs  = max(absA, absB);
    if ((absDiff / maxAbs) < EPSILON)
      return TRUE;
    printf("a=%g b=%g\n", a, b);
    return FALSE;
}

/*************************************************************************************/

void test_floating_point_numbers() {
  char  buf[256];
  float f1;
  double d1;

  printf("testing floating point... ");

  f1 = 1.25;
  assert(f1 == 1.25);
  d1 = 1.25;
  assert(d1 == 1.25);

  d1 = 0;
  d1 = f1;
  assert(d1 == 1.25);
  f1 = 0;
  f1 = d1;
  assert(f1 == 1.25);

  d1 = 1.234;
  assert(AlmostEqualDoubles(d1, 1.234) == TRUE);
  f1 = d1;
  assert(AlmostEqualFloats(f1, 1.234, 2) == TRUE);

  d1 = 1.2345;
  assert(AlmostEqualDoubles(d1, 1.2345) == TRUE);
  f1 = d1;
  assert(AlmostEqualFloats(f1, 1.2345, 2) == TRUE);


  // from string to number, and back to string

  d1 = atof("1.234");  // converts from string to double
  assert(AlmostEqualDoubles(d1, 1.234) == TRUE);
  f1 = d1;             // converts from double to float
  assert(AlmostEqualFloats(f1, 1.234, 2) == TRUE);

  /*
  sprintf(buf, "%f", d1);  // from double to string
  assert(buf[0] != 0);
  assert(strcmp(buf, "1.234") == 0);
  */

  sprintf(buf, "%g", d1);
  assert(buf[0] != 0);
  assert(strcmp(buf, "1.234") == 0);


  d1 = atof("12.34");
  assert(d1 == 12.34);
  f1 = d1;
  assert(AlmostEqualFloats(f1, 12.34, 2) == TRUE);

  /*
  sprintf(buf, "%f", d1);  // from double to string
  assert(buf[0] != 0);
  assert(strcmp(buf, "12.34") == 0);
  */

  sprintf(buf, "%g", d1);
  assert(buf[0] != 0);
  assert(strcmp(buf, "12.34") == 0);


  d1 = atof("1.234e25");
  assert(AlmostEqualDoubles(d1, 1.234e25) == TRUE);
  f1 = d1;
  assert(AlmostEqualFloats(f1, 1.234e25, 2) == TRUE);

  sprintf(buf, "%g", d1);
  assert(buf[0] != 0);
  //printf("\nbuf=%s\n", buf);
  //assert(strcmp(buf, "1.234e+025") == 0);


  printf("OK\n");

}

/*************************************************************************************/

void print_binn(binn *map) {
  unsigned char *p;
  int size, i;
  p = binn_ptr(map);
  size = binn_size(map);
  for(i=0; i<size; i++){
    printf("%02x ", p[i]);
  }
  puts("");
}

/*************************************************************************************/

void test1() {
  static const int fix_size = 512;
  int i, blobsize;
  char *ptr, *p2;
  binn *obj1, *list, *map, *obj;  //, *list2=INVALID_BINN, *map2=INVALID_BINN, *obj2=INVALID_BINN;
  binn value;
  // test values
  char vbyte, *pblob;
  signed short vint16;
  unsigned short vuint16;
  signed int vint32;
  unsigned int vuint32;
  signed long long int vint64;
  unsigned long long int vuint64;

  printf("testing binn 1... ");

  // CalcAllocation and CheckAllocation -------------------------------------------------

  assert(CalcAllocation(512, 512) == 512);
  assert(CalcAllocation(510, 512) == 512);
  assert(CalcAllocation(1, 512) == 512);
  assert(CalcAllocation(0, 512) == 512);

  assert(CalcAllocation(513, 512) == 1024);
  assert(CalcAllocation(512 + CHUNK_SIZE, 512) == 1024);
  assert(CalcAllocation(1025, 512) == 2048);
  assert(CalcAllocation(1025, 1024) == 2048);
  assert(CalcAllocation(2100, 1024) == 4096);

  //assert(CheckAllocation(xxx) == xxx);


  // binn_new() ----------------------------------------------------------------------

  // invalid create calls
  assert(binn_new(-1, 0, NULL) == INVALID_BINN);
  assert(binn_new(0, 0, NULL) == INVALID_BINN);
  assert(binn_new(5, 0, NULL) == INVALID_BINN);
  assert(binn_new(BINN_MAP, -1, NULL) == INVALID_BINN);
  ptr = (char *) &obj1;  // create a valid pointer
  assert(binn_new(BINN_MAP, -1, ptr) == INVALID_BINN);
  assert(binn_new(BINN_MAP, MIN_BINN_SIZE-1, ptr) == INVALID_BINN);

  // first valid create call
  obj1 = binn_new(BINN_LIST, 0, NULL);
  assert(obj1 != INVALID_BINN);

  assert(obj1->header == BINN_MAGIC);
  assert(obj1->type == BINN_LIST);
  assert(obj1->count == 0);
  assert(obj1->pbuf != NULL);
  assert(obj1->alloc_size > MAX_BINN_HEADER);
  assert(obj1->used_size == MAX_BINN_HEADER);
  assert(obj1->pre_allocated == FALSE);

  binn_free(obj1);


  // valid create call
  list = binn_new(BINN_LIST, 0, NULL);
  assert(list != INVALID_BINN);

  // valid create call
  map = binn_new(BINN_MAP, 0, NULL);
  assert(map != INVALID_BINN);

  // valid create call
  obj = binn_new(BINN_OBJECT, 0, NULL);
  assert(obj != INVALID_BINN);

  assert(list->header == BINN_MAGIC);
  assert(list->type == BINN_LIST);
  assert(list->count == 0);
  assert(list->pbuf != NULL);
  assert(list->alloc_size > MAX_BINN_HEADER);
  assert(list->used_size == MAX_BINN_HEADER);
  assert(list->pre_allocated == FALSE);

  assert(map->header == BINN_MAGIC);
  assert(map->type == BINN_MAP);
  assert(map->count == 0);
  assert(map->pbuf != NULL);
  assert(map->alloc_size > MAX_BINN_HEADER);
  assert(map->used_size == MAX_BINN_HEADER);
  assert(map->pre_allocated == FALSE);

  assert(obj->header == BINN_MAGIC);
  assert(obj->type == BINN_OBJECT);
  assert(obj->count == 0);
  assert(obj->pbuf != NULL);
  assert(obj->alloc_size > MAX_BINN_HEADER);
  assert(obj->used_size == MAX_BINN_HEADER);
  assert(obj->pre_allocated == FALSE);


  // test create with pre-allocated buffer ----------------------------------------------

  ptr = malloc(fix_size);
  assert(ptr != NULL);

  obj1 = binn_new(BINN_OBJECT, fix_size, ptr);
  assert(obj1 != INVALID_BINN);

  assert(obj1->header == BINN_MAGIC);
  assert(obj1->type == BINN_OBJECT);
  assert(obj1->count == 0);
  assert(obj1->pbuf != NULL);
  assert(obj1->alloc_size == fix_size);
  assert(obj1->used_size == MAX_BINN_HEADER);
  assert(obj1->pre_allocated == TRUE);


  // add values - invalid ---------------------------------------------------------------

  assert(binn_map_set(list, 55001, BINN_INT32, &i, 0) == FALSE);
  assert(binn_object_set(list, "test", BINN_INT32, &i, 0) == FALSE);

  assert(binn_list_add(map, BINN_INT32, &i, 0) == FALSE);
  assert(binn_object_set(map, "test", BINN_INT32, &i, 0) == FALSE);

  assert(binn_list_add(obj, BINN_INT32, &i, 0) == FALSE);
  assert(binn_map_set(obj, 55001, BINN_INT32, &i, 0) == FALSE);

  // invalid type
  assert(binn_list_add(list, -1, &i, 0) == FALSE);
  assert(binn_list_add(list, 0x1FFFF, &i, 0) == FALSE);
  assert(binn_map_set(map, 5501, -1, &i, 0) == FALSE);
  assert(binn_map_set(map, 5501, 0x1FFFF, &i, 0) == FALSE);
  assert(binn_object_set(obj, "test", -1, &i, 0) == FALSE);
  assert(binn_object_set(obj, "test", 0x1FFFF, &i, 0) == FALSE);

  // null pointers
  assert(binn_list_add(list, BINN_INT8, NULL, 0) == FALSE);
  assert(binn_list_add(list, BINN_INT16, NULL, 0) == FALSE);
  assert(binn_list_add(list, BINN_INT32, NULL, 0) == FALSE);
  assert(binn_list_add(list, BINN_INT64, NULL, 0) == FALSE);
  //assert(binn_list_add(list, BINN_STRING, NULL, 0) == TRUE);  //*
  assert(binn_map_set(map, 5501, BINN_INT8, NULL, 0) == FALSE);
  assert(binn_map_set(map, 5501, BINN_INT16, NULL, 0) == FALSE);
  assert(binn_map_set(map, 5501, BINN_INT32, NULL, 0) == FALSE);
  assert(binn_map_set(map, 5501, BINN_INT64, NULL, 0) == FALSE);
  //assert(binn_map_set(map, 5501, BINN_STRING, NULL, 0) == TRUE);  //*
  assert(binn_object_set(obj, "test", BINN_INT8, NULL, 0) == FALSE);
  assert(binn_object_set(obj, "test", BINN_INT16, NULL, 0) == FALSE);
  assert(binn_object_set(obj, "test", BINN_INT32, NULL, 0) == FALSE);
  assert(binn_object_set(obj, "test", BINN_INT64, NULL, 0) == FALSE);
  //assert(binn_object_set(obj, "test", BINN_STRING, NULL, 0) == TRUE);  //*

  // blobs with null pointers
  assert(binn_list_add(list, BINN_BLOB, NULL, -1) == FALSE);
  assert(binn_list_add(list, BINN_BLOB, NULL, 10) == FALSE);
  assert(binn_map_set(map, 5501, BINN_BLOB, NULL, -1) == FALSE);
  assert(binn_map_set(map, 5501, BINN_BLOB, NULL, 10) == FALSE);
  assert(binn_object_set(obj, "test", BINN_BLOB, NULL, -1) == FALSE);
  assert(binn_object_set(obj, "test", BINN_BLOB, NULL, 10) == FALSE);

  // blobs with negative values
  assert(binn_list_add(list, BINN_BLOB, &i, -1) == FALSE);
  assert(binn_list_add(list, BINN_BLOB, &i, -15) == FALSE);
  assert(binn_map_set(map, 5501, BINN_BLOB, &i, -1) == FALSE);
  assert(binn_map_set(map, 5501, BINN_BLOB, &i, -15) == FALSE);
  assert(binn_object_set(obj, "test", BINN_BLOB, &i, -1) == FALSE);
  assert(binn_object_set(obj, "test", BINN_BLOB, &i, -15) == FALSE);



  // read values - invalid 1 - empty binns -------------------------------------------

  ptr = binn_ptr(list);
  assert(ptr != NULL);
  assert(binn_list_get_value(ptr, 0, &value) == FALSE);
  assert(binn_list_get_value(ptr, 1, &value) == FALSE);
  assert(binn_list_get_value(ptr, 2, &value) == FALSE);
  assert(binn_list_get_value(ptr, -1, &value) == FALSE);

  ptr = binn_ptr(map);
  assert(ptr != NULL);
  assert(binn_list_get_value(ptr, 0, &value) == FALSE);
  assert(binn_list_get_value(ptr, 1, &value) == FALSE);
  assert(binn_list_get_value(ptr, 2, &value) == FALSE);
  assert(binn_list_get_value(ptr, -1, &value) == FALSE);

  ptr = binn_ptr(obj);
  assert(ptr != NULL);
  assert(binn_list_get_value(ptr, 0, &value) == FALSE);
  assert(binn_list_get_value(ptr, 1, &value) == FALSE);
  assert(binn_list_get_value(ptr, 2, &value) == FALSE);
  assert(binn_list_get_value(ptr, -1, &value) == FALSE);


  // add values - valid -----------------------------------------------------------------

  i = 0x1234;

  assert(binn_list_add(list, BINN_INT32, &i, 0) == TRUE);
  assert(binn_map_set(map, 5501, BINN_INT32, &i, 0) == TRUE);
  assert(binn_map_set(map, 5501, BINN_INT32, &i, 0) == FALSE);       // with the same ID
  assert(binn_object_set(obj, "test", BINN_INT32, &i, 0) == TRUE);
  assert(binn_object_set(obj, "test", BINN_INT32, &i, 0) == FALSE); // with the same name

  vbyte = 255;
  vint16 = -32000;
  vuint16 = 65000;
  vint32 = -65000000;
  vuint32 = 65000000;
  vint64 = -6500000000000000;
  vuint64 = 6500000000000000;
  blobsize = 150;
  pblob = malloc(blobsize);
  assert(pblob != NULL);
  memset(pblob, 55, blobsize);

  assert(binn_list_add(list, BINN_NULL, 0, 0) == TRUE);           // second
  assert(binn_list_add(list, BINN_UINT8, &vbyte, 0) == TRUE);     // third
  assert(binn_list_add(list, BINN_INT16, &vint16, 0) == TRUE);    // fourth
  assert(binn_list_add(list, BINN_UINT16, &vuint16, 0) == TRUE);  // fifth
  assert(binn_list_add(list, BINN_INT32, &vint32, 0) == TRUE);    // 6th
  assert(binn_list_add(list, BINN_UINT32, &vuint32, 0) == TRUE);  // 7th
  assert(binn_list_add(list, BINN_INT64, &vint64, 0) == TRUE);    // 8th
  assert(binn_list_add(list, BINN_UINT64, &vuint64, 0) == TRUE);  // 9th
  assert(binn_list_add(list, BINN_STRING, "this is the string", 0) == TRUE); // 10th
  assert(binn_list_add(list, BINN_BLOB, pblob, blobsize) == TRUE);           // 11th

  assert(binn_map_set(map, 99000, BINN_NULL, 0, 0) == TRUE);           // third
  assert(binn_map_set(map, 99001, BINN_UINT8, &vbyte, 0) == TRUE);     // fourth
  assert(binn_map_set(map, 99002, BINN_INT16, &vint16, 0) == TRUE);    // fifth
  assert(binn_map_set(map, 99003, BINN_UINT16, &vuint16, 0) == TRUE);  // 6th
  assert(binn_map_set(map, 99004, BINN_INT32, &vint32, 0) == TRUE);    // 7th
  assert(binn_map_set(map, 99005, BINN_UINT32, &vuint32, 0) == TRUE);  // 8th
  assert(binn_map_set(map, 99006, BINN_INT64, &vint64, 0) == TRUE);    // 9th
  assert(binn_map_set(map, 99007, BINN_UINT64, &vuint64, 0) == TRUE);  // 10th
  assert(binn_map_set(map, 99008, BINN_STRING, "this is the string", 0) == TRUE); // 11th
  assert(binn_map_set(map, 99009, BINN_BLOB, pblob, blobsize) == TRUE);           // 12th

  assert(binn_object_set(obj, "key0", BINN_NULL, 0, 0) == TRUE);           // third
  assert(binn_object_set(obj, "key1", BINN_UINT8, &vbyte, 0) == TRUE);     // fourth
  assert(binn_object_set(obj, "key2", BINN_INT16, &vint16, 0) == TRUE);    // fifth
  assert(binn_object_set(obj, "key3", BINN_UINT16, &vuint16, 0) == TRUE);  // 6th
  assert(binn_object_set(obj, "key4", BINN_INT32, &vint32, 0) == TRUE);    // 7th
  assert(binn_object_set(obj, "key5", BINN_UINT32, &vuint32, 0) == TRUE);  // 8th
  assert(binn_object_set(obj, "key6", BINN_INT64, &vint64, 0) == TRUE);    // 9th
  assert(binn_object_set(obj, "key7", BINN_UINT64, &vuint64, 0) == TRUE);  // 10th
  assert(binn_object_set(obj, "key8", BINN_STRING, "this is the string", 0) == TRUE); // 11th
  assert(binn_object_set(obj, "key9", BINN_BLOB, pblob, blobsize) == TRUE);           // 12th

  // blobs with size = 0
  assert(binn_list_add(list, BINN_BLOB, ptr, 0) == TRUE);
  assert(binn_list_add(list, BINN_STRING, "", 0) == TRUE);
  assert(binn_list_add(list, BINN_STRING, "after the empty items", 0) == TRUE);


  // test different id sizes on maps

  // positive values
  assert(binn_map_set(map, 0x09, BINN_UINT8, &vbyte, 0) == TRUE);
  assert(binn_map_set(map, 0x3F, BINN_UINT8, &vbyte, 0) == TRUE);
  assert(binn_map_set(map, 0x4F, BINN_UINT8, &vbyte, 0) == TRUE);
  assert(binn_map_set(map, 0xFFF, BINN_UINT8, &vbyte, 0) == TRUE);
  assert(binn_map_set(map, 0xFFFFF, BINN_UINT8, &vbyte, 0) == TRUE);
  assert(binn_map_set(map, 0xFFFFFFF, BINN_UINT8, &vbyte, 0) == TRUE);
  assert(binn_map_set(map, 0x7FFFFFFF, BINN_UINT8, &vbyte, 0) == TRUE);
  // negative values
  assert(binn_map_set(map, -0x09, BINN_UINT8, &vbyte, 0) == TRUE);
  assert(binn_map_set(map, -0x3F, BINN_UINT8, &vbyte, 0) == TRUE);
  assert(binn_map_set(map, -0x4F, BINN_UINT8, &vbyte, 0) == TRUE);
  assert(binn_map_set(map, -0xFFF, BINN_UINT8, &vbyte, 0) == TRUE);
  assert(binn_map_set(map, -0xFFFFF, BINN_UINT8, &vbyte, 0) == TRUE);
  assert(binn_map_set(map, -0xFFFFFFF, BINN_UINT8, &vbyte, 0) == TRUE);
  assert(binn_map_set(map, -0x7FFFFFFF, BINN_UINT8, &vbyte, 0) == TRUE);

  // positive values
  assert(binn_map_set(map, 0x09, BINN_UINT8, &vbyte, 0) == FALSE);
  assert(binn_map_set(map, 0x3F, BINN_UINT8, &vbyte, 0) == FALSE);
  assert(binn_map_set(map, 0x4F, BINN_UINT8, &vbyte, 0) == FALSE);
  assert(binn_map_set(map, 0xFFF, BINN_UINT8, &vbyte, 0) == FALSE);
  assert(binn_map_set(map, 0xFFFFF, BINN_UINT8, &vbyte, 0) == FALSE);
  assert(binn_map_set(map, 0xFFFFFFF, BINN_UINT8, &vbyte, 0) == FALSE);
  assert(binn_map_set(map, 0x7FFFFFFF, BINN_UINT8, &vbyte, 0) == FALSE);
  // negative values
  assert(binn_map_set(map, -0x09, BINN_UINT8, &vbyte, 0) == FALSE);
  assert(binn_map_set(map, -0x3F, BINN_UINT8, &vbyte, 0) == FALSE);
  assert(binn_map_set(map, -0x4F, BINN_UINT8, &vbyte, 0) == FALSE);
  assert(binn_map_set(map, -0xFFF, BINN_UINT8, &vbyte, 0) == FALSE);
  assert(binn_map_set(map, -0xFFFFF, BINN_UINT8, &vbyte, 0) == FALSE);
  assert(binn_map_set(map, -0xFFFFFFF, BINN_UINT8, &vbyte, 0) == FALSE);
  assert(binn_map_set(map, -0x7FFFFFFF, BINN_UINT8, &vbyte, 0) == FALSE);


  // add values to a fixed-size binn (pre-allocated buffer) --------------------------

  assert(binn_list_add(obj1, BINN_INT32, &i, 0) == FALSE);
  assert(binn_map_set(obj1, 55001, BINN_INT32, &i, 0) == FALSE);

  assert(binn_object_set(obj1, "test", BINN_UINT32, &vuint32, 0) == TRUE);
  assert(binn_object_set(obj1, "test", BINN_UINT32, &vuint32, 0) == FALSE);  // with the same name

  assert(binn_object_set(obj1, "key1", BINN_STRING, "this is the value", 0) == TRUE);
  assert(binn_object_set(obj1, "key2", BINN_STRING, "the second value", 0) == TRUE);

  // create a long string buffer to make the test. the string is longer than the available space
  // in the binn.
  ptr = malloc(fix_size);
  assert(ptr != NULL);
  p2 = ptr;
  for (i = 0; i < fix_size - 1; i++) {
    *p2 = 'A'; p2++;
  }
  *p2 = '\0';
  assert(strlen(ptr) == fix_size - 1);

  assert(binn_object_set(obj1, "v2", BINN_STRING, ptr, 0) == FALSE); // it fails because it uses a pre-allocated memory block

  assert(binn_object_set(obj, "v2", BINN_STRING, ptr, 0) == TRUE); // but this uses a dynamically allocated memory block, so it works with it
  assert(binn_object_set(obj, "Key00", BINN_STRING, "after the big string", 0) == TRUE); // and test the 'Key00' against the 'Key0'

  free(ptr); ptr = 0;

  assert(binn_object_set(obj, "list", BINN_LIST, binn_ptr(list), binn_size(list)) == TRUE);
  assert(binn_object_set(obj, "Key10", BINN_STRING, "after the list", 0) == TRUE); // and test the 'Key10' against the 'Key1'


  // read values - invalid 2 ------------------------------------------------------------






  // read keys --------------------------------------------------------------------------



  // binn_size - invalid and valid args --------------------------------------------

  assert(binn_size(NULL) == 0);

  assert(binn_size(list) == list->size);
  assert(binn_size(map) == map->size);
  assert(binn_size(obj) == obj->size);
  assert(binn_size(obj1) == obj1->size);


  // destroy them all -------------------------------------------------------------------

  binn_free(list);
  binn_free(map);
  binn_free(obj);
  binn_free(obj1);


  printf("OK\n");

}

/*************************************************************************************/

void test2(BOOL use_int_compression) {
  binn *list=INVALID_BINN, *map=INVALID_BINN, *obj=INVALID_BINN;
  binn value;
  BOOL vbool;
  int blobsize;
  char *pblob, *pstr;
  signed int vint32;
  double vdouble;

  char *str_list = "test list";
  char *str_map = "test map";
  char *str_obj = "test object";

  printf("testing binn 2 (use_int_compression = %d)... ", use_int_compression);

  blobsize = 150;
  pblob = malloc(blobsize);
  assert(pblob != NULL);
  memset(pblob, 55, blobsize);

  assert(list == INVALID_BINN);
  assert(map == INVALID_BINN);
  assert(obj == INVALID_BINN);

  // add values without creating before

  assert(binn_list_add_int32(list, 123) == FALSE);
  assert(binn_map_set_int32(map, 1001, 456) == FALSE);
  assert(binn_object_set_int32(obj, "int", 789) == FALSE);

  // create the structures

  list = binn_list();
  map = binn_map();
  obj = binn_object();

  assert(list != INVALID_BINN);
  assert(map != INVALID_BINN);
  assert(obj != INVALID_BINN);

  if (use_int_compression == FALSE) {
    list->disable_int_compression = TRUE;
    map->disable_int_compression = TRUE;
    obj->disable_int_compression = TRUE;
  }

  // add values without creating before

  assert(binn_list_add_int32(list, 123) == TRUE);
  assert(binn_map_set_int32(map, 1001, 456) == TRUE);
  assert(binn_object_set_int32(obj, "int", 789) == TRUE);

  // check the structures

  assert(list->header == BINN_MAGIC);
  assert(list->type == BINN_LIST);
  assert(list->count == 1);
  assert(list->pbuf != NULL);
  assert(list->alloc_size > MAX_BINN_HEADER);
  assert(list->used_size > MAX_BINN_HEADER);
  assert(list->pre_allocated == FALSE);

  assert(map->header == BINN_MAGIC);
  assert(map->type == BINN_MAP);
  assert(map->count == 1);
  assert(map->pbuf != NULL);
  assert(map->alloc_size > MAX_BINN_HEADER);
  assert(map->used_size > MAX_BINN_HEADER);
  assert(map->pre_allocated == FALSE);

  assert(obj->header == BINN_MAGIC);
  assert(obj->type == BINN_OBJECT);
  assert(obj->count == 1);
  assert(obj->pbuf != NULL);
  assert(obj->alloc_size > MAX_BINN_HEADER);
  assert(obj->used_size > MAX_BINN_HEADER);
  assert(obj->pre_allocated == FALSE);


  // continue adding values

  assert(binn_list_add_double(list, 1.23) == TRUE);
  assert(binn_map_set_double(map, 1002, 4.56) == TRUE);
  assert(binn_object_set_double(obj, "double", 7.89) == TRUE);

  assert(list->count == 2);
  assert(map->count == 2);
  assert(obj->count == 2);

  assert(binn_list_add_bool(list, TRUE) == TRUE);
  assert(binn_map_set_bool(map, 1003, TRUE) == TRUE);
  assert(binn_object_set_bool(obj, "bool", TRUE) == TRUE);

  assert(list->count == 3);
  assert(map->count == 3);
  assert(obj->count == 3);

  assert(binn_list_add_str(list, str_list) == TRUE);
  assert(binn_map_set_str(map, 1004, str_map) == TRUE);
  assert(binn_object_set_str(obj, "text", str_obj) == TRUE);

  assert(list->count == 4);
  assert(map->count == 4);
  assert(obj->count == 4);

  assert(binn_list_add_blob(list, pblob, blobsize) == TRUE);
  assert(binn_map_set_blob(map, 1005, pblob, blobsize) == TRUE);
  assert(binn_object_set_blob(obj, "blob", pblob, blobsize) == TRUE);

  assert(list->count == 5);
  assert(map->count == 5);
  assert(obj->count == 5);

  assert(binn_count(list) == 5);
  assert(binn_count(map) == 5);
  assert(binn_count(obj) == 5);

  assert(binn_size(list) == list->size);
  assert(binn_size(map) == map->size);
  assert(binn_size(obj) == obj->size);

  assert(binn_type(list) == BINN_LIST);
  assert(binn_type(map) == BINN_MAP);
  assert(binn_type(obj) == BINN_OBJECT);


  // try to read them

  // integer

  assert(binn_list_get_value(list, 1, &value) == TRUE);

  assert(value.header == BINN_MAGIC);
  assert(value.writable == FALSE);
  assert(value.allocated == FALSE);
  if (use_int_compression) {
    assert(value.type == BINN_UINT8);
    assert(value.ptr != &value.vuint8);  // it must return a pointer to the byte in the buffer
  } else {
    assert(value.type == BINN_INT32);
    assert(value.ptr == &value.vint);
  }
  assert(value.size == 0);
  assert(value.count == 0);
  assert(value.vint == 123);

  memset(&value, 0, sizeof(binn));

  assert(binn_map_get_value(map, 1001, &value) == TRUE);

  assert(value.header == BINN_MAGIC);
  assert(value.writable == FALSE);
  if (use_int_compression) {
    assert(value.type == BINN_UINT16);
    assert(value.ptr == &value.vuint16);
  } else {
    assert(value.type == BINN_INT32);
    assert(value.ptr == &value.vint);
  }
  assert(value.size == 0);
  assert(value.count == 0);
  assert(value.vint == 456);

  memset(&value, 0, sizeof(binn));

  assert(binn_object_get_value(obj, "int", &value) == TRUE);

  assert(value.header == BINN_MAGIC);
  assert(value.writable == FALSE);
  if (use_int_compression) {
    assert(value.type == BINN_UINT16);
    assert(value.ptr == &value.vuint16);
  } else {
    assert(value.type == BINN_INT32);
    assert(value.ptr == &value.vint);
  }
  assert(value.size == 0);
  assert(value.count == 0);
  assert(value.vint == 789);

  memset(&value, 0, sizeof(binn));


  // double

  assert(binn_list_get_value(list, 2, &value) == TRUE);

  assert(value.header == BINN_MAGIC);
  assert(value.writable == FALSE);
  assert(value.type == BINN_FLOAT64);
  assert(value.ptr == &value.vint);
  assert(value.size == 0);
  assert(value.count == 0);
  assert(value.vdouble == 1.23);

  memset(&value, 0, sizeof(binn));

  assert(binn_map_get_value(map, 1002, &value) == TRUE);

  assert(value.header == BINN_MAGIC);
  assert(value.writable == FALSE);
  assert(value.type == BINN_FLOAT64);
  assert(value.ptr == &value.vint);
  assert(value.size == 0);
  assert(value.count == 0);
  assert(value.vdouble == 4.56);

  memset(&value, 0, sizeof(binn));

  assert(binn_object_get_value(obj, "double", &value) == TRUE);

  assert(value.header == BINN_MAGIC);
  assert(value.writable == FALSE);
  assert(value.type == BINN_FLOAT64);
  assert(value.ptr == &value.vint);
  assert(value.size == 0);
  assert(value.count == 0);
  assert(value.vdouble == 7.89);

  memset(&value, 0, sizeof(binn));


  // bool

  assert(binn_list_get_value(list, 3, &value) == TRUE);

  assert(value.header == BINN_MAGIC);
  assert(value.writable == FALSE);
  assert(value.type == BINN_BOOL);
  assert(value.ptr == &value.vint);
  assert(value.size == 0);
  assert(value.count == 0);
  assert(value.vbool == TRUE);

  memset(&value, 0, sizeof(binn));

  assert(binn_map_get_value(map, 1003, &value) == TRUE);

  assert(value.header == BINN_MAGIC);
  assert(value.writable == FALSE);
  assert(value.type == BINN_BOOL);
  assert(value.ptr == &value.vint);
  assert(value.size == 0);
  assert(value.count == 0);
  assert(value.vbool == TRUE);

  assert(binn_object_get_value(obj, "bool", &value) == TRUE);

  assert(value.header == BINN_MAGIC);
  assert(value.writable == FALSE);
  assert(value.type == BINN_BOOL);
  assert(value.ptr == &value.vint);
  assert(value.size == 0);
  assert(value.count == 0);
  assert(value.vbool == TRUE);

  memset(&value, 0, sizeof(binn));


  // string

  assert(binn_list_get_value(list, 4, &value) == TRUE);

  assert(value.header == BINN_MAGIC);
  assert(value.writable == FALSE);
  assert(value.type == BINN_STRING);
  assert(value.ptr != 0);
  assert(value.size == strlen(str_list));
  assert(strcmp(value.ptr, str_list) == 0);
  assert(value.count == 0);

  memset(&value, 0, sizeof(binn));

  assert(binn_map_get_value(map, 1004, &value) == TRUE);

  assert(value.header == BINN_MAGIC);
  assert(value.writable == FALSE);
  assert(value.type == BINN_STRING);
  assert(value.size == strlen(str_map));
  assert(strcmp(value.ptr, str_map) == 0);
  assert(value.count == 0);

  memset(&value, 0, sizeof(binn));

  assert(binn_object_get_value(obj, "text", &value) == TRUE);

  assert(value.header == BINN_MAGIC);
  assert(value.writable == FALSE);
  assert(value.type == BINN_STRING);
  assert(value.size == strlen(str_obj));
  assert(strcmp(value.ptr, str_obj) == 0);
  assert(value.count == 0);

  memset(&value, 0, sizeof(binn));


  // blob

  assert(binn_list_get_value(list, 5, &value) == TRUE);

  assert(value.header == BINN_MAGIC);
  assert(value.writable == FALSE);
  assert(value.type == BINN_BLOB);
  assert(value.ptr != 0);
  assert(value.size == blobsize);
  assert(memcmp(value.ptr, pblob, blobsize) == 0);
  assert(value.count == 0);

  memset(&value, 0, sizeof(binn));

  assert(binn_map_get_value(map, 1005, &value) == TRUE);

  assert(value.header == BINN_MAGIC);
  assert(value.writable == FALSE);
  assert(value.type == BINN_BLOB);
  assert(value.ptr != 0);
  assert(value.size == blobsize);
  assert(memcmp(value.ptr, pblob, blobsize) == 0);
  assert(value.count == 0);

  memset(&value, 0, sizeof(binn));

  assert(binn_object_get_value(obj, "blob", &value) == TRUE);

  assert(value.header == BINN_MAGIC);
  assert(value.writable == FALSE);
  assert(value.type == BINN_BLOB);
  assert(value.ptr != 0);
  assert(value.size == blobsize);
  assert(memcmp(value.ptr, pblob, blobsize) == 0);
  assert(value.count == 0);

  memset(&value, 0, sizeof(binn));



  // read with other interface

  assert(binn_list_get_int32(list, 1, &vint32) == TRUE);
  assert(vint32 == 123);

  assert(binn_map_get_int32(map, 1001, &vint32) == TRUE);
  assert(vint32 == 456);

  assert(binn_object_get_int32(obj, "int", &vint32) == TRUE);
  assert(vint32 == 789);

  // double

  assert(binn_list_get_double(list, 2, &vdouble) == TRUE);
  assert(vdouble == 1.23);

  assert(binn_map_get_double(map, 1002, &vdouble) == TRUE);
  assert(vdouble == 4.56);

  assert(binn_object_get_double(obj, "double", &vdouble) == TRUE);
  assert(vdouble == 7.89);

  // bool

  assert(binn_list_get_bool(list, 3, &vbool) == TRUE);
  assert(vbool == TRUE);

  assert(binn_map_get_bool(map, 1003, &vbool) == TRUE);
  assert(vbool == TRUE);

  assert(binn_object_get_bool(obj, "bool", &vbool) == TRUE);
  assert(vbool == TRUE);

  // string

  assert(binn_list_get_str(list, 4, &pstr) == TRUE);
  assert(pstr != 0);
  assert(strcmp(pstr, str_list) == 0);

  assert(binn_map_get_str(map, 1004, &pstr) == TRUE);
  assert(pstr != 0);
  assert(strcmp(pstr, str_map) == 0);

  assert(binn_object_get_str(obj, "text", &pstr) == TRUE);
  assert(pstr != 0);
  assert(strcmp(pstr, str_obj) == 0);

  // blob

  value.ptr = 0;
  value.size = 0;
  assert(binn_list_get_blob(list, 5, &value.ptr, &value.size) == TRUE);
  assert(value.ptr != 0);
  assert(value.size == blobsize);
  assert(memcmp(value.ptr, pblob, blobsize) == 0);

  value.ptr = 0;
  value.size = 0;
  assert(binn_map_get_blob(map, 1005, &value.ptr, &value.size) == TRUE);
  assert(value.ptr != 0);
  assert(value.size == blobsize);
  assert(memcmp(value.ptr, pblob, blobsize) == 0);

  value.ptr = 0;
  value.size = 0;
  assert(binn_object_get_blob(obj, "blob", &value.ptr, &value.size) == TRUE);
  assert(value.ptr != 0);
  assert(value.size == blobsize);
  assert(memcmp(value.ptr, pblob, blobsize) == 0);



  // read with other interface

  assert(binn_list_int32(list, 1) == 123);
  assert(binn_map_int32(map, 1001) == 456);
  assert(binn_object_int32(obj, "int") == 789);

  // double

  assert(binn_list_double(list, 2) == 1.23);
  assert(binn_map_double(map, 1002) == 4.56);
  assert(binn_object_double(obj, "double") == 7.89);

  // bool

  assert(binn_list_bool(list, 3) == TRUE);
  assert(binn_map_bool(map, 1003) == TRUE);
  assert(binn_object_bool(obj, "bool") == TRUE);

  // string

  pstr = binn_list_str(list, 4);
  assert(pstr != 0);
  assert(strcmp(pstr, str_list) == 0);

  pstr = binn_map_str(map, 1004);
  assert(pstr != 0);
  assert(strcmp(pstr, str_map) == 0);

  pstr = binn_object_str(obj, "text");
  assert(pstr != 0);
  assert(strcmp(pstr, str_obj) == 0);

  // blob

  value.ptr = binn_list_blob(list, 5, &value.size);
  assert(value.ptr != 0);
  assert(value.size == blobsize);
  assert(memcmp(value.ptr, pblob, blobsize) == 0);

  value.ptr = binn_map_blob(map, 1005, &value.size);
  assert(value.ptr != 0);
  assert(value.size == blobsize);
  assert(memcmp(value.ptr, pblob, blobsize) == 0);

  value.ptr = binn_object_blob(obj, "blob", &value.size);
  assert(value.ptr != 0);
  assert(value.size == blobsize);
  assert(memcmp(value.ptr, pblob, blobsize) == 0);


  binn_free(list);
  binn_free(map);
  binn_free(obj);

  printf("OK\n");

}

/*************************************************************************************/

void test3() {
  static const int fix_size = 512;
  int i, id, type, count, size, header_size, blobsize;
  char *ptr, *p2, *pstr, key[256];
  binn *list, *map, *obj, *obj1;
  binn value;
  // test values
  char vbyte, *pblob;
  signed short vint16, *pint16;
  unsigned short vuint16, *puint16;
  signed int vint32, *pint32;
  unsigned int vuint32, *puint32;
  signed long long int vint64, *pint64;
  unsigned long long int vuint64, *puint64;

  printf("testing binn 3... ");

  list = binn_list();
  assert(list != INVALID_BINN);

  map = binn_map();
  assert(map != INVALID_BINN);

  obj = binn_object();
  assert(obj != INVALID_BINN);

  assert(list->header == BINN_MAGIC);
  assert(list->type == BINN_LIST);
  assert(list->count == 0);
  assert(list->pbuf != NULL);
  assert(list->alloc_size > MAX_BINN_HEADER);
  assert(list->used_size == MAX_BINN_HEADER);
  assert(list->pre_allocated == FALSE);

  assert(map->header == BINN_MAGIC);
  assert(map->type == BINN_MAP);
  assert(map->count == 0);
  assert(map->pbuf != NULL);
  assert(map->alloc_size > MAX_BINN_HEADER);
  assert(map->used_size == MAX_BINN_HEADER);
  assert(map->pre_allocated == FALSE);

  assert(obj->header == BINN_MAGIC);
  assert(obj->type == BINN_OBJECT);
  assert(obj->count == 0);
  assert(obj->pbuf != NULL);
  assert(obj->alloc_size > MAX_BINN_HEADER);
  assert(obj->used_size == MAX_BINN_HEADER);
  assert(obj->pre_allocated == FALSE);


  // test create with pre-allocated buffer ----------------------------------------------

  ptr = malloc(fix_size);
  assert(ptr != NULL);

  obj1 = binn_new(BINN_OBJECT, fix_size, ptr);
  assert(obj1 != INVALID_BINN);

  assert(obj1->header == BINN_MAGIC);
  assert(obj1->type == BINN_OBJECT);
  assert(obj1->count == 0);
  assert(obj1->pbuf != NULL);
  assert(obj1->alloc_size == fix_size);
  assert(obj1->used_size == MAX_BINN_HEADER);
  assert(obj1->pre_allocated == TRUE);


  // add values - invalid ---------------------------------------------------------------



  // read values - invalid 1 - empty binns -------------------------------------------

  ptr = binn_ptr(list);
  assert(ptr != NULL);
  assert(binn_list_read(ptr, 0, &type, &size) == NULL);
  assert(binn_list_read(ptr, 1, &type, &size) == NULL);
  assert(binn_list_read(ptr, 2, &type, &size) == NULL);
  assert(binn_list_read(ptr, -1, &type, &size) == NULL);

  ptr = binn_ptr(map);
  assert(ptr != NULL);
  assert(binn_map_read(ptr,     0, &type, &size) == NULL);
  assert(binn_map_read(ptr, 55001, &type, &size) == NULL);
  assert(binn_map_read(ptr,    -1, &type, &size) == NULL);

  ptr = binn_ptr(obj);
  assert(ptr != NULL);
  assert(binn_object_read(ptr, NULL,   &type, &size) == NULL);
  assert(binn_object_read(ptr, "",     &type, &size) == NULL);
  assert(binn_object_read(ptr, "test", &type, &size) == NULL);


  // add values - valid -----------------------------------------------------------------

  assert(binn_list_add(list, BINN_INT32, &i, 0) == TRUE);
  assert(binn_map_set(map, 5501, BINN_INT32, &i, 0) == TRUE);
  assert(binn_map_set(map, 5501, BINN_INT32, &i, 0) == FALSE);       // with the same ID
  assert(binn_object_set(obj, "test", BINN_INT32, &i, 0) == TRUE);
  assert(binn_object_set(obj, "test", BINN_INT32, &i, 0) == FALSE); // with the same name

  vbyte = 255;
  vint16 = -32000;
  vuint16 = 65000;
  vint32 = -65000000;
  vuint32 = 65000000;
  vint64 = -6500000000000000;
  vuint64 = 6500000000000000;
  blobsize = 150;
  pblob = malloc(blobsize);
  assert(pblob != NULL);
  memset(pblob, 55, blobsize);

  assert(binn_list_add(list, BINN_NULL, 0, 0) == TRUE);           // second
  assert(binn_list_add(list, BINN_UINT8, &vbyte, 0) == TRUE);     // third
  assert(binn_list_add(list, BINN_INT16, &vint16, 0) == TRUE);    // fourth
  assert(binn_list_add(list, BINN_UINT16, &vuint16, 0) == TRUE);  // fifth
  assert(binn_list_add(list, BINN_INT32, &vint32, 0) == TRUE);    // 6th
  assert(binn_list_add(list, BINN_UINT32, &vuint32, 0) == TRUE);  // 7th
  assert(binn_list_add(list, BINN_INT64, &vint64, 0) == TRUE);    // 8th
  assert(binn_list_add(list, BINN_UINT64, &vuint64, 0) == TRUE);  // 9th
  assert(binn_list_add(list, BINN_STRING, "this is the string", 0) == TRUE); // 10th
  assert(binn_list_add(list, BINN_BLOB, pblob, blobsize) == TRUE);           // 11th

  assert(binn_map_set(map, 99000, BINN_NULL, 0, 0) == TRUE);           // third
  assert(binn_map_set(map, 99001, BINN_UINT8, &vbyte, 0) == TRUE);     // fourth
  assert(binn_map_set(map, 99002, BINN_INT16, &vint16, 0) == TRUE);    // fifth
  assert(binn_map_set(map, 99003, BINN_UINT16, &vuint16, 0) == TRUE);  // 6th
  assert(binn_map_set(map, 99004, BINN_INT32, &vint32, 0) == TRUE);    // 7th
  assert(binn_map_set(map, 99005, BINN_UINT32, &vuint32, 0) == TRUE);  // 8th
  assert(binn_map_set(map, 99006, BINN_INT64, &vint64, 0) == TRUE);    // 9th
  assert(binn_map_set(map, 99007, BINN_UINT64, &vuint64, 0) == TRUE);  // 10th
  assert(binn_map_set(map, 99008, BINN_STRING, "this is the string", 0) == TRUE); // 11th
  assert(binn_map_set(map, 99009, BINN_BLOB, pblob, blobsize) == TRUE);           // 12th

  assert(binn_object_set(obj, "key0", BINN_NULL, 0, 0) == TRUE);           // third
  assert(binn_object_set(obj, "key1", BINN_UINT8, &vbyte, 0) == TRUE);     // fourth
  assert(binn_object_set(obj, "key2", BINN_INT16, &vint16, 0) == TRUE);    // fifth
  assert(binn_object_set(obj, "key3", BINN_UINT16, &vuint16, 0) == TRUE);  // 6th
  assert(binn_object_set(obj, "key4", BINN_INT32, &vint32, 0) == TRUE);    // 7th
  assert(binn_object_set(obj, "key5", BINN_UINT32, &vuint32, 0) == TRUE);  // 8th
  assert(binn_object_set(obj, "key6", BINN_INT64, &vint64, 0) == TRUE);    // 9th
  assert(binn_object_set(obj, "key7", BINN_UINT64, &vuint64, 0) == TRUE);  // 10th
  assert(binn_object_set(obj, "key8", BINN_STRING, "this is the string", 0) == TRUE); // 11th
  assert(binn_object_set(obj, "key9", BINN_BLOB, pblob, blobsize) == TRUE);           // 12th

  // blobs with size = 0
  assert(binn_list_add(list, BINN_BLOB, ptr, 0) == TRUE);
  assert(binn_list_add(list, BINN_STRING, "", 0) == TRUE);
  assert(binn_list_add(list, BINN_STRING, "after the empty items", 0) == TRUE);


  // add values to a fixed-size binn (pre-allocated buffer) --------------------------

  assert(binn_list_add(obj1, BINN_INT32, &i, 0) == FALSE);
  assert(binn_map_set(obj1, 55001, BINN_INT32, &i, 0) == FALSE);

  assert(binn_object_set(obj1, "test", BINN_UINT32, &vuint32, 0) == TRUE);
  assert(binn_object_set(obj1, "test", BINN_UINT32, &vuint32, 0) == FALSE);  // with the same name

  assert(binn_object_set(obj1, "key1", BINN_STRING, "this is the value", 0) == TRUE);
  assert(binn_object_set(obj1, "key2", BINN_STRING, "the second value", 0) == TRUE);

  // create a long string buffer to make the test. the string is longer than the available space
  // in the binn.
  ptr = malloc(fix_size);
  assert(ptr != NULL);
  p2 = ptr;
  for (i = 0; i < fix_size - 1; i++) {
    *p2 = 'A'; p2++;
  }
  *p2 = '\0';
  assert(strlen(ptr) == fix_size - 1);

  assert(binn_object_set(obj1, "v2", BINN_STRING, ptr, 0) == FALSE); // it fails because it uses a pre-allocated memory block

  assert(binn_object_set(obj, "v2", BINN_STRING, ptr, 0) == TRUE); // but this uses a dynamically allocated memory block, so it works with it
  assert(binn_object_set(obj, "Key00", BINN_STRING, "after the big string", 0) == TRUE); // and test the 'Key00' against the 'Key0'

  free(ptr); ptr = 0;

  assert(binn_object_set(obj, "list", BINN_LIST, binn_ptr(list), binn_size(list)) == TRUE);
  assert(binn_object_set(obj, "Key10", BINN_STRING, "after the list", 0) == TRUE); // and test the 'Key10' against the 'Key1'


  // read values - invalid 2 ------------------------------------------------------------






  // read keys --------------------------------------------------------------------------

  ptr = binn_ptr(map);
  assert(ptr != NULL);

  assert(binn_map_get_pair(ptr, -1, &id, &value) == FALSE);
  assert(binn_map_get_pair(ptr, 0, &id, &value) == FALSE);

  assert(binn_map_get_pair(ptr, 1, &id, &value) == TRUE);
  assert(id == 5501);
  assert(binn_map_get_pair(ptr, 2, &id, &value) == TRUE);
  assert(id == 99000);
  assert(binn_map_get_pair(ptr, 3, &id, &value) == TRUE);
  assert(id == 99001);
  assert(binn_map_get_pair(ptr, 10, &id, &value) == TRUE);
  assert(id == 99008);
  assert(binn_map_get_pair(ptr, 11, &id, &value) == TRUE);
  assert(id == 99009);


  ptr = binn_ptr(obj);
  assert(ptr != NULL);

  assert(binn_object_get_pair(ptr, -1, key, &value) == FALSE);
  assert(binn_object_get_pair(ptr, 0, key, &value) == FALSE);

  assert(binn_object_get_pair(ptr, 1, key, &value) == TRUE);
  assert(strcmp(key, "test") == 0);
  assert(binn_object_get_pair(ptr, 2, key, &value) == TRUE);
  assert(strcmp(key, "key0") == 0);
  assert(binn_object_get_pair(ptr, 3, key, &value) == TRUE);
  assert(strcmp(key, "key1") == 0);
  assert(binn_object_get_pair(ptr, 10, key, &value) == TRUE);
  assert(strcmp(key, "key8") == 0);
  assert(binn_object_get_pair(ptr, 11, key, &value) == TRUE);
  assert(strcmp(key, "key9") == 0);



  // read values - valid ----------------------------------------------------------------

  ptr = binn_ptr(obj1);
  assert(ptr != NULL);

  type = 0; size = 0;
  pstr = binn_object_read(ptr, "key1", &type, &size);
  assert(pstr != NULL);
  assert(type == BINN_STRING);
  assert(size > 0);
  assert(strcmp(pstr, "this is the value") == 0);

  type = 0; size = 0;
  pstr = binn_object_read(ptr, "key2", &type, &size);
  assert(pstr != NULL);
  assert(type == BINN_STRING);
  assert(size > 0);
  assert(strcmp(pstr, "the second value") == 0);

  type = 0; size = 0;
  pint32 = binn_object_read(ptr, "test", &type, &size);
  assert(pint32 != NULL);
  assert(type == BINN_UINT32);
  //assert(size > 0);
  assert(*pint32 == vuint32);



  ptr = binn_ptr(list);
  assert(ptr != NULL);

  type = 0; size = 0;
  pstr = binn_list_read(ptr, 2, &type, &size);
  assert(pstr != NULL);
  assert(type == BINN_NULL);
  //assert(size > 0);
  //assert(strcmp(pstr, "this is the value") == 0);

  type = 0; size = 0;
  p2 = binn_list_read(ptr, 3, &type, &size);
  assert(p2 != NULL);
  assert(type == BINN_UINT8);
  assert(*p2 == vbyte);

  type = 0; size = 0;
  pint16 = binn_list_read(ptr, 4, &type, &size);
  assert(pint16 != NULL);
  assert(type == BINN_INT16);
  assert(*pint16 == vint16);

  type = 0; size = 0;
  puint16 = binn_list_read(ptr, 5, &type, &size);
  assert(puint16 != NULL);
  assert(type == BINN_UINT16);
  assert(*puint16 == vuint16);

  type = 0; size = 0;
  pint32 = binn_list_read(ptr, 6, &type, &size);
  assert(pint32 != NULL);
  assert(type == BINN_INT32);
  assert(*pint32 == vint32);
  // in the second time the value must be the same...
  type = 0; size = 0;
  pint32 = binn_list_read(ptr, 6, &type, &size);
  assert(pint32 != NULL);
  assert(type == BINN_INT32);
  assert(*pint32 == vint32);

  type = 0; size = 0;
  puint32 = binn_list_read(ptr, 7, &type, &size);
  assert(puint32 != NULL);
  assert(type == BINN_UINT32);
  assert(*puint32 == vuint32);

  type = 0; size = 0;
  pint64 = binn_list_read(ptr, 8, &type, &size);
  assert(pint64 != NULL);
  assert(type == BINN_INT64);
  assert(*pint64 == vint64);
  // in the second time the value must be the same...
  type = 0; size = 0;
  pint64 = binn_list_read(ptr, 8, &type, &size);
  assert(pint64 != NULL);
  assert(type == BINN_INT64);
  assert(*pint64 == vint64);

  type = 0; size = 0;
  puint64 = binn_list_read(ptr, 9, &type, &size);
  assert(puint64 != NULL);
  assert(type == BINN_UINT64);
  assert(*puint64 == vuint64);

  type = 0; size = 0;
  pstr = binn_list_read(ptr, 10, &type, &size);
  assert(pstr != NULL);
  assert(type == BINN_STRING);
  assert(size > 0);
  assert(strcmp(pstr, "this is the string") == 0);

  type = 0; size = 0;
  p2 = binn_list_read(ptr, 11, &type, &size);
  assert(p2 != NULL);
  assert(type == BINN_BLOB);
  assert(size == blobsize);
  assert(memcmp(p2, pblob, blobsize) == 0);




  ptr = binn_ptr(map);
  assert(ptr != NULL);

  type = 0; size = 0;
  pstr = binn_map_read(ptr, 99000, &type, &size);
  assert(pstr != NULL);
  assert(type == BINN_NULL);
  //assert(size > 0);
  //assert(strcmp(pstr, "this is the value") == 0);

  type = 0; size = 0;
  p2 = binn_map_read(ptr, 99001, &type, &size);
  assert(p2 != NULL);
  assert(type == BINN_UINT8);
  assert(*p2 == vbyte);

  type = 0; size = 0;
  pint16 = binn_map_read(ptr, 99002, &type, &size);
  assert(pint16 != NULL);
  assert(type == BINN_INT16);
  assert(*pint16 == vint16);

  type = 0; size = 0;
  puint16 = binn_map_read(ptr, 99003, &type, &size);
  assert(puint16 != NULL);
  assert(type == BINN_UINT16);
  assert(*puint16 == vuint16);

  type = 0; size = 0;
  pint32 = binn_map_read(ptr, 99004, &type, &size);
  assert(pint32 != NULL);
  assert(type == BINN_INT32);
  assert(*pint32 == vint32);
  // in the second time the value must be the same...
  type = 0; size = 0;
  pint32 = binn_map_read(ptr, 99004, &type, &size);
  assert(pint32 != NULL);
  assert(type == BINN_INT32);
  assert(*pint32 == vint32);

  type = 0; size = 0;
  puint32 = binn_map_read(ptr, 99005, &type, &size);
  assert(puint32 != NULL);
  assert(type == BINN_UINT32);
  assert(*puint32 == vuint32);

  type = 0; size = 0;
  pint64 = binn_map_read(ptr, 99006, &type, &size);
  assert(pint64 != NULL);
  assert(type == BINN_INT64);
  assert(*pint64 == vint64);
  // in the second time the value must be the same...
  type = 0; size = 0;
  pint64 = binn_map_read(ptr, 99006, &type, &size);
  assert(pint64 != NULL);
  assert(type == BINN_INT64);
  assert(*pint64 == vint64);

  type = 0; size = 0;
  puint64 = binn_map_read(ptr, 99007, &type, &size);
  assert(puint64 != NULL);
  assert(type == BINN_UINT64);
  assert(*puint64 == vuint64);

  type = 0; size = 0;
  pstr = binn_map_read(ptr, 99008, &type, &size);
  assert(pstr != NULL);
  assert(type == BINN_STRING);
  assert(size > 0);
  assert(strcmp(pstr, "this is the string") == 0);

  type = 0; size = 0;
  p2 = binn_map_read(ptr, 99009, &type, &size);
  assert(p2 != NULL);
  assert(type == BINN_BLOB);
  assert(size == blobsize);
  assert(memcmp(p2, pblob, blobsize) == 0);




  ptr = binn_ptr(obj);
  assert(ptr != NULL);

  type = 0; size = 0;
  pstr = binn_object_read(ptr, "key0", &type, &size);
  assert(pstr != NULL);
  assert(type == BINN_NULL);
  //assert(size > 0);
  //assert(strcmp(pstr, "this is the value") == 0);

  type = 0; size = 0;
  p2 = binn_object_read(ptr, "key1", &type, &size);
  assert(p2 != NULL);
  assert(type == BINN_UINT8);
  assert(*p2 == vbyte);

  type = 0; size = 0;
  pint16 = binn_object_read(ptr, "key2", &type, &size);
  assert(pint16 != NULL);
  assert(type == BINN_INT16);
  assert(*pint16 == vint16);

  type = 0; size = 0;
  puint16 = binn_object_read(ptr, "key3", &type, &size);
  assert(puint16 != NULL);
  assert(type == BINN_UINT16);
  assert(*puint16 == vuint16);

  type = 0; size = 0;
  pint32 = binn_object_read(ptr, "key4", &type, &size);
  assert(pint32 != NULL);
  assert(type == BINN_INT32);
  assert(*pint32 == vint32);
  // in the second time the value must be the same...
  type = 0; size = 0;
  pint32 = binn_object_read(ptr, "key4", &type, &size);
  assert(pint32 != NULL);
  assert(type == BINN_INT32);
  assert(*pint32 == vint32);

  type = 0; size = 0;
  puint32 = binn_object_read(ptr, "key5", &type, &size);
  assert(puint32 != NULL);
  assert(type == BINN_UINT32);
  assert(*puint32 == vuint32);

  type = 0; size = 0;
  pint64 = binn_object_read(ptr, "key6", &type, &size);
  assert(pint64 != NULL);
  assert(type == BINN_INT64);
  assert(*pint64 == vint64);
  // in the second time the value must be the same...
  type = 0; size = 0;
  pint64 = binn_object_read(ptr, "key6", &type, &size);
  assert(pint64 != NULL);
  assert(type == BINN_INT64);
  assert(*pint64 == vint64);

  type = 0; size = 0;
  puint64 = binn_object_read(ptr, "key7", &type, &size);
  assert(puint64 != NULL);
  assert(type == BINN_UINT64);
  assert(*puint64 == vuint64);

  type = 0; size = 0;
  pstr = binn_object_read(ptr, "key8", &type, &size);
  assert(pstr != NULL);
  assert(type == BINN_STRING);
  assert(size > 0);
  assert(strcmp(pstr, "this is the string") == 0);

  type = 0; size = 0;
  p2 = binn_object_read(ptr, "key9", &type, &size);
  assert(p2 != NULL);
  assert(type == BINN_BLOB);
  assert(size == blobsize);
  assert(memcmp(p2, pblob, blobsize) == 0);

  type = 0; size = 0;
  p2 = binn_object_read(ptr, "v2", &type, &size);
  assert(p2 != NULL);
  assert(type == BINN_STRING);
  assert(size == fix_size - 1);
  assert(strlen(p2) == fix_size - 1);
  assert(p2[0] == 'A');
  assert(p2[1] == 'A');
  assert(p2[500] == 'A');
  assert(p2[fix_size-1] == 0);

  type = 0; size = 0;
  pstr = binn_object_read(ptr, "key00", &type, &size);
  assert(pstr != NULL);
  assert(type == BINN_STRING);
  assert(size > 0);
  assert(strcmp(pstr, "after the big string") == 0);

  type = 0; size = 0;
  p2 = binn_object_read(ptr, "list", &type, &size);
  assert(p2 != NULL);
  assert(type == BINN_LIST);
  assert(size > 0);
  //
  type = 0; size = 0;
  puint64 = binn_list_read(p2, 9, &type, &size);
  assert(puint64 != NULL);
  assert(type == BINN_UINT64);
  assert(*puint64 == vuint64);
  //
  type = 0; size = 0;
  pstr = binn_list_read(p2, 10, &type, &size);
  assert(pstr != NULL);
  assert(type == BINN_STRING);
  assert(size > 0);
  assert(strcmp(pstr, "this is the string") == 0);
  //
  type = 0; size = 0;
  pstr = binn_list_read(p2, 12, &type, &size);
  assert(pstr != NULL);
  assert(type == BINN_BLOB);   // empty blob
  assert(size == 0);
  //
  type = 0; size = 0;
  pstr = binn_list_read(p2, 13, &type, &size);
  assert(pstr != NULL);
  assert(type == BINN_STRING);   // empty string
  assert(size == 0);
  assert(strcmp(pstr, "") == 0);
  //
  type = 0; size = 0;
  pstr = binn_list_read(p2, 14, &type, &size);
  assert(pstr != NULL);
  assert(type == BINN_STRING);
  assert(size > 0);
  assert(strcmp(pstr, "after the empty items") == 0);

  type = 0; size = 0;
  pstr = binn_object_read(ptr, "key10", &type, &size);
  assert(pstr != NULL);
  assert(type == BINN_STRING);
  assert(size > 0);
  assert(strcmp(pstr, "after the list") == 0);




  // binn_ptr, IsValidBinnHeader, binn_is_valid...
  // also with invalid/null pointers, with pointers containing invalid data...

  assert(binn_ptr(NULL) == NULL);
  // pointers to invalid data
  //assert(binn_ptr(&type) == NULL);
  //assert(binn_ptr(&size) == NULL);
  //assert(binn_ptr(&count) == NULL);

  assert(IsValidBinnHeader(NULL) == FALSE);

  ptr = binn_ptr(obj);
  assert(ptr != NULL);
  // test the header
  size = 0;
  assert(IsValidBinnHeader(ptr, &type, &count, &size, &header_size) == TRUE);
  assert(type == BINN_OBJECT);
  assert(count == 15);
  assert(header_size >= MIN_BINN_SIZE && header_size <= MAX_BINN_HEADER);
  assert(size > MIN_BINN_SIZE);
  assert(size == obj->size);
  // test all the buffer
  assert(binn_is_valid(ptr, &type, &count, &size) == TRUE);
  assert(type == BINN_OBJECT);
  assert(count == 15);
  assert(size > MIN_BINN_SIZE);
  assert(size == obj->size);

  ptr = binn_ptr(map);
  assert(ptr != NULL);
  // test the header
  size = 0;
  assert(IsValidBinnHeader(ptr, &type, &count, &size, &header_size) == TRUE);
  assert(type == BINN_MAP);
  assert(count == 11);
  assert(header_size >= MIN_BINN_SIZE && header_size <= MAX_BINN_HEADER);
  assert(size > MIN_BINN_SIZE);
  assert(size == map->size);
  // test all the buffer
  assert(binn_is_valid(ptr, &type, &count, &size) == TRUE);
  assert(type == BINN_MAP);
  assert(count == 11);
  assert(size > MIN_BINN_SIZE);
  assert(size == map->size);

  ptr = binn_ptr(list);
  assert(ptr != NULL);
  // test the header
  size = 0;
  assert(IsValidBinnHeader(ptr, &type, &count, &size, &header_size) == TRUE);
  assert(type == BINN_LIST);
  assert(count == 14);
  assert(header_size >= MIN_BINN_SIZE && header_size <= MAX_BINN_HEADER);
  assert(size > MIN_BINN_SIZE);
  assert(size == list->size);
  // test all the buffer
  assert(binn_is_valid(ptr, &type, &count, &size) == TRUE);
  assert(type == BINN_LIST);
  assert(count == 14);
  assert(header_size >= MIN_BINN_SIZE && header_size <= MAX_BINN_HEADER);
  assert(size > MIN_BINN_SIZE);
  assert(size == list->size);



  // binn_size - invalid and valid args --------------------------------------------

  assert(binn_size(NULL) == 0);

  assert(binn_size(list) == list->size);
  assert(binn_size(map) == map->size);
  assert(binn_size(obj) == obj->size);
  assert(binn_size(obj1) == obj1->size);


  // destroy them all -------------------------------------------------------------------

  binn_free(list);
  binn_free(map);
  binn_free(obj);

  printf("OK\n");

}

/*************************************************************************************/

void test_invalid_binn() {

  char buffers[][20] = {
    { 0xE0 },
    { 0xE0, 0x7E },
    { 0xE0, 0x7E, 0x7F },
    { 0xE0, 0x7E, 0x7F, 0x12 },
    { 0xE0, 0x7E, 0x7F, 0x12, 0x34 },
    { 0xE0, 0x7E, 0x7F, 0x12, 0x34, 0x01 },
    { 0xE0, 0x7E, 0x7F, 0x12, 0x34, 0x7F },
    { 0xE0, 0x7E, 0x7F, 0x12, 0x34, 0xFF },
    { 0xE0, 0x7E, 0x7F, 0x12, 0x34, 0xFF, 0xFF },
    { 0xE0, 0x7E, 0x7F, 0x12, 0x34, 0xFF, 0xFF, 0xFF },
    { 0xE0, 0x7E, 0x7F, 0x12, 0x34, 0xFF, 0xFF, 0xFF, 0xFF },
    { 0xE0, 0x7E, 0x7F, 0x12, 0x34, 0xFF, 0xFF, 0xFF, 0xFF, 0x01 },
    { 0xE0, 0x7E, 0x7F, 0x12, 0x34, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
    { 0xE0, 0x7E, 0xFF },
    { 0xE0, 0x7E, 0xFF, 0x12 },
    { 0xE0, 0x7E, 0xFF, 0x12, 0x34 },
    { 0xE0, 0x7E, 0xFF, 0x12, 0x34, 0x01 },
    { 0xE0, 0x7E, 0xFF, 0x12, 0x34, 0x7F },
    { 0xE0, 0x7E, 0xFF, 0x12, 0x34, 0xFF },
    { 0xE0, 0x7E, 0xFF, 0x12, 0x34, 0xFF, 0xFF },
    { 0xE0, 0x7E, 0xFF, 0x12, 0x34, 0xFF, 0xFF, 0xFF },
    { 0xE0, 0x7E, 0xFF, 0x12, 0x34, 0xFF, 0xFF, 0xFF, 0xFF },
    { 0xE0, 0x7E, 0xFF, 0x12, 0x34, 0xFF, 0xFF, 0xFF, 0xFF, 0x01 },
    { 0xE0, 0x7E, 0xFF, 0x12, 0x34, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
    { 0xE0, 0x8E },
    { 0xE0, 0x8E, 0xFF },
    { 0xE0, 0x8E, 0xFF, 0x12 },
    { 0xE0, 0x8E, 0xFF, 0x12, 0x34 },
    { 0xE0, 0x8E, 0xFF, 0x12, 0x34, 0x01 },
    { 0xE0, 0x8E, 0xFF, 0x12, 0x34, 0x7F },
    { 0xE0, 0x8E, 0xFF, 0x12, 0x34, 0xFF },
    { 0xE0, 0x8E, 0xFF, 0x12, 0x34, 0xFF, 0xFF },
    { 0xE0, 0x8E, 0xFF, 0x12, 0x34, 0xFF, 0xFF, 0xFF },
    { 0xE0, 0x8E, 0xFF, 0x12, 0x34, 0xFF, 0xFF, 0xFF, 0xFF },
    { 0xE0, 0x8E, 0xFF, 0x12, 0x34, 0xFF, 0xFF, 0xFF, 0xFF, 0x01 },
    { 0xE0, 0x8E, 0xFF, 0x12, 0x34, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }
  };

  int count, size, i;
  char *ptr;

  puts("testing invalid binn buffers...");

  count = sizeof buffers / sizeof buffers[0];

  for (i=0; i < count; i++) {
    ptr = buffers[i];
    size = strlen(ptr);
    printf("checking invalid binn #%d   size: %d bytes\n", i, size);
    assert(binn_is_valid_ex(ptr, NULL, NULL, &size) == FALSE);
  }

  puts("OK");

}

/*************************************************************************************/

int main() {

  puts("\nStarting the unit/regression tests...\n");

  printf("sizeof(binn) = %d\n\n", sizeof(binn));

  test_binn_version();

  test_endianess();

  test_int64();

  test_floating_point_numbers();

  test1();

  test2(FALSE);
  test2(TRUE);

  test_binn2();

  test3();

  test_invalid_binn();

  puts("\nAll tests pass! :)\n");
  return 0;

}
