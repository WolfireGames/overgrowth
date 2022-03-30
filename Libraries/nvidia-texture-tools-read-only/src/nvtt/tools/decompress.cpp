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

#include <nvcore/StrLib.h>
#include <nvcore/StdStream.h>

#include <nvimage/Image.h>
#include <nvimage/DirectDrawSurface.h>

#include <nvimage/ImageIO.h>

#include "cmdline.h"

#include <time.h> // clock

int main(int argc, char *argv[])
{
	MyAssertHandler assertHandler;
	MyMessageHandler messageHandler;

	bool forcenormal = false;
	bool mipmaps = false;
	bool faces = false;

	nv::Path input;
	nv::Path output;

	// Parse arguments.
	for (int i = 1; i < argc; i++)
 	{
		if (strcmp("-forcenormal", argv[i]) == 0)
		{
			forcenormal = true;
		}
		else if (strcmp("-mipmaps", argv[i]) == 0)
		{
			mipmaps = true;
		}
		else if (strcmp("-faces", argv[i]) == 0)
		{
			faces = true;
		}
		else if (argv[i][0] != '-')
		{
			input = argv[i];

			if (i+1 < argc && argv[i+1][0] != '-')
			{
				output = argv[i+1];
			}
			else
			{
				output.copy(input.str());
				output.stripExtension();
				output.append(".tga");
			}

			break;
		}
	}
	
	printf("NVIDIA Texture Tools - Copyright NVIDIA Corporation 2007\n\n");

	if (input.isNull())
	{
		printf("usage: nvdecompress [options] infile [outfile]\n\n");

		printf("Note: the .tga extension is forced on outfile\n\n");

		printf("Input options:\n");
		printf("  -forcenormal \tThe input image is a normal map.\n");
		printf("  -mipmaps     \tDecompress all mipmaps.\n");
		printf("  -faces       \tDecompress all faces.\n");

 		return 1;
 	}
	
	// Load surface.
	nv::DirectDrawSurface dds(input);
	if (!dds.isValid())
	{
		fprintf(stderr, "The file '%s' is not a valid DDS file.\n", input.str());
		return 1;
	}

	if (!dds.isSupported() || dds.isTexture3D())
	{
		fprintf(stderr, "The file '%s' is not a supported DDS file.\n", input.str());
		return 1;
	}
	
	uint faceCount;
	if (dds.isTexture2D())
	{
		faceCount = 1;
	}
	else
	{
		nvCheck(dds.isTextureCube());
		faceCount = 6;
	}
	
	uint mipmapCount = dds.mipmapCount();
	
	clock_t start = clock();
 
	// apply arguments
	if (forcenormal)
	{
		dds.setNormalFlag(true);
	}
	if (!faces)
	{
		faceCount = 1;
	}
	if (!mipmaps)
	{
		mipmapCount = 1;
	}

	nv::Image mipmap;	
	nv::Path name;

	// strip extension, we force the tga extension
	output.stripExtension();

	// extract faces and mipmaps
	for (uint f = 0; f < faceCount; f++)
	{
		for (uint m = 0; m < mipmapCount; m++)
		{
			dds.mipmap(&mipmap, f, m);
	
			// set output filename, if we are doing faces and/or mipmaps
			name.copy(output);
			if (faces) name.appendFormat("_face%d", f);
			if (mipmaps) name.appendFormat("_mipmap%d", m);
			name.append(".tga");
			
			nv::StdOutputStream stream(name.str());
			if (stream.isError()) {
				fprintf(stderr, "Error opening '%s' for writting\n", name.str());
				return 1;
			}
			
			nv::ImageIO::saveTGA(stream, &mipmap);
		}
	}

	clock_t end = clock();
	printf("\rtime taken: %.3f seconds\n", float(end-start) / CLOCKS_PER_SEC);
	
	return 0;
}

