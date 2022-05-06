//-----------------------------------------------------------------------------
//           Name: text.cpp
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
#include "text.h"

#include <Graphics/textures.h>
#include <Graphics/graphics.h>
#include <Graphics/font_renderer.h>
#include <Graphics/shaders.h>
#include <Graphics/Billboard.h>
#include <Graphics/pxdebugdraw.h>
#include <Graphics/vbocontainer.h>

#include <Internal/error.h>
#include <Internal/profiler.h>

#include <Timing/timingevent.h>
#include <Timing/intel_gl_perf.h>

#include <Images/texture_data.h>
#include <Images/image_export.hpp>

#include <Memory/allocation.h>
#include <Compat/fileio.h>
#include <Scripting/angelscript/ascontext.h>
#include <Wrappers/glm.h>
#include <Logging/logdata.h>
#include <Utility/assert.h>

#include <SDL_assert.h>
#include <utf8/utf8.h>
#include <ft2build.h>
#include FT_FREETYPE_H

#include <vector>
#include <map>
#include <string>
#include <list>

TextAtlas g_text_atlas[kNumTextAtlas];
TextAtlasRenderer g_text_atlas_renderer;

// A rectangular image that accumulates Freetype bitmaps, and can be copied to
// an OpenGL texture
class TextCanvas {
public:
	typedef std::vector<unsigned char> MemoryBlock; 
	void Create(int width, int height);
    void AddBitmap(int x, int y, const FT_Bitmap& bitmap);
	void GetBGRA(MemoryBlock *mem_to_fill);
    void Clear();
    void SetPenColor(int r, int g, int b, int a);
    void RenderToConsole();
    int width() {return width_;}
    int height() {return height_;}
private:
    struct PenColor {
        unsigned char r,g,b,a;
    };
    PenColor pen_color_;
    int width_, height_;
    std::vector<unsigned char> pixels_;
};

void TextCanvas::Create( int width, int height ) {
    width_ = width;
    height_ = height;
    // Allocate 4 bytes per pixel
    pixels_.resize(width_ * height_ * 4, 0);
    pen_color_.r = 0;
    pen_color_.g = 0;
    pen_color_.b = 0;
    pen_color_.a = 255;
}

static int min(int a, int b){
    if(a < b){
        return a;
    } else {
        return b;
    }
}

static int max(int a, int b){
    if(a > b){
        return a;
    } else {
        return b;
    }
}

int mix(int a, int b, int amount) {
    return (a*(255-amount) + b*amount)/255;
}

void TextCanvas::AddBitmap( int x, int y, const FT_Bitmap& bitmap ) {
    int left   = x;
    int right  = x+bitmap.width;
    int bottom = y;
    int top    = y+bitmap.rows;
    left   = clamp(left,  0, width_);
    right  = clamp(right, 0, width_);
    top    = clamp(top,   1, height_);
    bottom = clamp(bottom,1, height_);
    int pen_alpha = pen_color_.a;
    for(int i = bottom; i < top; ++i){
        int index = (height_-i)*width_*4 + left*4;
        int bitmap_index = (i-y)*bitmap.width;
        for(int j = left; j < right; ++j){
            int opac = bitmap.buffer[bitmap_index+(j-x)];
            opac = (opac * pen_alpha) / 255;
            unsigned char* pixel =  &pixels_[index];
            pixel[0] = mix((int)pixel[0], (int)pen_color_.r, opac);   
            pixel[1] = mix((int)pixel[1], (int)pen_color_.g, opac);   
            pixel[2] = mix((int)pixel[2], (int)pen_color_.b, opac);   
            pixel[3] = mix((int)pixel[3], 255, opac);     
            index += 4;   
        } 
    }
}

void TextCanvas::RenderToConsole() {
    for(int i=0; i<height_; ++i){
        int index = i*width_;
        for(int j=0; j<width_; ++j){
            printf("%d",pixels_[index+j]/26);
        }
        printf("\n");
    }
}

void TextCanvas::GetBGRA( MemoryBlock *mem_to_fill ) {
	unsigned num_pixels = pixels_.size();
	mem_to_fill->resize(num_pixels);
	for(unsigned i=0; i<num_pixels; ++i){
		mem_to_fill->at(i) = pixels_[i];
	}
}

void TextCanvas::Clear() {
    for(unsigned char & pixel : pixels_){
        pixel = 0;
    }
}

void TextCanvas::SetPenColor( int r, int g, int b, int a ) {
    pen_color_.r = r;
    pen_color_.g = g;
    pen_color_.b = b;
    pen_color_.a = a;
}


void RenderCharacterToConsole(int face_id, uint32_t character){
    FontRenderer* font_renderer = FontRenderer::Instance();
    FT_GlyphSlot slot = font_renderer->RenderCharacterBitmap(face_id, character, FontRenderer::RCB_RENDER);
    FT_Bitmap &bitmap = slot->bitmap;
    for(int i=0; i<bitmap.rows; ++i){
        int index = i*bitmap.width;
        for(int j=0; j<bitmap.width; ++j){
            printf("%d",bitmap.buffer[index+j]/26);
        }
        printf("\n");
    }
}

TextureRef CreateTextureFromBGRABlock(const TextCanvas::MemoryBlock &bgra_block, int width, int height){\
    TextureRef texture_ref = Textures::Instance()->makeTexture(width, height, GL_RGBA, GL_RGBA, true);
    Textures::Instance()->bindTexture(texture_ref);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, &bgra_block[0]);
    Textures::Instance()->GenerateMipmap(texture_ref);
    return texture_ref;
}

struct TextCanvasTextureImpl {
    TextCanvas::MemoryBlock bgra_block;
    TextureRef texture_ref;
    TextCanvas text_canvas;
    FT_Vector pen;
    float pen_rotation;
    TextCanvasTextureImpl():pen_rotation(0.0f) {
        pen.x = 0;
        pen.y = 0;
    }
};

void TextCanvasTexture::Create( int width, int height) {
    TextCanvas &text_canvas             = impl_->text_canvas;
    TextCanvas::MemoryBlock &bgra_block = impl_->bgra_block;
    TextureRef &texture_ref             = impl_->texture_ref;
    text_canvas.Create(width, height);
    text_canvas.GetBGRA(&bgra_block);
    texture_ref = CreateTextureFromBGRABlock(bgra_block, width, height);
    Textures::Instance()->SetTextureName(texture_ref, "Text Canvas Texture");
}

