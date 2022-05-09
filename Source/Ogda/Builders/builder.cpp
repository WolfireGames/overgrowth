//-----------------------------------------------------------------------------
//           Name: builder.cpp
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
#include "builder.h"
#include <Utility/strings.h>

Builder::Builder(ActionBase* action, const std::string& path_ending_, const std::string& type_pattern_re)
    : action(action), path_ending(path_ending_) {
    try {
        type_pattern->Compile(type_pattern_re.c_str());
    } catch (const TRexParseException& pe) {
        LOGE << "Failed to compile the type_pattern regex " << type_pattern_re << " reason: " << pe.desc << std::endl;
    }
}

bool Builder::IsMatch(const Item& t) {
    if (endswith(t.GetPath().c_str(), path_ending.c_str()) && type_pattern->Match(t.type.c_str()))
        return true;
    else
        return false;
}

ManifestResult Builder::Run(const JobHandler& jh, const Item& t) {
    return action->Run(jh, t);
}

std::string Builder::GetBuilderName() const {
    return std::string(action.GetConst().GetName());
}

std::string Builder::GetBuilderVersion() const {
    return std::string(action.GetConst().GetVersion());
}

bool Builder::RunEvenOnIdenticalSource() {
    return action->RunEvenOnIdenticalSource();
}

bool Builder::StoreResultInDatabase() {
    return action->StoreResultInDatabase();
}
