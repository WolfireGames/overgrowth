//-----------------------------------------------------------------------------
//           Name: bytecolor.cpp
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
#include "bytecolor.h"

#include <Math/enginemath.h>
#include <Graphics/ColorWheel.h>
#include <Images/texture_data.h>

unsigned distance_squared(const ByteColor &a, const ByteColor &b) {
    return square((int)a.color[0] - (int)b.color[0]) +
           square((int)a.color[1] - (int)b.color[1]) +
           square((int)a.color[2] - (int)b.color[2]);
}

float hue_saturation_distance_squared(const ByteColor &a, const ByteColor &b) {
    vec3 hsv_a = RGBtoHSV(vec3(a.color[0] / 256.0f, a.color[1] / 256.0f, a.color[2] / 256.0f));
    vec3 hsv_b = RGBtoHSV(vec3(b.color[0] / 256.0f, b.color[1] / 256.0f, b.color[2] / 256.0f));
    float hue_a = hsv_a[0] / 360.0f;
    float hue_b = hsv_b[0] / 360.0f;
    float hue_difference = min(fabs(hue_a - hue_b), min(fabs(hue_a - (1.0f + hue_b)), fabs(1.0f + hue_a - hue_b)));
    return square(hue_difference) +
           square(hsv_a[1] - hsv_b[1]) +
           square(hsv_a[2] - hsv_b[2]);
}

int WeightIndex(const TextureData &data, int i, int j) {
    // TODO: check this
    return (i + j * data.GetWidth()) * 4;
}

ByteColor::ByteColor() {
    color[0] = 0;
    color[1] = 0;
    color[2] = 0;
}

ByteColor::ByteColor(unsigned char r, unsigned char g, unsigned char b) {
    Set(r, g, b);
}

void ByteColor::Set(unsigned char r, unsigned char g, unsigned char b) {
    color[0] = r;
    color[1] = g;
    color[2] = b;
}

const ByteColor &ByteColor::operator=(const ByteColor &other) {
    color[0] = other.color[0];
    color[1] = other.color[1];
    color[2] = other.color[2];
    return *this;
}