void FillTextureFromBGRABlock(const TextureRef &texture_ref, const TextCanvas::MemoryBlock &bgra_block, int width, int height) {
    Textures::Instance()->bindTexture(texture_ref);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, &bgra_block[0]);
    Textures::Instance()->GenerateMipmap(texture_ref);
} 

void TextCanvasTexture::UploadTextCanvasToTexture() {
    TextCanvas &text_canvas             = impl_->text_canvas;
    TextCanvas::MemoryBlock &bgra_block = impl_->bgra_block;
    TextureRef &texture_ref        = impl_->texture_ref;
    text_canvas.GetBGRA(&bgra_block);
    FillTextureFromBGRABlock(texture_ref, bgra_block, text_canvas.width(), text_canvas.height());
}

void TextCanvasTexture::ClearTextCanvas() {
    impl_->text_canvas.Clear();
}

void TextCanvasTexture::SetPenPosition(const vec2 &point){
    impl_->pen.x = (FT_Pos)(point[0]*64);
    impl_->pen.y = (FT_Pos)(-point[1]*64);
}

void TextCanvasTexture::AddText( const char *str, int length, const CanvasTextStyle& style, uint32_t char_limit ) {
    TextMetrics metrics;
    RenderText(str, length, style, metrics, TMF_DRAW, char_limit);
}

void TextCanvasTexture::AddTextMultiline( const char *str, int length, const CanvasTextStyle& style, uint32_t char_limit ) {
    TextMetrics metrics;
    RenderText(str, length, style, metrics, TMF_DRAW | TMF_AUTO_NEWLINE, char_limit);
}

void TextCanvasTexture::GetTextMetricsInfo( const char *str, int length, const CanvasTextStyle& style, TextMetrics &metrics, uint32_t char_limit ) {
    RenderText(str, length, style, metrics, TMF_METRICS, char_limit);
}

static void FTMatrixFromAngle(FT_Matrix *matrix, float degrees) {
    float angle = degrees / 180.0f * 3.1415927f;
    matrix->xx = (FT_Fixed)( cos( angle ) * 0x10000L );
    matrix->xy = (FT_Fixed)(-sin( angle ) * 0x10000L );
    matrix->yx = (FT_Fixed)( sin( angle ) * 0x10000L );
    matrix->yy = (FT_Fixed)( cos( angle ) * 0x10000L );
}

static void ClearTextMetrics(TextMetrics *metrics){
    metrics->advance[0] = 0;
    metrics->advance[1] = 0;
    metrics->bounds[0] = INT_MAX; // min horz
    metrics->bounds[1] = INT_MIN; // max horz
    metrics->bounds[2] = INT_MAX; // min vert
    metrics->bounds[3] = INT_MIN; // max vert
}

static void ApplyGlyphSlotToTextMetrics(TextMetrics *metrics, FT_GlyphSlot slot){
	int glyph_bounds[4];
	glyph_bounds[0] = metrics->advance[0] + slot->metrics.horiBearingX;
	glyph_bounds[1] = glyph_bounds[0] + slot->metrics.width;
	glyph_bounds[2] = metrics->advance[1] + slot->metrics.horiBearingY;
	glyph_bounds[3] = glyph_bounds[2] + slot->metrics.height;
	metrics->advance[0] += slot->advance.x;
	metrics->advance[1] += slot->advance.y;
	metrics->bounds[0] = min(metrics->bounds[0], glyph_bounds[0]);
	metrics->bounds[1] = max(metrics->bounds[1], glyph_bounds[1]);
	metrics->bounds[2] = min(metrics->bounds[2], glyph_bounds[2]);
	metrics->bounds[3] = max(metrics->bounds[3], glyph_bounds[3]);
}

void TextCanvasTexture::RenderText( const char *str, int length, const CanvasTextStyle& style, TextMetrics &metrics, uint32_t mode, uint32_t char_limit ) {
    std::vector<uint32_t> utf32_string;

    try {
        utf8::utf8to32(str,str+length, std::back_inserter(utf32_string));
    } catch( const utf8::not_enough_room& ner ) {
        LOGE << "Got utf8 exception \"" << ner.what() << "\" this might indicate invalid utf-8 data" << std::endl;
    }

    FontRenderer* font_renderer = FontRenderer::Instance();
    FT_Matrix matrix;
    FTMatrixFromAngle(&matrix, impl_->pen_rotation);
	int orig_pen_x = impl_->pen.x;
	int line_height = font_renderer->GetFontInfo(style.font_face_id, FontRenderer::INFO_HEIGHT);
    int line_start = 0;
    FT_Vector zero_vec = {0};
    ClearTextMetrics(&metrics);
	for(unsigned i=0; i<=utf32_string.size(); ++i){
        if(i == utf32_string.size() || utf32_string[i] == '\n'){ // Find line endings to draw entire lines at once
            TextMetrics line_metrics;
            bool need_to_reset = true;
            int next_end_id = 0;
            
            //Create newlines when the lines are too long.
            //This includes the entire string, not just what we want to show based on 
            //char limit
            while( need_to_reset ) {
                ClearTextMetrics(&line_metrics);
                need_to_reset = false;
                for(unsigned j=line_start; j<i; ++j) {
                    font_renderer->SetTransform( style.font_face_id, &matrix, &zero_vec );
                    FT_GlyphSlot slot = font_renderer->RenderCharacterBitmap(style.font_face_id, utf32_string[j], FontRenderer::RCB_METRIC);
                    ApplyGlyphSlotToTextMetrics(&line_metrics, slot);
        
                    if( mode & TMF_AUTO_NEWLINE ) {
                        if( line_metrics.advance[0] + impl_->pen.x + slot->metrics.width >= impl_->text_canvas.width()*64 ) {
                            for( int k = j; k > 0; k-- ) {
                                if( utf32_string[k] == ' ' ) {
                                    j = k;  
                                    break;
                                }
                            }
                            need_to_reset = true;
                            next_end_id = j;
                            break;
                        }
                    }
                }

                if( need_to_reset ) {
                    i = next_end_id;
                }
            }

            switch(style.alignment){
                case CanvasTextStyle::RIGHT:
                    if(mode & TMF_DRAW){
                        impl_->pen.x = impl_->text_canvas.width()*64 - line_metrics.bounds[1];
                    }
                    metrics.advance[0] = -line_metrics.bounds[1];
                    break;
                default:
                    LOGD << "Unhandled style.alignment " << CanvasTextStyle::GetAlignmentString(style.alignment) << std::endl;;
                    break;
            }

            for(unsigned j=line_start; j<i; ++j){
                if(mode & TMF_DRAW){
                    font_renderer->SetTransform( style.font_face_id, &matrix, &impl_->pen );
                } else {
                    font_renderer->SetTransform( style.font_face_id, &matrix, &zero_vec );
                }
                FT_GlyphSlot slot;
                if(mode & TMF_DRAW){
                    slot = font_renderer->RenderCharacterBitmap(style.font_face_id, utf32_string[j], FontRenderer::RCB_RENDER);
                    const FT_Bitmap &bitmap = slot->bitmap;
                    if( j < char_limit )
                    {
                        impl_->text_canvas.AddBitmap(slot->bitmap_left, -slot->bitmap_top, bitmap);
                        impl_->pen.x += slot->advance.x;
                        impl_->pen.y += slot->advance.y;
                    }
                } else {
                    slot = font_renderer->RenderCharacterBitmap(style.font_face_id, utf32_string[j], FontRenderer::RCB_METRIC);
                }
                ApplyGlyphSlotToTextMetrics(&metrics, slot);
            }
            if(i != utf32_string.size()){
                impl_->pen.y -= line_height;
                impl_->pen.x = orig_pen_x;
                metrics.advance[1] += line_height;
                metrics.advance[0] = 0;
            }
            line_start = i+1;
        }
    }
}

