//-----------------------------------------------------------------------------
//           Name: texture_data.cpp
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
#include "texture_data.h"

#include <Images/stbimage_wrapper.h>
#include <Images/image_export.hpp>

#include <Internal/common.h>
#include <Internal/datemodified.h>
#include <Internal/error.h>
#include <Internal/filesystem.h>
#include <Internal/profiler.h>

#include <Compat/fileio.h>
#include <Memory/allocation.h>
#include <Logging/logdata.h>
#include <Utility/assert.h>

#include <cstdlib>
#include <cstring>
#include <cmath>

using std::endl;
using std::min;
using std::string;
// TODO: avoid "using namespace" if possible
using namespace crnlib;

// TODO: this could be used elsewhere
// From http://graphics.stanford.edu/~seander/bithacks.html#DetermineIfPowerOf2
bool IsPow2(int v) {
    return (v & (v - 1)) == 0;
}

bool TextureData::IsCube() const {
    return is_cube;
}

bool TextureData::IsCompressed() const {
    return is_packed;
}

bool TextureData::HasMipmaps() const {
    if (mip_levels == 1) {
        return false;
    } else {
        return true;
    }
}

unsigned int TextureData::GetWidth() const {
    return width;
}

unsigned int TextureData::GetHeight() const {
    return height;
}

unsigned int TextureData::GetMipWidth(unsigned int mip) const {
    const unsigned face = 0;
    const unsigned index = face * mip_levels + mip;
    if (index < mip_widths.size()) {
        return mip_widths[index];
    } else {
        return 0;
    }
}

unsigned int TextureData::GetMipHeight(unsigned int mip) const {
    const unsigned face = 0;
    const unsigned index = face * mip_levels + mip;
    if (index < mip_heights.size()) {
        return mip_heights[index];
    } else {
        return 0;
    }
}

unsigned int TextureData::GetNumFaces() const {
    return num_faces;
}

unsigned int TextureData::GetMipLevels() const {
    return mip_levels;
}

GLenum TextureData::GetGLBaseFormat() const {
    return gl_base_format;
}

GLenum TextureData::GetGLInternalFormat() const {
    return gl_internal_format;
}

GLenum TextureData::GetGLType() const {
    return gl_type;
}

unsigned int TextureData::GetMipDataSize(unsigned int face, unsigned int mip) const {
    const unsigned index = face * mip_levels + mip;
    if (index < mip_data_sizes.size()) {
        return mip_data_sizes[index];
    } else {
        return 0;
    }
}

const char *TextureData::GetMipData(unsigned int face, unsigned int mip) const {
    if (is_loaded) {
        const mip_level *level = m_crnTex.get_level(face, mip);

        if (level->is_packed()) {
            const dxt_image *dxt = level->get_dxt_image();
            assert(dxt != NULL);
            return reinterpret_cast<const char *>(dxt->get_element_ptr());
        } else {
            const image_u8 *img = level->get_image();
            assert(img != NULL);
            return reinterpret_cast<const char *>(img->get_pixels());
        }
    } else {
        LOGE << "Unable to get MipData from texturedata, data is unloaded " << source_path << endl;
        return NULL;
    }
}

