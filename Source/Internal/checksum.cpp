//-----------------------------------------------------------------------------
//           Name: checksum.cpp
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
#include "checksum.h"

#include <Internal/error.h>
#include <Internal/profiler.h>
#include <Internal/common.h>

#include <Compat/fileio.h>
#include <Memory/allocation.h>

#include <stdio.h>
#include <string>

using std::endl;
using std::string;

// TODO: Maybe modifying this to be Adler32 would be a little better CRC32 even more so, lowering the risk for collisions for small changes.
//(If you make a 'b' a 'c' and another 'b' an 'a' in a file, this function will give you the same checksum value, i would consider this a possible common change)
unsigned short Checksum(const string& abs_path) {
    PROFILER_ZONE(g_profiler_ctx, "Checksum");
    unsigned short sum = 0;

    FILE* pFile;
    long lSize;
    unsigned char* buffer;
    size_t result;

    pFile = my_fopen(abs_path.c_str(), "rb");

#ifndef NO_ERR
    const int kBufSize = 512;
    char error_msg[kBufSize];
    while (pFile == NULL) {
        FormatString(error_msg, kBufSize, "Could not open file: %s. Retry?", abs_path.c_str());
        DisplayError("Error", error_msg, _ok_cancel);
        pFile = my_fopen(abs_path.c_str(), "rb");
    }
#endif

    if (pFile) {
        // obtain file size:
        fseek(pFile, 0, SEEK_END);
        lSize = ftell(pFile);
        rewind(pFile);

        // allocate memory to contain the whole file:
        buffer = (unsigned char*)alloc.stack.Alloc(sizeof(char) * lSize);

        if (buffer) {
#ifndef NO_ERR
            if (buffer == NULL) {
                FormatString(error_msg, kBufSize, "Could not allocate memory to checksum: %s.", abs_path.c_str());
                FatalError("Error", error_msg);
            }
#endif

            // copy the file into the buffer:
            result = fread(buffer, 1, lSize, pFile);
#ifndef NO_ERR
            if (result != (size_t)lSize) {
                FormatString(error_msg, kBufSize, "Could not read data from file: %s.", abs_path.c_str());
                FatalError("Error", error_msg);
            }
#endif

            {
                PROFILER_ZONE(g_profiler_ctx, "Actual summation");
                size_t short_count = lSize / 2;
                for (size_t i = 0; i < short_count; i++) {
                    // Might have trouble on wrong-endian machines, but those should probably just have different checksums built in anyhow, to keep this fast
                    sum += ((uint16_t*)buffer)[i];
                }
            }
            alloc.stack.Free(buffer);
        } else {
            LOGE << "Failed to allocate data from stack allocator, size: " << lSize << endl;
        }
        // terminate
        fclose(pFile);
    } else {
        LOGE << "Failed to open file " << abs_path << endl;
    }
    return sum;
}

// From http://www.cse.yorku.ca/~oz/hash.html
unsigned long djb2_string_hash(const char* cstr) {
    unsigned long hash = 5381;
    int c;

    while ((c = *cstr++))
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
}