// Get the coordinates of the cursor after a given letter in an edit field
void TextCanvasTexture::GetLetterPosXY(const char* str, const CanvasTextStyle &style, int letter, int coords[2]){
    std::vector<uint32_t> utf32_string;
    try {
        utf8::utf8to32(str,str+strlen(str), std::back_inserter(utf32_string));
    } catch( const utf8::not_enough_room& ner ) {
        LOGE << "Got utf8 exception \"" << ner.what() << "\" this might indicate invalid utf-8 data" << std::endl;
    }

    FontRenderer* font_renderer = FontRenderer::Instance();
    FT_Matrix matrix;
    FTMatrixFromAngle(&matrix, 0.0f);
    int line_height = font_renderer->GetFontInfo(style.font_face_id, FontRenderer::INFO_HEIGHT);
    int line_start = 0;
    FT_Vector zero_vec = {0};
    TextMetrics metrics;
    ClearTextMetrics(&metrics);
    for(int i=0; i<=(int)utf32_string.size(); ++i){
        if(str[i] == '\n' || i == (int)utf32_string.size()){ // Find line endings to draw entire lines at once
            TextMetrics line_metrics;
            ClearTextMetrics(&line_metrics);
            for(int j=line_start; j<i; ++j){
                font_renderer->SetTransform( style.font_face_id, &matrix, &zero_vec );
                FT_GlyphSlot slot = font_renderer->RenderCharacterBitmap(style.font_face_id, str[j], FontRenderer::RCB_METRIC);
                ApplyGlyphSlotToTextMetrics(&line_metrics, slot);
            }
            switch(style.alignment){
                case CanvasTextStyle::RIGHT:
                    metrics.advance[0] = -line_metrics.bounds[1];
                    break;
                default:
                    LOGD << "Unhandled style.alignment " << CanvasTextStyle::GetAlignmentString(style.alignment) << std::endl;;
                    break;
            }
            for(int j=line_start; j<i; ++j){
                font_renderer->SetTransform( style.font_face_id, &matrix, &zero_vec );
                FT_GlyphSlot slot = font_renderer->RenderCharacterBitmap(style.font_face_id, str[j], FontRenderer::RCB_METRIC);
                if(j == letter){
                    coords[0] = metrics.advance[0]/64;
                    coords[1] = metrics.advance[1]/64;
                }
                ApplyGlyphSlotToTextMetrics(&metrics, slot);
                if(j+1 == letter){
                    coords[0] = metrics.advance[0]/64;
                    coords[1] = metrics.advance[1]/64;
                }
            }
            if(str[i] == '\n'){
                metrics.advance[1] += line_height;
                metrics.advance[0] = 0;
            }
            line_start = i+1;
        }
    }
    switch(style.alignment){
        case CanvasTextStyle::RIGHT:
            coords[0] -= metrics.bounds[0]/64;
            break;
        default:
            LOGD << "Unhandled style.alignment " << CanvasTextStyle::GetAlignmentString(style.alignment) << std::endl;;
            break;
            
    }
}

