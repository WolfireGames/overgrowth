//-----------------------------------------------------------------------------
//           Name: textures.h
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
#pragma once

#include <Math/vec3.h>
#include <Math/vec4.h>

#include <Asset/Asset/texture.h>
#include <Graphics/textureref.h>
#include <Images/ddsformat.hpp>
#include <Internal/modid.h>

#include <opengl.h>

#include <memory.h>
#include <cstdlib>
#include <climits>
#include <vector>
#include <mutex>

#define TEX_SCREEN_DEPTH      1
#define TEX_NOISE             2
#define TEX_PURE_NOISE        3

#define TEX_TONE_MAPPED       2
#define TEX_INTERMEDIATE      3

#define TEX_COLOR             0
#define TEX_NORMAL            1
#define TEX_SPEC_CUBEMAP      2
#define TEX_SHADOW            4
#define TEX_PROJECTED_SHADOW  5
#define TEX_TRANSLUCENCY      5
#define TEX_BLOOD             6
#define TEX_FUR               7
#define TEX_TINT_MAP          8

#define TEX_DECAL_NORMAL      9
#define TEX_DECAL_COLOR       10

#define TEX_LIGHT_DECAL_DATA_BUFFER 15
#define TEX_CLUSTER_BUFFER    13

#define TEX_AMBIENT_GRID_DATA      11
#define TEX_AMBIENT_COLOR_BUFFER   12


//-----------------------------------------------------------------------------
// Class Definition
//-----------------------------------------------------------------------------
namespace PhoenixTextures {
    const unsigned int INVALID_ID = UINT_MAX;
    const unsigned int UNLOADED_ID = UINT_MAX-1;
    const unsigned int _texture_units = 32;

    const int _default_wrap = GL_REPEAT;
    const int _default_min = GL_LINEAR_MIPMAP_LINEAR;
    const int _default_max = GL_LINEAR;
}

class TextureData;

struct SubTexture {
    TextureData* texture_data;
    std::string texture_name;
    std::string load_name;  
    ModID modsource;
    int64_t orig_modified;
    int64_t load_modified;  
    SubTexture():
        texture_data(NULL)
      , orig_modified(0)
      , load_modified(0)
    {}
};

struct Texture {
    bool notify_on_dispose;
    std::string name;
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
};

inline std::ostream& operator<<(std::ostream& out, const Texture& tex )
{
    out << "(w:" << tex.width << ",h:" << tex.height << ",{size:" << tex.size << "})";
    for( size_t i = 0; i < tex.sub_textures.size(); i++ )
    {
        out << tex.sub_textures[i].texture_name << ":";
    }
    return out;
}

enum TextureSaveFlags{
    _SAV_ALPHA = (1<<0),
    _SAV_ALREADY_USR = (1<<1)
};

enum SampleFilter {
    FILTER_NEAREST,
    FILTER_LINEAR
};

enum SampleWrap {
    WRAP_CLAMP_TO_EDGE
};

class ProcessPool;

class Textures {
	public:
			enum MipmapParam { MIPMAPS, NO_MIPMAPS };

            void DrawImGuiDebug();
private:
            ProcessPool *process_pool; // For background compression of textures
            std::vector<Texture> textures;
            std::vector<unsigned int> free_spaces; // Unused texture slots in texture array
            std::mutex texture_mutex;
            GLuint m_wrap_t, m_wrap_s, min_filter, mag_filter; // These settings are applied to the next texture that is loaded or created
            unsigned int bound_texture[PhoenixTextures::_texture_units]; // Shadow GL texture units
            TextureAssetRef blank_texture_ref;
            TextureAssetRef blank_normal_texture_ref;

            TextureRef detail_color_texture_ref;
            TextureRef detail_normal_texture_ref;

            std::list<GLuint> deferred_delete_textures;

            int active_texture;

            std::string convert_file_type;

            //Default texture settings, tiling and mipmapped
            Textures();
            ~Textures();

            friend class TextureAsset;
            int AllocateFreeSlot();

			void InvalidateBindCacheInternal();
			void DisposeTexture(int which, bool free_space = true);
			void ReloadInternal();
			void TextureToVRAM(unsigned int which);
			void RebindTexture(int which_unit);
            int loadTexture(const std::string& name, unsigned int which, unsigned char flags);
            unsigned int makeCubemapArrayTextureInternal(int num_slices, int width, int height, GLint internal_format, GLint format, unsigned char flags);
            unsigned int makeFlatCubemapArrayTextureInternal(int num_slices, int width, int height, GLint internal_format, GLint format, unsigned char flags);
            unsigned int makeArrayTextureInternal(unsigned int num_slices, unsigned char flags = 0);
			unsigned int makeRectangularTextureInternal(int width, int height, GLint internal_format = GL_RGBA, GLint format = GL_RGBA);
			unsigned int makeTextureInternal(int width, int height, GLint internal_format = GL_RGBA, GLint format = GL_RGBA, bool mipmap = false, void* data = NULL);
			unsigned int makeTextureInternal(const TextureData &texture_data);
            unsigned int make3DTextureInternal(int* dims, GLint internal_format, GLint format, bool mipmap, void* data);
            unsigned int makeTextureColorInternal(int width, int height, GLint internal_format = GL_RGBA, GLint format = GL_RGBA, float red = 1.0f, float green = 1.0f, float blue = 1.0f, float alpha = 1.0f, bool mipmap = false);
			unsigned int makeTextureTestPatternInternal(int width, int height, GLint internal_format = GL_RGBA, GLint format = GL_RGBA, bool mipmap = false);
			unsigned int makeCubemapTextureInternal(int width, int height, GLint internal_format, GLint format, MipmapParam mipmap_param);
            unsigned int makeBufferTextureInternal(size_t size, GLint internal_format, const void* data = NULL);
			void SetActiveTexture(int which_unit);
			void RemoveTextureFromVRAM(int which);
            void ConvertToDXT5IfNeeded(SubTexture& texture, ModID modsource) const;
    public:
            void IncrementRefCount(unsigned int id);
            void DecrementRefCount(unsigned int id);
            static Textures* Instance()
            {
                static Textures instance;
                return &instance;
            }

