//-----------------------------------------------------------------------------
//           Name: comma_separated_list.h
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

// The CSL iterator iterates over the tokens of a comma-separated list, e.g.
// for "a, b,   c" it will return "a", "b" and "c".
struct CSLIterator {
    std::string str_;
    int start_, comma_pos_;

    CSLIterator(const std::string& str)
        : start_(0), comma_pos_(0) {
        str_ = str;
    }

    bool GetNext(std::string* str) {
        if (comma_pos_ == (int)str_.length()) {
            return false;
        }
        int end = (int)str_.length();
        comma_pos_ = (int)str_.length();
        for (int i = start_; i < end; ++i) {
            if (str_[i] == ',') {
                comma_pos_ = i;
                break;
            }
        }
        while (str_[start_] == ' ') {
            ++start_;
        }
        end = comma_pos_ - 1;
        while (str_[end] == ' ') {
            --end;
        }
        if (str) {
            (*str) = str_.substr(start_, end - start_ + 1);
        }
        start_ = comma_pos_ + 1;
        return true;
    }
};
