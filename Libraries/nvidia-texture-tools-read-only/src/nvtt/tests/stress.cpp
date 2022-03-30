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

#include <nvtt/nvtt.h>
#include <nvimage/Image.h>
#include <nvimage/ImageIO.h>
#include <nvimage/BlockDXT.h>
#include <nvimage/ColorBlock.h>
#include <nvcore/Ptr.h>
#include <nvcore/Debug.h>
#include <nvcore/StrLib.h>
#include <nvcore/StdStream.h>
#include <nvcore/TextWriter.h>
#include <nvcore/FileSystem.h>

#include <stdlib.h> // free
#include <string.h> // memcpy
#include <time.h> // clock


using namespace nv;

static const char * s_fileNames[] = {
	// Kodak image set
	"kodim01.png",
	"kodim02.png",
	"kodim03.png",
	"kodim04.png",
	"kodim05.png",
	"kodim06.png",
	"kodim07.png",
	"kodim08.png",
	"kodim09.png",
	"kodim10.png",
	"kodim11.png",
	"kodim12.png",
	"kodim13.png",
	"kodim14.png",
	"kodim15.png",
	"kodim16.png",
	"kodim17.png",
	"kodim18.png",
	"kodim19.png",
	"kodim20.png",
	"kodim21.png",
	"kodim22.png",
	"kodim23.png",
	"kodim24.png",
	// Waterloo image set
	"clegg.png",
	"frymire.png",
	"lena.png",
	"monarch.png",
	"peppers.png",
	"sail.png",
	"serrano.png",
	"tulips.png",
	// Epic image set
	"Bradley1.png",
	"Gradient.png",
	"MoreRocks.png",
	"Wall.png",
	"Rainbow.png",
	"Text.png",
};
const int s_fileCount = sizeof(s_fileNames)/sizeof(s_fileNames[0]);


struct MyOutputHandler : public nvtt::OutputHandler
{
	MyOutputHandler() : m_data(NULL), m_ptr(NULL) {}
	~MyOutputHandler()
	{
		free(m_data);
	}

	virtual void beginImage(int size, int width, int height, int depth, int face, int miplevel)
	{
		m_size = size;
		m_width = width;
		m_height = height;
		free(m_data);
		m_data = (unsigned char *)malloc(size);
		m_ptr = m_data;
	}
	
	virtual bool writeData(const void * data, int size)
	{
		memcpy(m_ptr, data, size);
		m_ptr += size;
		return true;
	}

	Image * decompress(nvtt::Format format)
	{
		int bw = (m_width + 3) / 4;
		int bh = (m_height + 3) / 4;

		AutoPtr<Image> img( new Image() );
		img->allocate(m_width, m_height);

		if (format == nvtt::Format_BC1)
		{
			BlockDXT1 * block = (BlockDXT1 *)m_data;

			for (int y = 0; y < bh; y++)
			{
				for (int x = 0; x < bw; x++)
				{
					ColorBlock colors;
					block->decodeBlock(&colors);

					for (int yy = 0; yy < 4; yy++)
					{
						for (int xx = 0; xx < 4; xx++)
						{
							Color32 c = colors.color(xx, yy);

							if (x * 4 + xx < m_width && y * 4 + yy < m_height)
							{
								img->pixel(x * 4 + xx, y * 4 + yy) = c;
							}
						}
					}

					block++;
				}
			}
		}

		return img.release();
	}

	int m_size;
	int m_width;
	int m_height;
	unsigned char * m_data;
	unsigned char * m_ptr;
};


float rmsError(const Image * a, const Image * b)
{
	nvCheck(a != NULL);
	nvCheck(b != NULL);
	nvCheck(a->width() == b->width());
	nvCheck(a->height() == b->height());

	int mse = 0;

	const uint count = a->width() * a->height();

	for (uint i = 0; i < count; i++)
	{
		Color32 c0 = a->pixel(i);
		Color32 c1 = b->pixel(i);

		int r = c0.r - c1.r;
		int g = c0.g - c1.g;
		int b = c0.b - c1.b;
		//int a = c0.a - c1.a;

		mse += r * r;
		mse += g * g;
		mse += b * b;
	}

	return sqrtf(float(mse) / count);
}


