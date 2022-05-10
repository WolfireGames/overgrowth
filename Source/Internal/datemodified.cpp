//-----------------------------------------------------------------------------
//           Name: datemodified.cpp
//      Developer: Wolfire Games LLC
//    Description:
//        License: Read below
//-----------------------------------------------------------------------------
//
//   Copyright 2022 Wolfire Games LLC
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//
//-----------------------------------------------------------------------------
#include "datemodified.h"

#ifndef NO_ERR
#include <Internal/error.h>
#endif
#include <Internal/profiler.h>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

#include <sys/stat.h>
#include <stdio.h>
#include <errno.h>
#include <cstring>
#include <string>
#include <ctime>

using std::string;

bool GetDateModifiedString(const char *file_name, char *buffer, int buffer_size) {
#ifdef _WIN32
    struct _stat buf;
    int result;
    char timebuf[26];
    errno_t err;

    // Get data associated with "crt_stat.c":
    const int buf_size = 2048;
    WCHAR path_utf16[buf_size];
    if (!MultiByteToWideChar(CP_UTF8, 0, file_name, -1, path_utf16, buf_size)) {
#ifndef NO_ERR
        FatalError("Error", "Error converting utf8 string to utf16: %s", file_name);
#endif
    }
    result = _wstat(path_utf16, &buf);

    // Check if statistics are valid:
    while (result != 0) {
#ifndef NO_ERR
        string error = "Could not find: " + string(file_name);
        ErrorResponse response = DisplayError("Error", error.c_str(), _ok_cancel_retry);
        if (response == _retry) {
            result = _wstat(path_utf16, &buf);
        } else if (response == _continue) {
            return false;
        }
#else
        return false;
#endif
    }

    err = ctime_s(timebuf, 26, &buf.st_mtime);
    if (err) {
#ifndef NO_ERR
        DisplayError("Error", "Problem getting date modified string.");
#endif
        return false;
    }
    strncpy(buffer, timebuf, buffer_size);
    return true;

#else  // standard unix implementation
    struct stat buf;
    int result;
    char timebuf[26];

    // Get data associated with "crt_stat.c":
    const char* path = file_name;
    result = stat(path, &buf);

    // Check if statistics are valid:
    if (result != 0) {
        const int BUF_SIZE = 512;
        char err_buf[BUF_SIZE];
#ifndef NO_ERR
        if (errno == ENOENT) {
            snprintf(err_buf, BUF_SIZE, "The file doesn't exist: %s", file_name);
            DisplayError("Error", err_buf);
        } else if (errno == EACCES) {
            snprintf(err_buf, BUF_SIZE, "I don't have permission to read that file:  %s", file_name);
            DisplayError("Error", err_buf);
        } else {
            snprintf(err_buf, BUF_SIZE, "Problem getting date modified:  %s", file_name);
            DisplayError("Error", err_buf);
        }
#endif
        return false;
    } else {
        sprintf(timebuf, "%li", buf.st_mtime);
        strncpy(buffer, timebuf, buffer_size);
        return true;
        /*unsigned short sum;
         for(int i=0; i<26; i++){
         sum += timebuf[i]*i;
         }*/
    }
#endif
}

int64_t GetDateModifiedInt64(const Path &path) {
    return GetDateModifiedInt64(path.GetFullPath());
}

int64_t GetDateModifiedInt64(const char *abs_path) {
    PROFILER_ZONE(g_profiler_ctx, "GetDateModifiedInt64");
#ifdef _WIN32
    struct _stat buf;
    int result;

    // Get data associated with "crt_stat.c":
    const int BUF_SIZE = 4096;
    WCHAR path_utf16[BUF_SIZE];

    {
        PROFILER_ZONE(g_profiler_ctx, "MultiByteToWideChar");
        if (!MultiByteToWideChar(CP_UTF8, 0, abs_path, -1, path_utf16, BUF_SIZE)) {
#ifndef NO_ERR
            FatalError("Error", "Error converting utf8 string to utf16: %s", abs_path);
#endif
            return -1;
        }
    }

    {
        PROFILER_ZONE(g_profiler_ctx, "_wstat");
        result = _wstat(path_utf16, &buf);
    }

    // Check if statistics are valid:
    while (result != 0) {
        return -1;
    }

    return (int64_t)buf.st_mtime;

#elif defined(__APPLE__) || defined(__linux__)
    struct stat buf;
    int result;

    // Get data associated with "crt_stat.c":
    const char* path = abs_path;
    result = stat(path, &buf);

    // Check if statistics are valid:
    if (result != 0) {
        return -1;
    } else {
        return (int64_t)buf.st_mtime;
    }
#endif
}
