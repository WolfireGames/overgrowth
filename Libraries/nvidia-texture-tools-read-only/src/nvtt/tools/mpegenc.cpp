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

#include <nvmath/Color.h>

#include <nvimage/Image.h>
#include <nvimage/DirectDrawSurface.h>

#include <nvtt/nvtt.h>

#include "cmdline.h"

extern "C" {
#include <libavcodec/avcodec.h>
//#include <libavformat/avformat.h>
}

// http://ffmpeg.mplayerhq.hu/general.html
// http://cekirdek.pardus.org.tr/~ismail/ffmpeg-docs/apiexample_8c-source.html


using namespace nv;

static float s_quality = 0.5f;

static AVFrame * createPicture(const Image & image)
{
	const uint w = image.width();
	const uint h = image.height();
	const uint size = w * h;
	
	AVFrame * picture = avcodec_alloc_frame();
	
	uint8_t * buffer = (uint8_t *)malloc((size * 3) / 2);
	
	picture->data[0] = buffer;
	picture->data[1] = buffer + size;
	picture->data[2] = buffer + size + size / 4;
	picture->linesize[0] = w;
	picture->linesize[1] = w / 2;
	picture->linesize[2] = w / 2;

	memset(buffer, 0, (size * 3) / 2);
	
	// Convert image to YCbCr 4:2:0
	
	// Y
	for (uint y=0;y<h;y++)
	{
		for (uint x=0;x<w;x++)
		{
			Color32 c = image.pixel(x, y);
			
			float R = (1 / 255.0f) * c.r;
			float G = (1 / 255.0f) * c.g;
			float B = (1 / 255.0f) * c.b;
			
			//float Y = 0.299f * R + 0.587f * G + 0.114f * B;
			float Y = 16  + (65.481f  * R + 128.553f * G +  24.966f * B);
			
			picture->data[0][y * picture->linesize[0] + x] = (uint8)clamp(Y, 0.0f, 255.0f);
		}
	}

	// Cb and Cr
	for (uint y=0;y<h/2;y++)
	{
		for (uint x=0;x<w/2;x++)
		{
			Color32 c0 = image.pixel(2*x+0, 2*y+0);
			Color32 c1 = image.pixel(2*x+1, 2*y+0);
			Color32 c2 = image.pixel(2*x+0, 2*y+1);
			Color32 c3 = image.pixel(2*x+1, 2*y+1);

			float R = (1 / 255.0f) * 0.25f * (c0.r + c1.r + c2.r + c3.r);
			float G = (1 / 255.0f) * 0.25f * (c0.g + c1.g + c2.g + c3.g);
			float B = (1 / 255.0f) * 0.25f * (c0.b + c1.b + c2.b + c3.b);
			
			//float Pb = - 0.168736f * R - 0.331264f * G + 0.5f * B;
			//float Pr = + 0.5f * R - 0.418688f * G - 0.081312f * B;
			float Cb = 128 + (-37.797f * R - 74.203f * G + 112.0f * B);
			float Cr = 128 + (112.0f * R - 93.786 * G - 18.214f * B);
			
			picture->data[1][y * picture->linesize[1] + x] = (uint8)clamp(Cb, 0.0f, 255.0f);;
			picture->data[2][y * picture->linesize[2] + x] = (uint8)clamp(Cr, 0.0f, 255.0f);;
		}
	}
	
	return picture;
}

static void pgm_save(unsigned char *buf, int wrap, int xsize, int ysize, const char * filename)
{
	FILE * f = fopen(filename, "w");
	fprintf(f,"P5\n%d %d\n%d\n",xsize, ysize, 255);
	
	for (int i = 0; i < ysize; i++)
		fwrite(buf + i * wrap,1,xsize,f);
	
	fclose(f);
}

static void savePicture(const AVFrame * picture, int w, int h)
{
	// @@ Combine planes.
	pgm_save(picture->data[0], picture->linesize[0], w, h, "test_y.pgm");
	pgm_save(picture->data[1], picture->linesize[1], w/2, h/2, "test_u.pgm");
	pgm_save(picture->data[2], picture->linesize[2], w/2, h/2, "test_v.pgm");
}

static double psnr(double d) {
	return -10.0*log(d)/log(10.0);
}


