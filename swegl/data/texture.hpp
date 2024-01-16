
#pragma once

namespace swegl
{

	#define MIPMAPS_COUNT 5

	typedef struct
	{
		unsigned int *m_bitmap;
		unsigned int m_width;
		unsigned int m_height;
	
	}Mipmap;

	class texture_t
	{
	public:
		int m_mipmapsCount;
		Mipmap *m_mipmaps;

		texture_t(const char *filename);
		texture_t(unsigned int rgb);
		texture_t(unsigned * data, int w, int h);
	};

}