            static void SaveCubeMapMipmapsHDR (TextureRef which, const char* filename, int width);
            static TextureRef LoadCubeMapMipmapsHDR (const char* filename);
            void FindTexturePath(const std::string& rel_path, char* resulting_path, bool* is_dds );
			void SaveToPNG( int texture, std::string path );
            void InvalidateBindCache();
            void Dispose();
            void Reload();
            void EnsureInVRAM(const TextureAssetRef &id);
            void EnsureInVRAM(const TextureRef &id);
            void EnsureInVRAM(int id);
            void drawTexture(TextureRef id, vec3 where, float size, float rotation = 0);
            void setWrap(GLenum wrap);
            void setWrap(GLenum wrap_s, GLenum wrap_t);
            void setFilters(GLenum min, GLenum mag);
            void bindTexture(const TextureAssetRef &tex_ref, int which_unit=0);
            void bindTexture(const TextureRef &tex_ref, int which_unit=0);
            void bindTexture(const unsigned int &tex_ref, int which_unit=0);
            void bindBlankTexture(int which_unit=0);
            void bindBlankNormalTexture(int which_unit=0);
            void GenerateMipmap(const TextureRef &tex_ref);
            void setSampleFilter( const TextureRef &tex_ref, enum SampleFilter filter );
            void setSampleWrap( const TextureRef &tex_ref, enum SampleWrap wrapping );
            void subImage( const TextureRef &tex_ref, void* data );
            void subImage( const TextureRef &destination, const TextureRef &source, int xoffset, int yoffset  );

            TextureRef getDetailColorArray();
            TextureRef getDetailNormalArray();

            std::string generateTextureName( const char* pre );

            TextureRef makeArrayTexture(unsigned int num_slices, unsigned char flags = 0);
            TextureRef makeCubemapArrayTexture(int num_slices, int width, int height, GLint internal_format, GLint format, unsigned char flags);
            TextureRef makeFlatCubemapArrayTexture(int num_slices, int width, int height, GLint internal_format, GLint format, unsigned char flags);
            TextureRef makeRectangularTexture(int width, int height, GLint internal_format=GL_RGBA, GLint format=GL_RGBA);
            TextureRef makeTexture(int width, int height, GLint internal_format=GL_RGBA, GLint format=GL_RGBA, bool mipmap = false, void* data = NULL);
            TextureRef makeTexture(const TextureData &texture_data);
            TextureRef make3DTexture(int width, int height, int depth, GLint internal_format, GLint format, bool mipmap, void* data);
            TextureRef makeTextureColor(int width, int height, GLint internal_format=GL_RGBA, GLint format=GL_RGBA, float red = 1.0f, float green = 1.0f, float blue = 1.0f, float alpha = 1.0f, bool mipmap = false);
            TextureRef makeTextureTestPattern(int width, int height, GLint internal_format=GL_RGBA, GLint format=GL_RGBA, bool mipmap = false);
            TextureRef makeCubemapTexture(int width, int height, GLint internal_format, GLint format, MipmapParam mipmap_param);
            TextureRef makeBufferTexture(size_t size, GLint internal_format, const void* data = NULL);
            void SetTextureName(const TextureRef& texture_ref, const char* name);

            void DeleteUnusedTextures();
            GLuint returnTexture(const TextureRef &which);
            GLuint returnTexture(const TextureAssetRef &which);
            
            //TextureRef returnTextureRefFullDetail(const std::string& name, unsigned char flags = 0);
            //TextureRef returnTextureRef(const std::string& name, bool no_convert = false, bool no_mipmap = false, bool srgb = false);
            //TextureRef returnTextureRef(const std::string& name, unsigned char flags = 0);
            
            int getWidth(const TextureRef &which);
            int getHeight(const TextureRef &which);

            int getWidth(const TextureAssetRef &which);
            int getHeight(const TextureAssetRef &which);

            int getReducedWidth(const TextureRef &which);
            int getReducedHeight(const TextureRef &which);

            int getReducedWidth(const TextureAssetRef &which);
            int getReducedHeight(const TextureAssetRef &which);

            const TextureRef GetBlankTextureRef();
            const TextureAssetRef GetBlankTextureAssetRef();
            const TextureRef GetBlankNormalTextureRef();
            const TextureAssetRef GetBlankNormalTextureAssetRef();

            bool IsRenderable(const TextureRef& texture);

            bool IsCompressed(const TextureRef& texture);
            bool IsCompressed(const TextureAssetRef& texture);
            bool ReloadAsCompressed(const TextureRef& texref);
            bool ReloadAsCompressed(const TextureAssetRef& texref);

            void loadArraySlice(const TextureRef& texref, unsigned int slice, const std::string& name);
            unsigned int loadArraySlice(const TextureRef& texref, const std::string& name);

            int GetActiveTexture();
            void ResetVRAM();

#ifdef TRACK_TEXTURE_REF
            void AddRefPtr(unsigned int id, TextureRef* ref);
            void RemoveRefPtr(unsigned int id, TextureRef* ref);
#endif
            void SetProcessPoolsEnabled( bool val );
            void ApplyAnisotropy();

            void updateBufferTexture(TextureRef& texture, size_t size, const void* data);
};
