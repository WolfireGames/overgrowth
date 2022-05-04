//-----------------------------------------------------------------------------
//           Name: textures.cpp
//      Developer: Wolfire Games LLC
//         Author: David Rosen
//    Description: The textures class loads, manages, and binds textures
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
#include "textures.h"

#include <Memory/allocation.h>

#include <Utility/gl_util.h>

#include <Compat/processpool.h>

#include <Graphics/textures.h>
#include <Graphics/shaders.h>
#include <Graphics/graphics.h>
#include <Graphics/graphics.h>
#include <Graphics/textureref.h>
#include <Graphics/ColorWheel.h>
#include <Graphics/converttexture.h>
#include <Graphics/vbocontainer.h>

#include <Internal/datemodified.h>
#include <Internal/stopwatch.h>
#include <Internal/timer.h>
#include <Internal/integer.h>
#include <Internal/common.h>
#include <Internal/cachefile.h>
#include <Internal/profiler.h>
#include <Internal/filesystem.h>
#include <Internal/config.h>

#include <Images/texture_data.h>
#include <Images/freeimage_wrapper.h>
#include <Images/image_export.hpp>

#include <Timing/timingevent.h>
#include <Timing/intel_gl_perf.h>

#include <Asset/Asset/texturedummy.h>
#include <Asset/assetmanager.h>

#include <Compat/fileio.h>
#include <Wrappers/glm.h>
#include <Logging/logdata.h>
#include <Memory/allocation.h>
#include <Utility/assert.h>
#include <Main/engine.h>

#include <imgui.h>

using namespace PhoenixTextures;

vec4 GetAverageColor( const char* abs_path ) {
    TextureData texture_data;
    texture_data.Load(abs_path);

    unsigned total_pixels = texture_data.GetWidth() *
        texture_data.GetHeight();
    unsigned total_bytes =  total_pixels * 32 / 8;

    float total_color[4] = {0};

    std::vector<unsigned char> img_data;
    img_data.resize(total_bytes);
    texture_data.GetUncompressedData(&img_data[0]);
    for(unsigned j=0; j<total_bytes; j+=4){
        total_color[0] += img_data[j+0];
        total_color[1] += img_data[j+1];
        total_color[2] += img_data[j+2];
        total_color[3] += img_data[j+3];
    }

    vec4 average_color;
    average_color[0] = total_color[0]/total_pixels;
    average_color[1] = total_color[1]/total_pixels;
    average_color[2] = total_color[2]/total_pixels;
    average_color[3] = total_color[3]/total_pixels;

    return average_color;
}

bool IsCompressedGLFormat(GLenum format) {
    switch (format) {
    case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
    case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
    case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
    case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
    case GL_COMPRESSED_SRGB_S3TC_DXT1_EXT:
    case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT:
    case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT:
    case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT:
        return true;
    case GL_DEPTH_COMPONENT24:
    case GL_RGB8:
    case GL_RGBA8:
    case GL_RGBA:
    case GL_SRGB8:
    case GL_SRGB8_ALPHA8:
        return false;
    default:
        LOG_ASSERT(false);
        return false;
    }
}

bool GetLinearGLFormat(GLenum format) {
    switch (format) {
    case GL_COMPRESSED_SRGB_S3TC_DXT1_EXT:
        return (bool) GL_COMPRESSED_RGB_S3TC_DXT1_EXT;

    case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT:
        return (bool) GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;

    case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT:
        return (bool) GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;

    case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT:
        return (bool) GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;

    case GL_SRGB8:
        return (bool) GL_RGB8;

    case GL_SRGB8_ALPHA8:
        return (bool) GL_RGBA8;

    case GL_SRGB_ALPHA:
        return (bool) GL_RGBA;
    }

    return format;
}


namespace {
#ifdef _WIN32
    const char* worker_app_path = "OvergrowthWorker.exe";
#endif
#ifdef __APPLE__
    const char* worker_app_path = "OvergrowthWorker";
#endif
#ifdef __LINUX__
    #ifdef __x86_64__
        const char* worker_app_path = "OvergrowthWorker.bin.x86_64";
    #else
        const char* worker_app_path = "OvergrowthWorker.bin.x86";
    #endif
#endif
}
    
/*
struct Texture {
int width, height, depth;
unsigned int size;        // for texture buffers
unsigned int num_slices;  // >1 only for array textures
bool cube_map;
GLuint gl_texture_id;
GLuint gl_buffer_id;      // for texture buffers
std::vector<SubTexture> sub_textures;
GLuint wrap_t, wrap_s, min_filter, mag_filter;
int ref_count;
bool no_mipmap;
bool no_reduce;
bool use_srgb;
bool no_live_update;
unsigned char flags;
GLenum internal_format;
GLenum format;
GLenum tex_target;
#ifdef TRACK_TEXTURE_REF
std::list<TextureRef*> ref_ptrs;
#endif
void Init();
};*/

const char* CStringFromGLEnum(GLenum val) {
    switch(val){
    case GL_ALPHA: return "GL_ALPHA";
    case GL_RGB: return "GL_RGB";
    case GL_RGBA: return "GL_RGBA";
    case GL_LUMINANCE: return "GL_LUMINANCE";
    case GL_LUMINANCE_ALPHA: return "GL_LUMINANCE_ALPHA";

    case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT: return "GL_COMPRESSED_RGBA_S3TC_DXT5_EXT";
    case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT: return "GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT";

    case GL_RGBA32F: return "GL_RGBA32F";
    case GL_R32UI: return "GL_R32UI";
    case GL_RGBA16F: return "GL_RGBA16F";
    case GL_RGB16F: return "GL_RGB16F";
    case GL_RGB32F: return "GL_RGB32F";
    case GL_DEPTH_COMPONENT: return "GL_DEPTH_COMPONENT";
    case GL_DEPTH_COMPONENT24: return "GL_DEPTH_COMPONENT24";

    case GL_UNSIGNED_BYTE: return "GL_UNSIGNED_BYTE";
    case GL_FLOAT: return "GL_FLOAT";

    case GL_NEAREST: return "GL_NEAREST";
    case GL_LINEAR: return "GL_LINEAR";
    case GL_NEAREST_MIPMAP_NEAREST: return "GL_NEAREST_MIPMAP_NEAREST";
    case GL_LINEAR_MIPMAP_NEAREST: return "GL_LINEAR_MIPMAP_NEAREST";
    case GL_NEAREST_MIPMAP_LINEAR: return "GL_NEAREST_MIPMAP_LINEAR";
    case GL_LINEAR_MIPMAP_LINEAR: return "GL_LINEAR_MIPMAP_LINEAR";

    case GL_CLAMP: return "GL_CLAMP";
    case GL_REPEAT: return "GL_REPEAT";
    case GL_CLAMP_TO_EDGE: return "GL_CLAMP_TO_EDGE";

    case GL_RGB4: return "GL_RGB4";
    case GL_RGB5: return "GL_RGB5";
    case GL_RGB8: return "GL_RGB8";
    case GL_RGB10: return "GL_RGB10";
    case GL_RGB12: return "GL_RGB12";
    case GL_RGB16: return "GL_RGB16";
    case GL_RGBA2: return "GL_RGBA2";
    case GL_RGBA4: return "GL_RGBA4";
    case GL_RGB5_A1: return "GL_RGB5_A1";
    case GL_RGBA8: return "GL_RGBA8";
    case GL_RGB10_A2: return "GL_RGB10_A2";
    case GL_RGBA12: return "GL_RGBA12";
    case GL_RGBA16: return "GL_RGBA16";

    case 0: return "NULL";
    default: return "Unknown GL enum";
    }
}

void Textures::DrawImGuiDebug() {
    enum TexType {
        kSimple, kArray, kCubemap, kBuffer, kUnknown
    };

    if(ImGui::TreeNode("Textures", "Textures: %d", (int)textures.size())){
        const char* color_tex_num = "\033FFFFFFFF";
        const char* color_cubemap = "\03399ecffFF";
        const char* color_array = "\03399ff87FF";
        const char* color_simple = "\033d1d1d1FF";
        const char* color_buffer = "\033bea0ffFF";
        const char* color_default = "\033FFFFFFFF";
        const char* color_label = "\0338dff87FF";
        const char* color_value = "\033FFFFFFFF";
        const char* color_enum = "\0339ccbd6FF";

        const int kBufSize = 256;
        char buf[kBufSize];
        for(int i=0, len=textures.size(); i<len; ++i){
            const Texture& tex = textures[i];
            bool drawable = false;
            TexType type;
            if(tex.cube_map){
                FormatString(buf, kBufSize, "%s%d:%s cubemap", color_tex_num, i, color_cubemap);
                type = kCubemap;
            } else if(tex.num_slices>1){
                FormatString(buf, kBufSize, "%s%d:%s texture array (%d slices)", color_tex_num, i, color_array, tex.num_slices);
                type = kArray;
            } else if(tex.sub_textures.size() == 1 && !tex.sub_textures[0].texture_name.empty()){
                FormatString(buf, kBufSize, "%s%d:%s %s", color_tex_num, i, color_simple, tex.sub_textures[0].texture_name.c_str());
                drawable = true;
                type = kSimple;
            } else if(tex.gl_buffer_id != UNLOADED_ID){
                FormatString(buf, kBufSize, "%s%d:%s texture buffer", color_tex_num, i, color_buffer);
                type = kBuffer;
            } else if(tex.gl_texture_id != UNLOADED_ID){
                FormatString(buf, kBufSize, "%s%d:%s generated texture", color_tex_num, i, color_simple);
                drawable = true;
                type = kSimple;
            } else {
                FormatString(buf, kBufSize, "%s%d: unknown", color_tex_num, i);
                type = kUnknown;
            }
            if(!tex.name.empty()){
                const char* color = color_default;
                switch(type){
                case kSimple:  color = color_simple;  break;
                case kArray:   color = color_array;   break;
                case kCubemap: color = color_cubemap; break;
                case kBuffer:  color = color_buffer;  break;
                case kUnknown: break;
                }
                FormatString(buf, kBufSize, "%s%d:%s %s", color_tex_num, i, color, tex.name.c_str());
            }

            if(type != kUnknown){
                ImGui::PushID(i);
                if(ImGui::TreeNode("","%s",buf)){
                    ImGui::PopID();
                    ImGui::Indent(ImGui::GetTreeNodeToLabelSpacing());
                    ImGui::Text("%sReferences:%s %d", color_label, color_value, tex.ref_count);
                    if(type == kBuffer){
                        ImGui::Text("%sSize: %s%d", color_label, color_value, tex.size);
                        ImGui::Text("%sFormat: %s%s", color_label, color_value, CStringFromGLEnum(tex.internal_format));
                    } else if(type == kArray){
                        ImGui::Text("%sSize: %s%d x %d x %d", color_label, color_value, tex.width, tex.height, tex.num_slices);
                    } else if(type == kSimple || type == kCubemap){
                        ImGui::Text("%sSize: %s%d x %d", color_label, color_value, tex.width, tex.height);
                    }
                    if(type == kArray || type == kSimple || type == kCubemap){
                        ImGui::Text("%s%s / %s", color_enum, CStringFromGLEnum(tex.internal_format), CStringFromGLEnum(tex.format));
                        ImGui::Text("%s%s / %s", color_enum, CStringFromGLEnum(tex.wrap_s), CStringFromGLEnum(tex.wrap_t));
                        ImGui::Text("%s%s / %s", color_enum, CStringFromGLEnum(tex.min_filter), CStringFromGLEnum(tex.mag_filter));
                        bool temp;
                        temp = tex.use_srgb;
                        ImGui::Checkbox("sRGB", &temp);
                        temp = !tex.no_mipmap;
                        ImGui::Checkbox("Mipmaps", &temp);
                    }
                    ImGui::Unindent(ImGui::GetTreeNodeToLabelSpacing());
                    ImGui::TreePop();
                } else {
                    ImGui::PopID();
                    if (ImGui::IsItemHovered() && drawable) {
                        if( tex.format != GL_DEPTH_COMPONENT ) {
                            ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.5, 0.5, 1.0, 0.5));
                            ImGui::BeginTooltip();
                            ImGui::PushTextWrapPos(450.0f);
                            Textures::Instance()->EnsureInVRAM(i);
                            ImGui::Image((ImTextureID)((uintptr_t)textures[i].gl_texture_id), ImVec2(128,128), ImVec2(0,0), ImVec2(1,1));
                            ImGui::PopTextWrapPos();
                            ImGui::EndTooltip();
                            ImGui::PopStyleColor(1);
                        }
                    }
                }
            }
        }
        ImGui::TreePop();
    }
}

Textures::Textures() {
    process_pool = new ProcessPool(worker_app_path);
    PROFILED_TEXTURE_MUTEX_LOCK
    for(unsigned int i=0; i<_texture_units; i++){
        bound_texture[i]=INVALID_ID;
    }
    m_wrap_t = _default_wrap;
    m_wrap_s = _default_wrap;
    min_filter = _default_min;
    mag_filter = _default_max;
    active_texture = 0;
    // TODO: Dispose thread pool when deleting

    if (config["save_as_crunch"].toNumber<bool>()) {
        convert_file_type = "_converted.crn";
    } else {
        convert_file_type = "_converted.dds";
    }
    PROFILED_TEXTURE_MUTEX_UNLOCK
}

static const float _inv_gamma = 1.0f/2.2f;
static const float float_to_byte = 255.0f;
static const float byte_to_float = 1.0f/float_to_byte;

const int kCubeMapHDRVersion = 2;

