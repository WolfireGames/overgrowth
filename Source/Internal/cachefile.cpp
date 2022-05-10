//-----------------------------------------------------------------------------
//           Name: cachefile.cpp
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
#include "cachefile.h"

#include <Internal/checksum.h>
#include <Internal/datemodified.h>
#include <Internal/filesystem.h>

bool CacheFile::CheckForCache(const std::string &path, const std::string &suffix, std::string *load_path, uint16_t *checksum) {
    ModID modsource;
    return CheckForCache(path, suffix, load_path, &modsource, checksum);
}

bool CacheFile::CheckForCache(const std::string &path, const std::string &suffix, std::string *load_path, ModID *load_modsource, uint16_t *checksum) {
    std::string cache_str = path + suffix;

    const int kMaxPaths = 5;
    char abs_cache_paths[kPathSize * kMaxPaths];
    ModID cache_modsources[kMaxPaths];
    char abs_base_paths[kPathSize * kMaxPaths];
    int num_cache_paths_found = FindFilePaths(cache_str.c_str(), abs_cache_paths, kPathSize, kMaxPaths, kAnyPath, false, NULL, cache_modsources);
    int num_base_paths_found = FindFilePaths(path.c_str(), abs_base_paths, kPathSize, kMaxPaths, kAnyPath, false, NULL, NULL);

    char *cache_path = &abs_cache_paths[0];
    ModID cache_modsource = cache_modsources[0];
    int64_t latest_cache_date_modified = -1;
    if (num_cache_paths_found > 0) {
        for (int path_index = 0; path_index < num_cache_paths_found; ++path_index) {
            char *curr_path = &abs_cache_paths[kPathSize * path_index];
            ModID curr_modsource = cache_modsources[path_index];
            int64_t date_modified = GetDateModifiedInt64(curr_path);
            if (date_modified > latest_cache_date_modified) {
                latest_cache_date_modified = date_modified;
                cache_path = curr_path;
                cache_modsource = curr_modsource;
            }
        }
    }

    char *base_path = &abs_base_paths[0];
    int64_t latest_base_date_modified = -1;
    if (num_base_paths_found > 0) {
        for (int path_index = 0; path_index < num_base_paths_found; ++path_index) {
            char *curr_path = &abs_base_paths[kPathSize * path_index];
            int64_t date_modified = GetDateModifiedInt64(curr_path);
            if (date_modified > latest_base_date_modified) {
                latest_base_date_modified = date_modified;
                base_path = curr_path;
            }
        }
    }

    if (latest_base_date_modified != -1) {
        *checksum = Checksum(base_path);
    } else {
        *checksum = 0;
    }

    if (latest_cache_date_modified != -1) {
        *load_path = cache_path;
        *load_modsource = cache_modsource;
        return true;
    }
    return false;
}