int main(int argc, char *argv[])
{
	const uint version = nvtt::version();
	const uint major = version / 100;
	const uint minor = version % 100;
	
	printf("NVIDIA Texture Tools %u.%u - Copyright NVIDIA Corporation 2007 - 2008\n\n", major, minor);
	
	bool fast = false;
	bool nocuda = false;
	bool showHelp = false;
	const char * outPath = "output";
	const char * regressPath = NULL;
	
	// Parse arguments.
	for (int i = 1; i < argc; i++)
	{
		if (strcmp("-fast", argv[i]) == 0)
		{
			fast = true;
		}
		else if (strcmp("-nocuda", argv[i]) == 0)
		{
			nocuda = true;
		}
		else if (strcmp("-help", argv[i]) == 0)
		{
			showHelp = true;
		}
		else if (strcmp("-out", argv[i]) == 0)
		{
			if (i+1 < argc && argv[i+1][0] != '-') outPath = argv[i+1];
		}
		else if (strcmp("-regress", argv[i]) == 0)
		{
			if (i+1 < argc && argv[i+1][0] != '-') regressPath = argv[i+1];
		}
	}

	if (showHelp)
	{
		printf("usage: nvtestsuite [options]\n\n");
		
		printf("Input options:\n");
		printf("  -regress <path>\tRegression directory.\n");

		printf("Compression options:\n");
		printf("  -fast          \tFast compression.\n");
		printf("  -nocuda        \tDo not use cuda compressor.\n");
		
		printf("Output options:\n");
		printf("  -out <path>    \tOutput directory.\n");

		return 1;
	}
	
	nvtt::InputOptions inputOptions;
	inputOptions.setMipmapGeneration(false);

	nvtt::CompressionOptions compressionOptions;
	compressionOptions.setFormat(nvtt::Format_BC1);
	if (fast)
	{
		compressionOptions.setQuality(nvtt::Quality_Fastest);
	}
	else
	{
		compressionOptions.setQuality(nvtt::Quality_Production);
	}

	nvtt::OutputOptions outputOptions;
	outputOptions.setOutputHeader(false);

	MyOutputHandler outputHandler;
	outputOptions.setOutputHandler(&outputHandler);

	nvtt::Compressor compressor;
	compressor.enableCudaAcceleration(!nocuda);

	FileSystem::createDirectory(outPath);

	Path csvFileName;
	csvFileName.format("%s/result.csv", outPath);
	StdOutputStream csvStream(csvFileName);
	TextWriter csvWriter(&csvStream);

	float totalTime = 0;
	float totalRMSE = 0;
	int failedTests = 0;
	float totalDiff = 0;

	for (int i = 0; i < s_fileCount; i++)
	{
		AutoPtr<Image> img( new Image() );
		
		if (!img->load(s_fileNames[i]))
		{
			printf("Input image '%s' not found.\n", s_fileNames[i]);
			return EXIT_FAILURE;
		}

		inputOptions.setTextureLayout(nvtt::TextureType_2D, img->width(), img->height());
		inputOptions.setMipmapData(img->pixels(), img->width(), img->height());

		printf("Compressing: \t'%s'\n", s_fileNames[i]);

		clock_t start = clock();

		compressor.process(inputOptions, compressionOptions, outputOptions);

		clock_t end = clock();
		printf("  Time: \t%.3f sec\n", float(end-start) / CLOCKS_PER_SEC);
		totalTime += float(end-start);

		AutoPtr<Image> img_out( outputHandler.decompress(nvtt::Format_BC1) );

		Path outputFileName;
		outputFileName.format("%s/%s", outPath, s_fileNames[i]);
		outputFileName.stripExtension();
		outputFileName.append(".tga");
		if (!ImageIO::save(outputFileName, img_out.ptr()))
		{
			printf("Error saving file '%s'.\n", outputFileName.str());
		}

		float rmse = rmsError(img.ptr(), img_out.ptr());
		totalRMSE += rmse;

		printf("  RMSE:  \t%.4f\n", rmse);

		// Output csv file
		csvWriter << "\"" << s_fileNames[i] << "\"," << rmse << "\n";

		if (regressPath != NULL)
		{
			Path regressFileName;
			regressFileName.format("%s/%s", regressPath, s_fileNames[i]);
			regressFileName.stripExtension();
			regressFileName.append(".tga");

			AutoPtr<Image> img_reg( new Image() );
			if (!img_reg->load(regressFileName.str()))
			{
				printf("Regression image '%s' not found.\n", regressFileName.str());
				return EXIT_FAILURE;
			}

			float rmse_reg = rmsError(img.ptr(), img_reg.ptr());

			float diff = rmse_reg - rmse;
			totalDiff += diff;

			const char * text = "PASSED";
			if (equal(diff, 0)) text = "PASSED";
			else if (diff < 0) {
				text = "FAILED";
				failedTests++;
			}

			printf("  Diff: \t%.4f (%s)\n", diff, text);
		}

		fflush(stdout);
	}

	totalRMSE /= s_fileCount;
	totalDiff /= s_fileCount;

	printf("Total Results:\n");
	printf("  Total Time: \t%.3f sec\n", totalTime / CLOCKS_PER_SEC);
	printf("  Average RMSE:\t%.4f\n", totalRMSE);

	if (regressPath != NULL)
	{
		printf("Regression Results:\n");
		printf("  Diff: %.4f\n", totalDiff);
		printf("  %d/%d tests failed.\n", failedTests, s_fileCount);
	}

	return EXIT_SUCCESS;
}

