//-----------------------------------------------------------------------------
//           Name: cachefile.h
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

#include <Internal/integer.h>
#include <Internal/modid.h>

#include <string>
#include <cstdio>

namespace CacheFile {
    bool CheckForCache(const std::string &path, const std::string &suffix, std::string *load_path, uint16_t *checksum);
    bool CheckForCache(const std::string &path, const std::string &suffix, std::string *load_path, ModID *load_modsource, uint16_t *checksum);
    
    class ScopedFileCloser {
    public:
        ScopedFileCloser(FILE* file):file_(file){}
        ~ScopedFileCloser(){fclose(file_);}

    private:
        FILE* file_;
    };
}
