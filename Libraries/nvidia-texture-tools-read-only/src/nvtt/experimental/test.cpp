
#include "nvtt_experimental.h"

/*
Errors in the original API:
- Too many memory copies.
- Implementation too complicated.
- Error output should not be in output options.
- Data driven interface. Follows the dialog model. Provide all the data upfront.
*/


// Output texture with mipmaps
void example0()
{
	CompressionOptions compressionOptions;
	OutputOptions outputOptions;
	
	Texture img;
	img.setTexture2D(format, w, h, 0, data);

	Compressor context;
	context.outputHeader(outputOptions);
	context.outputCompressed(img, compressionOptions, outputOptions);

	img.toLinear(2.2);	
	while (img.downsample(NVTT_FILTER_BOX))
	{
		img.toGamma(2.2);	
		outputCompressed(img, compressionOptions, outputOptions);		
	}
}


// Output texture with colored mipmaps
void example1()
{
	CompressionOptions compressionOptions;
	OutputOptions outputOptions;
	
	Texture img;
	img.setTexture2D(format, w, h, 0, data);

	Compressor context;
	context.outputHeader(outputOptions);
	context.outputCompressed(img, compressionOptions, outputOptions);

	img.toLinear(2.2);	
	while (img.downsample(NVTT_FILTER_BOX))
	{
		img.toGamma(2.2);
		
		Texture mipmap = img;
		mipmap.blend(color[i].r, color[i].g, color[i].b, 0.5f);
		
		context.outputCompressed(mipmap, compressionOptions, outputOptions);		
	}
}



