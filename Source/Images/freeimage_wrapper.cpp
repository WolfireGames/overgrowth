//-----------------------------------------------------------------------------
//           Name: freeimage_wrapper.cpp
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
#include "freeimage_wrapper.h"

#include <Compat/fileio.h>
#include <Compat/compat.h>

#include <Math/enginemath.h>
#include <Internal/error.h>
#include <Logging/logdata.h>
#include <opengl.h>

#include <crnlib.h>
#include <dds_defs.h>
#include <FreeImage.h>

#include <cstring>
#include <vector>

using namespace crnlib;
using std::string;

//http://graphics.stanford.edu/~seander/bithacks.html#IntegerLogDeBruijn
static const uint32_t MultiplyDeBruijnBitPosition[32] =
{
    0, 9, 1, 10, 13, 21, 2, 29, 11, 14, 16, 18, 22, 25, 3, 30,
    8, 12, 20, 28, 15, 17, 24, 7, 19, 27, 23, 6, 26, 5, 4, 31
};


static uint32_t uintLog2(uint32_t v) {
    v |= v >> 1; // first round down to one less than a power of 2
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;

    return MultiplyDeBruijnBitPosition[(uint32_t)(v * 0x07C4ACDDU) >> 27];
}


vec3 getInterpolatedColorUV(FIBITMAP* image, float x, float y) {
    x = max(0.01f,min(0.99f, x));
    y = max(0.01f,min(0.99f, y));
    return getInterpolatedColor(image, x*FreeImage_GetWidth(image)+0.5f, y*FreeImage_GetHeight(image)+1.0f);
}

vec3 getInterpolatedColor(FIBITMAP* image, float x, float y) {
    RGBQUAD top_left, top_right, bottom_left, bottom_right;
    FreeImage_GetPixelColor(image,
                            (unsigned int)x,
                            (unsigned int)y,
                            &top_left);
    FreeImage_GetPixelColor(image,
                            (unsigned int)x+1,
                            (unsigned int)y,
                            &top_right);
    FreeImage_GetPixelColor(image,
                            (unsigned int)x,
                            (unsigned int)y+1,
                            &bottom_left);
    FreeImage_GetPixelColor(image,
                            (unsigned int)x+1,
                            (unsigned int)y+1,
                            &bottom_right);
    
    float x_weight = x - (int)x;
    float y_weight = y - (int)y;
    
    RGBQUAD left;
    left.rgbRed = (BYTE)(top_left.rgbRed * (1-y_weight) + bottom_left.rgbRed * (y_weight));
    left.rgbGreen = (BYTE)(top_left.rgbGreen * (1-y_weight) + bottom_left.rgbGreen * (y_weight));
    left.rgbBlue = (BYTE)(top_left.rgbBlue * (1-y_weight) + bottom_left.rgbBlue * (y_weight));
    RGBQUAD right; 
    right.rgbRed = (BYTE)(top_right.rgbRed  * (1-y_weight) + bottom_right.rgbRed  * (y_weight));
    right.rgbGreen = (BYTE)(top_right.rgbGreen  * (1-y_weight) + bottom_right.rgbGreen  * (y_weight));
    right.rgbBlue = (BYTE)(top_right.rgbBlue  * (1-y_weight) + bottom_right.rgbBlue  * (y_weight));
    
    vec3 value(0.0f);
    value.r() = ((BYTE)(left.rgbRed * (1-x_weight) + right.rgbRed * (x_weight)))/255.0f;
    value.g() = ((BYTE)(left.rgbGreen * (1-x_weight) + right.rgbGreen * (x_weight)))/255.0f;
    value.b() = ((BYTE)(left.rgbBlue * (1-x_weight) + right.rgbBlue * (x_weight)))/255.0f;
    
    return value;
}

vec4 getInterpolatedRGBAUV(FIBITMAP* image, float x, float y) {
    x = max(0.01f,min(0.99f, x));
    y = max(0.01f,min(0.99f, y));
    return getInterpolatedRGBA(image, x*FreeImage_GetWidth(image)+0.5f, y*FreeImage_GetHeight(image)+1.0f);
}

