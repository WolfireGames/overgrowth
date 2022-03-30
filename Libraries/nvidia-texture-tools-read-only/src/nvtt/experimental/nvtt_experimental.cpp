
#include "nvtt_experimental.h"

struct NvttTexture
{
	NvttTexture() :
		m_constant(false),
		m_image(NULL),
		m_floatImage(NULL)
	{
	}
	
	~NvttTexture()
	{
		if (m_constant && m_image) m_image->unwrap();
		delete m_image;
		delete m_floatImage;
	}
	
	bool m_constant;
	Image * m_image;
	FloatImage * m_floatImage;
};

NvttTexture * nvttCreateTexture() 
{
	return new NvttTexture();
}
	
void nvttDestroyTexture(NvttTexture * tex)
{
	delete tex;
}

void nvttSetImageData(NvttImage * img, NvttInputFormat format, uint w, uint h, void * data)
{
	nvCheck(img != NULL);
	
	if (format == NVTT_InputFormat_BGRA_8UB)
	{
		img->m_constant = false;
		img->m_image->allocate(w, h);
		memcpy(img->m_image->pixels(), data, w * h * 4);
	}
	else
	{
		nvCheck(false);
	}
}

void nvttCompressImage(NvttImage * img, NvttFormat format)
{
	nvCheck(img != NULL);

	// @@ Invoke appropriate compressor.
}



#endif // NVTT_EXPERIMENTAL_H
