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

#include "CompressRGB.h"
#include "CompressionOptions.h"
#include "OutputOptions.h"

#include <nvimage/Image.h>
#include <nvimage/FloatImage.h>
#include <nvimage/PixelFormat.h>

#include <nvmath/Color.h>
#include <nvmath/Half.h>

#include <nvcore/Debug.h>

using namespace nv;
using namespace nvtt;

namespace 
{

	inline uint computePitch(uint w, uint bitsize)
	{
		uint p = w * ((bitsize + 7) / 8);

		// Align to 32 bits.
		return ((p + 3) / 4) * 4;
	}

	inline void convert_to_a8r8g8b8(const void * src, void * dst, uint w)
	{
		memcpy(dst, src, 4 * w);
	}

	inline void convert_to_x8r8g8b8(const void * src, void * dst, uint w)
	{
		memcpy(dst, src, 4 * w);
	}

} // namespace


// Pixel format converter.
void nv::compressRGB(const Image * image, const OutputOptions::Private & outputOptions, const CompressionOptions::Private & compressionOptions)
{
	nvCheck(image != NULL);

	const uint w = image->width();
	const uint h = image->height();

	uint bitCount;
	uint rmask, rshift, rsize;
	uint gmask, gshift, gsize;
	uint bmask, bshift, bsize;
	uint amask, ashift, asize;

	if (compressionOptions.bitcount != 0)
	{
		bitCount = compressionOptions.bitcount;
		nvCheck(bitCount == 8 || bitCount == 16 || bitCount == 24 || bitCount == 32);

		rmask = compressionOptions.rmask;
		gmask = compressionOptions.gmask;
		bmask = compressionOptions.bmask;
		amask = compressionOptions.amask;

		PixelFormat::maskShiftAndSize(rmask, &rshift, &rsize);
		PixelFormat::maskShiftAndSize(gmask, &gshift, &gsize);
		PixelFormat::maskShiftAndSize(bmask, &bshift, &bsize);
		PixelFormat::maskShiftAndSize(amask, &ashift, &asize);
	}
	else
	{
		rsize = compressionOptions.rsize;
		gsize = compressionOptions.gsize;
		bsize = compressionOptions.bsize;
		asize = compressionOptions.asize;

		bitCount = rsize + gsize + bsize + asize;
		nvCheck(bitCount <= 32);

		ashift = 0;
		bshift = ashift + asize;
		gshift = bshift + bsize;
		rshift = gshift + gsize;

		rmask = ((1 << rsize) - 1) << rshift;
		gmask = ((1 << gsize) - 1) << gshift;
		bmask = ((1 << bsize) - 1) << bshift;
		amask = ((1 << asize) - 1) << ashift;
	}

	const uint byteCount = bitCount / 8;


	// Determine pitch.
	uint pitch = computePitch(w, bitCount);

	uint8 * dst = malloc<uint8>(pitch + 4);

	for (uint y = 0; y < h; y++)
	{
		const Color32 * src = image->scanline(y);

		if (bitCount == 32 && rmask == 0xFF0000 && gmask == 0xFF00 && bmask == 0xFF && amask == 0xFF000000)
		{
			convert_to_a8r8g8b8(src, dst, w);
		}
		else if (bitCount == 32 && rmask == 0xFF0000 && gmask == 0xFF00 && bmask == 0xFF && amask == 0)
		{
			convert_to_x8r8g8b8(src, dst, w);
		}
		else
		{
			// Generic pixel format conversion.
			for (uint x = 0; x < w; x++)
			{
				uint c = 0;
				c |= PixelFormat::convert(src[x].r, 8, rsize) << rshift;
				c |= PixelFormat::convert(src[x].g, 8, gsize) << gshift;
				c |= PixelFormat::convert(src[x].b, 8, bsize) << bshift;
				c |= PixelFormat::convert(src[x].a, 8, asize) << ashift;
				
				// Output one byte at a time.
				for (uint i = 0; i < byteCount; i++)
				{
					*(dst + x * byteCount + i) = (c >> (i * 8)) & 0xFF;
				}
			}
			
			// Zero padding.
			for (uint x = w; x < pitch; x++)
			{
				*(dst + x) = 0;
			}
		}

		if (outputOptions.outputHandler != NULL)
		{
			outputOptions.outputHandler->writeData(dst, pitch);
		}
	}

	free(dst);
}


void nv::compressRGB(const FloatImage * image, const OutputOptions::Private & outputOptions, const CompressionOptions::Private & compressionOptions)
{
	nvCheck(image != NULL);

	const uint w = image->width();
	const uint h = image->height();

	const uint rsize = compressionOptions.rsize;
	const uint gsize = compressionOptions.gsize;
	const uint bsize = compressionOptions.bsize;
	const uint asize = compressionOptions.asize;

	nvCheck(rsize == 0 || rsize == 16 || rsize == 32);
	nvCheck(gsize == 0 || gsize == 16 || gsize == 32);
	nvCheck(bsize == 0 || bsize == 16 || bsize == 32);
	nvCheck(asize == 0 || asize == 16 || asize == 32);

	const uint bitCount = rsize + gsize + bsize + asize;
	const uint byteCount = bitCount / 8;
	const uint pitch = w * byteCount;

	uint8 * dst = (uint8 *)malloc<uint8>(pitch);

	for (uint y = 0; y < h; y++)
	{
		const float * rchannel = image->scanline(y, 0);
		const float * gchannel = image->scanline(y, 1);
		const float * bchannel = image->scanline(y, 2);
		const float * achannel = image->scanline(y, 3);

		union FLOAT
		{
			float f;
			uint32 u;
		};

		uint8 * ptr = dst;

		for (uint x = 0; x < w; x++)
		{
			FLOAT r, g, b, a;
			r.f = rchannel[x];
			g.f = gchannel[x];
			b.f = bchannel[x];
			a.f = achannel[x];

			if (rsize == 32) *((uint32 *)ptr) = r.u;
			else if (rsize == 16) *((uint16 *)ptr) = half_from_float(r.u);
			ptr += rsize / 8;

			if (gsize == 32) *((uint32 *)ptr) = g.u;
			else if (gsize == 16) *((uint16 *)ptr) = half_from_float(g.u);
			ptr += gsize / 8;

			if (bsize == 32) *((uint32 *)ptr) = b.u;
			else if (bsize == 16) *((uint16 *)ptr) = half_from_float(b.u);
			ptr += bsize / 8;

			if (asize == 32) *((uint32 *)ptr) = a.u;
			else if (asize == 16) *((uint16 *)ptr) = half_from_float(a.u);
			ptr += asize / 8;
		}

		if (outputOptions.outputHandler != NULL)
		{
			outputOptions.outputHandler->writeData(dst, pitch);
		}
	}

	free(dst);
}
