//-----------------------------------------------------------------------------
//           Name: win_compat.h
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
#pragma once
#if !PLATFORM_WINDOWS
#error Do not compile this.
#endif

#include <cstdio>
#include <climits>
#include <cstdlib>
#include <string>
#include <vector>

#ifndef PATH_MAX
#define PATH_MAX 2048
#endif

extern "C" {
void nonExistantFunctionToAlertYouToStubbedCode(void);
}
#define STUBBED(txt)                                                            \
    {                                                                           \
        static bool virgin = true;                                              \
        if (virgin) {                                                           \
            virgin = false;                                                     \
            fprintf(stderr, "STUBBED: %s at %s:%d\n", txt, __FILE__, __LINE__); \
        }                                                                       \
    }

char* strtok_r(char* str, const char* delim, char** nextp);

static const char* win_compat_err_str[] = {
    "No error",
    "Problem getting documents path",
    "Error converting utf16 string to utf8",
    "Write dir is too long.",
    "Error getting short-path version of write dir",
    "Short path version of write dir could not fit in buffer"};

void SetWorkingDir(const char* path);
