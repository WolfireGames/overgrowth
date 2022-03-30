// Copyright NVIDIA Corporation 2007 -- Ignacio Castano <icastano@nvidia.com>
// 
// Permission is hereby granted, free of charge, to any person
// obtaining a copy of this software and associated documentation
// files (the "Software"), to deal in the Software without
// restriction, including without limitation the rights to use,
// copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following
// conditions:
// 
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
// OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
// HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
// WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.

#ifndef NV_TT_H
#define NV_TT_H

// Function linkage
#if NVTT_SHARED

#if defined _WIN32 || defined WIN32 || defined __NT__ || defined __WIN32__ || defined __MINGW32__
#	ifdef NVTT_EXPORTS
#		define NVTT_API __declspec(dllexport)
#	else
#		define NVTT_API __declspec(dllimport)
#	endif
#endif

#if defined __GNUC__ >= 4
#	ifdef NVTT_EXPORTS
#		define NVTT_API __attribute__((visibility("default")))
#	endif
#endif

#endif // NVTT_SHARED

#if !defined NVTT_API
#	define NVTT_API
#endif

#define NVTT_VERSION 201

#define NVTT_FORBID_COPY(Class) \
	private: \
		Class(const Class &); \
		void operator=(const Class &); \
	public: \

#define NVTT_DECLARE_PIMPL(Class) \
	public: \
		struct Private; \
		Private & m


// Public interface.
namespace nvtt
{
	// Forward declarations.
	struct Texture;
	
	/// Supported compression formats.
	enum Format
	{
		// No compression.
		Format_RGB,
		Format_RGBA = Format_RGB,
		
		// DX9 formats.
		Format_DXT1,
		Format_DXT1a,   // DXT1 with binary alpha.
		Format_DXT3,
		Format_DXT5,
		Format_DXT5n,   // Compressed HILO: R=0, G=x, B=0, A=y
		
		// DX10 formats.
		Format_BC1 = Format_DXT1,
		Format_BC1a = Format_DXT1a,
		Format_BC2 = Format_DXT3,
		Format_BC3 = Format_DXT5,
		Format_BC3n = Format_DXT5n,
		Format_BC4,     // ATI1
		Format_BC5,     // 3DC, ATI2

		Format_DXT1n,
		Format_CTX1,
	};

	/// Pixel types.
	enum PixelType
	{
		PixelType_UnsignedNorm,
		PixelType_SignedNorm,
		PixelType_UnsignedInt,
		PixelType_SignedInt,
		PixelType_Float,
	};
	
	/// Quality modes.
	enum Quality
	{
		Quality_Fastest,
		Quality_Normal,
		Quality_Production,
		Quality_Highest,
	};

	/// Compression options. This class describes the desired compression format and other compression settings.
	struct CompressionOptions
	{
		NVTT_FORBID_COPY(CompressionOptions);
		NVTT_DECLARE_PIMPL(CompressionOptions);

		NVTT_API CompressionOptions();
		NVTT_API ~CompressionOptions();
		
		NVTT_API void reset();
		
		NVTT_API void setFormat(Format format);
		NVTT_API void setQuality(Quality quality);
		NVTT_API void setColorWeights(float red, float green, float blue, float alpha = 1.0f);
		
		NVTT_API void setExternalCompressor(const char * name);

		// Set color mask to describe the RGB/RGBA format.
		NVTT_API void setPixelFormat(unsigned int bitcount, unsigned int rmask, unsigned int gmask, unsigned int bmask, unsigned int amask);
		NVTT_API void setPixelFormat(unsigned char rsize, unsigned char gsize, unsigned char bsize, unsigned char asize);
		
		NVTT_API void setPixelType(PixelType pixelType);

		NVTT_API void setQuantization(bool colorDithering, bool alphaDithering, bool binaryAlpha, int alphaThreshold = 127);
	};

	/* 
	// DXGI_FORMAT_R16G16_FLOAT
	compressionOptions.setPixelType(PixelType_Float);
	compressionOptions.setPixelFormat2(16, 16, 0, 0);
	
	// DXGI_FORMAT_R32G32B32A32_FLOAT
	compressionOptions.setPixelType(PixelType_Float);
	compressionOptions.setPixelFormat2(32, 32, 32, 32);
	*/
	

