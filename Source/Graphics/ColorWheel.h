//-----------------------------------------------------------------------------
//           Name: ColorWheel.h
//      Developer: Wolfire Games LLC
//         Author: Phillip Isola
//    Description: Radial parameterization of color; color theory functions.
//                   Hue parameterized by angle in degrees. Saturation and value 
//                   parameterized by range [0,1].
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

vec4 HSVtoRGB(float h, float s, float v);
void RGBtoHSV(const vec4& rgb, float& h, float& s, float& v);
vec3 RGBtoHSV(const vec3 &rgb);
vec4 GetComplementRGB(const vec4& rgb);
vec4 RotateHueRGB(const vec4& rgb, float angle);
void GetComplementHSV(float& h, float& s, float& v);
void RotateHueHSV(float& h, float angle);