bool TextureData::Load(const char *abs_path) {
    is_loaded = false;
    m_crnTex.clear();

    source_path = abs_path;

    texture_file_types::format src_file_format = texture_file_types::determine_file_format(abs_path);
    if (src_file_format == texture_file_types::cFormatInvalid) {
        LOGE << "Unrecognized file type: " << abs_path << endl;
        return false;
    }

    {
        PROFILER_ENTER(g_profiler_ctx, "TextureData::Load actual file loading");
        FILE *pFile = my_fopen(abs_path, "rb");
        if (pFile) {
            // obtain file size:
            fseek(pFile, 0, SEEK_END);
            int lSize = ftell(pFile);
            rewind(pFile);

            // allocate memory to contain the whole file:
            char *buffer = (char *)alloc.stack.Alloc(sizeof(char) * lSize);

            const int kBufSize = 512;
            char error_msg[kBufSize];
#ifndef NO_ERR
            if (buffer == NULL) {
                FormatString(error_msg, kBufSize, "Could not allocate memory to checksum: %s.", abs_path);
                FatalError("Error", error_msg);
            }
#endif

            // copy the file into the buffer:
            size_t result = fread(buffer, 1, lSize, pFile);
#ifndef NO_ERR
            if (result != (size_t)lSize) {
                FormatString(error_msg, kBufSize, "Could not read data from file: %s.", abs_path);
                FatalError("Error", error_msg);
            }
#endif

            crnlib::buffer_stream buf_stream(buffer, lSize);
            buf_stream.set_name(abs_path);
            data_stream_serializer serializer(buf_stream);

            // terminate
            fclose(pFile);
            PROFILER_LEAVE(g_profiler_ctx);

            {
                PROFILER_ZONE(g_profiler_ctx, "TextureData::Load read_from_stream");
                if (!m_crnTex.read_from_stream(serializer, src_file_format)) {
                    if (m_crnTex.get_last_error().is_empty()) {
                        string contents = "Failed reading source file: " + string(abs_path);
                        LOGE << contents << endl;
                    } else {
                        LOGE << m_crnTex.get_last_error().get_ptr() << endl;
                    }
                    alloc.stack.Free(buffer);
                    return false;
                }
            }

            alloc.stack.Free(buffer);
        } else {
            LOGE << "fopen on texture file path: " << abs_path << " failed to open" << endl;
            return false;
        }
    }

    // try to determine color space
    string src(abs_path);
    for (char &i : src) {
        i = tolower(i);
    }
    if (src.rfind("_c.") != string::npos) {
        m_colorSpace = TextureData::sRGB;
    } else if (src.rfind("_color.") != string::npos) {
        m_colorSpace = TextureData::sRGB;
    } else if (src.rfind("_n.") != string::npos) {
        // linear
    } else if (src.rfind("_normal.") != string::npos) {
        // linear
    } else if (src.rfind("_norm.") != string::npos) {
        // linear
    } else {
        // LOGW << "File " << abs_path << " does not specify color space" << endl;
    }

    // vvv commented out the non-flipping exception because it was messing up the spawner thumbnails -David

    // for whatever reason uncompressed non-pow2 textures must not be flipped
    // if (m_crnTex.is_packed() || (IsPow2(m_crnTex.get_width()) && IsPow2(m_crnTex.get_height()))) {
    // need to flip it, apparently crunch does something different than nvImage
    /*{
        PROFILER_ZONE(g_profiler_ctx, "TextureData::Load flip_y");
        m_crnTex.flip_y(true);
    }*/
    //}

    if (!m_crnTex.is_packed()) {
        pixel_format format = m_crnTex.get_format();
        switch (format) {
            case PIXEL_FMT_A8R8G8B8:
                break;  // nothing to do

            default: {
                PROFILER_ZONE(g_profiler_ctx, "TextureData::Load m_crnTex.convert");
                dxt_image::pack_params p;
                m_crnTex.convert(PIXEL_FMT_A8R8G8B8, true, p);
            } break;
        }
    }

    is_loaded = true;
    ExtractMetaData();
    return true;
}

bool TextureData::EnsureInRAM() {
    if (!is_loaded) {
        LOGW << "Reloading " << source_path << " into ram from disk, it's needed again" << endl;
        return Load(source_path.c_str());
    }
    return true;
}

