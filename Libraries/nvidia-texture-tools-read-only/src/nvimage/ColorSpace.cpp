// This code is in the public domain -- jim@tilander.org

#include <nvcore/nvcore.h>

#include <nvmath/Color.h>
#include <nvimage/Image.h>

#include "ColorSpace.h"

namespace nv
{
	void ColorSpace::RGBtoYCoCg_R(Image* img)
	{
		const uint w = img->width();
		const uint h = img->height();
		
		for( uint y=0; y < h; y++ )
		{
			for( uint x=0; x < w; x++ )
			{
				Color32 pixel = img->pixel(x, y);
				
				const int r = pixel.r;
				const int g = pixel.g;
				const int b = pixel.b;
				
				const int Co = r - b;
				const int t  = b + Co/2;
				const int Cg = g - t;
				const int Y  = t + Cg/2;
				
				// Just saturate the chroma here (we loose out of one bit in each channel)
				// this just means that we won't have as high dynamic range. Perhaps a better option
				// is to loose the least significant bit instead?
				pixel.r = clamp(Co + 128, 0, 255);
				pixel.g = clamp(Cg + 128, 0, 255);
				pixel.b = 0;
				pixel.a = Y;
			}
		}
	}
	
	void ColorSpace::YCoCg_RtoRGB(Image* img)
	{
		const uint w = img->width();
		const uint h = img->height();
		
		for( uint y=0; y < h; y++ )
		{
			for( uint x=0; x < w; x++ )
			{
				Color32 pixel = img->pixel(x, y);
				
				const int Co = (int)pixel.r - 128;
				const int Cg = (int)pixel.g - 128;
				const int Y  =      pixel.a;
				
				const int t = Y - Cg/2;
				const int g = Cg + t;
				const int b = t - Co/2;
				const int r = b + Co;
				
				pixel.r = r;
				pixel.g = g;
				pixel.b = b;
				pixel.a = 1;
			}
		}
	}
}
