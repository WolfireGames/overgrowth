//-----------------------------------------------------------------------------
//           Name: image_export.cpp
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
#include "image_export.hpp"

#include <Images/image_export.hpp>
#include <Images/stbimage_wrapper.h>

#include <Internal/error.h>
#include <Internal/filesystem.h>
#include <Internal/common.h>

#include <Compat/fileio.h>
#include <Compat/compat.h>

#include <Memory/allocation.h>
#include <Logging/logdata.h>

#include <crnlib.h>

#include <vector>
#include <cstring>
#include <algorithm>
#include <cstdio>

using std::max;
using std::min;

namespace {
void RGBAtoYCOCG(unsigned char *data, unsigned long width, unsigned long height) {
    int index = 0;
    for (unsigned i = 0; i < width * height; i++) {
        const int r = data[index + 2];
        const int g = data[index + 1];
        const int b = data[index + 0];

        const int Co = r - b;
        const int t = b + Co / 2;
        const int Cg = g - t;
        const int Y = t + Cg / 2;

        data[index + 2] = min(max(Co + 128, 0), 255);
        data[index + 1] = min(max(Cg + 128, 0), 255);
        data[index + 0] = 0;
        data[index + 3] = Y;
        index += 4;
    }
}
}  // namespace

std::string ImageExport::FindEmptySequentialFile(const char *filename, const char *suffix) {
    char abs_path[kPathSize];
    for (int i = 1; i < 99999; i++) {
        FormatString(abs_path, kPathSize, "%s%s%05d%s", GetWritePath(CoreGameModID).c_str(), filename, i, suffix);
        if (!CheckFileAccess(abs_path)) {
            return abs_path;
        }
    }
    return filename;
}

void ImageExport::ScaleImageUp(const unsigned char *data,
                               int levels,
                               unsigned long *width_ptr,
                               unsigned long *height_ptr,
                               std::vector<unsigned char> *new_data_ptr) {
    unsigned long &width = *width_ptr;
    unsigned long &height = *height_ptr;
    std::vector<unsigned char> &new_data = *new_data_ptr;

    const int bytes_per_pixel = 4;
    size_t image_size = width * height * bytes_per_pixel;
    new_data.resize(image_size);
    memcpy(&(new_data[0]), data, image_size);

    for (int i = 0; i < levels; i++) {
        width *= 2;
        height *= 2;
        image_size = width * height * bytes_per_pixel;
        std::vector<unsigned char> new_new_data(image_size);
        for (unsigned i = 0; i < width; i++) {
            for (unsigned j = 0; j < height; j++) {
                for (int k = 0; k < bytes_per_pixel; k++) {
                    new_new_data[(i + j * width) * bytes_per_pixel + k] = new_data[((i / 2) + (j / 2) * width / 2) * bytes_per_pixel + k];
                }
            }
        }
        new_data = new_new_data;
    }
}

void ImageExport::SavePNG(const char *file_path, unsigned char *data, unsigned long width, unsigned long height, unsigned short levels) {
    std::vector<unsigned char> scaled_data;
    ImageExport::ScaleImageUp(data, levels, &width, &height, &scaled_data);
    CreateParentDirs(file_path);
#ifdef _WIN32
    createfile(file_path);
    std::string short_path(file_path);
    ShortenWindowsPath(short_path);
    if (!stbi_write_png(short_path.c_str(), width, height, 4, scaled_data.data(), width * 4)) {
        DisplayError("Error", "Problem exporting .png file");
    }
#else
    if (!stbi_write_png(file_path, width, height, 4, scaled_data.data(), width * 4)) {
        DisplayError("Error", "Problem exporting .png file");
    }
#endif
}

void ImageExport::SavePNGTransparent(const char *file_path, unsigned char *data, unsigned long width, unsigned long height, unsigned short levels) {
    std::vector<unsigned char> scaled_data;
    ImageExport::ScaleImageUp(data, levels, &width, &height, &scaled_data);

    int bytespp = 4;
    for (unsigned y = 0; y < height; y++) {
        unsigned char *bits = scaled_data.data() + y * width * 4;
        for (unsigned x = 0; x < width; x++) {
            // Set pixel color to green with a transparency of 128
            bits[0] = min(255, (int)((bits[0] / 255.0f / (scaled_data[y * width * 4 + x * 4 + 3] / 255.0f)) * 255));
            bits[1] = min(255, (int)((bits[1] / 255.0f / (scaled_data[y * width * 4 + x * 4 + 3] / 255.0f)) * 255));
            bits[2] = min(255, (int)((bits[2] / 255.0f / (scaled_data[y * width * 4 + x * 4 + 3] / 255.0f)) * 255));
            bits[3] = scaled_data[y * width * 4 + x * 4 + 3];
            // jump to next pixel
            bits += bytespp;
        }
    }

    CreateParentDirs(file_path);
#ifdef _WIN32
    createfile(file_path);
    std::string short_path(file_path);
    ShortenWindowsPath(short_path);
    if (!stbi_write_png(short_path.c_str(), width, height, 4, scaled_data.data(), width * 4)) {
        DisplayError("Error", "Problem exporting .png file");
    }
#else
    if (!stbi_write_png(file_path, width, height, 4, scaled_data.data(), width * 4)) {
        DisplayError("Error", "Problem exporting .png file");
    }
#endif
}

void ImageExport::SaveJPEG(const char *abs_path, unsigned char *data, unsigned long width, unsigned long height) {
    CreateParentDirs(abs_path);
#ifdef _WIN32
    createfile(abs_path);
    std::string short_path(abs_path);
    ShortenWindowsPath(short_path);
    if (!stbi_write_jpg(short_path.c_str(), width, height, 3, data, 85)) {
        DisplayError("Error", "Problem exporting .jpeg file");
    }
#else
    if (!stbi_write_jpg(abs_path, width, height, 3, data, 85)) {
        DisplayError("Error", "Problem exporting .jpeg file");
    }
#endif
}

void ImageExport::SavePNGofGrayscaleFloats(const char *file_path, std::vector<float> &data, unsigned long width, unsigned long height) {
    if (data.size() != width * height) return;
    unsigned char *byte_data = (unsigned char *)(OG_MALLOC(sizeof(*byte_data) * data.size() * 4));
    for (unsigned i = 0; i < data.size(); i++) {
        unsigned char data_i = (unsigned char)(data[i] * 255);
        byte_data[4 * i] = data_i;
        byte_data[4 * i + 1] = data_i;
        byte_data[4 * i + 2] = data_i;
        byte_data[4 * i + 3] = 255;
    }
    SavePNG(file_path, byte_data, width, height);
    OG_FREE(byte_data);
}
