//-----------------------------------------------------------------------------
//           Name: linux_compat.cpp
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
#include <Compat/platform.h>
#if !PLATFORM_LINUX
#error Do not compile this.
#endif

#include <Compat/compat.h>

#include <sys/sendfile.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#if defined(PLATFORM_64)
int os_copyfile(const char *source, const char *dest) {
    int input, output;
    if ((input = open(source, O_RDONLY)) == -1) {
        return -2;
    }

    if ((output = open(dest, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP)) == -1) {
        close(input);
        return -3;
    }

    // Here we use kernel-space copying for performance reasons
    // sendfile will work with non-socket output (i.e. regular file) on Linux 2.6.33+
    off_t bytesCopied = 0;
    struct stat fileinfo = {0};
    fstat(input, &fileinfo);
    int result = sendfile(output, input, &bytesCopied, fileinfo.st_size);

    close(input);
    close(output);

    return result;
}
#else
int os_copyfile(const char *source, const char *dest) {
    return -1;
}
#endif
