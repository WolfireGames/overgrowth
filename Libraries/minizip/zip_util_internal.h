#pragma once
#ifndef _ZIP_UTIL_INTERNAL_H
#define _ZIP_UTIL_INTERNAL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "zconf.h"
#include "zip.h"
#include "unzip.h"

int minizip_main(int argc, const char *argv[]);
int miniunzip_main(int argc, const char *argv[]);

int check_exist_file(const char* filename);
uLong filetime(const char *f, tm_zip *tmzip, uLong *dt);
int getFileCrc(const char* filenameinzip,void*buf,unsigned long size_buf,unsigned long* result_crc);
int isLargeFile(const char* filename);
int mymkdir(const char* dirname);
int makedir(const char *newdir);
void change_file_date(const char *filename, uLong dosdate, tm_unz tmu_date);
#ifdef __APPLE__
// In darwin and perhaps other BSD variants off_t is a 64 bit value, hence no need for specific 64 bit functions
#define FOPEN_FUNC(filename, mode) fopen(filename, mode)
#define FTELLO_FUNC(stream) ftello(stream)
#define FSEEKO_FUNC(stream, offset, origin) fseeko(stream, offset, origin)
#else
#define FOPEN_FUNC(filename, mode) fopen64(filename, mode)
#define FTELLO_FUNC(stream) ftello64(stream)
#define FSEEKO_FUNC(stream, offset, origin) fseeko64(stream, offset, origin)
#endif

#ifdef __cplusplus
}
#endif

#endif /*_ZIP_UTIL_INTERNAL_H*/
