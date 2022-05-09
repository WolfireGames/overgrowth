//-----------------------------------------------------------------------------
//           Name: palette.cpp
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
#include "palette.h"

#include <Internal/memwrite.h>

#include <tinyxml.h>

void ReadPaletteFromRAM(OGPalette &palette, const std::vector<char> &data) {
    int index = 0;
    int num_colors;
    memread(&num_colors, sizeof(int), 1, data, index);
    palette.resize(num_colors);
    for (int i = 0; i < num_colors; ++i) {
        LabeledColor &lc = palette[i];
        int str_len;
        memread(&str_len, sizeof(int), 1, data, index);
        lc.label.resize(str_len);
        memread(&lc.label[0], sizeof(char), str_len, data, index);
        memread(&lc.color, sizeof(vec3), 1, data, index);
        memread(&lc.channel, sizeof(char), 1, data, index);
    }
}

void WritePaletteToRAM(const OGPalette &palette, std::vector<char> &data) {
    int num_colors = palette.size();
    memwrite(&num_colors, sizeof(int), 1, data);
    for (int i = 0; i < num_colors; ++i) {
        const LabeledColor &lc = palette[i];
        int str_len = lc.label.size();
        memwrite(&str_len, sizeof(int), 1, data);
        memwrite(&lc.label[0], sizeof(char), str_len, data);
        memwrite(&lc.color, sizeof(vec3), 1, data);
        memwrite(&lc.channel, sizeof(char), 1, data);
    }
}

void WritePaletteToXML(const OGPalette &palette, TiXmlElement *palette_el) {
    for (const auto &i : palette) {
        TiXmlElement *color_el = new TiXmlElement("Color");
        palette_el->LinkEndChild(color_el);
        const std::string &label = i.label;
        const vec3 &color = i.color;
        const char &channel = i.channel;
        color_el->SetAttribute("label", label.c_str());
        color_el->SetAttribute("channel", channel);
        color_el->SetDoubleAttribute("red", color[0]);
        color_el->SetDoubleAttribute("green", color[1]);
        color_el->SetDoubleAttribute("blue", color[2]);
    }
}

void ReadPaletteFromXML(OGPalette &palette, const TiXmlElement *palette_el) {
    const TiXmlElement *color_el = palette_el->FirstChildElement("Color");
    while (color_el) {
        LabeledColor lc;
        vec3 &color = lc.color;
        std::string &label = lc.label;
        char &channel = lc.channel;
        color_el->QueryFloatAttribute("red", &color[0]);
        color_el->QueryFloatAttribute("green", &color[1]);
        color_el->QueryFloatAttribute("blue", &color[2]);
        int channel_int;
        color_el->QueryIntAttribute("channel", &channel_int);
        channel = channel_int;
        const char *c = color_el->Attribute("label");
        if (c) {
            label = c;
        }
        if (!label.empty() && channel < max_palette_elements) {
            palette.push_back(lc);
        }
        color_el = color_el->NextSiblingElement();
    }
}