void Textures::SaveCubeMapMipmapsHDR (TextureRef which, const char* filename, int width) {
    Textures::Instance()->bindTexture(which, 0);
    int mip_level = 0;
    int old_width = width;
    std::vector<float> data;
    while(width > 1){
        for (int i=0; i<6; i++) {
            int start_size = data.size();
            data.resize(data.size() + width * width * 4);
            glGetTexImage( GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, mip_level, GL_BGRA, GL_FLOAT, &data[start_size] );
        }
        ++mip_level;
        width/=2;
    }
    std::string path = std::string(filename)+".hdrcube";
    CreateParentDirs(path.c_str());
    FILE *cache_file = my_fopen(path.c_str(), "wb");
    if(cache_file){
        fwrite(&kCubeMapHDRVersion, sizeof(int), 1, cache_file);
        {
            int size = data.size();
            fwrite(&size, sizeof(int), 1, cache_file);
        }
        fwrite(&old_width, sizeof(int), 1, cache_file);
        fwrite(&data[0], sizeof(float), data.size(), cache_file);
        fclose(cache_file);
    }
}


TextureRef Textures::LoadCubeMapMipmapsHDR (const char* filename) {
    TextureRef tex;
    FILE *cache_file = my_fopen(filename, "rb");
    if(cache_file){
        int version;
        fread(&version, sizeof(int), 1, cache_file);
        if(version == kCubeMapHDRVersion){
            int size, width;
            fread(&size, sizeof(int), 1, cache_file);
            fread(&width, sizeof(int), 1, cache_file);
            float* data = (float*)alloc.stack.Alloc(sizeof(float) * size);
            fread(&data[0], sizeof(float), size, cache_file);
            fclose(cache_file);
            tex = Textures::Instance()->makeCubemapTexture(128, 128, GL_RGBA16F, GL_RGBA, Textures::MIPMAPS);
            Textures::Instance()->SetTextureName(tex, filename);
            int mip_level = 0;
            int index = 0;
            while(width > 1){
                for (int i=0; i<6; i++) {
                    glTexSubImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, mip_level, 0, 0, width, width, GL_BGRA, GL_FLOAT, &data[index]);
                    index += width * width * 4;
                }
                ++mip_level;
                width/=2;
            }
            alloc.stack.Free(data);
        }
    }
    return tex;
}

void Texture::Init() {
    width = 0;
    height = 0;
    size = 0;
    num_slices = 1;
    cube_map = false;
    gl_texture_id = UNLOADED_ID;
    gl_buffer_id = UNLOADED_ID;
    wrap_t = 0;
    wrap_s = 0;
    min_filter = 0;
    mag_filter = 0;
    ref_count = 1;
    no_mipmap = false;
    no_reduce = false;
    use_srgb = false;
    no_live_update = true;
    flags = 0;
    internal_format = GL_NONE;
    format = GL_NONE;
    tex_target = GL_NONE;
    sub_textures.clear();
    name.clear();
}

//#pragma optimize(", off)
void Textures::DisposeTexture(int which, bool free_space) {
    Texture &texture = textures[which];

    if( texture.notify_on_dispose ) {
        LOGI << "Disposed texture " << texture.name  << " with gl id " << texture.gl_texture_id << std::endl;
    }

    for (unsigned int i = 0; i < texture.sub_textures.size(); i++) {
        delete texture.sub_textures[i].texture_data;
    }

    texture.sub_textures.clear();

    RemoveTextureFromVRAM(which);

    texture.width = 0;
    texture.height = 0;
    texture.size = 0;
    texture.num_slices = 1;
    texture.name = "Disposed";
    texture.ref_count = 0;
    texture.gl_texture_id = UNLOADED_ID;
    texture.gl_buffer_id = UNLOADED_ID;

    if(free_space){
        free_spaces.push_back(which);
    }
}
//#pragma optimize("", on)

void Textures::Dispose() {
    blank_texture_ref.clear();
    blank_normal_texture_ref.clear();

    detail_color_texture_ref.clear();
    detail_normal_texture_ref.clear();

	PROFILED_TEXTURE_MUTEX_LOCK

    for (unsigned int i=0;i<textures.size();i++){
        DisposeTexture(i);
    }
    textures.clear();

    PROFILED_TEXTURE_MUTEX_UNLOCK
}

int conversion_num = 0;

void Textures::ReloadInternal() {
    for (unsigned int i=0;i<textures.size();i++){
        Texture &texture = textures[i];

        if (texture.no_live_update) {
            continue;
        }

        std::vector<unsigned int>::const_iterator it = std::find(free_spaces.begin(), free_spaces.end(), i);
        if (it != free_spaces.end()) {
            // no texture in this slot
            LOG_ASSERT(texture.width == 0);
            LOG_ASSERT(texture.height == 0);
            LOG_ASSERT(texture.sub_textures.empty());
            LOG_ASSERT(texture.gl_texture_id == UNLOADED_ID);
            LOG_ASSERT(texture.gl_buffer_id == UNLOADED_ID);
            continue;
        }

        LOG_ASSERT(!texture.sub_textures.empty());
        char abs_path[kPathSize];

        if (texture.sub_textures.size() == 1) {
            // ordinary texture
            LOG_ASSERT(texture.num_slices == 1);

            SubTexture &sub = texture.sub_textures[0];
            LOG_ASSERT(!sub.texture_name.empty());

            if (FindImagePath(sub.texture_name.c_str(), abs_path, kPathSize, kDataPaths | kModPaths | kWriteDir | kModWriteDirs, false) == 0){
                if(strcmp(abs_path, sub.load_name.c_str()) == 0 && GetDateModifiedInt64(abs_path) == sub.orig_modified){
                    continue;
                }
            }else{
                continue;
            }

            std::string name = sub.texture_name;
            unsigned char flags = texture.flags;
            DisposeTexture(i, false);
            loadTexture(name, i, flags | PX_NOCONVERT);
        } else {
            // array texture
            LOG_ASSERT(texture.num_slices > 1);

            LogSystem::LogData(LogSystem::debug, "tex", __FILE__, __LINE__) << "Checking if array texture " << i << " (with " << texture.sub_textures.size() << " slices) needs reload..." << std::endl;

            bool needs_reload = false;

            for (unsigned int s = 0; s < texture.sub_textures.size(); s++) {
                SubTexture &sub = texture.sub_textures[s];
                LOG_ASSERT(!sub.texture_name.empty());
                ModID modsource;
                if (FindImagePath(sub.texture_name.c_str(), abs_path, kPathSize, kAnyPath, false,NULL,true, true, &modsource) == 0){
                    if(strcmp(abs_path, sub.load_name.c_str()) == 0 && GetDateModifiedInt64(abs_path) == sub.orig_modified){
                        LogSystem::LogData(LogSystem::debug, "tex", __FILE__, __LINE__) << " slice " << s << ": \"" << sub.texture_name << "\" no" << std::endl;
                    } else {
                        LogSystem::LogData(LogSystem::debug, "tex", __FILE__, __LINE__) << " slice " << s << ": \"" << sub.texture_name << "\" yes" << std::endl;
                        delete sub.texture_data;
                        std::string dst_path = AssemblePath(GetWritePath(modsource), SanitizePath(sub.texture_name + convert_file_type));
                        ConvertImage(abs_path, dst_path, GetTempDDSPath(dst_path, true, conversion_num), TextureData::Fast);
                        sub.texture_data = new TextureData();
                        sub.texture_data->Load(dst_path.c_str());
                        sub.orig_modified = GetDateModifiedInt64(abs_path);
                        needs_reload = true;
                        texture.width = 0;
                        texture.height = 0;
                    }
                } else {
                    LogSystem::LogData(LogSystem::warning, "tex", __FILE__, __LINE__) << " slice " << s << ": \"" << sub.texture_name << "\" no longer exists!" << std::endl;
                }
            }
            if (needs_reload) {
                RemoveTextureFromVRAM(i);
            }
        }
    }
}

void Textures::Reload() {
	PROFILED_TEXTURE_MUTEX_LOCK
	ReloadInternal();
    PROFILED_TEXTURE_MUTEX_UNLOCK
}

//Set texture to wrap or clamp to edge
void Textures::setWrap(GLenum wrap) {
    m_wrap_s = wrap;
    m_wrap_t = wrap;
}

void Textures::setWrap(GLenum wrap_s, GLenum wrap_t) {
    m_wrap_s = wrap_s;
    m_wrap_t = wrap_t;
}

//Set minification and magnification filters
void Textures::setFilters(GLenum min, GLenum mag) {
    PROFILED_TEXTURE_MUTEX_LOCK
    min_filter = min;
    mag_filter = mag;
    PROFILED_TEXTURE_MUTEX_UNLOCK
}

void GetCompletePath(std::string *path_utf8_ptr) {
    std::string &path_utf8 = *path_utf8_ptr;
    const int buf_size = 4096;
    #ifdef _WIN32    
        WCHAR path_utf16[buf_size];
        if(!MultiByteToWideChar(CP_UTF8, 0, path_utf8.c_str(), -1, path_utf16, buf_size)){
            FatalError("Error", "Error converting utf8 string to utf16: %s", path_utf8.c_str());
        }
        WCHAR full_path_utf16[buf_size];
        GetFullPathNameW(path_utf16, 2048, full_path_utf16, NULL);
        char full_path_utf8[buf_size];
        if(!WideCharToMultiByte(CP_UTF8, 0, full_path_utf16, -1, full_path_utf8, buf_size, NULL, NULL)){
            DWORD err = GetLastError();
            FatalError("Error", "Error converting utf16 string to utf8.");
        }
        int str_len = strlen(full_path_utf8);
        for(int i=0; i<str_len; ++i){
            if(full_path_utf8[i] == '\\'){
                full_path_utf8[i] = '/';
            }
        }
        path_utf8 = full_path_utf8;        
    #else
        char full_path_utf8[buf_size];
        realpath(path_utf8.c_str(), full_path_utf8);
        path_utf8 = full_path_utf8;
    #endif
}

void Textures::IncrementRefCount(unsigned int id) {
    PROFILED_TEXTURE_MUTEX_LOCK
    if (id != INVALID_ID && id < textures.size()) {
        textures[id].ref_count++;
    }
    PROFILED_TEXTURE_MUTEX_UNLOCK
}

void Textures::DecrementRefCount(unsigned int id) {
    PROFILED_TEXTURE_MUTEX_LOCK
    if (id != INVALID_ID && id < textures.size()) {
        textures[id].ref_count--;
    }
    PROFILED_TEXTURE_MUTEX_UNLOCK
}

#ifdef TRACK_TEXTURE_REF
void Textures::AddRefPtr(unsigned int id, TextureRef* ref) {
	texture_mutex.lock();
    if (id != INVALID_ID && id < textures.size()) {
        textures[id].ref_ptrs.push_back(ref);
    }
	texture_mutex.unlock();
}

void Textures::RemoveRefPtr(unsigned int id, TextureRef* ref) {
	texture_mutex.lock();
    if (id != INVALID_ID && id < textures.size()) {
        textures[id].ref_ptrs.erase(
            std::find(textures[id].ref_ptrs.begin(),
                      textures[id].ref_ptrs.end(),
                      ref));
    }
	texture_mutex.unlock();
}
#endif

void Textures::GenerateMipmap(const TextureRef &tex_ref) {
	PROFILER_GPU_ZONE(g_profiler_ctx, "Textures::GenerateMipmap");
    bindTexture(tex_ref,0);
    glGenerateMipmap(GL_TEXTURE_2D);
}

void Textures::setSampleFilter( const TextureRef &tex_ref, enum SampleFilter filter )
{
    bindTexture(tex_ref);
    if( filter == FILTER_NEAREST )
    { 
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }
    else if( filter == FILTER_LINEAR )
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
}

void Textures::setSampleWrap( const TextureRef &tex_ref, enum SampleWrap wrapping )
{
    bindTexture(tex_ref);

    if( wrapping == WRAP_CLAMP_TO_EDGE )
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
}

void Textures::subImage( const TextureRef &tex_ref, void* data )
{
    if( tex_ref.valid() )
    {
        bindTexture( tex_ref );
        Texture &texture = textures[tex_ref.id];

        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, texture.width, texture.height, texture.format, GL_UNSIGNED_BYTE,data);
    }
    else
    {
        LOGE << "Unable to load data to texture." << std::endl;
    }
}