void TextureData::GetUncompressedData(unsigned char *data) {
    if (is_loaded) {
        pixel_format format = m_crnTex.get_format();
        // In both `terrain.cpp` and `textures.cpp`, both uses of this function imply that the only valid data format
        // expected is BGRA_8. Otherwise, one would expect the function signature to also return a `numBits` or similar.
        // Therefore, I'll proclaim `imageBits` obsolete, and comment out the lesser bit depths and the conditional.
        //int imageBits = 32;

        //int bytesPerPixel = imageBits / 8;
        int imageWidth = m_crnTex.get_width();
        int imageHeight = m_crnTex.get_height();

        //int heightDataSize = imageWidth * imageHeight;
        //int imageDataSize = heightDataSize * bytesPerPixel;

        image_u8 image;
        image_u8 *pImg = m_crnTex.get_level_image(0, 0, image);

        //if (imageBits == 8) {
        //    for (int y = 0; y < imageHeight; y++) {
        //        color_quad_u8 *bits = pImg->get_scanline(y);
        //        for (int x = 0; x < imageWidth; x++) {
        //            int curr_index = x + y * imageWidth;
        //            data[curr_index] = bits[x].a;
        //        }
        //    }
        //} else if (imageBits == 24) {
        //    for (int y = 0; y < imageHeight; y++) {
        //        color_quad_u8 *bits = pImg->get_scanline(y);
        //        for (int x = 0; x < imageWidth; x++) {
        //            int curr_index = x + y * imageWidth;
        //            data[4 * curr_index] = bits[x].b;
        //            data[4 * curr_index + 1] = bits[x].g;
        //            data[4 * curr_index + 2] = bits[x].r;
        //            data[4 * curr_index + 3] = 255;
        //        }
        //    }
        //} else if (imageBits == 32) {

        for (int y = 0; y < imageHeight; y++) {
            color_quad_u8 *bits = pImg->get_scanline(y);
            for (int x = 0; x < imageWidth; x++) {
                int curr_index = x + y * imageWidth;
                data[4 * curr_index] = bits[x].b;
                data[4 * curr_index + 1] = bits[x].g;
                data[4 * curr_index + 2] = bits[x].r;
                data[4 * curr_index + 3] = bits[x].a;
            }
        }
        
        // TODO: what is this?
        /*
        } else if (m_nImageBits == 96) {
            for(int y = 0; y < m_nImageHeight; y++) {
            BYTE *bits = FreeImage_GetScanLine(image, y);
            for(int x = 0; x < m_nImageWidth; x++) {
            float pixelf[3];
            for(int i=0; i<3; ++i){
            pixelf[i] = *((float*)(&(bits[i*4])));
            pixelf[i] = pow(pixelf[i], 1.0f/2.2f);
            }
            int curr_index = x + y*m_nImageWidth;
            m_nImageData[4*curr_index] = (unsigned char)(min(1.0f, pixelf[2]) * 255.0f);
            m_nImageData[4*curr_index+1] = (unsigned char)(min(1.0f, pixelf[1]) * 255.0f);
            m_nImageData[4*curr_index+2] = (unsigned char)(min(1.0f, pixelf[0]) * 255.0f);
            m_nImageData[4*curr_index+3] = 255;
            bits += bytesPerPixel;
        }
        */
    } else {
        LOGE << "Unable to load LoadUncompressedData from texturedata, data is unloaded " << endl;
    }
}

void TextureData::SetColorSpace(ColorSpace color_space) {
    m_colorSpace = color_space;
    ExtractMetaData();
}

bool TextureData::GenerateMipmaps() {
    bool result = false;
    if (is_loaded) {
        mipmapped_texture::generate_mipmap_params mipParams;
        // TODO: set parameters
        result = m_crnTex.generate_mipmaps(mipParams, false);

        ExtractMetaData();
    } else {
        LOGE << "Unable to GenerateMipmaps for texturedata, data unloaded " << source_path << endl;
    }

    return result;
}

bool TextureData::ConvertDXT(pixel_format format, ConversionQuality quality) {
    bool ret_val = false;

    if (is_loaded) {
        dxt_image::pack_params packParams;
        if (quality == Nice) {
            packParams.m_quality = cCRNDXTQualityUber;
            packParams.m_compressor = cCRNDXTCompressorCRN;
        } else {
            packParams.m_quality = cCRNDXTQualitySuperFast;
            packParams.m_compressor = cCRNDXTCompressorRYG;
        }
        // for whatever reason non-pow2 textures must be flipped
        if (!IsPow2(m_crnTex.get_width()) || !IsPow2(m_crnTex.get_height())) {
            m_crnTex.flip_y(true);
            ret_val = m_crnTex.convert(format, packParams);
            m_crnTex.flip_y(true);
        } else {
            ret_val = m_crnTex.convert(format, packParams);
        }

        ExtractMetaData();
    } else {
        LOGE << "Can't ConvertDXT to texturedata, data is unloaded " << source_path << endl;
    }

    return ret_val;
}

bool TextureData::SaveDDS(const char *abs_path) {
    bool result = false;
    if (is_loaded) {
        result = m_crnTex.write_to_file(abs_path, texture_file_types::cFormatDDS);
        if (!result) {
            LOGE << m_crnTex.get_last_error().get_ptr() << endl;
        }
    } else {
        LOGE << "Can't save texturedata as DDS to " << abs_path << " data is unloaded " << source_path << endl;
    }
    return result;
}

