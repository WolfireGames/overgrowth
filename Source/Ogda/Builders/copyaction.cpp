//-----------------------------------------------------------------------------
//           Name: copyaction.cpp
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
#include "copyaction.h"

#include <Ogda/item.h>
#include <Internal/filesystem.h>
#include <Logging/logdata.h>
#include <Ogda/jobhandler.h>

#include <string>

ManifestResult CopyAction::Run(const JobHandler& jh, const Item& y) {
    LOGD << "Running CopyAction of " << y << std::endl;
    std::string from = y.GetAbsPath();
    std::string to = AssemblePath(jh.output_folder, y.GetPath());

    CreateParentDirs(to);
    int size = copyfile(from, to);

    LOGD << "Copy size " << size << std::endl;

    if (size <= 0) {
        LOGE << "Unable to copy item " << y << std::endl;
    }

    return ManifestResult(jh, y, y.GetPath(), (size > 0), *this, y.type);
}
