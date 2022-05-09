//-----------------------------------------------------------------------------
//           Name: hudimage.h
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

#include <Math/vec2.h>
#include <Math/vec3.h>
#include <Math/vec4.h>

#include <Graphics/textureref.h>
#include <Asset/Asset/texture.h>

#include <string>
#include <vector>

class ASContext;
class TextCanvasTexture;

struct HUDImage {
    TextureRef texture_ref;
    TextureAssetRef texture_asset;
    vec3 pos;
    vec3 scale;
    vec2 tex_scale;
    vec2 tex_offset;
    vec4 color;
    bool text;
    bool flipped;

    float GetWidth() const;
    float GetHeight() const;
    bool SetImageFromPath(const std::string &path);
    void SetImageFromText(const TextCanvasTexture *text_canvas_texture);
};

struct HUDImages {
    std::vector<HUDImage> hud_images;
    HUDImages();
    void AttachToContext(ASContext *as_context);
    void Draw();

   private:
    HUDImage *AddImage();
};
