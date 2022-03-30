
#ifndef NVTT_EXPERIMENTAL_H
#define NVTT_EXPERIMENTAL_H

#include <nvtt/nvtt.h>

typedef struct NvttTexture NvttTexture;
typedef struct NvttOutputOptions NvttOutputOptions;


// Global functions
void nvttInitialize(...);
unsigned int nvttGetVersion();
const char * nvttGetErrorString(unsigned int error);


// Texture functions
NvttTexture * nvttCreateTexture();
void nvttDestroyTexture(NvttTexture * tex);

void nvttSetTexture2D(NvttTexture * tex, NvttInputFormat format, uint w, uint h, uint idx, void * data);

void nvttResize(NvttTexture * img, uint w, uint h);
unsigned int nvttDownsample(NvttTexture * img);

void nvttOutputCompressed(NvttTexture * img, NvttOutputFormat format);
void nvttOutputPixelFormat(NvttTexture * img, NvttOutputFormat format);




// How to control the compression parameters?

// Using many arguments:
// void nvttCompressImage(img, format, quality, r, g, b, a, ...);

// Using existing compression option class:
// compressionOptions = nvttCreateCompressionOptions();
// nvttSetCompressionOptionsFormat(compressionOptions, format);
// nvttSetCompressionOptionsQuality(compressionOptions, quality);
// nvttSetCompressionOptionsQuality(compressionOptions, quality);
// nvttSetCompressionOptionsColorWeights(compressionOptions, r, g, b, a);
// ...
// nvttCompressImage(img, compressionOptions);

// Using thread local context state:
// void nvttSetCompressionFormat(format);
// void nvttSetCompressionQuality(quality);
// void nvttSetCompressionColorWeights(r, g, b, a);
// ...
// nvttCompressImage(img);

// Using thread local context state, but with GL style function arguments:
// nvttCompressorParameteri(NVTT_FORMAT, format);
// nvttCompressorParameteri(NVTT_QUALITY, quality);
// nvttCompressorParameterf(NVTT_COLOR_WEIGHT_RED, r);
// nvttCompressorParameterf(NVTT_COLOR_WEIGHT_GREEN, g);
// nvttCompressorParameterf(NVTT_COLOR_WEIGHT_BLUE, b);
// nvttCompressorParameterf(NVTT_COLOR_WEIGHT_ALPHA, a);
// or nvttCompressorParameter4f(NVTT_COLOR_WEIGHTS, r, g, b, a);
// ...
// nvttCompressImage(img);

// How do we get the compressed output?
// - Using callbacks. (via new entrypoints, or through outputOptions)
// - Return it explicitely from nvttCompressImage.
// - Store it along the image, retrieve later explicitely with 'nvttGetCompressedData(img, ...)'

/*

// Global functions
void nvttInitialize(...);
unsigned int nvttGetVersion();
const char * nvttGetErrorString(unsigned int error);

// Context object
void nvttCreateContext();
void nvttDestroyContext();

void nvttSetParameter1i(unsigned int name, int value);

void nvttSetParameter1f(unsigned int name, float value);
void nvttSetParameter2f(unsigned int name, float v0, float v1);
void nvttSetParameter3f(unsigned int name, float v0, float v1, float v2);
void nvttSetParameter4f(unsigned int name, float v0, float v1, float v2, float v3);

// Image object
NvttImage * nvttCreateImage();
void nvttDestroyImage(NvttImage * img);

void nvttSetImageData(NvttImage * image, NvttInputFormat format, unsigned int w, unsigned int h, void * data);

void nvttSetImageParameter1i(NvttImage * image, unsigned int name, int value);
void nvttSetImageParameter1f(NvttImage * image, unsigned int name, float value);

void nvttResizeImage(NvttImage * image, unsigned int w, unsigned int h);
void nvttQuantizeImage(NvttImage * image, bool dither, unsigned int rbits, unsigned int gbits, unsigned int bbits, unsigned int abits);
void nvttCompressImage(NvttImage * image, void * buffer, int size);

*/


#endif // NVTT_EXPERIMENTAL_H
