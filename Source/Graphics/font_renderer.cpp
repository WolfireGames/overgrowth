//-----------------------------------------------------------------------------
//           Name: font_renderer.cpp
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
#include "font_renderer.h"

#include <Graphics/textures.h>
#include <Graphics/graphics.h>

#include <Internal/error.h>
#include <Compat/fileio.h>
#include <Internal/filesystem.h>
#include <Images/texture_data.h>
#include <Logging/logdata.h>

#include <vector>
#include <map>
#include <string>
#include <list>

// Keep track of which font files have been loaded, so we don't have to load them again
class FontFiles {
   public:
    // Using vector<char> just to have a convenient self-deleting structure
    // to keep track of a block of memory
    typedef std::vector<unsigned char> MemoryBlock;
    MemoryBlock *GetFontFileMemory(const std::string &path);

   private:
    typedef std::map<std::string, MemoryBlock *> FontFileMap;
    FontFileMap font_files_;
    std::list<MemoryBlock> memory_blocks;
};

FontFiles::MemoryBlock *FontFiles::GetFontFileMemory(const std::string &abs_path) {
    // Check if we already loaded this file
    FontFileMap::iterator it = font_files_.find(abs_path);
    if (it != font_files_.end()) {
        return it->second;
    } else {
        // File not loaded yet, so load it
        FILE *file = my_fopen(abs_path.c_str(), "rb");
        if (file == NULL) {
            FatalError("Error", "Failed to open font file: %s\n%s", abs_path.c_str(), strerror(errno));
        }
        // Determine the size of the file
        fseek(file, 0, SEEK_END);
        int len = ftell(file);
        fseek(file, 0, SEEK_SET);
        // Read the entire file
        memory_blocks.resize(memory_blocks.size() + 1);
        MemoryBlock &memory_block = memory_blocks.back();
        memory_block.resize(len);
        int c = fread(&memory_block[0], len, 1, file);
        if (c == 0) {
            FatalError("Error", "Failed to read font file: %s\n%s", abs_path.c_str(), strerror(errno));
        }
        fclose(file);
        font_files_[abs_path] = &memory_block;
        return &memory_block;
    }
}

struct FontFace {
    FT_Face face;
};

struct FontRendererImpl {
    FontFiles font_files_;
    FT_Library freetype_library_;
    typedef std::pair<std::string, int> FontData;
    typedef std::map<FontData, int> FontIDMap;
    typedef std::map<int, FontFace> FontMap;
    FontIDMap face_ids_;
    FontMap faces_;

    ~FontRendererImpl();

    int GetFontFaceID(const std::string &file_path, int pixel_height);
    void SetTransform(int face_id, FT_Matrix *matrix, FT_Vector *vector);
    FT_GlyphSlot RenderCharacterBitmap(int face_id, uint32_t character, FontRenderer::RCB_Flags flags);
    FontRendererImpl();
    void GetKerning(int face_id, char character_a, char character_b, FT_Vector *vec);
    int GetFontInfo(int font_face_id, FontRenderer::FontInfo font_info);
};

FontRendererImpl::~FontRendererImpl() {
    for (auto &it : faces_) {
        FontFace &face = it.second;
        FT_Done_Face(face.face);
        face.face = NULL;
    }

    faces_.clear();

    FT_Done_FreeType(freetype_library_);
    freetype_library_ = NULL;
}

int FontRendererImpl::GetFontFaceID(const std::string &file_path, int pixel_height) {
    // Check if we have already loaded this font at this size
    FontData font_data;
    static const int kBufSize = 1024;
    char abs_path[kBufSize];
    if (FindFilePath(file_path.c_str(), abs_path, kBufSize, kDataPaths | kModPaths) == -1) {
        FatalError("Error", "Could not find %s", file_path.c_str());
    }
    font_data.first = abs_path;
    font_data.second = pixel_height;
    FontIDMap::iterator it = face_ids_.find(font_data);
    if (it != face_ids_.end()) {
        return it->second;
    } else {
        FontFiles::MemoryBlock *memory_block = font_files_.GetFontFileMemory(abs_path);
        // Find unused id
        int unused_id = 0;
        while (faces_.find(unused_id) != faces_.end()) {
            ++unused_id;
        }
        face_ids_[font_data] = unused_id;
        // Create face from loaded font file
        FontFace &font_face = faces_[unused_id];
        FT_Face &face = font_face.face;
        int error = FT_New_Memory_Face(freetype_library_,
                                       (FT_Byte *)&memory_block->at(0),
                                       memory_block->size(),
                                       0 /*Get the first face in file*/,
                                       &face);
        if (error) {
            FatalError("Error", "Failed to extract face from font: %s", abs_path);
        }
        // Set face size
        error = FT_Set_Char_Size(
            face,              /* handle to face object           */
            0,                 /* char_width in 1/64th of points  */
            pixel_height * 64, /* char_height in 1/64th of points */
            72,                /* horizontal device resolution    */
            72);               /* vertical device resolution      */
        if (error) {
            FatalError("Error", "Failed to set font face size");
        }
        return unused_id;
    }
}

