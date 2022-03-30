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

#ifndef NV_TT_INPUTOPTIONS_H
#define NV_TT_INPUTOPTIONS_H

#include <nvcore/Ptr.h>
#include <nvmath/Vector.h>
#include <nvmath/Matrix.h>
#include <nvimage/Image.h>
#include <nvimage/FloatImage.h>
#include "nvtt.h"

namespace nvtt
{

	struct InputOptions::Private
	{
		Private() : images(NULL) {}
		
		WrapMode wrapMode;
		TextureType textureType;
		InputFormat inputFormat;
		AlphaMode alphaMode;
		
		uint faceCount;
		uint mipmapCount;
		uint imageCount;
		
		struct InputImage;
		InputImage * images;
		
		// Gamma conversion.
		float inputGamma;
		float outputGamma;
		
		// Color transform.
		ColorTransform colorTransform;
		nv::Matrix linearTransform;
		float colorOffsets[4];
		uint swizzleTransform[4];
		
		// Mipmap generation options.
		bool generateMipmaps;
		int maxLevel;
		MipmapFilter mipmapFilter;
		
		// Kaiser filter parameters.
		float kaiserWidth;
		float kaiserAlpha;
		float kaiserStretch;
		
		// Normal map options.
		bool isNormalMap;
		bool normalizeMipmaps;
		bool convertToNormalMap;
		nv::Vector4 heightFactors;
		nv::Vector4 bumpFrequencyScale;
		
		// Adjust extents.
		uint maxExtent;
		RoundMode roundMode;
		
		bool premultiplyAlpha;

		// @@ These are computed in nvtt::compress, so they should be mutable or stored elsewhere...
		mutable uint targetWidth;
		mutable uint targetHeight;
		mutable uint targetDepth;
		mutable uint targetMipmapCount;
		
		void computeTargetExtents() const;
		
		int realMipmapCount() const;
		
		const nv::Image * image(uint face, uint mipmap) const;
		const nv::Image * image(uint idx) const;

		const nv::FloatImage * floatImage(uint idx) const;

	};

	// Internal image structure.
	struct InputOptions::Private::InputImage
	{
		InputImage() {}
		
		bool hasValidData() const { return uint8data != NULL || floatdata != NULL; }
		
		int mipLevel;
		int face;
		
		int width;
		int height;
		int depth;
		
		nv::AutoPtr<nv::Image> uint8data;
		nv::AutoPtr<nv::FloatImage> floatdata;
	};

} // nvtt namespace

#endif // NV_TT_INPUTOPTIONS_H