// Get the cursor index given a mouse click position (text canvas space)
int TextCanvasTexture::GetCursorPos(const char* str, const CanvasTextStyle& style, const int coords[2], uint32_t char_limit) {
    int best_letter = 0;
    int best_distance = INT_MAX;
    int length = strlen(str);
    TextMetrics total_metrics;
    RenderText(str, length, style, total_metrics, TMF_METRICS, char_limit);  
    FontRenderer* font_renderer = FontRenderer::Instance();
    FT_Matrix matrix;
    FTMatrixFromAngle(&matrix, 0.0f);
    int line_height = font_renderer->GetFontInfo(style.font_face_id, FontRenderer::INFO_HEIGHT);
    int click_line = coords[1] / (line_height/64);
    int curr_line = 0;
    int line_start = 0;
    FT_Vector zero_vec = {0};
    TextMetrics metrics;
    ClearTextMetrics(&metrics);
    for(int i=0; i<=length; ++i){
        if(str[i] == '\n' || i == length){ // Find line endings to draw entire lines at once
            TextMetrics line_metrics;
            ClearTextMetrics(&line_metrics);
            for(int j=line_start; j<i; ++j){
                font_renderer->SetTransform( style.font_face_id, &matrix, &zero_vec );
                FT_GlyphSlot slot = font_renderer->RenderCharacterBitmap(style.font_face_id, str[j], FontRenderer::RCB_METRIC);
                ApplyGlyphSlotToTextMetrics(&line_metrics, slot);
            }
            switch(style.alignment){
            case CanvasTextStyle::RIGHT:
                metrics.advance[0] = -line_metrics.bounds[1];
                break;
            default:
                LOGD << "Unhandled style.alignment " << CanvasTextStyle::GetAlignmentString(style.alignment) << std::endl;;
                break;
            }
            for(int j=line_start; j<i; ++j){
                font_renderer->SetTransform( style.font_face_id, &matrix, &zero_vec );
                FT_GlyphSlot slot = font_renderer->RenderCharacterBitmap(style.font_face_id, str[j], FontRenderer::RCB_METRIC);
                if(curr_line == click_line){
                    int pos_x = metrics.advance[0]/64;
                    switch(style.alignment){
                    case CanvasTextStyle::RIGHT:
                        pos_x -= total_metrics.bounds[0]/64;
                        break;
                    default:
                        LOGD << "Unhandled style.alignment " << CanvasTextStyle::GetAlignmentString(style.alignment) << std::endl;;
                        break;
                    }
                    int distance = abs(pos_x - coords[0]);
                    if(distance < best_distance){
                        best_letter = j;
                        best_distance = distance;
                    }
                }
                ApplyGlyphSlotToTextMetrics(&metrics, slot);
                if(curr_line == click_line){
                    int pos_x = metrics.advance[0]/64;
                    switch(style.alignment){
                    case CanvasTextStyle::RIGHT:
                        pos_x -= total_metrics.bounds[0]/64;
                        break;
                    default:
                        LOGD << "Unhandled style.alignment " << CanvasTextStyle::GetAlignmentString(style.alignment) << std::endl;;
                        break;
                    }
                    int distance = abs(pos_x - coords[0]);
                    if(distance < best_distance){
                        best_letter = j+1;
                        best_distance = distance;
                    }
                }
            }
            if(str[i] == '\n'){
                metrics.advance[1] += line_height;
                metrics.advance[0] = 0;
                ++curr_line;
            }
            line_start = i+1;
        }
    }
    if(click_line > curr_line){
        best_letter = length;
    }
    return best_letter;
} 

void TextCanvasTexture::DebugDrawBillboard(const vec3 &pos, float scale, int lifespan_int) {
    DDLifespan lifespan = LifespanFromInt(lifespan_int);

    RemoveDebugDrawBillboard();
    debug_draw_billboard_id = DebugDraw::Instance()->AddBillboard(impl_->texture_ref, pos, scale, vec4(1.0f), kPremultiplied, lifespan);
}

void TextCanvasTexture::RemoveDebugDrawBillboard()
{
    if( debug_draw_billboard_id != -1 )
    {
        DebugDraw::Instance()->Remove(debug_draw_billboard_id);
        debug_draw_billboard_id = -1;
    }
}

TextCanvasTexture::TextCanvasTexture() : debug_draw_billboard_id(-1){
    impl_ = new TextCanvasTextureImpl();
}

TextCanvasTexture::~TextCanvasTexture() {
    RemoveDebugDrawBillboard();
    delete impl_;
}

TextCanvasTexture::TextCanvasTexture(const TextCanvasTexture& other) {
    *impl_ = *other.impl_;
}

void TextCanvasTexture::Reset() {
    delete impl_;
    impl_ = new TextCanvasTextureImpl();
}

void TextCanvasTexture::SetPenColor( int r, int g, int b, int a ) {
    impl_->text_canvas.SetPenColor(r,g,b,a);
}

void TextCanvasTexture::SetPenRotation( float degrees ) {
    impl_->pen_rotation = degrees;
}

TextureRef TextCanvasTexture::GetTexture() const {
    return impl_->texture_ref;
}

int ASGetFontFaceID(const std::string& path, int pixel_height) {
    return FontRenderer::Instance()->GetFontFaceID(path, pixel_height);
}

static void TextStyleDefaultConstructor(CanvasTextStyle *self) {
    new(self) CanvasTextStyle();
}

static void TextStyledestructor(void *memory) {
    ((CanvasTextStyle*)memory)->~CanvasTextStyle();
}

static void TextStyleAlignment(int alignment, CanvasTextStyle* self) {
    self->alignment = (CanvasTextStyle::Alignment)alignment; 
}

static void ASAddText(TextCanvasTexture *tct, const std::string& str, const CanvasTextStyle &style, uint32_t char_count_output) {
    tct->AddText(str.c_str(), str.length(), style, char_count_output);
}
static void ASAddTextMultiline(TextCanvasTexture *tct, const std::string& str, const CanvasTextStyle &style, uint32_t char_count_output) {
    tct->AddTextMultiline(str.c_str(), str.length(), style, char_count_output);
}

static void ASGetTextMetricsInfo(TextCanvasTexture *tct, const std::string& str, const CanvasTextStyle &style, TextMetrics &metrics, uint32_t char_count_output) {
    tct->GetTextMetricsInfo(str.c_str(), str.length(), style, metrics, char_count_output);
}

void ASTextContext::ASDrawTextAtlas(const std::string &path, int pixel_height, int flags, const std::string &txt, int x, int y, vec4 color) {
    ASDrawTextAtlas2(path,pixel_height,flags,txt,x,y,color,UINT32MAX);
}

void ASTextContext::ASDrawTextAtlas2(const std::string &path, int pixel_height, int flags, const std::string &txt, int x, int y, vec4 color, uint32_t char_limit) {
    if(!text_atlas_renderer_setup){
        text_atlas_renderer.Init();
        text_atlas_renderer_setup = true;
    }
    if(path.length() >= CachedTextAtlas::kPathSize){
        DisplayError("Error", "Font path is too long");        
    } else {
        int found = -1;
        for(int i=0; i<atlases.num_atlases; ++i){
            const CachedTextAtlas& cached = atlases.cached[i];
            if(strcmp(cached.path, path.c_str()) == 0 && 
                pixel_height == cached.pixel_height &&
                flags == cached.flags) 
            {
                found = i;
            }
        }
        if(found == -1){
            if(atlases.num_atlases >= CachedTextAtlases::kMaxAtlases){
                DisplayError("Error", "Too many cached text atlases");
            } else {
                found = atlases.num_atlases++;
                CachedTextAtlas* new_atlas = &atlases.cached[found];
                strncpy(new_atlas->path, path.c_str(), CachedTextAtlas::kPathSize);
                new_atlas->pixel_height = pixel_height;
                new_atlas->flags = flags;
                new_atlas->atlas.Create(path.c_str(), pixel_height, 
                    font_renderer, flags);
            }
        }
        if(found != -1){
            TextAtlas* atlas = &atlases.cached[found].atlas;
            int pos[] = {x, y};
            text_atlas_renderer.num_characters = 0;
            text_atlas_renderer.AddText(atlas, txt.c_str(), pos, font_renderer, char_limit);
            text_atlas_renderer.Draw(atlas, graphics, 0, color);
        }
    }
}
    