	/// Wrap modes.
	enum WrapMode
	{
		WrapMode_Clamp,
		WrapMode_Repeat,
		WrapMode_Mirror,
	};
	
	/// Texture types.
	enum TextureType
	{
		TextureType_2D,
		TextureType_Cube,
	//	TextureType_3D,
	};
	
	/// Input formats.
	enum InputFormat
	{
		InputFormat_BGRA_8UB,
		InputFormat_RGBA_32F,
	};
	
	/// Mipmap downsampling filters.
	enum MipmapFilter
	{
		MipmapFilter_Box,       ///< Box filter is quite good and very fast.
		MipmapFilter_Triangle,  ///< Triangle filter blurs the results too much, but that might be what you want.
		MipmapFilter_Kaiser,    ///< Kaiser-windowed Sinc filter is the best downsampling filter.
	};
	
	/// Texture resize filters.
	enum ResizeFilter
	{
		ResizeFilter_Box,
		ResizeFilter_Triangle,
		ResizeFilter_Kaiser,
		ResizeFilter_Mitchell,
	};
	
	/// Color transformation.
	enum ColorTransform
	{
		ColorTransform_None,
		ColorTransform_Linear,      ///< Not implemented.
		ColorTransform_Swizzle,     ///< Not implemented.
		ColorTransform_YCoCg,       ///< Transform into r=Co, g=Cg, b=0, a=Y
		ColorTransform_ScaledYCoCg, ///< Not implemented.
	};
	
	/// Extents rounding mode.
	enum RoundMode
	{
		RoundMode_None,
		RoundMode_ToNextPowerOfTwo,
		RoundMode_ToNearestPowerOfTwo,
		RoundMode_ToPreviousPowerOfTwo,
	};
	
	/// Alpha mode.
	enum AlphaMode
	{
		AlphaMode_None,
		AlphaMode_Transparency,
		AlphaMode_Premultiplied,
	};

	/// Input options. Specify format and layout of the input texture.
	struct InputOptions
	{
		NVTT_FORBID_COPY(InputOptions);
		NVTT_DECLARE_PIMPL(InputOptions);

		NVTT_API InputOptions();
		NVTT_API ~InputOptions();
		
		// Set default options.
		NVTT_API void reset();
		
		// Setup input layout.
		NVTT_API void setTextureLayout(TextureType type, int w, int h, int d = 1);
		NVTT_API void resetTextureLayout();
		
		// Set mipmap data. Copies the data.
		NVTT_API bool setMipmapData(const void * data, int w, int h, int d = 1, int face = 0, int mipmap = 0);
		NVTT_API bool setMipmapChannelData(const void * data, int channel, int w, int h, int d = 1, int face = 0, int mipmap = 0);
		
		// Describe the format of the input.
		NVTT_API void setFormat(InputFormat format);
		
		// Set the way the input alpha channel is interpreted. @@ Not implemented!
		NVTT_API void setAlphaMode(AlphaMode alphaMode);
		
		// Set gamma settings.
		NVTT_API void setGamma(float inputGamma, float outputGamma);
		
		// Set texture wrappign mode.
		NVTT_API void setWrapMode(WrapMode mode);
		
		// Set mipmapping options.
		NVTT_API void setMipmapFilter(MipmapFilter filter);
		NVTT_API void setMipmapGeneration(bool enabled, int maxLevel = -1);
		NVTT_API void setKaiserParameters(float width, float alpha, float stretch);

		// Set normal map options.
		NVTT_API void setNormalMap(bool b);
		NVTT_API void setConvertToNormalMap(bool convert);
		NVTT_API void setHeightEvaluation(float redScale, float greenScale, float blueScale, float alphaScale);
		NVTT_API void setNormalFilter(float sm, float medium, float big, float large);
		NVTT_API void setNormalizeMipmaps(bool b);
		
		// Set color transforms.
		NVTT_API void setColorTransform(ColorTransform t);
		NVTT_API void setLinearTransform(int channel, float w0, float w1, float w2, float w3);
		NVTT_API void setLinearTransform(int channel, float w0, float w1, float w2, float w3, float offset);
		NVTT_API void setSwizzleTransform(int x, int y, int z, int w);
		