void Textures::subImage( const TextureRef &destination, const TextureRef &source, int xoffset, int yoffset  )
{
    if( destination.valid() && source.valid() )
    {
        Texture &source_tex = textures[source.id];
        Texture &dst_tex = textures[destination.id];
        if(source_tex.sub_textures.empty()){
            DisplayError("Error", "Sub image has no sub_textures");
            return;
        }
        TextureData* tex_data = source_tex.sub_textures[0].texture_data;
        if(!tex_data){
            DisplayError("Error", "Trying to subImage from texture with no data");
            return;
        }

        tex_data->EnsureInRAM();

        if(tex_data->GetGLInternalFormat() != dst_tex.internal_format){
            // only continue if error is because sRGB/linear mismatch
            if (GetLinearGLFormat(tex_data->GetGLInternalFormat()) != GetLinearGLFormat(dst_tex.internal_format)) {
                // TODO: could convert source texture to target format
                LOGE << "Trying to subImage texture of format " << tex_data->GetGLInternalFormat() << " into texture of different format " << dst_tex.internal_format << std::endl;
                return;
            }
        }

        PROFILER_GPU_ZONE(g_profiler_ctx, "Textures::subImage");
        Graphics::Instance()->DebugTracePrint(source_tex.sub_textures[0].texture_name.c_str());

        bindTexture( destination );

        bool regen_mipmap = (false == textures[destination.id].no_mipmap);
        
        if(tex_data->IsCompressed()) {
            int reduction_factor = Graphics::Instance()->config_.texture_reduction_factor();
            int dataMipLevel = 0;
            int mipLevels = tex_data->GetMipLevels();
            LOG_ASSERT(mipLevels > 0);

            int targetWidth = tex_data->GetWidth() / reduction_factor;
            int targetHeight = tex_data->GetHeight() / reduction_factor;

            if(targetWidth < 4)
                targetWidth = 4;
            if(targetHeight < 4)
                targetHeight = 4;

            for(int i = 0; i < mipLevels; ++i) {
                int width = tex_data->GetMipWidth(dataMipLevel);
                int height = tex_data->GetMipHeight(dataMipLevel);

                if (width > targetWidth && height > targetHeight) {
                    dataMipLevel++;
                } else {
                    break;
                }
            }

            for (int i = 0; i < mipLevels; i++) {
                int width = tex_data->GetMipWidth(dataMipLevel);
                int height = tex_data->GetMipHeight(dataMipLevel);

                if(width < 4 || height < 4) {
                    // TODO: we should also set TEXTURE_MAX_LEVEL
                    // so we don't end up using the mip levels which are not valid
                    // currently since this is only used by decals and those are 512x512
                    // you really shouldn't get far enough away that it matters
                    break;
                }

                glCompressedTexSubImage2D(GL_TEXTURE_2D, i, xoffset, yoffset, width, height, dst_tex.internal_format, tex_data->GetMipDataSize(0, dataMipLevel), tex_data->GetMipData(0, dataMipLevel));

                if(dataMipLevel + 1 < mipLevels)
                    dataMipLevel++;

                xoffset = (xoffset + 1) / 2;
                yoffset = (yoffset + 1) / 2;
            }
            regen_mipmap = false;
        } else {
            glTexSubImage2D(GL_TEXTURE_2D, 0, xoffset, yoffset, tex_data->GetMipWidth(0), tex_data->GetMipHeight(0), tex_data->GetGLBaseFormat(), tex_data->GetGLType(), tex_data->GetMipData(0, 0));
        }

        if( regen_mipmap )
        {
            glGenerateMipmap(GL_TEXTURE_2D);
        }
    } else {
        LOGE << __FUNCTION__ << ": Source or destination is invalid" << std::endl;
    }
}

