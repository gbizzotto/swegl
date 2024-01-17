
#include <memory.h>
#include <stdio.h>
#include <swegl/data/texture.hpp>

namespace swegl
{

	texture_t::texture_t(const char *filename)
	{
		unsigned int file_size;
		unsigned int data_offset;
		unsigned int data_size;
		unsigned short bits_per_pixel;
		int width;
		int height;
		int dummy;
		int texel_count;
		unsigned char r,g,b;
		unsigned int *bitmap;
		unsigned int *previousbitmap;

		FILE *fin = fopen(filename, "rb");
		if (fin == NULL)
			return;

		dummy = fread(&dummy, 1, 2, fin); // magic number
		dummy = fread(&file_size, 1, 4, fin);
		dummy = fread(&dummy, 1, 2, fin); // RFU
		dummy = fread(&dummy, 1, 2, fin); // RFU
		dummy = fread(&data_offset, 1, 4, fin); 
		dummy = fread(&dummy, 1, 4, fin); // header size
		dummy = fread(&width, 1, 4, fin);
		dummy = fread(&height, 1, 4, fin);
		dummy = fread(&dummy, 1, 2, fin); // color planes
		dummy = fread(&bits_per_pixel, 1, 2, fin);
		dummy = fread(&dummy, 1, 4, fin); // compression mode
		dummy = fread(&data_size, 1, 4, fin);
		dummy = fread(&dummy, 1, 4, fin); // horizontal resolution
		dummy = fread(&dummy, 1, 4, fin); // vertical resolution
		dummy = fread(&dummy, 1, 4, fin); // palette size
		dummy = fread(&dummy, 1, 4, fin); // number of important colors

		m_mipmapsCount = MIPMAPS_COUNT;

		texel_count = width*height;
		m_mipmaps = std::make_unique<mipmap_t[]>(MIPMAPS_COUNT);
		if (m_mipmaps == nullptr)
			return;
		m_mipmaps[0].m_bitmap = new unsigned int[texel_count];
		if (m_mipmaps[0].m_bitmap == nullptr)
			return;
		m_mipmaps[0].m_width = width;
		m_mipmaps[0].m_height = height;
		bitmap = m_mipmaps[0].m_bitmap;
		memset(bitmap, 0, texel_count*sizeof(unsigned int));

		int lineoffset = texel_count;
		while (lineoffset > 0)
		{
			lineoffset -= width;
			for (int i=0 ; i<width ; i++)
			{
				dummy = fread(&b, 1, 1, fin);
				dummy = fread(&g, 1, 1, fin);
				dummy = fread(&r, 1, 1, fin);
				bitmap[lineoffset+i] = (r<<16)|(g<<8)|b;
			}
			dummy = fread(&dummy, 1, (width*3)%4, fin); // skipping padding bytes aligning lines on 32bits
		}

		//Calculating mipmaps	
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
				return;
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
	}

	/**
	 * Build a 1-pixel dummy texture
	 */
	texture_t::texture_t(unsigned int rgb)
	{
		m_mipmaps = std::make_unique<mipmap_t[]>(1);
		m_mipmaps[0].m_height = 1;
		m_mipmaps[0].m_width = 1;
		m_mipmaps[0].m_bitmap = new unsigned int[1];
		m_mipmaps[0].m_bitmap[0] = rgb;
	}

	texture_t::texture_t(unsigned int * data, int w, int h)
	{
		m_mipmapsCount = 0;
		m_mipmaps = std::make_unique<mipmap_t[]>(1);
		m_mipmaps[0].m_bitmap = data;
		m_mipmaps[0].m_width = w;
		m_mipmaps[0].m_height = h;
	}

}
