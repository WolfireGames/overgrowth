//-----------------------------------------------------------------------------
//           Name: dimgui_graph.h
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

#include <imgui.h>

template<unsigned int d_elems>
class ImGUIGraph {
    int offset;
    int last_min_adj;
    int last_max_adj;
    float minV, maxV;
    std::array<float, d_elems> elements;

public:
    ImGUIGraph() {
        offset = 0;
        minV = 0.0f;
        maxV = 1.0f;
        for(int i = 0; i < elements.size(); i++) {
            elements[i] = 0.0f;
        }
    }

    void Push(float value) {
        elements[offset] = value;

        if(value > maxV) {
            maxV = value;
            last_max_adj = 0;
        } 

        if(value < minV) {
            minV = value;
            last_min_adj = 0;
        }

        //Only shrink if the gap between the scales is more than 1.0f
        if(abs(maxV - minV) > 0.5f) {
            if(last_min_adj > d_elems) {
                minV = mix(minV, maxV, 0.99f);
            }
            if(last_max_adj > d_elems) {
                maxV = mix(maxV, minV, 0.99f);
            }
        }

        offset++;
        if(offset >= d_elems) {
            offset = 0;
        }
    }

    void Draw(const char* title) {
        ImGui::PlotLines(title, (float*)elements.data(), d_elems, offset, "", minV, maxV, ImVec2(300, 200));
    }
};
