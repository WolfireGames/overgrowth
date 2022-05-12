//-----------------------------------------------------------------------------
//           Name: image_export.hpp
//      Developer: Wolfire Games LLC
//         Author: Phillip Isola
//    Description: Wraps up FreeImage library
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

#include <Images/ddsformat.hpp>

#include <string>
#include <vector>

namespace ImageExport {
struct CubemapFace {
    std::vector<unsigned char> pixels;
};
struct CubemapMipLevel {
    CubemapFace faces[6];
};
struct CubemapMipmaps {
    std::vector<CubemapMipLevel> mips;
};

void SaveJPEG(const char *abs_path, unsigned char *data, unsigned long width, unsigned long height);
void SavePNG(const char *file_path, unsigned char *data, unsigned long width, unsigned long height, unsigned short levels = 0);
void SavePNGTransparent(const char *file_path, unsigned char *data, unsigned long width, unsigned long height, unsigned short levels = 0);
void SavePNGofGrayscaleFloats(const char *file_path, std::vector<float> &data, unsigned long width, unsigned long height);
std::string FindEmptySequentialFile(const char *filename, const char *suffix);
void ScaleImageUp(const unsigned char *data, int levels, unsigned long *width, unsigned long *height, std::vector<unsigned char> *new_data);
}  // namespace ImageExport
