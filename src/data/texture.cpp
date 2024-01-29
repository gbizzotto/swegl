
#include <memory>
#include <stdio.h>
#include <swegl/data/texture.hpp>

namespace swegl
{

	/**
	 * Build a 1-pixel dummy texture
	 */
	texture_t::texture_t(unsigned int rgb)
	{
		unsigned int * buffer = new unsigned int[1];
		*buffer = rgb;
		m_mipmaps.push_back(std::make_shared<mipmap_t>(buffer, 1, 1));
	}

	texture_t::texture_t(unsigned int * data, int w, int h)
	{
		m_mipmaps.push_back(std::make_shared<mipmap_t>(data, (unsigned int)w, (unsigned int)h));
	}

	void texture_t::produce_mipmaps()
	{
		/*
		//Calculating mipmaps
		unsigned int *previousbitmap;	
		for (int mm=1 ; mm<MIPMAPS_COUNT ; mm++)
		{
			width /= 2;
			height /= 2;

			if (mm==8)
				mm=8;
			if (width < 2 || height < 2) {
				for ( ; mm<MIPMAPS_COUNT ; mm++) {
					m_mipmaps[mm].m_width = 0;
					m_mipmaps[mm].m_height = 0;
					m_mipmaps[mm].m_bitmap = NULL;
				}
				break;
			}

			texel_count = width*height;
			m_mipmaps[mm].m_bitmap = new unsigned int[texel_count];
			if (m_mipmaps[mm].m_bitmap == NULL)
			{
				free(bitmap);
				return texture_t(nullptr, 0, 0);;
			}
			m_mipmaps[mm].m_width = width;
			m_mipmaps[mm].m_height = height;
			previousbitmap = m_mipmaps[mm-1].m_bitmap;
			bitmap = m_mipmaps[mm].m_bitmap;
				//memset(bitmap, 0, texel_count*sizeof(unsigned int));
			for (int y=0 ; y<height ; y++)
			{
				for (int x=0 ; x<width ; x++)
				{
					r = (int) ( (previousbitmap[(y*2  )*width*2+(x*2)] >> 16)
							   +(previousbitmap[(y*2+1)*width*2+(x*2)] >> 16)
							   +(previousbitmap[(y*2  )*width*2+(x*2+1)] >> 16)
							   +(previousbitmap[(y*2+1)*width*2+(x*2+1)] >> 16)
							  ) / 4;
					g = (int) ( ((previousbitmap[(y*2  )*width*2+(x*2)] >> 8) & 0xFF)
							   +((previousbitmap[(y*2+1)*width*2+(x*2)] >> 8) & 0xFF)
							   +((previousbitmap[(y*2  )*width*2+(x*2+1)] >> 8) & 0xFF)
							   +((previousbitmap[(y*2+1)*width*2+(x*2+1)] >> 8) & 0xFF)
							  ) / 4;
					b = (int) ( (previousbitmap[(y*2  )*width*2+(x*2)] & 0xFF)
							   +(previousbitmap[(y*2+1)*width*2+(x*2)] & 0xFF)
							   +(previousbitmap[(y*2  )*width*2+(x*2+1)] & 0xFF)
							   +(previousbitmap[(y*2+1)*width*2+(x*2+1)] & 0xFF)
							  ) / 4;
				
					bitmap[y*width+x] = (r<<16)|(g<<8)|b;
				}
			}
		}
		*/
	}
}