bool TextureData::SaveCRN(const char *abs_path, crn_format format, ConversionQuality quality) {
    bool result = false;
    if (is_loaded) {
        crn_comp_params params;
        params.m_format = format;
        if (quality == Nice) {
            params.m_quality_level = cCRNMaxQualityLevel;
            params.m_dxt_quality = cCRNDXTQualityUber;
        } else {
            params.m_quality_level = cCRNMinQualityLevel;
            params.m_dxt_quality = cCRNDXTQualitySuperFast;
        }
        result = m_crnTex.write_to_file(abs_path, texture_file_types::cFormatCRN, &params);
    } else {
        LOGE << "Trying to run SaveCRN on unloaded texturedata object " << endl;
    }
    return result;
}

bool TextureData::IsInRAM() {
    return is_loaded;
}

void TextureData::UnloadData() {
    m_crnTex.clear();
    is_loaded = false;
}

void TextureData::ExtractMetaData() {
    is_packed = m_crnTex.is_packed();
    is_cube = (m_crnTex.get_num_faces() == 6);

    width = m_crnTex.get_width();
    height = m_crnTex.get_height();

    mip_levels = m_crnTex.get_num_levels();
    num_faces = m_crnTex.get_num_faces();

    mip_widths.clear();
    mip_heights.clear();
    mip_data_sizes.clear();

    for (int k = 0; k < num_faces; k++) {
        for (int i = 0; i < mip_levels; i++) {
            const mip_level *level = m_crnTex.get_level(k, i);
            mip_widths.push_back(level->get_width());
            mip_heights.push_back(level->get_height());
            if (level->is_packed()) {
                const dxt_image *dxt = level->get_dxt_image();
                LOG_ASSERT(dxt != NULL);
                mip_data_sizes.push_back(dxt->get_size_in_bytes());
            } else {
                const image_u8 *img = level->get_image();
                assert(img != NULL);
                mip_data_sizes.push_back(img->get_size_in_bytes());
            }
        }
    }

    switch (m_crnTex.get_format()) {
        case crnlib::PIXEL_FMT_DXT1:
        case crnlib::PIXEL_FMT_DXT1A:
        case crnlib::PIXEL_FMT_DXT2:
        case crnlib::PIXEL_FMT_DXT3:
        case crnlib::PIXEL_FMT_DXT4:
        case crnlib::PIXEL_FMT_DXT5:
            // LOGE << "Unsupported pixel format in texture " << source_path << endl;
            gl_base_format = GL_NONE;
            break;

        case crnlib::PIXEL_FMT_R8G8B8:
            gl_base_format = GL_RGB;
            break;

        case crnlib::PIXEL_FMT_A8R8G8B8:
            gl_base_format = GL_RGBA;
            break;

        case crnlib::PIXEL_FMT_INVALID:
        case crnlib::PIXEL_FMT_3DC:
        case crnlib::PIXEL_FMT_DXN:
        case crnlib::PIXEL_FMT_DXT5A:
        case crnlib::PIXEL_FMT_DXT5_CCxY:
        case crnlib::PIXEL_FMT_DXT5_xGxR:
        case crnlib::PIXEL_FMT_DXT5_xGBR:
        case crnlib::PIXEL_FMT_DXT5_AGBR:
        case crnlib::PIXEL_FMT_ETC1:
        case crnlib::PIXEL_FMT_L8:
        case crnlib::PIXEL_FMT_A8:
        case crnlib::PIXEL_FMT_A8L8:
            LOGE << "Unsupported pixel format in texture " << source_path << endl;
            gl_base_format = GL_NONE;
            break;
    }

    if (m_colorSpace == Linear) {
        switch (m_crnTex.get_format()) {
            case crnlib::PIXEL_FMT_DXT1:
                gl_internal_format = GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
                break;

            case crnlib::PIXEL_FMT_DXT1A:
                gl_internal_format = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
                break;

            case crnlib::PIXEL_FMT_DXT2:
            case crnlib::PIXEL_FMT_DXT3:
                gl_internal_format = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
                break;

            case crnlib::PIXEL_FMT_DXT4:
            case crnlib::PIXEL_FMT_DXT5:
                gl_internal_format = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
                break;

            case crnlib::PIXEL_FMT_R8G8B8:
                gl_internal_format = GL_RGB8;
                break;

            case crnlib::PIXEL_FMT_A8R8G8B8:
                gl_internal_format = GL_RGBA8;
                break;

            case crnlib::PIXEL_FMT_INVALID:
            case crnlib::PIXEL_FMT_3DC:
            case crnlib::PIXEL_FMT_DXN:
            case crnlib::PIXEL_FMT_DXT5A:
            case crnlib::PIXEL_FMT_DXT5_CCxY:
            case crnlib::PIXEL_FMT_DXT5_xGxR:
            case crnlib::PIXEL_FMT_DXT5_xGBR:
            case crnlib::PIXEL_FMT_DXT5_AGBR:
            case crnlib::PIXEL_FMT_ETC1:
            case crnlib::PIXEL_FMT_L8:
            case crnlib::PIXEL_FMT_A8:
            case crnlib::PIXEL_FMT_A8L8:
                LOGE << "Unsupported pixel format in texture " << source_path << endl;
                gl_internal_format = GL_NONE;
                break;
        }
    } else {
        switch (m_crnTex.get_format()) {
            case crnlib::PIXEL_FMT_DXT1:
                gl_internal_format = GL_COMPRESSED_SRGB_S3TC_DXT1_EXT;
                break;

            case crnlib::PIXEL_FMT_DXT1A:
                gl_internal_format = GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT;
                break;

            case crnlib::PIXEL_FMT_DXT2:
            case crnlib::PIXEL_FMT_DXT3:
                gl_internal_format = GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT;
                break;

            case crnlib::PIXEL_FMT_DXT4:
            case crnlib::PIXEL_FMT_DXT5:
                gl_internal_format = GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT;
                break;

            case crnlib::PIXEL_FMT_R8G8B8:
                gl_internal_format = GL_SRGB8;
                break;

            case crnlib::PIXEL_FMT_A8R8G8B8:
                gl_internal_format = GL_SRGB8_ALPHA8;
                break;

            case crnlib::PIXEL_FMT_INVALID:
            case crnlib::PIXEL_FMT_3DC:
            case crnlib::PIXEL_FMT_DXN:
            case crnlib::PIXEL_FMT_DXT5A:
            case crnlib::PIXEL_FMT_DXT5_CCxY:
            case crnlib::PIXEL_FMT_DXT5_xGxR:
            case crnlib::PIXEL_FMT_DXT5_xGBR:
            case crnlib::PIXEL_FMT_DXT5_AGBR:
            case crnlib::PIXEL_FMT_ETC1:
            case crnlib::PIXEL_FMT_L8:
            case crnlib::PIXEL_FMT_A8:
            case crnlib::PIXEL_FMT_A8L8:
                LOGE << "Unsupported pixel format in texture " << source_path << endl;
                gl_internal_format = GL_NONE;
                break;
        }
    }

    LOG_ASSERT(m_crnTex.is_valid());
    switch (m_crnTex.get_format()) {
        case crnlib::PIXEL_FMT_DXT1:
        case crnlib::PIXEL_FMT_DXT1A:
        case crnlib::PIXEL_FMT_DXT2:
        case crnlib::PIXEL_FMT_DXT3:
        case crnlib::PIXEL_FMT_DXT4:
        case crnlib::PIXEL_FMT_DXT5:
            // LOGE << "Unsupported pixel format in texture " << source_path << endl;
            gl_type = GL_NONE;
            break;

        case crnlib::PIXEL_FMT_R8G8B8:
            gl_type = GL_UNSIGNED_BYTE;
            break;

        case crnlib::PIXEL_FMT_A8R8G8B8:
            gl_type = GL_UNSIGNED_INT_8_8_8_8_REV;
            break;

        case crnlib::PIXEL_FMT_INVALID:
        case crnlib::PIXEL_FMT_3DC:
        case crnlib::PIXEL_FMT_DXN:
        case crnlib::PIXEL_FMT_DXT5A:
        case crnlib::PIXEL_FMT_DXT5_CCxY:
        case crnlib::PIXEL_FMT_DXT5_xGxR:
        case crnlib::PIXEL_FMT_DXT5_xGBR:
        case crnlib::PIXEL_FMT_DXT5_AGBR:
        case crnlib::PIXEL_FMT_ETC1:
        case crnlib::PIXEL_FMT_L8:
        case crnlib::PIXEL_FMT_A8:
        case crnlib::PIXEL_FMT_A8L8:
            LOGE << "Unsupported pixel format in texture " << source_path << endl;
            gl_type = GL_NONE;
            break;
    }
}
