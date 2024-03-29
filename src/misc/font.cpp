
#include <SDL.h>

#include <stdio.h>
#include <swegl/misc/font.hpp>

font_t::font_t(const char *filename)
{
	int dummy;

	FILE *fin = fopen(filename, "rb");
	if (fin == NULL)
		return;

	dummy = fread(&dummy, 1, 2, fin); // magic number
	dummy = fread(&this->file_size, 1, 4, fin);
	dummy = fread(&dummy, 1, 2, fin); // RFU
	dummy = fread(&dummy, 1, 2, fin); // RFU
	dummy = fread(&this->data_offset, 1, 4, fin); 
	dummy = fread(&dummy, 1, 4, fin); // header size
	dummy = fread(&this->width, 1, 4, fin);
	dummy = fread(&this->height, 1, 4, fin);
	dummy = fread(&dummy, 1, 2, fin); // color planes
	dummy = fread(&this->bits_per_pixel, 1, 2, fin);
	dummy = fread(&dummy, 1, 4, fin); // compression mode
	dummy = fread(&this->data_size, 1, 4, fin);
	dummy = fread(&dummy, 1, 4, fin); // horizontal resolution
	dummy = fread(&dummy, 1, 4, fin); // vertical resolution
	dummy = fread(&dummy, 1, 4, fin); // palette size
	dummy = fread(&dummy, 1, 4, fin); // number of important colors

	std::unique_ptr<unsigned char[]> buffer(new unsigned char[data_size]);

	if (data_size != fread(buffer.get(), 1, data_size, fin))
		return;

	data = std::make_unique<unsigned char[]>(16*16*256); // 256 chars, 16*16 pixels

	for (int ch=0 ; ch<256 ; ch++)
	{
		for (int pix=0 ; pix<16*16 ; pix++)
		{
			int ych = 15-(ch/16);
			int xch = ch%16;
			int ypix = 15-(pix/16);
			int xpix = pix%16;
			data[ch*16*16 + pix] = buffer.get()[(  (4096*ych)+(256*ypix)+(16*xch)+xpix  )
			                                       *this->bits_per_pixel/8];
		}
	}
}

void font_t::Print(const char * str, const unsigned x_, unsigned y, SDL_Surface *surface)
{
	auto x = x_;
	for (size_t c = 0; str[c] != 0; ++c)
	{
		if (str[c] == '\n')
		{
			y += 20;
			x = x_;
			continue;
		}
		unsigned char *bitmap = &data[16*16* (str[c])];
		for (unsigned int j=0 ; j<16 ; j++)
			for (unsigned int i=0 ; i<16 ; i++)
				if (bitmap[j*16+i])
					((unsigned int*)surface->pixels)[(int)((y+j)*surface->pitch/4 + x+i + 16*c)] = bitmap[j*16+i]<<16 | bitmap[j*16+i]<<8 | bitmap[j*16+i];
	}
}