		// Set resizing options.
		NVTT_API void setMaxExtents(int d);
		NVTT_API void setRoundMode(RoundMode mode);

		// Set whether or not to premultiply color by alpha
		NVTT_API void setPremultiplyAlpha(bool b);
	};
	
	
	/// Output handler.
	struct OutputHandler
	{
		virtual ~OutputHandler() {}
		
		/// Indicate the start of a new compressed image that's part of the final texture.
		virtual void beginImage(int size, int width, int height, int depth, int face, int miplevel) = 0;
		
		/// Output data. Compressed data is output as soon as it's generated to minimize memory allocations.
		virtual bool writeData(const void * data, int size) = 0;
	};

	/// Error codes.
	enum Error
	{
		Error_Unknown,
		Error_InvalidInput,
		Error_UnsupportedFeature,
		Error_CudaError,
  		Error_FileOpen,
  		Error_FileWrite,
        Error_UnsupportedOutputFormat,
	};
	
	/// Error handler.
	struct ErrorHandler
	{
		virtual ~ErrorHandler() {}
		
		// Signal error.
		virtual void error(Error e) = 0;
	};

	/// Container.
	enum Container
	{
		Container_DDS,
		Container_DDS10,
	};
	

	/// Output Options. This class holds pointers to the interfaces that are used to report the output of 
	/// the compressor to the user.
	struct OutputOptions
	{
		NVTT_FORBID_COPY(OutputOptions);
		NVTT_DECLARE_PIMPL(OutputOptions);

		NVTT_API OutputOptions();
		NVTT_API ~OutputOptions();
		
		// Set default options.
		NVTT_API void reset();
		
		NVTT_API void setFileName(const char * fileName);
		
		NVTT_API void setOutputHandler(OutputHandler * outputHandler);
		NVTT_API void setErrorHandler(ErrorHandler * errorHandler);
		NVTT_API void setOutputHeader(bool outputHeader);
		NVTT_API void setContainer(Container container);
	};


	/// Texture compressor.
	struct Compressor
	{
		NVTT_FORBID_COPY(Compressor);
		NVTT_DECLARE_PIMPL(Compressor);

		NVTT_API Compressor();
		NVTT_API ~Compressor();

		NVTT_API void enableCudaAcceleration(bool enable);
		NVTT_API bool isCudaAccelerationEnabled() const;

		// Main entrypoint of the compression library.
		NVTT_API bool process(const InputOptions & inputOptions, const CompressionOptions & compressionOptions, const OutputOptions & outputOptions) const;
		
		// Estimate the size of compressing the input with the given options.
		NVTT_API int estimateSize(const InputOptions & inputOptions, const CompressionOptions & compressionOptions) const;

		NVTT_API void outputCompressed(const Texture & tex, const OutputOptions & outputOptions);
	};

	
	/// Texture data.
	struct Texture
	{
		NVTT_DECLARE_PIMPL(Texture);

		NVTT_API Texture();
		NVTT_API ~Texture();

		Texture(const Texture & tex);
		void operator=(const Texture & tex);

		NVTT_API bool load(const char * fileName); // @@ Input callbacks?

		NVTT_API void setType(TextureType type);
		NVTT_API void setTexture2D(InputFormat format, int w, int h, int idx, void * data);

		// Resizing
		NVTT_API void resize(int w, int h, ResizeFilter filter);
		NVTT_API bool buildMipmap(MipmapFilter filter);

		// Color transforms.
		NVTT_API void toLinear(float gamma);
		NVTT_API void toGamma(float gamma);
		NVTT_API void transform(const float w0[4], const float w1[4], const float w2[4], const float w3[4], const float offset[4]);
		NVTT_API void swizzle(int r, int g, int b, int a);
		NVTT_API void scaleBias(int channel, float scale, float bias);
		NVTT_API void normalize();
		NVTT_API void blend(float r, float g, float b, float a);
		NVTT_API void premultiplyAlpha();
	};


	// Return string for the given error code.
	NVTT_API const char * errorString(Error e);

	// Return NVTT version.
	NVTT_API unsigned int version();

	// Set callbacks.
	//NVTT_API void setErrorCallback(ErrorCallback callback);
	//NVTT_API void setMemoryCallbacks(...);	
	
} // nvtt namespace

#endif // NV_TT_H
