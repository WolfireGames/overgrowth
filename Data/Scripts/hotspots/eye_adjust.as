//-----------------------------------------------------------------------------
//           Name: eye_adjust.as
//      Developer: Wolfire Games LLC
//    Script Type: Hotspot
//        License: Read below
//-----------------------------------------------------------------------------
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

bool inside = false;

void Init() {
    // No initialization needed
}

void HandleEvent(string event, MovementObject@ mo) {
    if (event == "enter") {
        inside = true;
    } else if (event == "exit") {
        inside = false;
    }
}

void Update() {
    if (!inside) {
        return;
    }
    AdjustHDRSettings();
}

void AdjustHDRSettings() {
    float white_point = params.GetFloat("HDR White point");
    float black_point = params.GetFloat("HDR Black point");
    float bloom_mult = params.GetFloat("HDR Bloom multiplier");

    SetHDRWhitePoint(mix(GetHDRWhitePoint(), white_point, 0.05));
    SetHDRBlackPoint(mix(GetHDRBlackPoint(), black_point, 0.05));
    SetHDRBloomMult(mix(GetHDRBloomMult(), bloom_mult, 0.05));
}

void SetParameters() {
    params.AddFloatSlider("HDR White point", GetHDRWhitePoint(), "min:0,max:2,step:0.001,text_mult:100");
    params.AddFloatSlider("HDR Black point", GetHDRBlackPoint(), "min:0,max:2,step:0.001,text_mult:100");
    params.AddFloatSlider("HDR Bloom multiplier", GetHDRBloomMult(), "min:0,max:5,step:0.001,text_mult:100");
}