TextMetrics ASTextContext::ASGetTextAtlasMetrics(const std::string &path, int pixel_height, int flags, const std::string &txt ) {
    return ASGetTextAtlasMetrics2(path,pixel_height,flags,txt,UINT32MAX);
}

TextMetrics ASTextContext::ASGetTextAtlasMetrics2(const std::string &path, int pixel_height, int flags, const std::string &txt, uint32_t char_limit ) {
    TextMetrics metrics;
        
    if(!text_atlas_renderer_setup){
        text_atlas_renderer.Init();
        text_atlas_renderer_setup = true;
    }
    if(path.length() >= CachedTextAtlas::kPathSize){
        DisplayError("Error", "Font path is too long");
    } else {
        int found = -1;
        for(int i=0; i<atlases.num_atlases; ++i){
            CachedTextAtlas& cached = atlases.cached[i];
            if(strcmp(cached.path, path.c_str()) == 0 &&
                pixel_height == cached.pixel_height &&
                flags == cached.flags)
            {
                found = i;
            }
        }
        if(found == -1){
            if(atlases.num_atlases >= CachedTextAtlases::kMaxAtlases){
                DisplayError("Error", "Too many cached text atlases");
            } else {
                found = atlases.num_atlases++;
                CachedTextAtlas* new_atlas = &atlases.cached[found];
                strncpy(new_atlas->path, path.c_str(), CachedTextAtlas::kPathSize);
                new_atlas->pixel_height = pixel_height;
                new_atlas->flags = flags;
                new_atlas->atlas.Create(path.c_str(), pixel_height,
                                        font_renderer, flags);
            }
        }
        if(found != -1){
            TextAtlas* atlas = &atlases.cached[found].atlas;

            text_atlas_renderer.num_characters = 0;
                
            metrics = text_atlas_renderer.GetMetrics(atlas, txt.c_str(), font_renderer, char_limit);
        }
    }
        
    return metrics;
        
}

// TODO: this should not be a global variable right here
ASTextContext g_as_text_context;

void DisposeTextAtlases() {
    
    for(int i=0; i<g_as_text_context.atlases.num_atlases; ++i){
        g_as_text_context.atlases.cached[i].atlas.Dispose();
    }
    
    // Reset the cached atlas counter
    g_as_text_context.atlases.num_atlases = 0;
    
}

void AttachTextCanvasTextureToASContext( ASContext* ctx ) {
    g_as_text_context.font_renderer = FontRenderer::Instance();
    g_as_text_context.graphics = Graphics::Instance();
    ctx->RegisterObjectType("TextStyle", sizeof(CanvasTextStyle), asOBJ_VALUE | asOBJ_APP_CLASS_CD);
    ctx->RegisterObjectProperty("TextStyle", "int font_face_id", offsetof(CanvasTextStyle, font_face_id));
    ctx->RegisterObjectBehaviour("TextStyle", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(TextStyleDefaultConstructor),  asCALL_CDECL_OBJLAST); 
    ctx->RegisterObjectBehaviour("TextStyle", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(TextStyledestructor), asCALL_CDECL_OBJLAST);
    ctx->RegisterObjectMethod("TextStyle", "void SetAlignment(int i)", asFUNCTION(TextStyleAlignment), asCALL_CDECL_OBJLAST);
    ctx->DocsCloseBrace();
    ctx->RegisterObjectType("TextMetrics", sizeof(TextMetrics), asOBJ_VALUE | asOBJ_POD | asOBJ_APP_CLASS_CA);
    ctx->RegisterObjectProperty("TextMetrics", "int advance_x", asOFFSET(TextMetrics, advance));
    ctx->RegisterObjectProperty("TextMetrics", "int advance_y", asOFFSET(TextMetrics, advance)+sizeof(int));
    ctx->RegisterObjectProperty("TextMetrics", "int bounds_x", asOFFSET(TextMetrics, bounds)+(2*sizeof(int)));
    ctx->RegisterObjectProperty("TextMetrics", "int bounds_y", asOFFSET(TextMetrics, bounds)+(3*sizeof(int)));
    ctx->RegisterObjectProperty("TextMetrics", "float ascenderRatio", asOFFSET(TextMetrics, ascenderRatio));
    
    ctx->DocsCloseBrace();
    ctx->RegisterObjectType("TextCanvasTexture", 0, asOBJ_REF | asOBJ_NOCOUNT);
    ctx->RegisterObjectMethod("TextCanvasTexture", "void Create(int width, int height)", asMETHOD(TextCanvasTexture, Create), asCALL_THISCALL);
    ctx->RegisterObjectMethod("TextCanvasTexture", "void ClearTextCanvas()", asMETHOD(TextCanvasTexture,  ClearTextCanvas), asCALL_THISCALL);
    ctx->RegisterObjectMethod("TextCanvasTexture", "void UploadTextCanvasToTexture()", asMETHOD(TextCanvasTexture, UploadTextCanvasToTexture), asCALL_THISCALL);
   
    ctx->RegisterObjectMethod("TextCanvasTexture", "void DebugDrawBillboard(const vec3 &in pos, float scale, int lifespan)", asMETHOD(TextCanvasTexture, DebugDrawBillboard), asCALL_THISCALL);
    ctx->RegisterObjectMethod("TextCanvasTexture", "void AddText(const string &in, const TextStyle &in, uint char_limit)", asFUNCTION(ASAddText), asCALL_CDECL_OBJFIRST);
    ctx->RegisterObjectMethod("TextCanvasTexture", "void AddTextMultiline(const string &in, const TextStyle &in, uint char_limit)", asFUNCTION(ASAddTextMultiline), asCALL_CDECL_OBJFIRST);
    ctx->RegisterObjectMethod("TextCanvasTexture", "void GetTextMetrics(const string &in, const TextStyle &in, TextMetrics &out, uint char_limit)", asFUNCTION(ASGetTextMetricsInfo), asCALL_CDECL_OBJFIRST);
    ctx->RegisterObjectMethod("TextCanvasTexture", "void SetPenPosition(const vec2 &in)", asMETHOD(TextCanvasTexture, SetPenPosition), asCALL_THISCALL);
    ctx->RegisterObjectMethod("TextCanvasTexture", "void SetPenColor(int r, int g, int b, int a)", asMETHOD(TextCanvasTexture, SetPenColor), asCALL_THISCALL);
    ctx->RegisterObjectMethod("TextCanvasTexture", "void SetPenRotation(float rotation)", asMETHOD(TextCanvasTexture, SetPenRotation), asCALL_THISCALL);
    ctx->DocsCloseBrace();
    ctx->RegisterEnum("TextAtlasFlags");
    ctx->RegisterEnumValue("TextAtlasFlags", "kSmallLowercase", TextAtlas::kSmallLowercase);
    ctx->DocsCloseBrace();
    ctx->RegisterGlobalFunction("int GetFontFaceID(const string &in path, int pixel_height)", asFUNCTION(ASGetFontFaceID), asCALL_CDECL);
    ctx->RegisterGlobalFunction("void DisposeTextAtlases()", asFUNCTION(DisposeTextAtlases), asCALL_CDECL);

    ctx->RegisterGlobalFunctionThis("void DrawTextAtlas(const string &in path, int pixel_height, int flags, const string &in txt, int x, int y, vec4 color)",
                                    asMETHOD(ASTextContext, ASDrawTextAtlas), asCALL_THISCALL_ASGLOBAL, &g_as_text_context);

    ctx->RegisterGlobalFunctionThis("void DrawTextAtlas2(const string &in path, int pixel_height, int flags, const string &in txt, int x, int y, vec4 color, uint char_limit)",
                                    asMETHOD(ASTextContext, ASDrawTextAtlas2), asCALL_THISCALL_ASGLOBAL, &g_as_text_context);

    ctx->RegisterGlobalFunctionThis("TextMetrics GetTextAtlasMetrics(const string &in path, int pixel_height, int flags, const string &in txt )",
                                    asMETHOD(ASTextContext, ASGetTextAtlasMetrics), asCALL_THISCALL_ASGLOBAL, &g_as_text_context);
    
    ctx->RegisterGlobalFunctionThis("TextMetrics GetTextAtlasMetrics2(const string &in path, int pixel_height, int flags, const string &in txt, uint char_limit )",
                                    asMETHOD(ASTextContext, ASGetTextAtlasMetrics2), asCALL_THISCALL_ASGLOBAL, &g_as_text_context);
    
}

