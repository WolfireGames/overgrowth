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

#include "Texture.h"

using namespace nvtt;


Texture::Texture() : m(*new Texture::Private())
{
}

Texture::~Texture()
{
	delete &m;
}

Texture::Texture(const Texture & tex) : m(*new Texture::Private())
{
	// @@ Not implemented.
}

void Texture::operator=(const Texture & tex)
{
	// @@ Not implemented.
}

bool Texture::load(const char * fileName)
{
	// @@ Not implemented.
	return false;
}

void Texture::setType(TextureType type)
{
	m.type = type;
}

void Texture::setTexture2D(InputFormat format, int w, int h, int idx, void * data)
{
	// @@ Not implemented.
}

void Texture::resize(int w, int h, ResizeFilter filter)
{
	// if cubemap, make sure w==h.s
	// @@ Not implemented.
}

bool Texture::buildMipmap(MipmapFilter filter)
{
	// @@ Not implemented.
    return false;
}

// Color transforms.
void Texture::toLinear(float gamma)
{
	foreach(i, m.imageArray)
	{
		m.imageArray[i].toLinear(0, 3, gamma);
	}
}

void Texture::toGamma(float gamma)
{
	foreach(i, m.imageArray)
	{
		m.imageArray[i].toGamma(0, 3, gamma);
	}
}

void Texture::transform(const float w0[4], const float w1[4], const float w2[4], const float w3[4], const float offset[4])
{
	// @@ Not implemented.
}

void Texture::swizzle(int r, int g, int b, int a)
{
	foreach(i, m.imageArray)
	{
		m.imageArray[i].swizzle(0, r, g, b, a);
	}
}

void Texture::scaleBias(int channel, float scale, float bias)
{
	foreach(i, m.imageArray)
	{
		m.imageArray[i].scaleBias(channel, 1, scale, bias);
	}
}

void Texture::normalize()
{
	foreach(i, m.imageArray)
	{
		m.imageArray[i].normalize(0);
	}
}

void Texture::blend(float r, float g, float b, float a)
{
	// @@ Not implemented.
}

void Texture::premultiplyAlpha()
{
	// @@ Not implemented.
}