static void encodeFrame(const Image & image, CodecID format, Array<uint8> & frame)
{
	AVFrame * picture = createPicture(image);
	
	AVCodec * encoder = avcodec_find_encoder(format);

	if (encoder == NULL)
	{
		printf("MPEG encoder not found.\n");
		exit(1);
	}

	AVCodecContext * encoder_context = avcodec_alloc_context();

	//encoder_context->me_method = 0;
	encoder_context->width = image.width();
	encoder_context->height = image.height();
	encoder_context->pix_fmt = PIX_FMT_YUV420P;
	//encoder_context->pix_fmt = PIX_FMT_YUV422P;
	//encoder_context->pix_fmt = PIX_FMT_YUVJ420P;
	
	encoder_context->time_base = (AVRational){1,25};   // required parameter. 25 fps?
	encoder_context->bit_rate = 400000;   // Quality?
	//encoder_context->bit_rate = 200000;   // Default
	//encoder_context->bit_rate_tolerance = 20000;
	//encoder_context->qmin = ?;
	//encoder_context->qmax = ?;
	//encoder_context->qcompress = ?;
	//encoder_context->qblur = ?;
	
	encoder_context->flags |= CODEC_FLAG_PSNR;
	encoder_context->qcompress = s_quality;
	//encoder_context->qblur = 1.0f;
	//encoder_context->global_quality = FF_QP2LAMBDA * 0;
	//encoder_context->max_qdiff = 3;
	


	
	// Intra frames only
	encoder_context->gop_size = 0;

	if (avcodec_open(encoder_context, encoder) < 0)
	{
		printf("MPEG encoder initialization failed.\n");
		exit(1);
	}

	frame.resize(1024 * 1024, 0);	// resize and initialize to 0.
	
	int out_size = avcodec_encode_video(encoder_context, frame.mutableBuffer(), frame.size(), picture);
	frame.resize(out_size);
	
	// Append sequence end code.
	frame.append(0x00);
	frame.append(0x00);
	frame.append(0x01);
	frame.append(0xb7);
	
	int in_size = image.width() * image.height() * 3;
	printf("Image size %d -> %d (1:%d)\n", in_size, out_size, in_size/out_size);
	printf("PSNR = %4.2f\n", psnr(encoder_context->coded_frame->error[0]/(encoder_context->width*encoder_context->height*255.0*255.0)));

	
	avcodec_close(encoder_context);
	av_free(encoder_context);
	av_free(picture);
}

static void decodeFrame(const Array<uint8> & frame, CodecID format)
{
	AVCodec * decoder = avcodec_find_decoder(format);
	if (decoder == NULL) {
		printf("MPEG decoder not found.\n");
		exit(1);
	}

	AVCodecContext * decoder_context = avcodec_alloc_context();
	AVFrame * picture = avcodec_alloc_frame();
	
	if (decoder->capabilities & CODEC_CAP_TRUNCATED)
		decoder_context->flags |= CODEC_FLAG_TRUNCATED; /* we do not send complete frames */
	
	
	if (avcodec_open(decoder_context, decoder) < 0) {
		printf("MPEG decoder initialization failed.\n");
		exit(1);
	}
	
	//memset(picture->data[0], 0, in_size / 2);
	
	int got_picture = 0;
	int len = avcodec_decode_video(decoder_context, picture, &got_picture, frame.buffer(), frame.size());
	
	printf("decoded %d bytes\n", len);
	
	if (len < 0) {
		printf("Error while decoding frame.\n");
		exit(1);
	}
	
	if (!got_picture) {
		printf("Did not get any picture.\n");
		exit(1);
	}
	
	//nvDebugCheck(outbuf_size == len);
	//nvDebugCheck(got_picture == true);

	savePicture(picture, decoder_context->width, decoder_context->height);
	
	avcodec_close(decoder_context);
	av_free(decoder_context);
	av_free(picture);
}



int main(int argc, char *argv[])
{
	MyAssertHandler assertHandler;
	MyMessageHandler messageHandler;

	nv::Path input;
	nv::Path output;

	// Parse arguments.
	for (int i = 1; i < argc; i++)
	{
		if (strcmp(argv[i], "-q") == 0 || strcmp(argv[i], "--quality") == 0)
		{
			if (i+1 < argc && argv[i+1][0] != '-')
			{
				s_quality = atof(argv[i+1]);
				i++;
			}
		}
			
		else if (argv[i][0] != '-')
		{
			input = argv[i];

			if (i+1 < argc && argv[i+1][0] != '-')
			{
				output = argv[i+1];
				i++;
			}
			else
			{
				output.copy(input.str());
				output.stripExtension();
				output.append(".mpeg");
			}

			break;
		}
	}

	printf("NVIDIA Texture Tools - Copyright NVIDIA Corporation 2007-2008\n\n");

	if (input.isNull())
	{
		printf("usage: nvmpegcompress [options] infile [outfile]\n\n");
		
		return 1;
	}

	// Load image.
	Image image;
	if (!image.load(input))
	{
		fprintf(stderr, "The file '%s' is not a supported image type.\n", input.str());
		return 1;
	}
	
	// Initialize codecs.
	avcodec_init();
	avcodec_register_all();

	//CodecID format = CODEC_ID_MPEG1VIDEO;
	CodecID format = CODEC_ID_MPEG2VIDEO;
	//CodecID format = CODEC_ID_MJPEG;
	//CodecID format = CODEC_ID_THEORA;
	//CodecID format = CODEC_ID_H264;
	
	// Encode frame.
	Array<uint8> frame;
	encodeFrame(image, format, frame);

	// Save resulting I-frame.
	StdOutputStream outputStream(output.str());
	if (outputStream.isError())
	{
		printf("Error opening '%s' for writing.\n", output.str());
		return 1;
	}

	outputStream.serialize(frame.mutableBuffer(), frame.size());

	//decodeFrame(frame, format);
	
	// @@ Compare image against original, and compute RMS.
	
	return 0;
}