void Textures::TextureToVRAM(unsigned int which) {
    PROFILER_GPU_ZONE(g_profiler_ctx, "TextureToVRAM");
    CHECK_GL_ERROR();

    if (!deferred_delete_textures.empty()) {
        for (std::list<GLuint>::iterator it = deferred_delete_textures.begin(); it != deferred_delete_textures.end(); it++) {
            GLuint tex_id = *it;
            LOGI << "Deferred delete of texture " << tex_id << std::endl;
            glDeleteTextures(1, &tex_id);
        }
        deferred_delete_textures.clear();
    }

    Texture& tex = textures[which];

    if(tex.sub_textures.empty()){
        LOGW << "Calling TextureToVRAM on a texture with no sub_textures" << std::endl;
        return;
    }
    
    Graphics::Instance()->DebugTracePrint(tex.sub_textures[0].texture_name.c_str());

    bool tex_compressed = tex.sub_textures[0].texture_data->IsCompressed();
    // TODO: silently fix the issue
    for(int i=1, len=tex.sub_textures.size(); i<len; ++i){
        const SubTexture &s = tex.sub_textures[i];
        if(s.texture_data->IsCompressed() != tex_compressed) {
            LOGE << "TextureToVRAM failed (compression mismatch) for " << s.load_name <<  std::endl;
            return;
        }
    }

    if(tex.gl_texture_id != UNLOADED_ID)return;

    for( unsigned i = 0; i < tex.sub_textures.size(); i++ ) {
        tex.sub_textures[i].texture_data->EnsureInRAM();
    }
                
    bool is_array = (tex.sub_textures.size() > 1);
    bool is_cube = tex.sub_textures[0].texture_data->IsCube();

    if (is_array && is_cube) {
        // GL 4.0 feature, we have minimum requirement 3.2
        LOGE << "cube map arrays not supported" << std::endl;
        return;
    }

    // FIXME?
    if (tex.use_srgb) {
        if (tex.sub_textures[0].texture_data->GetColorSpace() != TextureData::sRGB) {
            //LOGW << "Texture " << tex.sub_textures[0].load_name << " uses sRGB color space but does not specify it in file name" << std::endl;
        }
        tex.sub_textures[0].texture_data->SetColorSpace(TextureData::sRGB);
    } else {
        if (tex.sub_textures[0].texture_data->GetColorSpace() != TextureData::Linear) {
            //LOGW << "Texture " << tex.sub_textures[0].load_name << " specifies sRGB color space in file name but linear internally" << std::endl;
        }
    }

	// TODO fill this here
	GLenum& internal_format = tex.internal_format;

    unsigned int num_mips = tex.sub_textures[0].texture_data->GetMipLevels();

    if (is_array) {
        // set texture size to size of smallest slice
        LOG_ASSERT(tex.width == 0);
        LOG_ASSERT(tex.height == 0);

        unsigned int width = tex.sub_textures[0].texture_data->GetWidth();
        unsigned int height = tex.sub_textures[0].texture_data->GetHeight();

            for (unsigned int i = 1; i < tex.sub_textures.size(); i++) {
                width = std::min(width, tex.sub_textures[i].texture_data->GetWidth());
                height = std::min(height, tex.sub_textures[i].texture_data->GetHeight());
                num_mips = std::min(num_mips, tex.sub_textures[i].texture_data->GetMipLevels());
            }

        tex.width = width;
        tex.height = height;
    }
    
    if(!tex.no_reduce){
        tex.width /= Graphics::Instance()->config_.texture_reduction_factor();
        tex.height /= Graphics::Instance()->config_.texture_reduction_factor();
    }
    
    glGenTextures( 1, &tex.gl_texture_id );
    LOG_ASSERT(tex.gl_texture_id != INVALID_ID);
    LOG_ASSERT(tex.gl_texture_id != UNLOADED_ID);

    if( tex_compressed == false ) {
        PROFILER_GPU_ZONE(g_profiler_ctx, "Uncompressed");
        tex.tex_target = is_array ? GL_TEXTURE_2D_ARRAY : GL_TEXTURE_2D;
        glBindTexture(tex.tex_target, tex.gl_texture_id);

        if(!tex.no_mipmap && num_mips > 1){
            glTexParameteri(tex.tex_target, GL_TEXTURE_MIN_FILTER, tex.min_filter);
        } else {
            glTexParameteri(tex.tex_target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        }
        glTexParameteri(tex.tex_target, GL_TEXTURE_MAG_FILTER, tex.mag_filter);

        glTexParameteri(tex.tex_target, GL_TEXTURE_WRAP_S, tex.wrap_s);
        glTexParameteri(tex.tex_target, GL_TEXTURE_WRAP_T, tex.wrap_t);

        if(Graphics::Instance()->config_.anisotropy())
            glTexParameterf(tex.tex_target, GL_TEXTURE_MAX_ANISOTROPY_EXT, Graphics::Instance()->config_.anisotropy());
        
        CHECK_GL_ERROR();

        internal_format = GL_RGBA;

        //unsigned int totalTextureSize = 0;
        if (is_array) {
            TextureData* tex_data = tex.sub_textures[0].texture_data;
            tex.internal_format = tex_data->GetGLInternalFormat();
            tex.format = GL_BGRA;
            glTexImage3D(tex.tex_target, 0, tex_data->GetGLInternalFormat(), tex_data->GetWidth(), tex_data->GetHeight(), tex.num_slices, 0, GL_BGRA, tex_data->GetGLType(), NULL);
        }

        for (unsigned int slice = 0; slice < tex.num_slices; slice++) {
            TextureData* tex_data = tex.sub_textures[slice].texture_data;
            // TODO: check formats
            if(!tex_data){
                FatalError("Error", "Compressed slice in uncompressed texture array");
            }

            tex_data->EnsureInRAM();

            if (!tex.no_mipmap && !tex_data->HasMipmaps()) {
                PROFILER_GPU_ZONE(g_profiler_ctx, "GenerateMipmaps");
                bool success = tex_data->GenerateMipmaps();
                if (!success) {
                    FatalError("Error", "Mipmap generation failed");
                }
                assert(tex_data->HasMipmaps());
            }

            unsigned int startMip = 0;
            while ( startMip < tex_data->GetMipLevels() && (int)tex_data->GetMipWidth(startMip) > tex.width ) {
                startMip++;
            }

            internal_format = tex_data->GetGLInternalFormat();
            if (is_array) {
                tex.format = tex_data->GetGLBaseFormat();
                LOG_ASSERT(tex_data->GetWidth() == tex.sub_textures[0].texture_data->GetWidth() &&
                    tex_data->GetHeight() == tex.sub_textures[0].texture_data->GetHeight());
                for (unsigned int i = startMip; i < num_mips; i++ ) {
                    PROFILER_GPU_ZONE(g_profiler_ctx, "glTexSubImage3D");
                glTexSubImage3D(tex.tex_target, i - startMip, 0, 0, slice, tex_data->GetMipWidth(i), tex_data->GetMipHeight(i), 1,
                    tex_data->GetGLBaseFormat(),
                    tex_data->GetGLType(),
                    tex_data->GetMipData(0, i));
                }
            } else {
                tex.format = tex_data->GetGLBaseFormat();
                for (unsigned int i = startMip; i < num_mips; i++ ) {
                    PROFILER_GPU_ZONE(g_profiler_ctx, "glTexImage2D");
                glTexImage2D(tex.tex_target,
                    i - startMip,
                    internal_format,
                    tex_data->GetMipWidth(i),
                    tex_data->GetMipHeight(i),
                    0,
                    tex_data->GetGLBaseFormat(),
                    tex_data->GetGLType(),
                    tex_data->GetMipData(0, i));
                }
            }
            //totalTextureSize = 4 * tex_data->GetWidth() * tex_data->GetHeight();
        }
        
        CHECK_GL_ERROR();
    } else {
        PROFILER_GPU_ZONE(g_profiler_ctx, "Compressed");
        int skip_levels = 0;
        if(!tex.no_reduce){
            skip_levels = Graphics::Instance()->config_.texture_reduce();
        }
        
        if(is_cube) {
            tex.tex_target = GL_TEXTURE_CUBE_MAP;
            glBindTexture(tex.tex_target, tex.gl_texture_id);

            glTexParameteri(tex.tex_target, GL_TEXTURE_MIN_FILTER, tex.min_filter);
            glTexParameteri(tex.tex_target, GL_TEXTURE_MAG_FILTER, tex.mag_filter);

            glTexParameteri(tex.tex_target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(tex.tex_target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(tex.tex_target, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

            tex.cube_map = true;

            if(Graphics::Instance()->config_.anisotropy()) {
                glTexParameterf(tex.tex_target, GL_TEXTURE_MAX_ANISOTROPY_EXT, Graphics::Instance()->config_.anisotropy());
            }

            TextureData* tex_data = tex.sub_textures[0].texture_data;

            LOG_ASSERT(tex_data);

            internal_format = tex_data->GetGLInternalFormat();

            tex_data->EnsureInRAM();

            uint32_t totalTextureSize = 0;
            for(unsigned int j = 0; j < tex_data->GetNumFaces(); j++) {
                for(unsigned int i = 0; i < tex_data->GetMipLevels(); i++) {
                    int width = tex_data->GetMipWidth(i);
                    int height = tex_data->GetMipHeight(i);
                    if( width == 0 || height == 0 ) {
                        break;
                    }
                    if( (int)i>=skip_levels ) {
                        if(tex_data->IsCompressed()) {
                        glCompressedTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + j, 
                                                  i-skip_levels, 
                                                  internal_format, 
                                                  width, 
                                                  height, 
                                                  0, 
                                                  tex_data->GetMipDataSize(j, i),
                                                  tex_data->GetMipData(GL_TEXTURE_CUBE_MAP_POSITIVE_X + j, i));
                        } else {
                            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + j, 
                                                      i-skip_levels, 
                                                      internal_format, 
                                                      width, 
                                                      height, 
                                                      0, 
                                                      tex_data->GetGLBaseFormat(),
                                                      tex_data->GetGLType(),
                                                      tex_data->GetMipData(GL_TEXTURE_CUBE_MAP_POSITIVE_X + j, i));
                        }
                        totalTextureSize += tex_data->GetMipDataSize(j, i);
                    }
                }
            }

            LOG_ASSERT(tex.sub_textures.size() == 1);
        } else {
            LOG_ASSERT(!tex.sub_textures.empty());
            tex.tex_target = is_array ? GL_TEXTURE_2D_ARRAY : GL_TEXTURE_2D;
            
            glBindTexture(tex.tex_target, tex.gl_texture_id);

            glTexParameteri(tex.tex_target, GL_TEXTURE_MIN_FILTER, tex.min_filter);
            glTexParameteri(tex.tex_target, GL_TEXTURE_MAG_FILTER, tex.mag_filter);

            glTexParameteri(tex.tex_target, GL_TEXTURE_WRAP_S, tex.wrap_s);
            glTexParameteri(tex.tex_target, GL_TEXTURE_WRAP_T, tex.wrap_t);

            if(Graphics::Instance()->config_.anisotropy())
                glTexParameterf(tex.tex_target, GL_TEXTURE_MAX_ANISOTROPY_EXT, Graphics::Instance()->config_.anisotropy());


            if (!is_array)
            {
                TextureData* tex_data = tex.sub_textures[0].texture_data;

                LOG_ASSERT(tex_data);
    
                internal_format = tex_data->GetGLInternalFormat();

                tex_data->EnsureInRAM();

                int width = tex_data->GetWidth();
                int height = tex_data->GetHeight();

                uint32_t totalTextureSize = 0;
                int temp_skip_levels = skip_levels + 0;
                for(unsigned int i = 0; i < tex_data->GetMipLevels(); i++)
                {
                    if( width == 0 && height == 0 ) {
                        break;
                    }
                    if( (int)i >= temp_skip_levels ) {
                        if(tex_data->IsCompressed()) {
                            glCompressedTexImage2D(tex.tex_target,
                                                      i-temp_skip_levels, 
                                                      internal_format, 
                                                      tex_data->GetMipWidth(i),
                                                      tex_data->GetMipHeight(i),
                                                      0, 
                                                      tex_data->GetMipDataSize(0, i),
                                                      tex_data->GetMipData(0, i));
                        } else {
                            glTexImage2D(tex.tex_target,
                                                      i-temp_skip_levels, 
                                                      internal_format, 
                                                      width, 
                                                      height, 
                                                      0, 
                                                      tex_data->GetGLBaseFormat(),
                                                      tex_data->GetGLType(),
                                                      tex_data->GetMipData(0, i));
                        }
                        CHECK_GL_ERROR();
                        totalTextureSize += tex_data->GetMipDataSize(0, i);
                    }
                }
            } else {
                // array texture
                TextureData* tex_data = tex.sub_textures[0].texture_data;

                LOG_ASSERT(tex_data);

                int width = tex.width;
                int height = tex.height;

                internal_format = tex_data->GetGLInternalFormat();

                tex_data->EnsureInRAM();

                for (unsigned int mip = 0; mip < num_mips; mip++) {
					assert( tex.tex_target == GL_TEXTURE_3D ||  tex.tex_target == GL_TEXTURE_2D_ARRAY  );
					int image_size = tex_data->GetMipDataSize(0, mip + tex_data->GetMipLevels() - num_mips) * tex.num_slices;
                    glTexImage3D(tex.tex_target,
                                          mip,
                                          internal_format, 
                                          width,
                                          height,
                                          tex.num_slices,
                                          0, 
										  GL_RGBA,
										  GL_UNSIGNED_BYTE,
                                          NULL);
                        width = std::max(1, (width + 1) / 2);
                        height = std::max(1, (height + 1) / 2);
                }

                for (unsigned int slice = 0; slice < tex.num_slices; slice++) {
                    TextureData* tex_data = tex.sub_textures[slice].texture_data;

                    int width = tex_data->GetWidth();
                    int height = tex_data->GetHeight();

                    LOG_ASSERT(tex_data->IsCompressed());
                    LOG_ASSERT(num_mips <= tex_data->GetMipLevels());

                    // in case the slice is larger than others find the starting level
                    unsigned int start_mip;
                    for (start_mip = 0; start_mip < tex_data->GetMipLevels(); start_mip++) {
                        if (width == tex.width && height == tex.height) {
                            break;
                        }
                        width = std::max(1, (width + 1) / 2);
                        height = std::max(1, (height + 1) / 2);
                    }

                    for (unsigned int i = start_mip; i < tex_data->GetMipLevels(); i++) {
                        glCompressedTexSubImage3D(tex.tex_target,
                                                  i - start_mip,
                                                  0,
                                                  0,
                                                  slice,
                                                  width, 
                                                  height, 
                                                  1,
                                                  internal_format,
                                                  tex_data->GetMipDataSize(0, i),
                                                  tex_data->GetMipData(0, i));

                        width = std::max(1, (width + 1) / 2);
                        height = std::max(1, (height + 1) / 2);
                    }
                }
            }
            glBindTexture(tex.tex_target, 0);
        }
    }

        if(tex.gl_texture_id == UNLOADED_ID) {
            DisplayError("Error","Texture ID is overlapping with UNLOADED_ID");
        }

    RebindTexture(GetActiveTexture());

    
	if(config["texture_minimize_ram"].toBool() == true){
        //LOGI << "Unloading texture " << tex.sub_textures[0].texture_data->GetPath() << " from RAM to save space" << std::endl;
        tex.sub_textures[0].texture_data->UnloadData();
    }

    LOG_ASSERT(tex.tex_target != 0);

    CHECK_GL_ERROR();
}

void Textures::drawTexture(TextureRef id, vec3 where, float size, float rotation) {
    CHECK_GL_ERROR();

    // no coloring
    Graphics* graphics = Graphics::Instance();
    Shaders* shaders = Shaders::Instance();

    int shader_id = shaders->returnProgram("simple_2d #TEXTURE #FLIPPED");
    shaders->createProgram(shader_id);
    int shader_attrib_vert_coord = shaders->returnShaderAttrib("vert_coord", shader_id);
    int shader_attrib_tex_coord = shaders->returnShaderAttrib("tex_coord", shader_id);
    int uniform_mvp_mat = shaders->returnShaderVariable("mvp_mat", shader_id);
    int uniform_color = shaders->returnShaderVariable("color", shader_id);
    shaders->setProgram(shader_id);
    CHECK_GL_ERROR();

    GLState gl_state;
    gl_state.blend = true;
    gl_state.cull_face = false;
    gl_state.depth_write = false;
    gl_state.depth_test = false;
    graphics->setGLState(gl_state);
    CHECK_GL_ERROR();


    glm::mat4 proj_mat;
    proj_mat = glm::ortho(0.0f, (float)graphics->window_dims[0], 0.0f, (float)graphics->window_dims[1]);
    glm::mat4 modelview_mat(1.0f);
    modelview_mat = glm::translate(modelview_mat, glm::vec3(where[0]-size*0.5f,where[1]-size*0.5f,0));
    modelview_mat = glm::scale(modelview_mat, glm::vec3(size,size,1.0f));
    modelview_mat = glm::translate(modelview_mat, glm::vec3(0.5f,0.5f,0.5f));
    modelview_mat = glm::rotate(modelview_mat, rotation, glm::vec3(0.0f, 0.0f, 1.0f));
    modelview_mat = glm::translate(modelview_mat, glm::vec3(-0.5f,-0.5f,-0.5f));
    glm::mat4 mvp_mat = proj_mat * modelview_mat;
    CHECK_GL_ERROR();

    graphics->EnableVertexAttribArray(shader_attrib_vert_coord);
    CHECK_GL_ERROR();
    graphics->EnableVertexAttribArray(shader_attrib_tex_coord);
    CHECK_GL_ERROR();
    static const GLfloat verts[] = {
        0, 0, 0, 0,
        1, 0, 1, 0,
        1, 1, 1, 1,
        0, 1, 0, 1
    };
    static const GLuint indices[] = {
        0, 1, 2, 
        0, 2, 3
    };
    static VBOContainer vert_vbo;
    static VBOContainer index_vbo;
    static bool vbo_filled = false;
    if(!vbo_filled) {
        vert_vbo.Fill(kVBOFloat | kVBOStatic, sizeof(verts), (void*)verts);
        index_vbo.Fill(kVBOElement | kVBOStatic, sizeof(indices), (void*)indices);
        vbo_filled = true;
    }
    vert_vbo.Bind();
    index_vbo.Bind();

    glVertexAttribPointer(shader_attrib_vert_coord, 2, GL_FLOAT, false, 4*sizeof(GLfloat), 0);
    CHECK_GL_ERROR();
    glVertexAttribPointer(shader_attrib_tex_coord, 2, GL_FLOAT, false, 4*sizeof(GLfloat), (void*)(sizeof(GL_FLOAT)*2));
    CHECK_GL_ERROR();

    int num_indices = 6;

    glUniformMatrix4fv(uniform_mvp_mat, 1, false, (GLfloat*)&mvp_mat);
    CHECK_GL_ERROR();
    vec4 color(1.0f);
    glUniform4fv(uniform_color, 1, &color[0]);
    CHECK_GL_ERROR();

    bindTexture(id, 0);
    CHECK_GL_ERROR();
    graphics->DrawElements(GL_TRIANGLES, num_indices, GL_UNSIGNED_INT, 0);
    CHECK_GL_ERROR();

    graphics->BindElementVBO(0);
    graphics->BindArrayVBO(0);
    graphics->ResetVertexAttribArrays();
    CHECK_GL_ERROR();
}

int Textures::AllocateFreeSlot() {
    if(free_spaces.empty()) {
        int id = (int)textures.size();
        textures.resize(id+1);
        textures[id].ref_count = 1;
	    textures[id].notify_on_dispose = false;
        return id;
    } else {
        int which_space = free_spaces.back();
        free_spaces.resize(free_spaces.size()-1);
        LOG_ASSERT(textures[which_space].ref_count == 0);
        LOG_ASSERT(textures[which_space].gl_texture_id == UNLOADED_ID);
        LOG_ASSERT(textures[which_space].name == "Disposed");
        textures[which_space].notify_on_dispose = false;
        textures[which_space].ref_count = 1;
        return which_space;
    }
}

void Textures::DeleteUnusedTextures() {
	PROFILED_TEXTURE_MUTEX_LOCK
    int num_deleted = 0;
    for (unsigned i=0; i<textures.size(); i++) {
        if( textures[i].ref_count <= 0 &&
            (textures[i].gl_texture_id != UNLOADED_ID ||
             !textures[i].sub_textures.empty()))
        {
            //printf("Deleting %s\n", textures[i].texture_name.c_str());
            DisposeTexture(i);
            ++num_deleted;
        } else if(textures[i].ref_count > 0){
            //printf("Keeping %s\n", textures[i].texture_name.c_str());
        }
    }
    PROFILED_TEXTURE_MUTEX_UNLOCK
}

unsigned int Textures::makeCubemapTextureInternal(int width, int height, GLint internal_format, GLint format, MipmapParam mipmap_param) {    
    CHECK_GL_ERROR();

    unsigned which = AllocateFreeSlot();

    Texture& texture = textures[which];
    texture.Init();
    texture.cube_map = true;
    texture.width = width;
    texture.height = height;
    texture.tex_target = GL_TEXTURE_CUBE_MAP;
    texture.internal_format = internal_format;
    texture.format = format;
    
    texture.use_srgb = (internal_format == GL_SRGB_ALPHA);
  
    glGenTextures(1, &texture.gl_texture_id);
    LOG_ASSERT(texture.gl_texture_id != INVALID_ID);
    LOG_ASSERT(texture.gl_texture_id != UNLOADED_ID);

    glBindTexture(texture.tex_target, texture.gl_texture_id);
    texture.mag_filter = GL_LINEAR;
    if(mipmap_param == NO_MIPMAPS){
        texture.min_filter = GL_LINEAR;
    } else {
        texture.min_filter = GL_LINEAR_MIPMAP_LINEAR;
    }

    texture.wrap_s = GL_CLAMP_TO_EDGE;
    texture.wrap_t = GL_CLAMP_TO_EDGE;

    glTexParameteri(texture.tex_target, GL_TEXTURE_MIN_FILTER, texture.min_filter);
    glTexParameteri(texture.tex_target, GL_TEXTURE_MAG_FILTER, texture.mag_filter);
    glTexParameteri(texture.tex_target, GL_TEXTURE_WRAP_S, texture.wrap_s);
    glTexParameteri(texture.tex_target, GL_TEXTURE_WRAP_T, texture.wrap_t);
    glTexParameteri(texture.tex_target, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    int level = 0;
    unsigned int totalTextureSize = 0;
    while((width >= 1 && height >= 1) && (mipmap_param == MIPMAPS || level == 0)) {
        for (int i=0; i<6; i++) {
            if(internal_format == GL_RGBA16F) {
                if(GLAD_GL_ARB_texture_float) {
                    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, level, internal_format, width, height, 0, format, GL_FLOAT, NULL);
                    totalTextureSize += width*height*8;
                } else {
                    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, level, GL_RGBA, width,  height,  0, format, GL_UNSIGNED_BYTE, NULL);
                    totalTextureSize += width*height*4;
                }
            } else {
                glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, level, internal_format, width,  height,  0, format, GL_UNSIGNED_BYTE, NULL);
                totalTextureSize += width*height*4;
            }
        }
        width /= 2;
        height /= 2;
        ++level;
    }
    CHECK_GL_ERROR();
    return which;
}

TextureRef Textures::makeCubemapTexture(int width, int height, GLint internal_format, GLint format, MipmapParam mipmap_param) {
	PROFILED_TEXTURE_MUTEX_LOCK
	unsigned int t = makeCubemapTextureInternal(width, height, internal_format, format, mipmap_param);
    PROFILED_TEXTURE_MUTEX_UNLOCK
    TextureRef tex_ref(t);
    DecrementRefCount(t);
	return tex_ref;
}

unsigned int Textures::makeTextureInternal(const TextureData &texture_data) {
    CHECK_GL_ERROR();
    unsigned which = AllocateFreeSlot();

    TextureData* texture_data_ptr = new TextureData(texture_data);
    textures[which].Init();
    textures[which].sub_textures.resize(1);
    textures[which].sub_textures[0].texture_data = texture_data_ptr;
    textures[which].width = texture_data.GetWidth();
    textures[which].height = texture_data.GetHeight();

    textures[which].wrap_s = m_wrap_s;
    textures[which].wrap_t = m_wrap_t;
    textures[which].min_filter = min_filter;
    textures[which].mag_filter = mag_filter;

    textures[which].no_mipmap = true;

    textures[which].internal_format = texture_data.GetGLInternalFormat();
    textures[which].format = texture_data.GetGLBaseFormat();
    textures[which].tex_target = GL_TEXTURE_2D;

    TextureToVRAM(which);

    return which;
}

TextureRef Textures::makeTexture(const TextureData &texture_data) {
	PROFILED_TEXTURE_MUTEX_LOCK
	unsigned int t = makeTextureInternal(texture_data);
    PROFILED_TEXTURE_MUTEX_UNLOCK
    TextureRef tex_ref(t);
    DecrementRefCount(t); 
	return tex_ref;
}

