//-----------------------------------------------------------------------------
//           Name: levellistcreator.cpp
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
#include "levellistcreator.h"

#include <fstream>
#include <iostream>

#include <tinyxml.h>
#include <XML/xml_helper.h>
#include <Version/version.h>
#include <Internal/filesystem.h>
#include <Ogda/jobhandler.h>

ManifestResult LevelListCreator::Run(const JobHandler& jh, const Manifest& manifest) {
    std::string destination("level_list");

    std::string full_path = AssemblePath(jh.output_folder, destination);

    std::ofstream f(full_path.c_str(), std::ios::out | std::ios::binary);

    LOGI << "Writing level_list: " << full_path << std::endl;

    std::vector<ManifestResult>::const_iterator itr = manifest.ResultsBegin();

    for (; itr != manifest.ResultsEnd(); itr++) {
        if (itr->type == "level")
            f << itr->dest << std::endl;
    }

    f.close();

    return ManifestResult(jh, destination, true, *this, "level_list");
}
