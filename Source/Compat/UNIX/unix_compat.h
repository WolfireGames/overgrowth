//-----------------------------------------------------------------------------
//           Name: unix_compat.h
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

#if !PLATFORM_UNIX
#error Do not compile this.
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <climits>
#include <cassert>
#include <cstdlib>
#include <cstdio>
#include <string>
#include <vector>

#ifndef PATH_MAX
#define PATH_MAX 2048
#endif

extern "C" {
void nonExistantFunctionToAlertYouToStubbedCode(void);
}
// #define STUBBED(txt) nonExistantFunctionToAlertYouToStubbedCode()
#define STUBBED(txt)                                                            \
    {                                                                           \
        static bool virgin = true;                                              \
        if (virgin) {                                                           \
            virgin = false;                                                     \
            fprintf(stderr, "STUBBED: %s at %s:%d\n", txt, __FILE__, __LINE__); \
        }                                                                       \
    }
#define MessageBox(hwnd, title, text, buttons)                            \
    {                                                                     \
        STUBBED("MSGBOX");                                                \
        fprintf(stderr, "MSGBOX: %s...%s (%s)\n", title, text, #buttons); \
    }
#define _open(x, y) open(x, y, S_IREAD | S_IWRITE)
#define _O_RDONLY O_RDONLY
#define _lseek(x, y, z) lseek(x, y, z)
#define _close(x) close(x)

#define _ASSERT(x) assert(x)

// end of unix_compat.h ...
