//-----------------------------------------------------------------------------
//           Name: nv_image.h
//      Developer: Wolfire Games LLC
//    Description: This is a file from an Nvidia image library, with some
//                 modification made by Wolfire.
//        License: Read below
//-----------------------------------------------------------------------------
//
// nvImage.h - Image support class
//
// The nvImage class implements an interface for a multipurpose image
// object. This class is useful for loading and formating images
// for use as textures. The class supports dds, png, and hdr formats.
//
// Author: Evan Hart
// Email: sdkfeedback@nvidia.com
//
// Copyright (c) NVIDIA Corporation. All rights reserved.
////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <opengl.h>

#include <vector>
#include <cassert>

namespace nv2 {

class Image {
   public:
    Image();
    virtual ~Image();

    // return the width of the image
    int getWidth() const { return _width; }

    // return the height of the image
    int getHeight() const { return _height; }

    // return the dpeth of the image (0 for images with no depth)
    int getDepth() const { return _depth; }

    // return the number of mipmap levels available for the image
    int getMipLevels() const { return _levelCount; }

    // return the number of cubemap faces available for the image (0 for non-cubemap images)
    int getFaces() const { return _faces; }

    // return the format of the image data (GL_RGB, GL_BGR, etc)
    GLenum getFormat() const { return _format; }

    // return the suggested internal format for the data
    GLenum getInternalFormat() const { return _internalFormat; }

    // return the type of the image data
    GLenum getType() const { return _type; }

    // return the Size in bytes of a level of the image
    int getImageSize(int level = 0) const;

    // return whether the data is a crompressed format
    bool isCompressed() const {
        switch (_format) {
            case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
            case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
            case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
            case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
            case GL_COMPRESSED_LUMINANCE_LATC1_EXT:
            case GL_COMPRESSED_SIGNED_LUMINANCE_LATC1_EXT:
            case GL_COMPRESSED_LUMINANCE_ALPHA_LATC2_EXT:
            case GL_COMPRESSED_SIGNED_LUMINANCE_ALPHA_LATC2_EXT:
                return true;
        }
        return false;
    }

    // return whether the image represents a cubemap
    bool isCubeMap() const { return _faces > 0; }

    // return whether the image represents a volume
    bool isVolume() const { return _depth > 0; }

    // get a pointer to level data
    const void* getLevel(int level, GLenum face = GL_TEXTURE_CUBE_MAP_POSITIVE_X) const;
    void* getLevel(int level, GLenum face = GL_TEXTURE_CUBE_MAP_POSITIVE_X);

    // initialize an image from a file
    bool loadImageFromFile(const char* file);

    // convert a suitable image from a cubemap cross to a cubemap (returns false for unsuitable images)
    bool convertCrossToCubemap();

    // load an image from memory, for the purposes of saving
    bool setImage(int width, int height, GLenum format, GLenum type, const void* data);

    // save an image to a file
    bool saveImageToFile(const char* file);

   protected:
    int _width;
    int _height;
    int _depth;
    int _levelCount;
    int _faces;
    GLenum _format;
    GLenum _internalFormat;
    GLenum _type;
    int _elementSize;

    // pointers to the levels
    std::vector<GLubyte*> _data;

    void freeData();
    void flipSurface(GLubyte* surf, int width, int height, int depth);

    //
    // Static elements used to dispatch to proper sub-readers
    //
    //////////////////////////////////////////////////////////////
    struct FormatInfo {
        const char* extension;
        bool (*reader)(const char* file, Image& i);
        bool (*writer)(const char* file, Image& i);
    };

    static FormatInfo formatTable[];

    static bool readDDS(const char* file, Image& i);

    static void flip_blocks_dxtc1(GLubyte* ptr, unsigned int numBlocks);
    static void flip_blocks_dxtc3(GLubyte* ptr, unsigned int numBlocks);
    static void flip_blocks_dxtc5(GLubyte* ptr, unsigned int numBlocks);
};
}  // namespace nv2