unsigned int Textures::makeTextureInternal(int width, int height, GLint internal_format, GLint format, bool mipmap, void* data) {
    PROFILER_ZONE(g_profiler_ctx, "Textures::makeTextureInternal(int width, int height, GLint internal_format, GLint format, bool mipmap, void* data)");
    CHECK_GL_ERROR();
    unsigned which = AllocateFreeSlot();

    Texture& texture = textures[which];
    texture.Init();
    texture.sub_textures.resize(1);
    texture.width = width;
    texture.height = height;
    texture.internal_format = internal_format;
    texture.format = format;
    texture.tex_target = GL_TEXTURE_2D;
  
    glGenTextures(1, &texture.gl_texture_id);
    LOG_ASSERT(texture.gl_texture_id != INVALID_ID);
    LOG_ASSERT(texture.gl_texture_id != UNLOADED_ID);

    CHECK_GL_ERROR();
    glBindTexture(texture.tex_target, texture.gl_texture_id);
    if(internal_format == GL_RGBA16F) {
        if(GLAD_GL_ARB_texture_float) {
            glTexImage2D(texture.tex_target, 0, internal_format, width, height, 0,
                        format, GL_FLOAT, data);
        } else {
            glTexImage2D(texture.tex_target, 0, GL_RGBA, width, height, 0,
                    format, GL_UNSIGNED_BYTE, data);
        }
    } else if (IsCompressedGLFormat(internal_format)) {
        int data_size = ((width + 3) / 4) * ((height + 3) / 4) * 16;
        std::vector<char> zeros(data_size, 0);
        PROFILER_ZONE(g_profiler_ctx, "glCompressedTexImage2D");
        int temp_width = width;
        int temp_height = height;
        int temp_level = 0;
        glCompressedTexImage2D(texture.tex_target, temp_level, internal_format, temp_width, temp_height, 0, data_size, &zeros[0]);
        while(mipmap && (temp_width > 1 || temp_height > 1)) {
            ++temp_level;
            if(temp_width > 1){
                temp_width /= 2;
            }
            if(temp_height > 1){
                temp_height /= 2;
            }
            data_size = ((temp_width + 3) / 4) * ((temp_height + 3) / 4) * 16;
            glCompressedTexImage2D(texture.tex_target, temp_level, internal_format, temp_width, temp_height, 0, data_size, &zeros[0]);
        }
    } else {
        glTexImage2D(texture.tex_target, 0, internal_format, width, height, 0,
                    format, GL_UNSIGNED_BYTE, data);
    }
    CHECK_GL_ERROR();
    if (mipmap) texture.min_filter = GL_LINEAR_MIPMAP_LINEAR;
    else texture.min_filter = GL_LINEAR;
    texture.mag_filter = GL_LINEAR;
    texture.wrap_s = GL_CLAMP_TO_EDGE;
    texture.wrap_t = GL_CLAMP_TO_EDGE;
    glTexParameteri(texture.tex_target, GL_TEXTURE_WRAP_S, texture.wrap_s);
    glTexParameteri(texture.tex_target, GL_TEXTURE_WRAP_T, texture.wrap_t);
    glTexParameteri(texture.tex_target, GL_TEXTURE_MIN_FILTER, texture.min_filter);
    glTexParameteri(texture.tex_target, GL_TEXTURE_MAG_FILTER, texture.mag_filter);
    if(Graphics::Instance()->config_.anisotropy()) {
        glTexParameterf(texture.tex_target, GL_TEXTURE_MAX_ANISOTROPY_EXT, Graphics::Instance()->config_.anisotropy());
    }
    CHECK_GL_ERROR();
    if(mipmap && !IsCompressedGLFormat(internal_format)) {
        PROFILER_ZONE(g_profiler_ctx, "glGenerateMipmap");
        glGenerateMipmap(texture.tex_target);
    }
    CHECK_GL_ERROR();

    return which;
}