vec4 getInterpolatedRGBA(FIBITMAP* image, float x, float y) {
    RGBQUAD top_left, top_right, bottom_left, bottom_right;
    FreeImage_GetPixelColor(image,
        (unsigned int)x,
        (unsigned int)y,
        &top_left);
    FreeImage_GetPixelColor(image,
        (unsigned int)x+1,
        (unsigned int)y,
        &top_right);
    FreeImage_GetPixelColor(image,
        (unsigned int)x,
        (unsigned int)y+1,
        &bottom_left);
    FreeImage_GetPixelColor(image,
        (unsigned int)x+1,
        (unsigned int)y+1,
        &bottom_right);

    float x_weight = x - (int)x;
    float y_weight = y - (int)y;

    RGBQUAD left;
    left.rgbRed = (BYTE)(top_left.rgbRed * (1-y_weight) + bottom_left.rgbRed * (y_weight));
    left.rgbGreen = (BYTE)(top_left.rgbGreen * (1-y_weight) + bottom_left.rgbGreen * (y_weight));
    left.rgbBlue = (BYTE)(top_left.rgbBlue * (1-y_weight) + bottom_left.rgbBlue * (y_weight));
    left.rgbReserved = (BYTE)(top_left.rgbReserved * (1-y_weight) + bottom_left.rgbReserved * (y_weight));
    RGBQUAD right; 
    right.rgbRed = (BYTE)(top_right.rgbRed  * (1-y_weight) + bottom_right.rgbRed  * (y_weight));
    right.rgbGreen = (BYTE)(top_right.rgbGreen  * (1-y_weight) + bottom_right.rgbGreen  * (y_weight));
    right.rgbBlue = (BYTE)(top_right.rgbBlue  * (1-y_weight) + bottom_right.rgbBlue  * (y_weight));
    right.rgbReserved = (BYTE)(top_right.rgbReserved  * (1-y_weight) + bottom_right.rgbReserved  * (y_weight));

    vec4 value(0.0f);
    value.r() = ((BYTE)(left.rgbRed * (1-x_weight) + right.rgbRed * (x_weight)))/255.0f;
    value.g() = ((BYTE)(left.rgbGreen * (1-x_weight) + right.rgbGreen * (x_weight)))/255.0f;
    value.b() = ((BYTE)(left.rgbBlue * (1-x_weight) + right.rgbBlue * (x_weight)))/255.0f;
    value.a() = ((BYTE)(left.rgbReserved * (1-x_weight) + right.rgbReserved * (x_weight)))/255.0f;

    return value;
}

vec3 getColor(FIBITMAP* image, float x, float y) {
    RGBQUAD color;
    FreeImage_GetPixelColor(image,
                            (unsigned int)x,
                            (unsigned int)y,
                            &color);
    return vec3(color.rgbRed,color.rgbGreen,color.rgbBlue);
}

unsigned int getWidth(FIBITMAP* image)
{
	return FreeImage_GetWidth(image);
}

unsigned int getHeight(FIBITMAP* image)
{
	return FreeImage_GetHeight(image);
}

int getPixelColor(FIBITMAP* image, unsigned int x, unsigned int y, FIquad* value )
{
	RGBQUAD quad;
	int ret = FreeImage_GetPixelColor(image,x,y,&quad);
	value->rgbRed= quad.rgbRed;
	value->rgbGreen = quad.rgbGreen;
	value->rgbBlue = quad.rgbBlue;
	value->rgbReserved = quad.rgbReserved;
	return ret;
}

void UnloadBitmap(FIBITMAP* image)
{
	FreeImage_Unload(image);
}

fiTYPE getImageType(FIBITMAP* image)
{
	return (fiTYPE)FreeImage_GetImageType(image);
}

unsigned int getBPP(FIBITMAP* image)
{
	return FreeImage_GetBPP(image);
}

uint8_t* getScanLine(FIBITMAP* image, int scanline)
{
	return FreeImage_GetScanLine(image,scanline);
}

/** Generic image loader -- from FreeImage documentation: 
http://internap.dl.sourceforge.net/sourceforge/freeimage/FreeImage3110.pdf
@param lpszPathName Pointer to the full file name
@param flag Optional load flag constant
@return Returns the loaded dib if successful, returns NULL otherwise
*/
FIBITMAP* GenericLoader(const char* abs_path, int flag) {
#ifdef _WIN32
	string path_str = abs_path;
	ShortenWindowsPath(path_str);
	const char* path = path_str.c_str();
#else
	const char *path = abs_path;
#endif
    FREE_IMAGE_FORMAT fif = FIF_UNKNOWN;
    // check the file signature and deduce its format
    // (the second argument is currently not used by FreeImage)
    fif = FreeImage_GetFileType(path, 0);
    if(fif == FIF_UNKNOWN) {
        // no signature ?
        // try to guess the file format from the file extension
        fif = FreeImage_GetFIFFromFilename(path);
    }
    // check that the plugin has reading capabilities ...
    if((fif != FIF_UNKNOWN) && FreeImage_FIFSupportsReading(fif)) {
        // ok, let's load the file
        FIBITMAP *dib = FreeImage_Load(fif, path, flag);
        // unless a bad file format, we are done !
        return dib;
    }
    return NULL;
}

FIBitmapContainer::FIBitmapContainer(FIBITMAP* _image)
{
    image = _image;
}

FIBitmapContainer::~FIBitmapContainer()
{
    reset(NULL);
}

void FIBitmapContainer::reset(FIBITMAP* _image)
{
    if(image){
        FreeImage_Unload(image);
        image = NULL;
    }
    image = _image;
}

FIBITMAP* FIBitmapContainer::get()
{
    return image;
}

const FIBITMAP* FIBitmapContainer::get() const
{
    return image;
}
