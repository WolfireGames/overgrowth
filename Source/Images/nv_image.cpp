//-----------------------------------------------------------------------------
//           Name: nv_image.cpp
//      Developer: Wolfire Games LLC
//    Description: This is a file from an Nvidia image library, with some
//                 modification made by Wolfire.
//        License: Read below
//-----------------------------------------------------------------------------
//
// nvImage.cpp - Image support class
//
// The nvImage class implements an interface for a multipurpose image
// object. This class is useful for loading and formating images
// for use as textures. The class supports dds, png, and hdr formats.
//
// This file implements the format independent interface.
//
// Author: Evan Hart
// Email: sdkfeedback@nvidia.com
//
// Copyright (c) NVIDIA Corporation. All rights reserved.
////////////////////////////////////////////////////////////////////////////////
#include "nv_image.h"

#include <Utility/assert.h>

#include <cstring>
#include <algorithm>

using std::max;
using std::vector;

#ifdef WIN32
#define strcasecmp _stricmp
#endif

namespace nv2 {

Image::FormatInfo Image::formatTable[] = {
    {"dds", Image::readDDS, 0}};

//
//
////////////////////////////////////////////////////////////
Image::Image() : _width(0), _height(0), _depth(0), _levelCount(0), _faces(0), _format(GL_RGBA), _internalFormat(GL_RGBA8), _type(GL_UNSIGNED_BYTE), _elementSize(0) {
}

//
//
////////////////////////////////////////////////////////////
Image::~Image() {
    freeData();
}

//
//
////////////////////////////////////////////////////////////
void Image::freeData() {
    for (auto &it : _data) {
        delete[] it;
    }
    _data.clear();
}

//
//
////////////////////////////////////////////////////////////
int Image::getImageSize(int level) const {
    bool compressed = isCompressed();
    int w = _width >> level;
    int h = _height >> level;
    int d = _depth >> level;
    w = (w) ? w : 1;
    h = (h) ? h : 1;
    d = (d) ? d : 1;
    int bw = (compressed) ? (w + 3) / 4 : w;
    int bh = (compressed) ? (h + 3) / 4 : h;
    int elementSize = _elementSize;

    return bw * bh * d * elementSize;
}

//
//
////////////////////////////////////////////////////////////
const void *Image::getLevel(int level, GLenum face) const {
    LOG_ASSERT(level < _levelCount);
    LOG_ASSERT(_faces == 0 || (face >= GL_TEXTURE_CUBE_MAP_POSITIVE_X && face <= GL_TEXTURE_CUBE_MAP_NEGATIVE_Z));

    face = face - GL_TEXTURE_CUBE_MAP_POSITIVE_X;

    LOG_ASSERT((face * _levelCount + level) < _data.size());
    return _data[face * _levelCount + level];
}

//
//
////////////////////////////////////////////////////////////
void *Image::getLevel(int level, GLenum face) {
    LOG_ASSERT(level < _levelCount);
    LOG_ASSERT(_faces == 0 || (face >= GL_TEXTURE_CUBE_MAP_POSITIVE_X && face <= GL_TEXTURE_CUBE_MAP_NEGATIVE_Z));

    face = face - GL_TEXTURE_CUBE_MAP_POSITIVE_X;

    LOG_ASSERT((face * _levelCount + level) < _data.size());
    return _data[face * _levelCount + level];
}

//
//
////////////////////////////////////////////////////////////
bool Image::loadImageFromFile(const char *file) {
    const char *extension;
    extension = strrchr(file, '.');

    if (extension)
        extension++;  // start looking after the .
    else
        return false;

    int formatCount = sizeof(Image::formatTable) / sizeof(Image::FormatInfo);

    // try to match by format first
    for (int ii = 0; ii < formatCount; ii++) {
        if (!strcasecmp(formatTable[ii].extension, extension)) {
            // extension matches, load it
            return formatTable[ii].reader(file, *this);
        }
    }

    return false;
}

//
//
////////////////////////////////////////////////////////////
void Image::flipSurface(GLubyte *surf, int width, int height, int depth) {
    unsigned int lineSize;

    depth = (depth) ? depth : 1;

    if (!isCompressed()) {
        lineSize = _elementSize * width;
        unsigned int sliceSize = lineSize * height;

        GLubyte *tempBuf = new GLubyte[lineSize];

        for (int ii = 0; ii < depth; ii++) {
            GLubyte *top = surf + ii * sliceSize;
            GLubyte *bottom = top + (sliceSize - lineSize);

            for (int jj = 0; jj < (height >> 1); jj++) {
                memcpy(tempBuf, top, lineSize);
                memcpy(top, bottom, lineSize);
                memcpy(bottom, tempBuf, lineSize);

                top += lineSize;
                bottom -= lineSize;
            }
        }

        delete[] tempBuf;
    } else {
        void (*flipblocks)(GLubyte *, unsigned int);
        width = (width + 3) / 4;
        height = (height + 3) / 4;
        unsigned int blockSize = 0;

        switch (_format) {
            case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
                blockSize = 8;
                flipblocks = &Image::flip_blocks_dxtc1;
                break;
            case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
                blockSize = 16;
                flipblocks = &Image::flip_blocks_dxtc3;
                break;
            case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
                blockSize = 16;
                flipblocks = &Image::flip_blocks_dxtc5;
                break;
            default:
                return;
        }

        lineSize = width * blockSize;
        GLubyte *tempBuf = new GLubyte[lineSize];

        GLubyte *top = surf;
        GLubyte *bottom = surf + (height - 1) * lineSize;

        for (unsigned int j = 0; j < max((unsigned int)height >> 1, (unsigned int)1); j++) {
            if (top == bottom) {
                flipblocks(top, width);
                break;
            }

            flipblocks(top, width);
            flipblocks(bottom, width);

            memcpy(tempBuf, top, lineSize);
            memcpy(top, bottom, lineSize);
            memcpy(bottom, tempBuf, lineSize);

            top += lineSize;
            bottom -= lineSize;
        }
        delete[] tempBuf;
    }
}

//
//
////////////////////////////////////////////////////////////
bool Image::convertCrossToCubemap() {
    // can't already be a cubemap
    if (isCubeMap())
        return false;

    // mipmaps are not supported
    if (_levelCount != 1)
        return false;

    // compressed textures are not supported
    if (isCompressed())
        return false;

    // this function only supports vertical cross format for now (3 wide by 4 high)
    if ((_width / 3 != _height / 4) || (_width % 3 != 0) || (_height % 4 != 0) || (_depth != 0))
        return false;

    // get the source data
    GLubyte *data = _data[0];

    int fWidth = _width / 3;
    int fHeight = _height / 4;

    // remove the old pointer from the vector
    _data.pop_back();

    GLubyte *face = new GLubyte[fWidth * fHeight * _elementSize];
    GLubyte *ptr;

    // extract the faces

    // positive X
    ptr = face;
    for (int j = 0; j < fHeight; j++) {
        memcpy(ptr, &data[((_height - (fHeight + j + 1)) * _width + 2 * fWidth) * _elementSize], fWidth * _elementSize);
        ptr += fWidth * _elementSize;
    }
    _data.push_back(face);

    // negative X
    face = new GLubyte[fWidth * fHeight * _elementSize];
    ptr = face;
    for (int j = 0; j < fHeight; j++) {
        memcpy(ptr, &data[(_height - (fHeight + j + 1)) * _width * _elementSize], fWidth * _elementSize);
        ptr += fWidth * _elementSize;
    }
    _data.push_back(face);

    // positive Y
    face = new GLubyte[fWidth * fHeight * _elementSize];
    ptr = face;
    for (int j = 0; j < fHeight; j++) {
        memcpy(ptr, &data[((4 * fHeight - j - 1) * _width + fWidth) * _elementSize], fWidth * _elementSize);
        ptr += fWidth * _elementSize;
    }
    _data.push_back(face);

    // negative Y
    face = new GLubyte[fWidth * fHeight * _elementSize];
    ptr = face;
    for (int j = 0; j < fHeight; j++) {
        memcpy(ptr, &data[((2 * fHeight - j - 1) * _width + fWidth) * _elementSize], fWidth * _elementSize);
        ptr += fWidth * _elementSize;
    }
    _data.push_back(face);

    // positive Z
    face = new GLubyte[fWidth * fHeight * _elementSize];
    ptr = face;
    for (int j = 0; j < fHeight; j++) {
        memcpy(ptr, &data[((_height - (fHeight + j + 1)) * _width + fWidth) * _elementSize], fWidth * _elementSize);
        ptr += fWidth * _elementSize;
    }
    _data.push_back(face);

    // negative Z
    face = new GLubyte[fWidth * fHeight * _elementSize];
    ptr = face;
    for (int j = 0; j < fHeight; j++) {
        for (int i = 0; i < fWidth; i++) {
            memcpy(ptr, &data[(j * _width + 2 * fWidth - (i + 1)) * _elementSize], _elementSize);
            ptr += _elementSize;
        }
    }
    _data.push_back(face);

    // set the new # of faces, width and height
    _faces = 6;
    _width = fWidth;
    _height = fHeight;

    // delete the old pointer
    delete[] data;

    return true;
}

//
//
////////////////////////////////////////////////////////////
bool Image::setImage(int width, int height, GLenum format, GLenum type, const void *data) {
    // check parameters before destroying the old image
    int elementSize;
    GLenum internalFormat;

    switch (format) {
        case GL_ALPHA:
            switch (type) {
                case GL_UNSIGNED_BYTE:
                    internalFormat = GL_R8;
                    elementSize = 1;
                    break;
                case GL_UNSIGNED_SHORT:
                    internalFormat = GL_R16;
                    elementSize = 2;
                    break;
                case GL_FLOAT:
                    internalFormat = GL_R32F;
                    elementSize = 4;
                    break;
                case GL_HALF_FLOAT:
                    internalFormat = GL_R16F;
                    elementSize = 2;
                    break;
                default:
                    return false;  // format/type combo not supported
            }
            break;
        case GL_RGB:
            switch (type) {
                case GL_UNSIGNED_BYTE:
                    internalFormat = GL_RGB8;
                    elementSize = 3;
                    break;
                case GL_UNSIGNED_SHORT:
                    internalFormat = GL_RGB16;
                    elementSize = 6;
                    break;
                case GL_FLOAT:
                    internalFormat = GL_RGB32F;
                    elementSize = 12;
                    break;
                case GL_HALF_FLOAT:
                    internalFormat = GL_RGB16F;
                    elementSize = 6;
                    break;
                default:
                    return false;  // format/type combo not supported
            }
            break;
        case GL_RGBA:
            switch (type) {
                case GL_UNSIGNED_BYTE:
                    internalFormat = GL_RGBA8;
                    elementSize = 4;
                    break;
                case GL_UNSIGNED_SHORT:
                    internalFormat = GL_RGBA16;
                    elementSize = 8;
                    break;
                case GL_FLOAT:
                    internalFormat = GL_RGBA32F;
                    elementSize = 16;
                    break;
                case GL_HALF_FLOAT:
                    internalFormat = GL_RGBA16F;
                    elementSize = 8;
                    break;
                default:
                    return false;  // format/type combo not supported
            }
            break;
        default:
            // bad format
            return false;
            break;
    }

    // clear old data
    freeData();

    GLubyte *newImage = new GLubyte[width * height * elementSize];
    memcpy(newImage, data, width * height * elementSize);

    _data.push_back(newImage);

    _width = width;
    _height = height;
    _elementSize = elementSize;
    _internalFormat = internalFormat;
    _levelCount = 1;
    _faces = 0;
    _depth = 0;
    _format = format;
    _type = type;

    return true;
}

//
//
////////////////////////////////////////////////////////////
bool Image::saveImageToFile(const char *file) {
    const char *extension;
    extension = strrchr(file, '.');

    if (extension)
        extension++;  // start looking after the .
    else
        return false;

    int formatCount = sizeof(Image::formatTable) / sizeof(Image::FormatInfo);

    // try to match by format first
    for (int ii = 0; ii < formatCount; ii++) {
        if (!strcasecmp(formatTable[ii].extension, extension)) {
            // extension matches, load it
            if (formatTable[ii].writer) {
                return formatTable[ii].writer(file, *this);
            }
        }
    }

    return false;
}

}  // namespace nv2