bool TextAtlas::Pack(unsigned char* pixels, int atlas_dims[2], 
                     int font_face_id, int lowercase_font_face_id,
                     FontRenderer* font_renderer, std::vector<CharacterInfo>& alphabet) 
{
    PROFILER_ZONE(g_profiler_ctx, "TextAtlas::Pack");

    for(int i=0, len=atlas_dims[0] * atlas_dims[1]; i<len; ++i){
        pixels[i] = 0;
    }
    int draw[] = {0,0};
    int max_height = 0;

    std::vector<CharacterInfo>::iterator character = alphabet.begin();

    for(; character != alphabet.end(); character++){
        bool lowercase = (character->lowercase && lowercase_font_face_id != -1);
        FT_GlyphSlot slot = 
            font_renderer->RenderCharacterBitmap(
                lowercase ? lowercase_font_face_id : font_face_id,
                character->codepoint, FontRenderer::RCB_RENDER);
        FT_Bitmap* bitmap = &slot->bitmap;
        if(character->width + draw[0] > atlas_dims[0]){
            draw[0] = 0;
            draw[1] += max_height;
            max_height = 0;
        }
        character->pos[0] = draw[0];
        character->pos[1] = draw[1];
        max_height = max(max_height, character->height);
        for(int x=0; x < character->width; ++x){
            for(int y=0; y < character->height; ++y){
                if(draw[0] + x < atlas_dims[0] && draw[1] + y < atlas_dims[1]) {
                    pixels[(draw[1]+y) * atlas_dims[0] + draw[0]+x] = bitmap->buffer[y*character->width+x];
                } else {
                    return false;
                }
            }
        }
        draw[0] += character->width;
    }
    return true;
}

void TextAtlas::Create(const char* path, int _pixel_height, FontRenderer* font_renderer, int flags) {    
    PROFILER_ZONE(g_profiler_ctx, "TextAtlas::Create");

    int font_face_id = font_renderer->GetFontFaceID(path, _pixel_height);

    // For fonts that only have uppercase letters, we might want to use 'lowercase' letters that are just
    // the uppercase ones at a smaller size
    int lowercase_font_face_id = -1;    
    if(flags & kSmallLowercase){
        lowercase_font_face_id = font_renderer->GetFontFaceID(path, _pixel_height * 2/3);
    }

    // Get metrics for each letter
    std::vector<CharacterInfo>::iterator character = alphabet.begin();
    for(; character != alphabet.end(); character++){
        bool lowercase = (character->lowercase && lowercase_font_face_id != -1);
        FT_GlyphSlot slot = font_renderer->RenderCharacterBitmap(
            lowercase ? lowercase_font_face_id : font_face_id,
            character->codepoint, FontRenderer::RCB_METRIC);
        //FT_Bitmap* bitmap = &slot->bitmap;
        character->width = slot->metrics.width / 64;
        character->height = slot->metrics.height / 64;
        character->advance_x = slot->metrics.horiAdvance / 64;
        character->bearing[0] = slot->metrics.horiBearingX / 64;
        character->bearing[1] = slot->metrics.horiBearingY / 64;
    }
    
    // Pack all characters into the smallest possible atlas
    atlas_dims[0] = 32;
    atlas_dims[1] = 32;
    unsigned char* pixels = (unsigned char*)OG_MALLOC(atlas_dims[0] * atlas_dims[1]);
    while(!Pack(pixels, atlas_dims, font_face_id, lowercase_font_face_id, font_renderer, alphabet)){
        OG_FREE(pixels);
        atlas_dims[0] *= 2;
        atlas_dims[1] *= 2;
        pixels = (unsigned char*)OG_MALLOC(atlas_dims[0] * atlas_dims[1]);
    }

    // Prepare atlas to send to GPU as a texture
    unsigned char* atlas_export;
    atlas_export = (unsigned char*)OG_MALLOC(atlas_dims[0] * atlas_dims[1] * 4);
    for(int i=0, len=atlas_dims[0] * atlas_dims[1] * 4; i<len; i+=4){
        for(int j=0; j<4; ++j){
            atlas_export[i+j] = 255;
        }
        float pixel = pixels[i/4] / 255.0f;
        pixel = (float)pow(pixel, 1.0/2.2); // Convert from linear to sRGB
        atlas_export[i+3] = (unsigned char)(pixel*255.0f);
    }

    // TODO: do this using the Textures class instead of bare OpenGL calls
    GLuint gl_tex;
    glGenTextures(1, &gl_tex);
    tex = gl_tex;
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 
        atlas_dims[0], atlas_dims[1], 0, GL_RGBA, 
        GL_UNSIGNED_BYTE, atlas_export);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Export the atlas for debug purposes?
    static const bool kSaveAtlas = false;
    if(kSaveAtlas){
        ImageExport::SavePNG("text_atlas_test.png", atlas_export, atlas_dims[0], atlas_dims[1], 0);
    }

    OG_FREE(atlas_export);
    OG_FREE(pixels);
    
    pixel_height = _pixel_height;
}