unsigned int Textures::make3DTextureInternal(int* dims, GLint internal_format, GLint format, bool mipmap, void* data) {
    CHECK_GL_ERROR();
    unsigned which = AllocateFreeSlot();

    Texture& texture = textures[which];
    texture.Init();
    texture.width = dims[0];
    texture.height = dims[1];
    texture.depth = dims[2];
    texture.internal_format = internal_format;
    texture.format = format;
    texture.tex_target = GL_TEXTURE_3D;

    glGenTextures(1, &texture.gl_texture_id);
    LOG_ASSERT(texture.gl_texture_id != INVALID_ID);
    LOG_ASSERT(texture.gl_texture_id != UNLOADED_ID);

    CHECK_GL_ERROR();
    glBindTexture(texture.tex_target, texture.gl_texture_id);
    if(internal_format == GL_RGBA16F) {
        glTexImage3D(texture.tex_target, 0, internal_format, dims[0], dims[1], dims[2], 0, format, GL_FLOAT, data);
    } else {
        glTexImage3D(texture.tex_target, 0, internal_format, dims[0], dims[1], dims[2], 0, format, GL_UNSIGNED_BYTE, data);
    }
    CHECK_GL_ERROR();
    if (mipmap) glTexParameteri(texture.tex_target, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    else glTexParameteri(texture.tex_target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(texture.tex_target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(texture.tex_target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(texture.tex_target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(texture.tex_target, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    if(Graphics::Instance()->config_.anisotropy()) {
        glTexParameterf(texture.tex_target, GL_TEXTURE_MAX_ANISOTROPY_EXT, Graphics::Instance()->config_.anisotropy());
    }
    CHECK_GL_ERROR();
    if(mipmap) {
        PROFILER_ZONE(g_profiler_ctx, "glGenerateMipmap");
        glGenerateMipmap(texture.tex_target);
    }
    CHECK_GL_ERROR();

    return which;
}

TextureRef Textures::makeTexture(int width, int height, GLint internal_format, GLint format, bool mipmap, void* data) {
    PROFILED_TEXTURE_MUTEX_LOCK 
	unsigned int t = makeTextureInternal(width, height, internal_format, format, mipmap, data);
    PROFILED_TEXTURE_MUTEX_UNLOCK
    TextureRef tex_ref(t);
    DecrementRefCount(t);
	return tex_ref;
}

TextureRef Textures::make3DTexture(int width, int height, int depth, GLint internal_format, GLint format, bool mipmap, void* data) {
    PROFILED_TEXTURE_MUTEX_LOCK
    int dims[3] = {width, height, depth};
    unsigned int t = make3DTextureInternal(dims, internal_format, format, mipmap, data);
    PROFILED_TEXTURE_MUTEX_UNLOCK
    TextureRef tex_ref(t);
    DecrementRefCount(t);
	return tex_ref;
}

unsigned int Textures::makeTextureColorInternal(int width, int height, GLint internal_format, GLint format, float red, float green, float blue, float alpha, bool mipmap) {
    CHECK_GL_ERROR();
    unsigned which = AllocateFreeSlot();

    Texture& texture = textures[which];
    texture.Init();
    texture.width = width;
    texture.height = height;
    texture.internal_format = internal_format;
    texture.format = format;
    texture.no_mipmap = !mipmap;
    texture.tex_target = GL_TEXTURE_2D;

    std::vector<GLubyte> data(width*height*4);

    unsigned char red_byte = (unsigned char)(red*255);
    unsigned char green_byte = (unsigned char)(green*255);
    unsigned char blue_byte = (unsigned char)(blue*255);
    unsigned char alpha_byte = (unsigned char)(alpha*255);

    int index;
    for (int j = 0; j < width; j++) {
        for (int i = 0; i < height; i++) {
            index = (i+j*height)*4;
            data[index+0] = red_byte;
            data[index+1] = green_byte;
            data[index+2] = blue_byte;
            data[index+3] = alpha_byte;
        }
    }

    glGenTextures(1, &textures[which].gl_texture_id);
    LOG_ASSERT(textures[which].gl_texture_id != INVALID_ID);
    LOG_ASSERT(textures[which].gl_texture_id != UNLOADED_ID);

    glBindTexture(texture.tex_target, texture.gl_texture_id);

    unsigned int totalTextureSize = 0;
    if(internal_format == GL_RGBA16F) {
        if(GLAD_GL_ARB_texture_float) {
            glTexImage2D(texture.tex_target, 0, internal_format, width, height, 0,
                        format, GL_FLOAT, &data[0]);
            totalTextureSize += width*height*8;
        } else {
            glTexImage2D(texture.tex_target, 0, GL_RGBA, width, height, 0,
                    format, GL_UNSIGNED_BYTE, &data[0]);
            totalTextureSize += width*height*4;
        }
    } else {
        glTexImage2D(texture.tex_target, 0, internal_format, width, height, 0,
                    format, GL_UNSIGNED_BYTE, &data[0]);
        totalTextureSize += width*height*4;
    }
    if (mipmap) glTexParameteri(texture.tex_target, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    else glTexParameteri(texture.tex_target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(texture.tex_target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(texture.tex_target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(texture.tex_target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    if(mipmap) {
        glGenerateMipmap(texture.tex_target);
        totalTextureSize *= 4;
        totalTextureSize /= 3;
    }

    CHECK_GL_ERROR();
    
    RebindTexture(GetActiveTexture());

    return which;
}

TextureRef Textures::makeTextureColor(int width, int height, GLint internal_format, GLint format, float red, float green, float blue, float alpha, bool mipmap) {
	PROFILED_TEXTURE_MUTEX_LOCK
	unsigned int t = makeTextureColorInternal(width, height, internal_format, format, red, green, blue, alpha, mipmap);
    PROFILED_TEXTURE_MUTEX_UNLOCK
    TextureRef tex_ref(t);
    DecrementRefCount(t);
	return tex_ref;
}

unsigned int Textures::makeTextureTestPatternInternal(int width, int height, GLint internal_format, GLint format, bool mipmap) {
    CHECK_GL_ERROR();
    unsigned which = AllocateFreeSlot();

    Texture& texture = textures[which];
    texture.Init();
    texture.width = width;
    texture.height = height;
    texture.internal_format = internal_format;
    texture.format = format;
    texture.tex_target = GL_TEXTURE_2D;

    std::vector<GLubyte> data(width*height*4);

    int index;
    int multiple = width/32;
    int cornerWidth = multiple*4;
    for (int j = 0; j < width; j++) {
        for (int i = 0; i < height; i++) {
            index = (i+j*width)*4;
            //Color the corners
            if( (i < cornerWidth && j < cornerWidth ) || 
                (i > height-cornerWidth && j > height-cornerWidth) ||
                (i > height-cornerWidth && j < cornerWidth ) ||
                (i < cornerWidth && j > height-cornerWidth) )
            {
                data[index+0] = 255;
            }
            else
            {
                data[index+0] = ((i/multiple+j/multiple)%2)*255;
            }

            data[index+1] = data[index];
            data[index+2] = data[index];
            data[index+3] = 128;
        }
    }

    glGenTextures(1, &texture.gl_texture_id);
    LOG_ASSERT(texture.gl_texture_id != INVALID_ID);
    LOG_ASSERT(texture.gl_texture_id != UNLOADED_ID);

    glBindTexture(texture.tex_target, texture.gl_texture_id);
    if(internal_format == GL_RGBA16F) {
        if(GLAD_GL_ARB_texture_float) {
            glTexImage2D(texture.tex_target, 0, internal_format, width, height, 0,
                        format, GL_FLOAT, &data[0]);
        } else {
            glTexImage2D(texture.tex_target, 0, GL_RGBA, width, height, 0,
                    format, GL_UNSIGNED_BYTE, &data[0]);
        }
    } else {
        glTexImage2D(texture.tex_target, 0, internal_format, width, height, 0,
                    format, GL_UNSIGNED_BYTE, &data[0]);
    }
    if (mipmap) glTexParameteri(texture.tex_target, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    else glTexParameteri(texture.tex_target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(texture.tex_target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(texture.tex_target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(texture.tex_target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    if(mipmap) {
        glGenerateMipmap(texture.tex_target);
    }//GLuint
    CHECK_GL_ERROR();
    
    RebindTexture(GetActiveTexture());

    return which;
}

TextureRef Textures::makeTextureTestPattern(int width, int height, GLint internal_format, GLint format, bool mipmap) {
	PROFILED_TEXTURE_MUTEX_LOCK
	unsigned int t = makeTextureTestPatternInternal(width, height, internal_format, format, mipmap);
    PROFILED_TEXTURE_MUTEX_UNLOCK
    TextureRef tex_ref(t);
    DecrementRefCount(t);
	return tex_ref;
}


unsigned int Textures::makeArrayTextureInternal(unsigned int num_slices, unsigned char flags) {
    unsigned which = AllocateFreeSlot();

    Texture& texture = textures[which];
    texture.Init();
    texture.num_slices = 0;
    texture.sub_textures.reserve(num_slices);
    texture.use_srgb = (flags & PX_SRGB) != 0;
    texture.no_mipmap = (flags & PX_NOMIPMAP)!=0;
    texture.no_reduce = (flags & PX_NOREDUCE)!=0;
    texture.no_live_update = (flags & PX_NOLIVEUPDATE) != 0;;

    texture.wrap_s = m_wrap_s;
    texture.wrap_t = m_wrap_t;
    texture.min_filter = min_filter;
    texture.mag_filter = mag_filter;
    texture.tex_target = GL_TEXTURE_2D_ARRAY;

    return which;
}


unsigned int Textures::makeCubemapArrayTextureInternal(int num_slices, int width, int height, GLint internal_format, GLint format, unsigned char flags) {
    unsigned which = AllocateFreeSlot();

    Texture& texture = textures[which];
    texture.Init();
    texture.num_slices = num_slices;
    texture.cube_map = true;
    texture.width = width;
    texture.height = height;
    texture.tex_target = GL_TEXTURE_CUBE_MAP_ARRAY_ARB;

    texture.use_srgb = (flags & PX_SRGB) != 0;
    texture.no_mipmap = (flags & PX_NOMIPMAP)!=0;
    texture.no_reduce = (flags & PX_NOREDUCE)!=0;

    CHECK_GL_ERROR();
    glGenTextures(1, &texture.gl_texture_id);
    LOG_ASSERT(texture.gl_texture_id != INVALID_ID);
    LOG_ASSERT(texture.gl_texture_id != UNLOADED_ID);

    CHECK_GL_ERROR();
    glBindTexture(texture.tex_target, texture.gl_texture_id);
    if(texture.no_mipmap){
        glTexParameteri(texture.tex_target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    } else {
        glTexParameteri(texture.tex_target, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    }
    glTexParameteri(texture.tex_target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(texture.tex_target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(texture.tex_target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(texture.tex_target, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    CHECK_GL_ERROR();
    int level = 0;
    std::vector<float> pixels(width*height*num_slices*6*4);
    for(int i=0, len=pixels.size(); i<len; i+=16){
        pixels[i+0] = 1.0f;
        pixels[i+1] = 0.0f;
        pixels[i+2] = 0.0f;
        pixels[i+3] = 0.0f;

        pixels[i+4] = 0.0f;
        pixels[i+5] = 1.0f;
        pixels[i+6] = 0.0f;
        pixels[i+7] = 0.0f;

        pixels[i+8] = 0.0f;
        pixels[i+9] = 0.0f;
        pixels[i+10] = 1.0f;
        pixels[i+11] = 0.0f;

        pixels[i+12] = 1.0f;
        pixels[i+13] = 1.0f;
        pixels[i+14] = 1.0f;
        pixels[i+15] = 1.0f;
    }
    CHECK_GL_ERROR();
    glTexImage3D(GL_TEXTURE_CUBE_MAP_ARRAY_ARB, level, internal_format, width, height, num_slices*6, 0, format, GL_FLOAT, &pixels[0]);
    CHECK_GL_ERROR();
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP_ARRAY_ARB);
    CHECK_GL_ERROR();
    return which;
}


unsigned int Textures::makeFlatCubemapArrayTextureInternal(int num_slices, int width, int height, GLint internal_format, GLint format, unsigned char flags) {
    unsigned which = AllocateFreeSlot();

    Texture& texture = textures[which];
    texture.Init();
    texture.num_slices = num_slices;
    texture.width = width*6;
    texture.height = height;
    texture.tex_target = GL_TEXTURE_2D_ARRAY;

    texture.format = format;
    texture.internal_format = internal_format;

    texture.use_srgb = (flags & PX_SRGB) != 0;
    texture.no_mipmap = (flags & PX_NOMIPMAP)!=0;
    texture.no_reduce = (flags & PX_NOREDUCE)!=0;

    CHECK_GL_ERROR();
    glGenTextures(1, &texture.gl_texture_id);
    LOG_ASSERT(texture.gl_texture_id != INVALID_ID);
    LOG_ASSERT(texture.gl_texture_id != UNLOADED_ID);

    CHECK_GL_ERROR();
    glBindTexture(texture.tex_target, texture.gl_texture_id);
    texture.wrap_s = m_wrap_s;
    texture.wrap_t = m_wrap_t;
    texture.min_filter = min_filter;
    texture.mag_filter = mag_filter;
    glTexParameteri(texture.tex_target, GL_TEXTURE_MIN_FILTER, texture.min_filter);
    glTexParameteri(texture.tex_target, GL_TEXTURE_MAG_FILTER, texture.mag_filter);
    glTexParameteri(texture.tex_target, GL_TEXTURE_WRAP_S, texture.wrap_s);
    glTexParameteri(texture.tex_target, GL_TEXTURE_WRAP_T, texture.wrap_t);
    CHECK_GL_ERROR();
    std::vector<float> pixels(width*height*num_slices*6*4, 0.0f);
    glTexImage3D(texture.tex_target, 0, internal_format, width*6, height, num_slices, 0, format, GL_FLOAT, &pixels[0]);
    glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
    CHECK_GL_ERROR();

    RebindTexture(GetActiveTexture());
    return which;
}

TextureRef Textures::getDetailColorArray() {
    PROFILED_TEXTURE_MUTEX_LOCK
    if (!detail_color_texture_ref.valid()) {
        setWrap(GL_REPEAT, GL_REPEAT);
        unsigned int t = makeArrayTextureInternal(0, PX_SRGB);
        PROFILED_TEXTURE_MUTEX_UNLOCK
        detail_color_texture_ref = TextureRef(t);
        DecrementRefCount(t);
        SetTextureName(detail_color_texture_ref, "Detail Texture Array - Color");
    } else {
        PROFILED_TEXTURE_MUTEX_UNLOCK
    }

    return detail_color_texture_ref;
}


TextureRef Textures::getDetailNormalArray() {
    PROFILED_TEXTURE_MUTEX_LOCK
    if (!detail_normal_texture_ref.valid()) {
        setWrap(GL_REPEAT, GL_REPEAT);
        unsigned int t = makeArrayTextureInternal(0, 0);
        PROFILED_TEXTURE_MUTEX_UNLOCK
        detail_normal_texture_ref = TextureRef(t);
        DecrementRefCount(t);
        SetTextureName(detail_normal_texture_ref, "Detail Texture Array - Normals");
    } else {
        PROFILED_TEXTURE_MUTEX_UNLOCK
    }

    return detail_normal_texture_ref;
}

TextureRef Textures::makeArrayTexture(unsigned int num_slices, unsigned char flags) {
	PROFILED_TEXTURE_MUTEX_LOCK
	unsigned int t = makeArrayTextureInternal(num_slices, flags);
    PROFILED_TEXTURE_MUTEX_UNLOCK
    TextureRef tex_ref(t);
    DecrementRefCount(t);
	return tex_ref;
}

TextureRef Textures::makeCubemapArrayTexture(int num_slices, int width, int height, GLint internal_format, GLint format, unsigned char flags) {
    PROFILED_TEXTURE_MUTEX_LOCK
    unsigned int t = makeCubemapArrayTextureInternal(num_slices, width, height, internal_format, format, flags);
    PROFILED_TEXTURE_MUTEX_UNLOCK
    TextureRef tex_ref(t);
    DecrementRefCount(t);
	return tex_ref;
}

TextureRef Textures::makeFlatCubemapArrayTexture(int num_slices, int width, int height, GLint internal_format, GLint format, unsigned char flags) {
    PROFILED_TEXTURE_MUTEX_LOCK
    unsigned int t = makeFlatCubemapArrayTextureInternal(num_slices, width, height, internal_format, format, flags);
    PROFILED_TEXTURE_MUTEX_UNLOCK
    TextureRef tex_ref(t);
    DecrementRefCount(t);
	return tex_ref;
}


bool IsCompressedFile(char* path) {
    int end = strlen(path) - 1;
    if (end >= 3 &&
        path[end - 3] == '.'
        && (
        (path[end - 2] == 'd' &&
         path[end - 1] == 'd' &&
         path[end - 0] == 's')
        ||
        (path[end - 2] == 'c' &&
         path[end - 1] == 'r' &&
         path[end - 0] == 'n')
        ))
    {
        return true;
    }
    return false;
}

int Textures::loadTexture(const std::string& rel_path, unsigned int which, unsigned char flags) {
    static const int kBufSize = 256;
    char buf[kBufSize];
    FormatString(buf, kBufSize, "loadTexture: %s", rel_path.c_str());
    PROFILER_ZONE_DYNAMIC_STRING(g_profiler_ctx, buf);

    // Extract flag bools from bitfield
    //bool no_convert = (flags & PX_NOCONVERT)!=0;
    bool no_mipmap = (flags & PX_NOMIPMAP)!=0;
    bool use_srgb = (flags & PX_SRGB)!=0;
    bool no_live_update = (flags & PX_NOLIVEUPDATE)!=0;
    bool no_reduce = (flags & PX_NOREDUCE)!=0;
    bool no_convert = (flags & PX_NOCONVERT)!=0;

	if(config["no_texture_convert"].toBool() == true){
		no_convert = true;
	}

    // Get absolute path to texture (or compressed equivalent)
    char abs_path[kPathSize];
    PathFlags res_source;
    ModID modsource;
    int err = FindImagePath(rel_path.c_str(), abs_path, kPathSize, kDataPaths | kModPaths | kWriteDir | kAbsPath | kModWriteDirs, true, &res_source, true, true, &modsource );
    if(err == -1){
        //DisplayError("Error", "Could not find texture: %s", rel_path.c_str());
        return kLoadErrorMissingFile;
    }

    // Check if dds file
    bool dds_suffix = IsCompressedFile(abs_path);

    if(!dds_suffix && !no_convert && err == 0 && res_source != kAbsPath){
        std::string dst_path = AssemblePath(GetWritePath(modsource), SanitizePath(rel_path + convert_file_type));
        if(process_pool->NumProcesses() > 0){
            std::ostringstream oss;
            std::string src_complete = abs_path;
            GetCompletePath(&src_complete);
            std::string dst_complete = dst_path;
            GetCompletePath(&dst_complete);
            std::string tmp_complete = GetTempDDSPath(dst_path,true,conversion_num);
            GetCompletePath(&tmp_complete);
            oss << "ConvertTexture \"" << src_complete << "\" \"" << dst_complete << 
                "\" \"" << tmp_complete << "\"";
            ++conversion_num;
            process_pool->Schedule(oss.str());
        } else {
            ConvertImage(abs_path, dst_path, GetTempDDSPath(dst_path,true,conversion_num), TextureData::Fast);
        }

        strncpy(abs_path, dst_path.c_str(), kPathSize-1);
    }

    if(which>=textures.size()){
        textures.resize(which+1);
    }
    Texture& tex = textures[which];

    tex.Init();
    tex.flags = flags;
    tex.sub_textures.resize(1);
    tex.sub_textures[0].texture_name = rel_path;
    tex.sub_textures[0].load_name = abs_path;
    tex.sub_textures[0].modsource = modsource;
    tex.no_mipmap = no_mipmap;
    tex.no_reduce = no_reduce;
    tex.no_live_update = no_live_update;
    tex.sub_textures[0].orig_modified = GetDateModifiedInt64(abs_path);
    tex.sub_textures[0].texture_data = new TextureData();

    // Try loading as .crn
    if (dds_suffix) {
        std::string crn_path(abs_path);
        unsigned int crnlength = crn_path.length();
        crn_path[crnlength - 3] = 'c';
        crn_path[crnlength - 2] = 'r';
        crn_path[crnlength - 1] = 'n';
        if (FileExists(crn_path.c_str(), kDataPaths | kModPaths)) {
            PROFILER_ZONE(g_profiler_ctx, "Loading crunch texture");
            if(!tex.sub_textures[0].texture_data->Load(crn_path.c_str())) {
                //DisplayError("Error", "Failed to load texture: %s", crn_path.c_str());
                return kLoadErrorMissingFile;
            }
        } else {
            PROFILER_ZONE(g_profiler_ctx, "Loading dds texture");
            if (!tex.sub_textures[0].texture_data->Load(abs_path)) {
                //DisplayError("Error", "Failed to load texture: %s", abs_path);
                return kLoadErrorMissingFile;
            }

            ConvertToDXT5IfNeeded(tex.sub_textures[0], modsource);
        }
    } else {
        PROFILER_ZONE(g_profiler_ctx, "Loading uncompressed texture data");
        tex.sub_textures[0].texture_data->Load(abs_path);
    }

    tex.width = tex.sub_textures[0].texture_data->GetWidth();
    tex.height = tex.sub_textures[0].texture_data->GetHeight();
        if(/*Graphics::Instance()->config_.texture_reduce() && */!tex.no_reduce) {
            //int extra_reduce = 1;
            //if(tex.flags & PX_NOCONVERT){
            //    extra_reduce = 0;
            //}
        }
        // TODO: generate mipmaps?

    // Check if width and height are powers of two
    bool pot = IsPow2(tex.width) && IsPow2(tex.height);

    if(!pot && !(tex.no_reduce && tex.no_mipmap)){
        std::ostringstream oss;
        oss << "Dimensions of " << rel_path << " are " << tex.width
            << " x " << tex.height << ", should be powers of two.";
        DisplayError("Warning",oss.str().c_str());
    }    

    tex.wrap_s = m_wrap_s;
    tex.wrap_t = m_wrap_t;
    tex.min_filter = min_filter;
    tex.mag_filter = mag_filter;

    tex.use_srgb = use_srgb;
    return kLoadOk;
}


void Textures::loadArraySlice(const TextureRef& texref, unsigned int slice, const std::string& rel_path) {
    PROFILED_TEXTURE_MUTEX_LOCK

    LOG_ASSERT(texref.valid());

    Texture &tex = textures[texref.id];
    if (tex.gl_texture_id != UNLOADED_ID) {
        tex.width = 0;
        tex.height = 0;
        // adding new slices, need to reupload texture to gl
        // can't free old texture right here, we're not on main thread
        LOGI << "Deferring delete of texture " << tex.gl_texture_id << std::endl;
        deferred_delete_textures.push_back(tex.gl_texture_id);
        tex.gl_texture_id = UNLOADED_ID;
    }

    // cube map arrays not supported yet, maybe not ever since it's GL 4.0 feature
    LOG_ASSERT(!tex.cube_map);

    char abs_path[kPathSize];
    ModID modsource;
    int err = FindImagePath(rel_path.c_str(), abs_path, kPathSize, kDataPaths | kModPaths | kWriteDir | kModWriteDirs, true, NULL, true, true, &modsource);
    if(err == -1){
        FatalError("Error", "Could not find texture: %s", rel_path.c_str());
    }

    bool dds_suffix = IsCompressedFile(abs_path);
    
    std::string dst_path = std::string(abs_path);

    if(!dds_suffix && err == 0){
        dst_path = AssemblePath(GetWritePath(modsource), SanitizePath(rel_path + convert_file_type));
        if(process_pool->NumProcesses() > 0){
            std::ostringstream oss;
            std::string src_complete = abs_path;
            GetCompletePath(&src_complete);
            std::string dst_complete = dst_path;
            GetCompletePath(&dst_complete);
            std::string tmp_complete = GetTempDDSPath(dst_path,true,conversion_num);
            GetCompletePath(&tmp_complete);
            oss << "ConvertTexture \"" << src_complete << "\" \"" << dst_complete << 
                "\" \"" << tmp_complete << "\"";
            ++conversion_num;
            process_pool->Schedule(oss.str());
        } else {
            ConvertImage(abs_path, dst_path, GetTempDDSPath(dst_path,true,conversion_num), TextureData::Fast);
            dds_suffix = true;
        }
        strncpy(abs_path, dst_path.c_str(), kPathSize-1);
    }

    if (slice >= tex.num_slices) {
        tex.num_slices++;
        tex.sub_textures.resize(tex.num_slices);
        LOG_ASSERT(slice < tex.num_slices);
    }
    tex.sub_textures[slice].texture_name = rel_path;
    tex.sub_textures[slice].load_name = abs_path;
    tex.sub_textures[slice].modsource = modsource;
    tex.sub_textures[slice].orig_modified = GetDateModifiedInt64(abs_path);

    const bool kPrintAllSliceLoads = false;

    tex.sub_textures[slice].texture_data = new TextureData();

    // Try loading as .crn
    if (dds_suffix) {
        std::string crn_path(dst_path);
        unsigned int crnlength = crn_path.length();
        crn_path[crnlength - 3] = 'c';
        crn_path[crnlength - 2] = 'r';
        crn_path[crnlength - 1] = 'n';
        if (FileExists(crn_path.c_str(), kDataPaths | kModPaths)) {
            if (!tex.sub_textures[slice].texture_data->Load(crn_path.c_str())) {
                FatalError("Error", "Failed to load texture: %s", crn_path.c_str());
            }
        } else {
            if (!tex.sub_textures[slice].texture_data->Load(dst_path.c_str())) {
                FatalError("Error", "Failed to load texture: %s", dst_path.c_str());
            }

            ConvertToDXT5IfNeeded(tex.sub_textures[slice], modsource);
        }
    } else {
        tex.sub_textures[slice].texture_data->Load(abs_path);
    }

    // TODO: generate mipmaps?
    if(kPrintAllSliceLoads){
        LOGI << "Loading texture array slice " << slice << " from " << abs_path
         << " size " << tex.sub_textures[slice].texture_data->GetWidth()
         << "x" << tex.sub_textures[slice].texture_data->GetHeight()
         << std::endl;
    }

    // first slice we load determines if we're compressed
    if (tex.num_slices == 1) {
        LOG_ASSERT(tex.width == 0);
        LOG_ASSERT(tex.height == 0);

        int width = tex.sub_textures[slice].texture_data->GetWidth();
        int height = tex.sub_textures[slice].texture_data->GetHeight();

        // Check if width and height are powers of two
        bool pot = IsPow2(width) && IsPow2(height);

        if(!pot && !(tex.no_reduce && tex.no_mipmap)){
            std::ostringstream oss;
            oss << "Dimensions of " << rel_path << " are " << width
                << " x " << height << ", should be powers of two.";
            DisplayError("Error",oss.str().c_str());
        }
    }

    PROFILED_TEXTURE_MUTEX_UNLOCK
}


unsigned int Textures::loadArraySlice(const TextureRef& texref, const std::string& rel_path) {
    unsigned int slice;

    {
        PROFILED_TEXTURE_MUTEX_LOCK

        LOG_ASSERT(texref.valid());
        Texture &tex = textures[texref.id];

        // check if it's already loaded
        bool first_slice = tex.sub_textures.empty();
        // tex.compressed is only valid if we already have loaded something
        // and if we're loading the first slice we can't reuse anything anyway
        if (!first_slice) {
            for (slice = 0; slice < tex.sub_textures.size(); slice++) {
                if (rel_path == tex.sub_textures[slice].texture_name) {
                    PROFILED_TEXTURE_MUTEX_UNLOCK
                    return slice;
                    break;
                }
            }
        } else {
            slice = 0;
        }

		PROFILED_TEXTURE_MUTEX_UNLOCK
    }

    loadArraySlice(texref, slice, rel_path);

    return slice;
}


unsigned int Textures::makeRectangularTextureInternal(int width, int height, GLint internal_format, GLint format) {
    CHECK_GL_ERROR();
    unsigned which = AllocateFreeSlot();

    Texture& texture = textures[which];
    texture.Init();
    texture.width = width;
    texture.height = height;
    texture.internal_format = internal_format;
    texture.format = format;
    texture.tex_target = GL_TEXTURE_2D;
    
    glGenTextures(1, &texture.gl_texture_id);
    LOG_ASSERT(texture.gl_texture_id != INVALID_ID);
    LOG_ASSERT(texture.gl_texture_id != UNLOADED_ID);

    CHECK_GL_ERROR();
    glBindTexture(texture.tex_target, texture.gl_texture_id);
    CHECK_GL_ERROR();
    unsigned int totalTextureSize = 0;
    if(internal_format == GL_RGBA16F || internal_format == GL_RGBA32F || internal_format == GL_DEPTH_COMPONENT
        || internal_format == GL_DEPTH_COMPONENT24) {
        if(Graphics::Instance()->features().HDR_enable()) {
            glTexImage2D(texture.tex_target, 0, internal_format, width, height, 0,
                        format, GL_UNSIGNED_BYTE, NULL);
            totalTextureSize += width*height*8;
            int level = 0;
            while(width > 1 || height > 1){
                width = max(1, width/2);
                height = max(1, height/2);
                ++level;
                glTexImage2D(texture.tex_target, level, internal_format, width, height, 0,
                    format, GL_UNSIGNED_BYTE, NULL);
                totalTextureSize += width*height*8;
            }
            CHECK_GL_ERROR();
        } else {
            glTexImage2D(texture.tex_target, 0, GL_RGBA, width, height, 0,
                    format, GL_UNSIGNED_BYTE, NULL);
            totalTextureSize += width*height*4;
            int level = 0;
            while(width > 1 || height > 1){
                width = max(1, width/2);
                height = max(1, height/2);
                ++level;
                glTexImage2D(texture.tex_target, level, GL_RGBA, width, height, 0,
                    format, GL_UNSIGNED_BYTE, NULL);
                totalTextureSize += width*height*4;
            }
            CHECK_GL_ERROR();
        }
    } else {
        glTexImage2D(texture.tex_target, 0, internal_format, width, height, 0,
                    format, GL_UNSIGNED_BYTE, NULL);
        totalTextureSize += width*height*4;
        int level = 0;
        while(width > 1 || height > 1){
            width = max(1, width/2);
            height = max(1, height/2);
            ++level;
            glTexImage2D(texture.tex_target, level, internal_format, width, height, 0,
                format, GL_UNSIGNED_BYTE, NULL);
            totalTextureSize += width*height*4;
        }
        CHECK_GL_ERROR();
    }
    CHECK_GL_ERROR();
    glTexParameteri(texture.tex_target, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    CHECK_GL_ERROR();
    glTexParameteri(texture.tex_target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    CHECK_GL_ERROR();
    glTexParameteri(texture.tex_target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    CHECK_GL_ERROR();
    glTexParameteri(texture.tex_target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    CHECK_GL_ERROR();
    
    CHECK_GL_ERROR();
    glTexParameteri(texture.tex_target, GL_TEXTURE_COMPARE_MODE, GL_NONE);

    CHECK_GL_ERROR();

    texture.wrap_s = GL_CLAMP_TO_EDGE;
    texture.wrap_t = GL_CLAMP_TO_EDGE;
    texture.mag_filter = GL_LINEAR;
    texture.min_filter = GL_LINEAR_MIPMAP_LINEAR;
    
    return which;
}

TextureRef Textures::makeRectangularTexture(int width, int height, GLint internal_format, GLint format) {
	PROFILED_TEXTURE_MUTEX_LOCK
	unsigned int t = makeRectangularTextureInternal(width, height, internal_format, format);
	PROFILED_TEXTURE_MUTEX_UNLOCK
    TextureRef tex_ref(t);
    DecrementRefCount(t);
	return tex_ref;
}

void Textures::SetTextureName(const TextureRef& texture_ref, const char* name) {
    PROFILED_TEXTURE_MUTEX_LOCK
    if( textures[texture_ref.id].notify_on_dispose ) {
        LOGI << "Giving texture with gl texture id " << textures[texture_ref.id].gl_texture_id << " the name " << name << std::endl;
    }

    textures[texture_ref.id].name = name;
    PROFILED_TEXTURE_MUTEX_UNLOCK
}

unsigned int Textures::makeBufferTextureInternal(size_t size, GLint internal_format, const void* data) {
    CHECK_GL_ERROR();
    unsigned which = AllocateFreeSlot();

    Texture& texture = textures[which];
    texture.Init();
    texture.width = 0;
    texture.height = 0;
    texture.size = size;
    texture.internal_format = internal_format;
    texture.tex_target = GL_TEXTURE_BUFFER;

    glGenBuffers(1, &texture.gl_buffer_id);
    CHECK_GL_ERROR();
    glBindBuffer(GL_TEXTURE_BUFFER, texture.gl_buffer_id);
    glBufferData(GL_TEXTURE_BUFFER, size, data, GL_DYNAMIC_DRAW);
    CHECK_GL_ERROR();

    glGenTextures(1, &texture.gl_texture_id);
    LOG_ASSERT(texture.gl_texture_id != INVALID_ID);
    LOG_ASSERT(texture.gl_texture_id != UNLOADED_ID);

    CHECK_GL_ERROR();
    glBindTexture(texture.tex_target, texture.gl_texture_id);
    glTexBuffer(texture.tex_target, internal_format, texture.gl_buffer_id);

    CHECK_GL_ERROR();

    return which;
}

TextureRef Textures::makeBufferTexture(size_t size, GLint internal_format, const void* data) {
    PROFILED_TEXTURE_MUTEX_LOCK
    unsigned int t = makeBufferTextureInternal(size, internal_format, data);
    PROFILED_TEXTURE_MUTEX_UNLOCK
    TextureRef tex_ref(t);
    DecrementRefCount(t);
	return tex_ref;
}

//Return the OpenGL id of a texture
GLuint Textures::returnTexture(const TextureRef &which_texture) {
    PROFILED_TEXTURE_MUTEX_LOCK
    GLuint ret = textures[which_texture.id].gl_texture_id;
    PROFILED_TEXTURE_MUTEX_UNLOCK
	return ret;
}

GLuint Textures::returnTexture(const TextureAssetRef &which_texture) {
    PROFILED_TEXTURE_MUTEX_LOCK
    GLuint ret = textures[which_texture->id].gl_texture_id;
    PROFILED_TEXTURE_MUTEX_UNLOCK
	return ret;
}

int Textures::getWidth(const TextureRef &which) {
    return textures[which.id].width;
}

int Textures::getHeight(const TextureRef &which) {
    return textures[which.id].height;
}

int Textures::getWidth(const TextureAssetRef &which) {
    return textures[which->id].width;
}

int Textures::getHeight(const TextureAssetRef &which) {
    return textures[which->id].height;
}

/** 
 *  @brief Returns the width multiplied with the texture reduction factor if the texture is reduced
 */
int Textures::getReducedWidth(const TextureRef &which){
    return (int) (textures[which.id].width * (textures[which.id].no_reduce ? 1.0f : Graphics::Instance()->config_.texture_reduction_factor()));
}

/** 
 *  @brief Returns the height multiplied with the texture reduction factor if the texture is reduced
 */
int Textures::getReducedHeight(const TextureRef &which) {
    return (int) (textures[which.id].height * (textures[which.id].no_reduce ? 1.0f : Graphics::Instance()->config_.texture_reduction_factor()));
}

/** 
 *  @brief Returns the width multiplied with the texture reduction factor if the texture is reduced
 */
int Textures::getReducedWidth(const TextureAssetRef &which){
    return (int) (textures[which->id].width * (textures[which->id].no_reduce ? 1.0f : Graphics::Instance()->config_.texture_reduction_factor()));
}

/** 
 *  @brief Returns the height multiplied with the texture reduction factor if the texture is reduced
 */
int Textures::getReducedHeight(const TextureAssetRef &which) {
    return (int) (textures[which->id].height * (textures[which->id].no_reduce ? 1.0f : Graphics::Instance()->config_.texture_reduction_factor()));
}

void Textures::EnsureInVRAM(const TextureAssetRef &id) {
    EnsureInVRAM(id->id);    
}

void Textures::EnsureInVRAM(const TextureRef &id) {
    EnsureInVRAM(id.id);
}

void Textures::EnsureInVRAM(int which_texture) {
    PROFILED_TEXTURE_MUTEX_LOCK
    if(textures[which_texture].gl_texture_id == UNLOADED_ID) {
        PROFILED_TEXTURE_MUTEX_UNLOCK
        TextureToVRAM(which_texture);
    } else {
        PROFILED_TEXTURE_MUTEX_UNLOCK
    }
}
            
void Textures::RebindTexture(int unit) {
    if(bound_texture[unit] == INVALID_ID) {
        return;
    }
    Texture &texture = textures[bound_texture[unit]];
    // GL_INVALID_OPERATION raised on first level load without this check
    if( texture.ref_count <= 0 ) {
        LOGE << "Trying to rebind texture with no references " << texture.name << std::endl;
    } else if (texture.gl_texture_id == INVALID_ID || texture.gl_texture_id == UNLOADED_ID) {
        LOGE << "Trying to rebind invalid texture id." << std::endl;
    } else {
        glBindTexture(texture.tex_target, texture.gl_texture_id);
    }
}

void Textures::SetActiveTexture(int which_unit) {
    if(active_texture != which_unit){
        glActiveTexture(GL_TEXTURE0+which_unit);
        active_texture = which_unit;
    }
}

int Textures::GetActiveTexture() {
    return active_texture;
}

void Textures::bindTexture(const TextureAssetRef &which_texture_ref, int which_unit) {
    bindTexture(which_texture_ref->GetTextureRef(), which_unit);
}

void Textures::bindTexture(const TextureRef &which_texture_ref, int which_unit) {
    bindTexture(which_texture_ref.id, which_unit);
}

void Textures::bindTexture(const unsigned int &which_texture, int which_unit) {
    CHECK_GL_ERROR();
    if(which_texture == INVALID_ID) {
        LOGE << "Binding invalid teture ID" << std::endl;
        return;
    }
    CHECK_GL_ERROR();
    //Only bind this texture if it is not already bound, to save on overhead
    PROFILED_TEXTURE_MUTEX_LOCK

    if( textures[which_texture].ref_count <= 0 ) {
        LOGE << "Trying to rebind texture with no references " << textures[which_texture].name << std::endl;
    }

    bool load_to_vram = textures[which_texture].gl_texture_id == UNLOADED_ID;
    if(load_to_vram) {
        PROFILED_TEXTURE_MUTEX_UNLOCK
        TextureToVRAM(which_texture);
        PROFILED_TEXTURE_MUTEX_LOCK
    }
    if(textures[which_texture].gl_texture_id == UNLOADED_ID){
        PROFILED_TEXTURE_MUTEX_UNLOCK
        return;
    }

    SetActiveTexture(which_unit);

    if (bound_texture[which_unit] != INVALID_ID) {
        // if new texture has different target (2d, cube map) than previous
        // clear the previous bind
        GLenum old_tex_target = textures[bound_texture[which_unit]].tex_target;
        if (textures[which_texture].tex_target != old_tex_target) {
            if( old_tex_target != GL_NONE ) {
                glBindTexture(old_tex_target, 0);
            }
        }
    }

    bound_texture[which_unit]=which_texture;
    RebindTexture(which_unit);
    Shaders* shaders = Shaders::Instance();
    if(shaders->bound_program!=-1) {
        shaders->SetUniformInt(shaders->GetTexUniform(which_unit), which_unit);
    }

    CHECK_GL_ERROR();
    PROFILED_TEXTURE_MUTEX_UNLOCK
	return;
}

void Textures::InvalidateBindCacheInternal() {
    SetActiveTexture(1); // To force update in case active_texture this is out of sync too
    SetActiveTexture(0);
    for (unsigned int i=0; i<_texture_units; i++) {
        if (bound_texture[i] != INVALID_ID) {
            SetActiveTexture(i);
            if (textures[bound_texture[i]].cube_map) {
                glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
            } else {
                glBindTexture(GL_TEXTURE_2D, 0);
            }
        bound_texture[i] = INVALID_ID;
        }
    }
}

void Textures::InvalidateBindCache() {
	PROFILED_TEXTURE_MUTEX_LOCK
	InvalidateBindCacheInternal();
	PROFILED_TEXTURE_MUTEX_UNLOCK
}

void Textures::bindBlankTexture(int which_unit) {
    if(!blank_texture_ref.valid()) {
        blank_texture_ref = Engine::Instance()->GetAssetManager()->LoadSync<TextureAsset>("Data/Textures/diffuse.tga", PX_SRGB, 0x0);
    }
    bindTexture(blank_texture_ref->GetTextureRef(), which_unit);
}

void Textures::bindBlankNormalTexture(int which_unit) {
    if(!blank_normal_texture_ref.valid()) {
        blank_normal_texture_ref = Engine::Instance()->GetAssetManager()->LoadSync<TextureAsset>("Data/Textures/diffusebump.tga");
    }
    bindTexture(blank_normal_texture_ref->GetTextureRef(), which_unit);
}

const TextureRef Textures::GetBlankTextureRef() {
    if(!blank_texture_ref.valid()) {
        blank_texture_ref = Engine::Instance()->GetAssetManager()->LoadSync<TextureAsset>("Data/Textures/diffuse.tga", PX_SRGB, 0x0);
    }
    return blank_texture_ref->GetTextureRef();
}

const TextureAssetRef Textures::GetBlankTextureAssetRef() {
    if(!blank_texture_ref.valid()) {
        blank_texture_ref = Engine::Instance()->GetAssetManager()->LoadSync<TextureAsset>("Data/Textures/diffuse.tga", PX_SRGB, 0x0);
    }
    return blank_texture_ref;
}

const TextureRef Textures::GetBlankNormalTextureRef() {
    if(!blank_normal_texture_ref.valid()) {
        blank_normal_texture_ref = Engine::Instance()->GetAssetManager()->LoadSync<TextureAsset>("Data/Textures/diffusebump.tga");
    }
    return blank_normal_texture_ref->GetTextureRef();
}

const TextureAssetRef Textures::GetBlankNormalTextureAssetRef() {
    if(!blank_normal_texture_ref.valid()) {
        blank_normal_texture_ref = Engine::Instance()->GetAssetManager()->LoadSync<TextureAsset>("Data/Textures/diffusebump.tga");
    }
    return blank_normal_texture_ref;
}

bool Textures::IsRenderable(const TextureRef& texture) {
    PROFILED_TEXTURE_MUTEX_LOCK
	bool ret = true;
    if (texture.valid()) {
        const Texture &t = textures[texture.id];
        if (t.sub_textures.size() != 1) {
            ret = false; // array or empty texture
        } else {
            const TextureData* td = t.sub_textures[0].texture_data;
            if (td != NULL) {
                ret = false; // loaded from disk
            }
        }
    } else {
        DisplayError("Error", "Checking if an invalid texture is renderable");
        ret = false;
    }

    PROFILED_TEXTURE_MUTEX_UNLOCK
    return ret;
}

bool Textures::IsCompressed(const TextureAssetRef& texture) {
    return IsCompressed(texture->GetTextureRef());
}

bool Textures::IsCompressed(const TextureRef& texture) {
    PROFILED_TEXTURE_MUTEX_LOCK
	bool ret = false;
    if(texture.valid()){
        const Texture &t = textures[texture.id];
        if (t.sub_textures.empty()) {
            //DisplayError("Error","Checking if an empty texture is compressed");
            ret = false;
        } else {
            const TextureData* td = t.sub_textures[0].texture_data;
            if (td == NULL) {
                // WHY does this happen?
                //DisplayError("Error","Checking if an empty texture is compressed");
                ret = false;
            } else {
                assert(td != NULL);
                ret = td->IsCompressed();
            }
        }
    } else {
        DisplayError("Error","Checking if an invalid texture is compressed");
        ret = false;
    }

    PROFILED_TEXTURE_MUTEX_UNLOCK
    return ret;
}

bool Textures::ReloadAsCompressed(const TextureAssetRef& texref) {
    return ReloadAsCompressed(texref->GetTextureRef());
}

bool Textures::ReloadAsCompressed(const TextureRef& texref) {
    // takes an uncompressed texture
    // saves it in compressed format
    // and replaces the current one with that
    LOG_ASSERT(texref.valid());

    Texture &texture = textures[texref.id];

    LOG_ASSERT(texture.sub_textures.size() == 1);
    LOG_ASSERT(!texture.sub_textures[0].texture_data->IsCompressed());
    std::string newName = texture.sub_textures[0].texture_name + "_converted.dds";
    std::string newPath = AssemblePath(GetWritePath(texture.sub_textures[0].modsource), newName);

    LOGI << "Compressing texture " << texture.sub_textures[0].texture_name << " saving to " << newPath << std::endl;

    CreateParentDirs(newPath.c_str());

    for (unsigned int i = 0; i < texture.sub_textures.size(); i++) {
        TextureData *tex_data = texture.sub_textures[i].texture_data;
        LOG_ASSERT(tex_data != NULL);
        if (!tex_data->HasMipmaps()) {
            tex_data->GenerateMipmaps();
        }
        LOG_ASSERT(tex_data->HasMipmaps());
        tex_data->ConvertDXT(crnlib::PIXEL_FMT_DXT5, TextureData::Fast);
    }

    texture.sub_textures[0].texture_data->SaveDDS(newPath.c_str());

    char abs_path[kPathSize];
    ModID modsource;
    if(FindImagePath(newName.c_str(), abs_path, kPathSize, kWriteDir | kModWriteDirs, true, NULL, true, true, &modsource) == -1) {
        LOGE << "Texture not found after writing it" << std::endl;
        return false;
    }

    RemoveTextureFromVRAM(texref.id);

    texture.sub_textures[0].load_name = abs_path;
    texture.sub_textures[0].modsource = modsource;

    return true;
}

void Textures::SaveToPNG( int texture, std::string path ) {
	GLint width, height;

	glBindTexture(GL_TEXTURE_2D, texture);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width );
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height );

	void* data = OG_MALLOC( sizeof(GLuint) * width * height );
	glGetTexImage( GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8_REV, data );

	ImageExport::SavePNG(path.c_str(), (unsigned char*)data, width, height);

	OG_FREE(data);
}

void Textures::ResetVRAM() {
    for(unsigned int i=0;i<textures.size();i++){
        RemoveTextureFromVRAM(i);
    }
}

void Textures::RemoveTextureFromVRAM( int which ) {
    if(textures[which].gl_texture_id != UNLOADED_ID && textures[which].gl_texture_id != INVALID_ID){
        if(glIsTexture(textures[which].gl_texture_id)){
            glDeleteTextures(1,&textures[which].gl_texture_id);
        }
        textures[which].gl_texture_id = UNLOADED_ID;
    }   

    if(textures[which].gl_buffer_id != UNLOADED_ID && textures[which].gl_buffer_id != INVALID_ID){
        if(glIsBuffer(textures[which].gl_buffer_id)) {
            glDeleteBuffers(1,&textures[which].gl_buffer_id);
        }
        textures[which].gl_buffer_id = UNLOADED_ID;
    }
}

void Textures::ConvertToDXT5IfNeeded(SubTexture& texture, ModID modsource) const {
    switch (texture.texture_data->GetGLInternalFormat()) {
        case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
        case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
        case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
        case GL_COMPRESSED_SRGB_S3TC_DXT1_EXT:
        case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT:
        case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT:
        {
            std::string dst_path = AssemblePath(GetWritePath(modsource), SanitizePath(texture.texture_name + convert_file_type));
            texture.texture_data->ConvertDXT(crnlib::PIXEL_FMT_DXT5, TextureData::Nice);

            CreateParentDirs(dst_path);

            texture.texture_data->SaveDDS(dst_path.c_str());

            LOGW << texture.texture_name << " is not a DXT5 file, and will be converted" << std::endl;
        }
    }
}

Textures::~Textures() {
    LOGI << "Waiting for process pool tasks to complete..." << std::endl;
    process_pool->ClearQueuedTasks();
    process_pool->WaitForTasksToComplete();
    delete process_pool;
}

void Textures::SetProcessPoolsEnabled( bool val ) {
    if(val){
        LOGW << "Process pools feature disabled" << std::endl;
    } else {
        process_pool->Resize(0);
    }
}

void Textures::ApplyAnisotropy() {
    for (unsigned int i=0;i<textures.size();i++){
        bindTexture(i,0);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, Graphics::Instance()->config_.anisotropy());
    }
}

void Textures::updateBufferTexture(TextureRef& texture_ref, size_t size, const void* data) {
    PROFILER_ZONE(g_profiler_ctx, "updateBufferTexture");
    if (!texture_ref.valid()) {
        return;
    }
    Texture &texture = textures[texture_ref.id];
    LOG_ASSERT(texture.gl_texture_id != 0);
    LOG_ASSERT(texture.gl_buffer_id != 0);
    LOG_ASSERT(texture.tex_target == GL_TEXTURE_BUFFER);
    glBindBuffer(GL_TEXTURE_BUFFER, texture.gl_buffer_id);
    if (size > texture.size) {
        PROFILER_ZONE(g_profiler_ctx, "Resize buffer");
        // resize the buffer
        glBufferData(GL_TEXTURE_BUFFER, size, NULL, GL_DYNAMIC_DRAW);
        texture.size = size;
    } else {
        PROFILER_ZONE(g_profiler_ctx, "Discard? buffer");
        glBufferData(GL_TEXTURE_BUFFER, texture.size, NULL, GL_DYNAMIC_DRAW);
        glBindBuffer(GL_TEXTURE_BUFFER, texture.gl_buffer_id);
    }
    const bool kUseMapRange = false;
    if(kUseMapRange) {
        PROFILER_ZONE(g_profiler_ctx, "glMapBufferRange");
        //Again, i don't believe that using unsynchronized here is necessarily safe, can we guarantee this?
        //same as in vbocontainer
        void* mapped = glMapBufferRange(GL_TEXTURE_BUFFER, 0, size, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT | GL_MAP_INVALIDATE_RANGE_BIT | GL_MAP_UNSYNCHRONIZED_BIT );
        //void* mapped = glMapBufferRange(GL_TEXTURE_BUFFER, 0, size, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT | GL_MAP_INVALIDATE_RANGE_BIT );
        CHECK_GL_ERROR();
        {
            PROFILER_ZONE(g_profiler_ctx, "memcpy");
            memcpy(mapped, data, size);
        }
        glUnmapBuffer(GL_TEXTURE_BUFFER);
    } else {
        PROFILER_ZONE(g_profiler_ctx, "glBufferSubData");    
        glBufferSubData(GL_TEXTURE_BUFFER, 0, size, data);
    }
}

