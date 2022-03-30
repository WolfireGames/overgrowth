//-----------------------------------------------------------------------------
//           Name: ColorWheel.cpp
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
#include "ColorWheel.h"

#include <Math/enginemath.h>

#include <algorithm>

vec3 HSVtoRGB(const vec3 &hsv) {
    float h = hsv[0];
    while (h < 0) h += 360;
    int hi = (int)(h/60);
    float f = h/60 - hi;
    hi = hi%6;
    float p = hsv[2]*(1-hsv[1]);
    float q = hsv[2]*(1-(f*hsv[1]));
    float t = hsv[2]*(1-((1-f)*hsv[1]));
    
    switch (hi) {
        case 0:
            return vec3(hsv[2],t,p);
            break;
        case 1:
            return vec3(q,hsv[2],p);
            break;
        case 2:
            return vec3(p,hsv[2],t);
            break;
        case 3:
            return vec3(p,q,hsv[2]);
            break;
        case 4:
            return vec3(t,p,hsv[2]);
            break;
        case 5:
            return vec3(hsv[2],p,q);
            break;
    }
    return vec3(0,0,0);
}

vec4 HSVtoRGB(float h, float s, float v) {
    vec3 rgb = HSVtoRGB(vec3(h,s,v));
    return vec4(rgb,0.0f);
}

vec3 RGBtoHSV(const vec3 &rgb){
    vec3 hsv;

    // calc hue
    float max_val = max(max(rgb[0],rgb[1]), rgb[2]);
    float min_val = min(min(rgb[0],rgb[1]), rgb[2]);
    float range = max_val - min_val;
    
    if (max_val == min_val) hsv[0] = 0;
    else if (max_val == rgb[0]) {
        hsv[0] = (float)(((int)(60 * ((rgb[1] - rgb[2])/range)))%360);
    }
    else if (max_val == rgb[1]) {
        hsv[0] = (float)(60 * ((rgb[2] - rgb[0])/range) + 120);
    }
    else {
        hsv[0] = (float)(60 * ((rgb[0] - rgb[1])/range) + 240);
    }
    
    // calc saturation
    if (max_val == 0) {
        hsv[1] = 0;
    }
    else {
        hsv[1] = range/max_val;
    }
    
    // calc value
    hsv[2] = max_val;
    
    return hsv;
}

void RGBtoHSV(const vec4& rgb, float& h, float& s, float& v) {
    vec3 hsv = RGBtoHSV(rgb.xyz());
    h = hsv[0];
    s = hsv[1];
    v = hsv[2];
}

vec4 RotateHueRGB(const vec4& rgb, float angle) {

    float h, s, v;
    RGBtoHSV(rgb, h, s, v);
    RotateHueHSV(h,angle);
    return HSVtoRGB(h,s,v);
}

vec4 GetComplementRGB(const vec4& rgb) {
    return RotateHueRGB(rgb,180);
}

void RotateHueHSV(float& h, float angle) {
    h += angle;
}

void GetComplementHSV(float& h, float& s, float& v) {
    RotateHueHSV(h,180);
}

