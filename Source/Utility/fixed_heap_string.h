//-----------------------------------------------------------------------------
//           Name: fixed_heap_string.h
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

#include <string>

#include <Utility/strings.h>
#include <Memory/allocation.h>

template <int d_size>
class fixed_heap_string {
    char* data;

   public:
    fixed_heap_string() {
        data = static_cast<char*>(OG_MALLOC(d_size));
        data[0] = '\0';
    }

    fixed_heap_string(const char* v) {
        data = static_cast<char*>(OG_MALLOC(d_size));
        set(v);
    }

    fixed_heap_string(const fixed_heap_string<d_size>* rhs) {
        data = static_cast<char*>(OG_MALLOC(d_size));
        set(rhs->c_str());
    }

    fixed_heap_string(const fixed_heap_string<d_size>& rhs) {
        data = static_cast<char*>(OG_MALLOC(d_size));
        set(rhs.c_str());
    }

    ~fixed_heap_string() {
        OG_FREE(data);
    }

    std::string str() const {
        return std::string(data);
    }

    const char* c_str() const {
        return data;
    }

    char* ptr() {
        return data;
    }

    size_t size() const {
        return strlen(data);
    }

    size_t max_size() {
        return d_size - 1;
    }

    int set(const char* str) {
        return strscpy(data, str, d_size);
    }

    int set(const std::string& other) {
        return strscpy(data, other.c_str(), d_size);
    }

    operator const char*() const {
        return data;
    }

    bool operator==(const std::string& rh) const {
        return strmtch(data, rh.c_str());
    }

    fixed_heap_string<d_size>& operator=(const fixed_heap_string<d_size>* rhs) {
        set(rhs->c_str());
        return *this;
    }

    fixed_heap_string<d_size>& operator=(const fixed_heap_string<d_size>& rhs) {
        set(rhs.c_str());
        return *this;
    }
};
