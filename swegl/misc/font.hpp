
#pragma once

#include <memory>
#include <SDL.h>

class font_t
{
	unsigned int file_size;
	unsigned int data_offset;

	unsigned int width;
	unsigned int height;
	unsigned short bits_per_pixel;
	unsigned int data_size;
	std::unique_ptr<unsigned char[]> data;

public:
	font_t(const char * filename);
	void Print(const char * str, unsigned x, unsigned y, SDL_Surface *surface);
};
