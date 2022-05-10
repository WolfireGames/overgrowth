//-----------------------------------------------------------------------------
//           Name: palette.h
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

#include <Math/vec3.h>

#include <string>
#include <vector>

enum PaletteChannel { _pm_red,
                      _pm_green,
                      _pm_blue,
                      _pm_alpha,
                      _pm_other };

const int max_palette_elements = 5;

struct LabeledColor {
    std::string label;
    vec3 color;
    char channel;
};

typedef std::vector<LabeledColor> OGPalette;

class TiXmlElement;
void ReadPaletteFromXML(OGPalette &palette, const TiXmlElement *palette_el);
void WritePaletteToXML(const OGPalette &palette, TiXmlElement *palette_el);
void ReadPaletteFromRAM(OGPalette &palette, const std::vector<char> &data);
void WritePaletteToRAM(const OGPalette &palette, std::vector<char> &data);