void TextAtlas::Dispose() {
    if(tex != -1){
        const GLuint gl_tex = tex;
        glDeleteTextures(1, &gl_tex);
        tex = -1;
    }
}

TextAtlas::TextAtlas():
    tex(-1)
{
    static const unsigned int kAsciiFirst = 0x20;
    static const unsigned int kAsciiLast = 0x7E;
    static const unsigned int kLatinBase2First = 0xA1;
    static const unsigned int kLatinBase2Last = 0xFF;

    for( uint32_t i = kAsciiFirst; i <= kAsciiLast; i++ ) {
        CharacterInfo character;
        character.codepoint = i;

        //Unicode and ascii lower case range
        if( i >= 0x61 && i <= 0x7A ) {
            character.lowercase = true;
        } else {
            character.lowercase = false;
        }
        alphabet.push_back( character );
    }

    for( uint32_t i = kLatinBase2First; i <= kLatinBase2Last; i++ ) {
        CharacterInfo character;
        character.codepoint = i;
        //Lower case unicode latin base ranges
        if( (i >= 0xDF && i <= 0xF6) || ( i > 0xF8 && i <= 0xFF ) ) {
            character.lowercase = true;
        } else {
            character.lowercase = false;
        }
        alphabet.push_back( character );
    }
}

TextAtlas::~TextAtlas() {
    LOG_ASSERT(tex == -1); // We should already have disposed of the text atlas GL resources
}

void TextAtlasRenderer::Init() {
    CHECK_GL_ERROR();
    num_characters = 0;

    CHECK_GL_ERROR();
    Shaders* shaders = Shaders::Instance();
    shader_id = shaders->returnProgram("simple_2d #TEXTURE");
    shaders->createProgram(shader_id);
    shader_attrib_vert_coord = shaders->returnShaderAttrib("vert_coord", shader_id);
    shader_attrib_tex_coord = shaders->returnShaderAttrib("tex_coord", shader_id);
    uniform_mvp_mat = shaders->returnShaderVariable("mvp_mat", shader_id);
    uniform_color = shaders->returnShaderVariable("color", shader_id);
}

void TextAtlasRenderer::Dispose() {
    index_vbo.Dispose();
    vert_vbo.Dispose();
}

void TextAtlasRenderer::AddText(TextAtlas* atlas, const char* text, int pos[2], FontRenderer* font_renderer, uint32_t char_output_limit ) {
    PROFILER_ZONE(g_profiler_ctx, "TextAtlas::AddText");
    //int line_height = font_renderer->GetFontInfo(0, FontRenderer::INFO_HEIGHT) / 64; // This is the unadjusted size -- MjB
    
    std::vector<uint32_t> utf32_string;

    try {
        utf8::utf8to32(text,text+strlen(text), std::back_inserter(utf32_string));
    } catch( const utf8::not_enough_room& ner ) {
        LOGE << "Got utf8 exception \"" << ner.what() << "\" this might indicate invalid utf-8 data, \"" << text << "\"" << std::endl;
    }

    int draw_pos[2];
    memcpy(draw_pos, pos, sizeof(draw_pos));
    int last_c = -1;
    for(size_t i=0, len=utf32_string.size(); i<len && i<char_output_limit; ++i){
        if(num_characters < kMaxCharacters){
            uint32_t c = utf32_string[i];
            if(c == '\n'){
                draw_pos[1] += atlas->pixel_height;
                draw_pos[0] = pos[0];
            }

            std::vector<CharacterInfo>::iterator character;
            for( character = atlas->alphabet.begin();
                 character != atlas->alphabet.end();
                 character++ ) {
                if( (int64_t)character->codepoint == c )
                    break;
            }

            if(character != atlas->alphabet.end()){
                int kerning_x = 0;
                if(last_c != -1){
                    FT_Vector vec;
                    font_renderer->GetKerning(0, last_c, c, &vec);
                    kerning_x = vec.x / 64;    
                }
                draw_pos[0] += kerning_x;

                int vert_index = num_characters * 4 * TextAtlasRenderer::kFloatsPerVert;
                verts[vert_index++] = (float)draw_pos[0] + character->bearing[0];
                verts[vert_index++] = (float)draw_pos[1] - character->bearing[1];
                verts[vert_index++] = (character->pos[0]) / (float)atlas->atlas_dims[0];
                verts[vert_index++] = (character->pos[1]) / (float)atlas->atlas_dims[1];

                verts[vert_index++] = (float)draw_pos[0] + character->width + character->bearing[0];
                verts[vert_index++] = (float)draw_pos[1] - character->bearing[1];
                verts[vert_index++] = (character->pos[0] + character->width) / (float)atlas->atlas_dims[0];
                verts[vert_index++] = (character->pos[1]) / (float)atlas->atlas_dims[1];

                verts[vert_index++] = (float)draw_pos[0] + character->width + character->bearing[0];
                verts[vert_index++] = (float)draw_pos[1] + character->height - character->bearing[1];
                verts[vert_index++] = (character->pos[0] + character->width) / (float)atlas->atlas_dims[0];
                verts[vert_index++] = (character->pos[1] + character->height) / (float)atlas->atlas_dims[1];

                verts[vert_index++] = (float)draw_pos[0] + character->bearing[0];
                verts[vert_index++] = (float)draw_pos[1] + character->height - character->bearing[1];
                verts[vert_index++] = (character->pos[0]) / (float)atlas->atlas_dims[0];
                verts[vert_index++] = (character->pos[1] + character->height) / (float)atlas->atlas_dims[1];

                int indices_index = num_characters * 6;
                int vert_start = num_characters * 4;
                indices[indices_index++] = vert_start + 0; 
                indices[indices_index++] = vert_start + 1;
                indices[indices_index++] = vert_start + 2;
                indices[indices_index++] = vert_start + 0;
                indices[indices_index++] = vert_start + 2;
                indices[indices_index++] = vert_start + 3;

                ++num_characters;
                draw_pos[0] += character->advance_x;
                last_c = c;
            }
        }
    }
}

