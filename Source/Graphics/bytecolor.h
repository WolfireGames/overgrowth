//-----------------------------------------------------------------------------
//           Name: bytecolor.h
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

#include <Math/vec4.h>
#include <Math/vec3.h>

#include <Graphics/textures.h>

struct ByteColor {
    unsigned char color[3];

    ByteColor();
    ByteColor(unsigned char r, unsigned char g, unsigned char b);
    void Set(unsigned char r, unsigned char g, unsigned char b);
    const ByteColor &operator=(const ByteColor &other);
};

vec4 GetAverageColor(const char *abs_path);
unsigned distance_squared(const ByteColor &a, const ByteColor &b);
float hue_saturation_distance_squared(const ByteColor &a, const ByteColor &b);
int WeightIndex(const TextureData &data, int i, int j);
