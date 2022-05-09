//-----------------------------------------------------------------------------
//           Name: converttexture.h
//      Developer: Wolfire Games LLC
//    Description:
//        License: Read below
//-----------------------------------------------------------------------------
//
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

#include <Images/texture_data.h>

#include <string>

bool ConvertImage(std::string src, std::string dst, std::string temp_conversion_path, TextureData::ConversionQuality quality);

// Get a location that we can use to write a temporary .dds file, so that we don't end
// up with an incomplete dds file if the conversion fails halfway through
std::string GetTempDDSPath(std::string dst, bool write_path, int concurrent_id);
