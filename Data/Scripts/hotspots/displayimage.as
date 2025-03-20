//-----------------------------------------------------------------------------
//           Name: displayimage.as
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

bool is_active = false;
string image_path;
float scale;
vec4 tint;

void Init() {
    // No initialization needed
}

void SetParameters() {
    params.AddString("[0] Display Image Path", "Data/UI/spawner/thumbs/Hotspot/sign_icon.png");
    image_path = params.GetString("[0] Display Image Path");

    params.AddFloatSlider("[1] Scale", 0.5, "min:0.0,max:1.0,step:0.1,text_mult:100");
    scale = params.GetFloat("[1] Scale");

    params.AddIntSlider("[2a] Red Tint", 255, "min:0,max:255");
    params.AddIntSlider("[2b] Green Tint", 255, "min:0,max:255");
    params.AddIntSlider("[2c] Blue Tint", 255, "min:0,max:255");
    params.AddIntSlider("[2d] Alpha Tint", 255, "min:0,max:255");

    tint.x = IntToFloatColor(params.GetInt("[2a] Red Tint"));
    tint.y = IntToFloatColor(params.GetInt("[2b] Green Tint"));
    tint.z = IntToFloatColor(params.GetInt("[2c] Blue Tint"));
    tint.a = IntToFloatColor(params.GetInt("[2d] Alpha Tint"));
}

float IntToFloatColor(int value) {
    return clamp(float(value), 0.0f, 255.0f) / 255.0f;
}

void Reset() {
    is_active = false;
}

void HandleEvent(string event, MovementObject@ mo) {
    if (!mo.controlled) {
        return;
    }
    if (event == "enter") {
        if (image_path.length() > 0 && FileExists(image_path)) {
            is_active = true;
        }
    } else if (event == "exit") {
        is_active = false;
    }
}

void Draw() {
    if (!is_active) {
        return;
    }
    HUDImage@ image = hud.AddImage();
    image.SetImageFromPath(image_path);

    vec2 screen_dims(GetScreenWidth(), GetScreenHeight());
    vec2 image_dims(image.GetWidth(), image.GetHeight());

    float screen_aspect_ratio = screen_dims.x / screen_dims.y;
    float image_aspect_ratio = image_dims.x / image_dims.y;

    float fill_scale = (screen_aspect_ratio <= image_aspect_ratio) ?
        screen_dims.x / image_dims.x : screen_dims.y / image_dims.y;

    float image_scale = scale * fill_scale;
    vec2 image_pos = (screen_dims - image_dims * image_scale) * 0.5f;

    image.scale = vec3(image_scale, image_scale, 0.0f);
    image.position = vec3(image_pos.x, image_pos.y, 0.0f);
    image.color = tint;
}
