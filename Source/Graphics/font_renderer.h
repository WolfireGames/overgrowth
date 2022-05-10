//-----------------------------------------------------------------------------
//           Name: font_renderer.h
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

#include <Internal/integer.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <string>

struct FontRendererImpl;

class FontRenderer {
   public:
    enum FontInfo { INFO_ASCENDER,
                    INFO_DESCENDER,
                    INFO_HEIGHT };
    enum RCB_Flags { RCB_METRIC = 1,
                     RCB_RENDER = 2 };
    void PreLoadFont(const std::string& file_path);
    int GetFontFaceID(const std::string& file_path, int pixel_height);
    int GetFontInfo(int font_face_id, FontInfo font_info);
    void SetTransform(int face_id, FT_Matrix* matrix, FT_Vector* vector);
    FT_GlyphSlot RenderCharacterBitmap(int face_id, uint32_t character, RCB_Flags flags);

    static FontRenderer* Instance(FontRenderer* ptr = NULL) {
        static FontRenderer* font_renderer = NULL;
        if (ptr) {
            font_renderer = ptr;
        }
        return font_renderer;
    }
    void GetKerning(int face_id, char character_a, char character_b, FT_Vector* vec);
    FontRenderer();
    FontRenderer(const FontRenderer& other);
    ~FontRenderer();

   private:
    FontRendererImpl* impl_;
};
