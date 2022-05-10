//-----------------------------------------------------------------------------
//           Name: dxt5action.cpp
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
#include "dxt5action.h"

#include <Images/image_export.hpp>
#include <Images/texture_data.h>
#include <Ogda/jobhandler.h>
#include <Internal/filesystem.h>
#include <Graphics/converttexture.h>
#include <Images/freeimage_wrapper.h>
#include <Internal/common.h>
#include <Internal/filesystem.h>
#include <Internal/datemodified.h>

ManifestResult DXT5Action::Run(const JobHandler& jh, const Item& item) {
    std::string full_source_path = item.GetAbsPath();
    std::string partial_dest_path = item.GetPath() + "_converted.dds";
    std::string full_dest_path = AssemblePath(jh.output_folder, partial_dest_path);
    std::string temp_path = AssemblePath(jh.output_folder, partial_dest_path + ".tmp");

    bool suc = ConvertImage(full_source_path, full_dest_path, temp_path, TextureData::Nice);

    return ManifestResult(jh, item, partial_dest_path, suc, *this, "dxt5");
}
