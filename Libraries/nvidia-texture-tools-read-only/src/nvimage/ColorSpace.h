// This code is in the public domain -- jim@tilander.org

#ifndef NV_IMAGE_COLORSPACE_H
#define NV_IMAGE_COLORSPACE_H

namespace nv 
{
	class Image;
	
	// Defines simple mappings between different color spaces and encodes them in the 
	// input image.
	namespace ColorSpace
	{
		void RGBtoYCoCg_R(Image* img);
		void YCoCg_RtoRGB(Image* img);
	}
}



#endif