TextMetrics TextAtlasRenderer::GetMetrics( TextAtlas* atlas, const char* text, FontRenderer* font_renderer, uint32_t char_limit ) {
    PROFILER_ZONE(g_profiler_ctx, "TextAtlas::GetMetrics");
    
    TextMetrics metrics;
    metrics.advance[0] = 0;
    metrics.advance[1] = 0;
    
    metrics.bounds[0] = 0;
    metrics.bounds[1] = 0;
    metrics.bounds[2] = 0;
    metrics.bounds[3] = atlas->pixel_height;
    
    int last_c = -1;
    for(int i=0, len=strlen(text); i<len; ++i){
        
        char c = text[i];
        if(c == '\n') {
            
            metrics.bounds[2] = max( metrics.bounds[2], metrics.advance[0] );
            metrics.bounds[3] += atlas->pixel_height;
            
            metrics.advance[0] = 0;
            metrics.advance[1] += atlas->pixel_height;
            
        }

        std::vector<CharacterInfo>::iterator character;
        for( character = atlas->alphabet.begin();
             character != atlas->alphabet.end();
             character++ ) {
            if( (int64_t)character->codepoint == c )
                break;
        }
        
        if(character != atlas->alphabet.end()){
        
            int kerning_x = 0;
            if(last_c != -1) {
                FT_Vector vec;
                font_renderer->GetKerning(0, last_c, c, &vec);
                kerning_x = vec.x / 64;
            }
            
            metrics.advance[0] += kerning_x;
            
            metrics.advance[0] += character->advance_x;

            last_c = c;
        }
    }
    
    metrics.bounds[2] = max( metrics.bounds[2], metrics.advance[0] );
    
    int line_height = font_renderer->GetFontInfo(0, FontRenderer::INFO_HEIGHT) / 64;
    int ascender_height = font_renderer->GetFontInfo(0, FontRenderer::INFO_ASCENDER) / 64;
    
    metrics.ascenderRatio = ((float)abs(ascender_height)/(float)abs(line_height));
    
    return metrics;
    
}

void TextAtlasRenderer::Draw(TextAtlas* atlas, Graphics* graphics, char flags, const vec4& color) {
	if(num_characters == 0){
		return;
	}
    Shaders* shaders = Shaders::Instance();

    PROFILER_GPU_ZONE(g_profiler_ctx, "TextAtlasRenderer::Draw");
    CHECK_GL_ERROR();

    vert_vbo.Fill(sizeof(verts)*num_characters/kMaxCharacters, verts);
    index_vbo.Fill(sizeof(indices)*num_characters/kMaxCharacters, indices);

    CHECK_GL_ERROR();
    GLState gl_state;
    gl_state.blend = true;
    gl_state.cull_face = false;
    gl_state.depth_write = false;
    gl_state.depth_test = false;
    CHECK_GL_ERROR();

    graphics->setGLState(gl_state);
    CHECK_GL_ERROR();

    shaders->setProgram(shader_id);
    CHECK_GL_ERROR();
    glm::mat4 proj_mat;
    proj_mat = glm::ortho(0.0f, (float)graphics->window_dims[0], (float)graphics->window_dims[1], 0.0f);

    CHECK_GL_ERROR();
    graphics->EnableVertexAttribArray(shader_attrib_vert_coord);
    graphics->EnableVertexAttribArray(shader_attrib_tex_coord);
    vert_vbo.Bind();
    index_vbo.Bind();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, atlas->tex);
    glVertexAttribPointer(shader_attrib_vert_coord, 2, GL_FLOAT, false, 4*sizeof(GLfloat), (const void*)vert_vbo.offset());
    glVertexAttribPointer(shader_attrib_tex_coord, 2, GL_FLOAT, false, 4*sizeof(GLfloat), (const void*)(vert_vbo.offset()+sizeof(GL_FLOAT)*2));
    CHECK_GL_ERROR();
    
    int num_indices = sizeof(indices)*num_characters/kMaxCharacters/sizeof(float);

    if(flags & kTextShadow){
        glm::mat4 offset_mat;
        offset_mat[3][0] += 1.0f;
        offset_mat[3][1] += 1.0f;
        glm::mat4 mat = proj_mat * offset_mat;
        glUniformMatrix4fv(uniform_mvp_mat, 1, false, (GLfloat*)&mat);
        glUniform4f(uniform_color, 0.0f, 0.0f, 0.0f, color.a() / 2.0f );
        graphics->DrawElements(GL_TRIANGLES, num_indices, GL_UNSIGNED_INT, (const void*)index_vbo.offset());
    }

    glUniformMatrix4fv(uniform_mvp_mat, 1, false, (GLfloat*)&proj_mat);
    glUniform4fv(uniform_color, 1, &color[0]);
    graphics->DrawElements(GL_TRIANGLES, num_indices, GL_UNSIGNED_INT, (const void*)index_vbo.offset());
    
    graphics->BindElementVBO(0);
    graphics->BindArrayVBO(0);
    graphics->ResetVertexAttribArrays();
    CHECK_GL_ERROR();
}

TextAtlasRenderer::TextAtlasRenderer():
    vert_vbo(  65536, kVBOFloat | kVBOStream ),
    index_vbo(  65536/4 , kVBOElement | kVBOStream )
{

}
