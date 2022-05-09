//-----------------------------------------------------------------------------
//           Name: jsonseekerbase.cpp
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
#include "jsonseekerbase.h"

#include <JSON/jsonhelper.h>
#include <Logging/logdata.h>
#include <fstream>
#include <string>

std::vector<Item> JSONSeekerBase::Search(const Item& item) {
    std::fstream fs(item.GetAbsPath().c_str(), std::fstream::in);

    if (fs.good()) {
        SimpleJSONWrapper json;
        std::string err;
        if (json.parseIstream(fs, err)) {
            return SearchJSON(item, json.getRoot());
        } else {
            LOGE << "Unable to parse " << item << " in json parser, reason: " << err << std::endl;
        }
    } else {
        LOGE << "Unable to open " << item << " in json parser" << std::endl;
        return std::vector<Item>();
    }
}
