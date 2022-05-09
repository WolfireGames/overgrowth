//-----------------------------------------------------------------------------
//           Name: freeimage_wrapper.h
//      Developer: Wolfire Games LLC
//         Author: Phillip Isola
//    Description: Wraps up FreeImage library
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

#include <Images/ddsformat.hpp>
#include <Internal/integer.h>

#include <string>

struct FIquad {
#if FREEIMAGE_COLORORDER == FREEIMAGE_COLORORDER_BGR
    uint8_t rgbBlue;
    uint8_t rgbGreen;
    uint8_t rgbRed;
#else
    uint8_t rgbRed;
    uint8_t rgbGreen;
    uint8_t rgbBlue;
#endif  // FREEIMAGE_COLORORDER
    uint8_t rgbReserved;
};

enum fiFORMAT {
    FIWF_UNKNOWN = -1,
    FIWF_BMP = 0,
    FIWF_ICO = 1,
    FIWF_JPEG = 2,
    FIWF_JNG = 3,
    FIWF_KOALA = 4,
    FIWF_LBM = 5,
    FIWF_IFF = FIWF_LBM,
    FIWF_MNG = 6,
    FIWF_PBM = 7,
    FIWF_PBMRAW = 8,
    FIWF_PCD = 9,
    FIWF_PCX = 10,
    FIWF_PGM = 11,
    FIWF_PGMRAW = 12,
    FIWF_PNG = 13,
    FIWF_PPM = 14,
    FIWF_PPMRAW = 15,
    FIWF_RAS = 16,
    FIWF_TARGA = 17,
    FIWF_TIFF = 18,
    FIWF_WBMP = 19,
    FIWF_PSD = 20,
    FIWF_CUT = 21,
    FIWF_XBM = 22,
    FIWF_XPM = 23,
    FIWF_DDS = 24,
    FIWF_GIF = 25,
    FIWF_HDR = 26,
    FIWF_FAXG3 = 27,
    FIWF_SGI = 28,
    FIWF_EXR = 29,
    FIWF_J2K = 30,
    FIWF_JP2 = 31,
    FIWF_PFM = 32,
    FIWF_PICT = 33,
    FIWF_RAW = 34
};

enum fiTYPE {
    FIWT_UNKNOWN = 0,  // unknown type
    FIWT_BITMAP = 1,   // standard image			: 1-, 4-, 8-, 16-, 24-, 32-bit
    FIWT_UINT16 = 2,   // array of unsigned short	: unsigned 16-bit
    FIWT_INT16 = 3,    // array of short			: signed 16-bit
    FIWT_UINT32 = 4,   // array of unsigned long	: unsigned 32-bit
    FIWT_INT32 = 5,    // array of long			: signed 32-bit
    FIWT_FLOAT = 6,    // array of float			: 32-bit IEEE floating point
    FIWT_DOUBLE = 7,   // array of double			: 64-bit IEEE floating point
    FIWT_COMPLEX = 8,  // array of FICOMPLEX		: 2 x 64-bit IEEE floating point
    FIWT_RGB16 = 9,    // 48-bit RGB image			: 3 x 16-bit
    FIWT_RGBA16 = 10,  // 64-bit RGBA image		: 4 x 16-bit
    FIWT_RGBF = 11,    // 96-bit RGB float image	: 3 x 32-bit IEEE floating point
    FIWT_RGBAF = 12    // 128-bit RGBA float image	: 4 x 32-bit IEEE floating point
};

struct FIBITMAP;

class FIBitmapContainer {
    FIBITMAP* image;

   public:
    FIBitmapContainer(FIBITMAP* _image = NULL);
    ~FIBitmapContainer();
    void reset(FIBITMAP* _image);
    FIBITMAP* get();
    const FIBITMAP* get() const;
};

vec3 getInterpolatedColor(FIBITMAP* image, float x, float y);
vec3 getInterpolatedColorUV(FIBITMAP* image, float x, float y);
FIBITMAP* GenericLoader(const char* lpszPathName, int flag = 0);
vec3 getColor(FIBITMAP* image, float x, float y);
vec4 getInterpolatedRGBAUV(FIBITMAP* image, float x, float y);
vec4 getInterpolatedRGBA(FIBITMAP* image, float x, float y);
unsigned int getWidth(FIBITMAP* image);
unsigned int getHeight(FIBITMAP* image);
int getPixelColor(FIBITMAP* image, unsigned int x, unsigned int y, FIquad* value);
void UnloadBitmap(FIBITMAP* image);
fiTYPE getImageType(FIBITMAP* image);
unsigned int getBPP(FIBITMAP* image);
uint8_t* getScanLine(FIBITMAP* image, int scanline);
