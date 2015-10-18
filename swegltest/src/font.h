
#ifndef SWE_FONT
#define SWE_FONT

#include <SDL/SDL.h>

class Font
{
	unsigned int file_size;
	unsigned int data_offset;

	unsigned int width;
	unsigned int height;
	unsigned short bits_per_pixel;
	unsigned int data_size;
	unsigned char *data;

public:
	Font(const char * filename);
	void Print(char * str, unsigned x, unsigned y, SDL_Surface *surface);
};

#endif // SWE_FONT
