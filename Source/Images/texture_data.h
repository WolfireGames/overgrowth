//-----------------------------------------------------------------------------
//           Name: texture_data.h
//      Developer: Wolfire Games LLC
//         Author: Phillip Isola
//    Description: Generic storage texture image data.
//                 Reading and writing from/to various image formats.
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
#pragma once

#include <Wrappers/crn.h>

#include <opengl.h>

#include <ostream>
#include <vector>

class StackAllocator;

class TextureData {
   public:
    enum ColorSpace {
        Linear,
        sRGB
    };

    enum ConversionQuality {
        Nice,
        Fast
    };

    TextureData() : m_colorSpace(Linear),
                    is_loaded(false),
                    height(0),
                    width(0),
                    num_faces(0),
                    mip_levels(0),
                    gl_base_format(GL_NONE),
                    gl_internal_format(GL_NONE),
                    gl_type(GL_NONE) {}

    bool IsCube() const;
    bool IsCompressed() const;
    bool HasMipmaps() const;
    unsigned int GetWidth() const;
    unsigned int GetHeight() const;
    unsigned int GetMipWidth(unsigned int mip) const;
    unsigned int GetMipHeight(unsigned int mip) const;
    unsigned int GetNumFaces() const;
    unsigned int GetMipLevels() const;
    GLenum GetGLBaseFormat() const;  // for glTexImage2D, may only be called on uncompressed textures
    GLenum GetGLInternalFormat() const;
    GLenum GetGLType() const;  // for glTexImage2D, may only be called on uncompressed textures
    ColorSpace GetColorSpace() const { return m_colorSpace; }
    unsigned int GetMipDataSize(unsigned int face, unsigned int mip) const;
    const char* GetMipData(unsigned int face, unsigned int mip) const;
    void GetUncompressedData(unsigned char* data);
    void SetColorSpace(ColorSpace color_space);

    bool Load(const char* abs_path);
    bool EnsureInRAM();

    bool GenerateMipmaps();
    bool ConvertDXT(crnlib::pixel_format format, ConversionQuality quality);
    bool SaveDDS(const char* abs_path);
    bool SaveCRN(const char* abs_path, crn_format format, ConversionQuality quality);

    bool IsInRAM();
    void UnloadData();

    inline std::string GetPath() { return source_path; }

   private:
    std::string source_path;

    void ExtractMetaData();

    bool is_loaded;
    bool is_cube;
    bool is_packed;
    unsigned int width, height;
    std::vector<int> mip_widths;
    std::vector<int> mip_heights;
    std::vector<int> mip_data_sizes;
    int num_faces, mip_levels;
    GLenum gl_base_format, gl_internal_format, gl_type;
    int mip_data_size;

    crnlib::mipmapped_texture m_crnTex;
    ColorSpace m_colorSpace;
};

inline std::ostream& operator<<(std::ostream& out, const TextureData& td) {
    unsigned int totalBytes = td.GetWidth() * td.GetHeight() * 32 / 8;
    out << "TextureData(w:" << td.GetWidth() << ",h:" << td.GetHeight() << ",size:" << totalBytes << ")";
    return out;
}

bool IsPow2(int v);
