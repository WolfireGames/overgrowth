//-----------------------------------------------------------------------------
//           Name: asarglist.h
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

#include <JSON/jsonhelper.h>

#include <angelscript.h>

#include <list>
#include <vector>

enum ASArgType {_as_char, _as_int, _as_unsigned, _as_float, _as_double, 
              _as_address, _as_object, _as_bool, _as_string, _as_json };

struct ASArg {
    ASArgType type;
    void* data;
    std::string strData;
    SimpleJSONWrapper jsonData;
};

class ASArglist {
    public:
    std::vector<ASArg> args;
    std::list<asBYTE> local_char_copies;
    std::list<asDWORD> local_int_copies;
    std::list<asDWORD> local_unsigned_copies;
    std::list<float> local_float_copies;
    std::list<double> local_double_copies;

    ASArglist(){}
    
    unsigned size() const {
        return (unsigned) args.size();
    }

    const ASArg& operator[](int which) const{
        return args[which];
    }

    void clear() {
        args.clear();
        local_char_copies.clear();
        local_int_copies.clear();
        local_unsigned_copies.clear();
        local_float_copies.clear();
        local_double_copies.clear();
    }

    void Add(const char &val) {
        local_char_copies.push_back(val);
        args.resize(args.size()+1);
        args.back().type = _as_char;
        args.back().data = (void*)&(local_char_copies.back());
    }

    void Add(const bool &val) {
        local_char_copies.push_back(val);
        args.resize(args.size()+1);
        args.back().type = _as_bool;
        args.back().data = (void*)&(local_char_copies.back());
    }

    void Add(const int &val) {
        local_int_copies.push_back(val);
        args.resize(args.size()+1);
        args.back().type = _as_int;
        args.back().data = (void*)&(local_int_copies.back());
    }

    void Add(const unsigned &val) {
        local_unsigned_copies.push_back(val);
        args.resize(args.size()+1);
        args.back().type = _as_unsigned;
        args.back().data = (void*)&(local_unsigned_copies.back());
    }

    void Add(const float &val) {
        local_float_copies.push_back(val);
        args.resize(args.size()+1);
        args.back().type = _as_float;
        args.back().data = (void*)&(local_float_copies.back());
    }

    void Add(const double &val) {
        local_double_copies.push_back(val);
        args.resize(args.size()+1);
        args.back().type = _as_double;
        args.back().data = (void*)&(local_double_copies.back());
    }

    void AddObject(void *val) {
        args.resize(args.size()+1);
        args.back().type = _as_object;
        args.back().data = val;
    }

    void AddAddress(void *val) {
        args.resize(args.size()+1);
        args.back().type = _as_address;
        args.back().data = val;
    }

    void AddString(std::string *str) {
        args.resize(args.size()+1);
        args.back().type = _as_string;
        args.back().data = str;
    }
};
