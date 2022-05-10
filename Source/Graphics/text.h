//-----------------------------------------------------------------------------
//           Name: text.h
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

#include <Graphics/textureref.h>
#include <Graphics/vbocontainer.h>
#include <Graphics/vboringcontainer.h>
#include <Graphics/font_renderer.h>

#include <Math/vec3.h>
#include <Math/vec4.h>

#include <Asset/Asset/texture.h>

#include <vector>

struct TextCanvasTextureImpl;
class vec2;
class Graphics;

struct CanvasTextStyle {
    enum Alignment { LEFT = 1,
                     CENTER = 2,
                     RIGHT = 3 };
    static inline const char* GetAlignmentString(Alignment a) {
        switch (a) {
            case LEFT:
                return "LEFT";
            case CENTER:
                return "CENTER";
            case RIGHT:
                return "RIGHT";
            default:
                return "(unknown alignment)";
        }
    }
    int font_face_id;
    Alignment alignment;
    CanvasTextStyle() : font_face_id(-1),
                        alignment(LEFT) {}
};

struct TextMetrics {
    int advance[2];  // cursor position in points (64/pixel)
    int bounds[4];   // bounding box of text in points (64/pixel)
    float ascenderRatio;
};

// A texture on which text can be rendered
class TextCanvasTexture {
   public:
    void Create(int width, int height);
    void ClearTextCanvas();
    void UploadTextCanvasToTexture();
    void AddText(const char* str, int length, const CanvasTextStyle& style, uint32_t char_output_limit);
    void AddTextMultiline(const char* str, int length, const CanvasTextStyle& style, uint32_t char_limit);
    void DebugDrawBillboard(const vec3& pos, float scale, int lifespan);
    void RemoveDebugDrawBillboard();
    void Reset();
    // (0,0) is the top-left corner of the text canvas
    // The pen position is at the bottom-left corner of the text
    void SetPenPosition(const vec2& point);
    void SetPenColor(int r, int g, int b, int a);
    void SetPenRotation(float degrees);
    static void GetLetterPosXY(const char* str, const CanvasTextStyle& style, int letter, int coords[2]);
    int GetCursorPos(const char* str, const CanvasTextStyle& style, const int coords[2], uint32_t char_limit);
    TextCanvasTexture();
    ~TextCanvasTexture();
    TextCanvasTexture(const TextCanvasTexture& other);
    TextureRef GetTexture() const;
    void GetTextMetricsInfo(const char* str, int length, const CanvasTextStyle& style, TextMetrics& metrics, uint32_t char_limit);

   private:
    enum TextModeFlags { TMF_METRICS = 1,
                         TMF_DRAW = 2,
                         TMF_AUTO_NEWLINE = 4 };
    void RenderText(const char* str, int length, const CanvasTextStyle& style, TextMetrics& metrics, uint32_t mode, uint32_t char_limit);
    TextCanvasTextureImpl* impl_;

    int debug_draw_billboard_id;
};

struct CharacterInfo {
    uint32_t codepoint;
    bool lowercase;
    int width;
    int height;
    int advance_x;
    int pos[2];
    int bearing[2];
};

// A texture containing a list of common latin characters, including ascii
class TextAtlas {
   public:
    void Create(const char* path, int pixel_height, FontRenderer* font_renderer, int flags);
    void Dispose();
    TextAtlas();
    ~TextAtlas();
    enum Flags {
        kSmallLowercase = 1 << 0
    };
    int atlas_dims[2];
    std::vector<CharacterInfo> alphabet;
    static bool Pack(unsigned char* pixels, int atlas_dims[2],
                     int font_face_id, int lowercase_font_face_id,
                     FontRenderer* font_renderer, std::vector<CharacterInfo>& alphabet);
    int tex;
    int pixel_height;
};

struct CachedTextAtlas {
    static const unsigned kPathSize = 512;
    char path[kPathSize];
    int pixel_height;
    int flags;
    TextAtlas atlas;
};

struct CachedTextAtlases {
    static const int kMaxAtlases = 32;
    int num_atlases;
    CachedTextAtlas cached[kMaxAtlases];
    CachedTextAtlases() : num_atlases(0) {}
};

class TextAtlasRenderer {
   public:
    void Init();
    void Dispose();
    void AddText(TextAtlas* text_atlas, const char* text, int pos[2], FontRenderer* font_renderer, uint32_t char_output_limit);
    TextMetrics GetMetrics(TextAtlas* text_atlas, const char* text, FontRenderer* font_renderer, uint32_t char_output_limit);
    void Draw(TextAtlas* atlas, Graphics* graphics, char flags, const vec4& color);
    enum TextFlags {
        kTextShadow = 1
    };
    TextAtlasRenderer();

    int num_characters;
    static const int kFloatsPerVert = 4;
    static const int kMaxCharacters = 1024;
    unsigned indices[kMaxCharacters * 6];
    float verts[kMaxCharacters * 4 * kFloatsPerVert];  // 2v2t
    VBORingContainer vert_vbo;
    VBORingContainer index_vbo;

   private:
    int shader_id;
    int shader_attrib_vert_coord;
    int shader_attrib_tex_coord;
    int uniform_mvp_mat;
    int uniform_color;
};

class ASContext;
void AttachTextCanvasTextureToASContext(ASContext* ctx);
void DisposeTextAtlases();
void TestTextAtlas();

enum {
    kTextAtlasMono,
    kTextAtlasDynamic,
    kNumTextAtlas
};

struct ASTextContext {
    FontRenderer* font_renderer;
    Graphics* graphics;
    CachedTextAtlases atlases;
    TextAtlasRenderer text_atlas_renderer;
    bool text_atlas_renderer_setup;
    ASTextContext() : text_atlas_renderer_setup(false) {}

    void ASDrawTextAtlas(const std::string& path, int pixel_height, int flags, const std::string& txt, int x, int y, vec4 color);
    void ASDrawTextAtlas2(const std::string& path, int pixel_height, int flags, const std::string& txt, int x, int y, vec4 color, uint32_t char_limit);
    TextMetrics ASGetTextAtlasMetrics(const std::string& path, int pixel_height, int flags, const std::string& txt);
    TextMetrics ASGetTextAtlasMetrics2(const std::string& path, int pixel_height, int flags, const std::string& txt, uint32_t char_limit);
};