FontRendererImpl::FontRendererImpl() {
    int error = FT_Init_FreeType(&freetype_library_);
    if (error) {
        FatalError("Error", "Could not initialize freetype library");
    }
}

void FontRendererImpl::SetTransform(int face_id, FT_Matrix *matrix, FT_Vector *vector) {
    FT_Set_Transform(faces_[face_id].face, matrix, vector);
}

FT_GlyphSlot FontRendererImpl::RenderCharacterBitmap(int face_id, uint32_t character, FontRenderer::RCB_Flags flags) {
    FT_Face face = faces_[face_id].face;
    // Get index of a glyph
    int glyph_index = FT_Get_Char_Index(face, character);
    // Load glyph into slot
    int error = FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT);
    if (error) {
        FatalError("Error", "Failed to load font glyph into slot");
    }
    // Render glyph in slot
    if (flags & FontRenderer::RCB_RENDER) {
        error = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);
        if (error) {
            FatalError("Error", "Failed to render glyph");
        }
    }
    return face->glyph;
}

void FontRendererImpl::GetKerning(int face_id, char character_a, char character_b, FT_Vector *vec) {
    FT_Face face = faces_[face_id].face;
    FT_Bool use_kerning = FT_HAS_KERNING(face);
    if (use_kerning) {
        int glyph_index_a = FT_Get_Char_Index(face, character_a);
        int glyph_index_b = FT_Get_Char_Index(face, character_b);
        FT_Get_Kerning(face, glyph_index_a, glyph_index_b, FT_KERNING_DEFAULT, vec);
    } else {
        vec->x = 0;
        vec->y = 0;
    }
}

void FontRenderer::SetTransform(int face_id, FT_Matrix *matrix, FT_Vector *vector) {
    impl_->SetTransform(face_id, matrix, vector);
}

FT_GlyphSlot FontRenderer::RenderCharacterBitmap(int face_id, uint32_t character, RCB_Flags flags) {
    return impl_->RenderCharacterBitmap(face_id, character, flags);
}

int FontRenderer::GetFontFaceID(const std::string &file_path, int pixel_height) {
    return impl_->GetFontFaceID(file_path, pixel_height);
}

FontRenderer::FontRenderer() {
    impl_ = new FontRendererImpl();
}

FontRenderer::~FontRenderer() {
    delete impl_;
}

void FontRenderer::PreLoadFont(const std::string &rel_path) {
    char abs_path[kPathSize];
    if (FindFilePath(rel_path.c_str(), abs_path, kPathSize, kModPaths | kDataPaths) == -1) {
        FatalError("Error", "Could not find font file: %s", rel_path.c_str());
    } else {
        impl_->font_files_.GetFontFileMemory(abs_path);
    }
}

void FontRenderer::GetKerning(int face_id, char character_a, char character_b, FT_Vector *vec) {
    impl_->GetKerning(face_id, character_a, character_b, vec);
}

int FontRendererImpl::GetFontInfo(int font_face_id, FontRenderer::FontInfo font_info) {
    const FT_Face &face = faces_[font_face_id].face;
    const FT_Size_Metrics &metrics = face->size->metrics;
    switch (font_info) {
        case FontRenderer::INFO_ASCENDER:
            return metrics.ascender;
        case FontRenderer::INFO_DESCENDER:
            return metrics.descender;
        case FontRenderer::INFO_HEIGHT:
            return metrics.height;
        default:
            return 0;
    }
}

int FontRenderer::GetFontInfo(int font_face_id, FontInfo font_info) {
    return impl_->GetFontInfo(font_face_id, font_info);
}
