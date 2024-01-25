
#include "headers.hpp"

#include <swegl/misc/image.hpp>
#include <swegl/misc/sdl.hpp>


int main(int argc, char *argv[])
{
	swegl::sdl_t sdl(10, 1600, 800, 600, "test_1");

	swegl::texture_t texture = swegl::read_png_file(argv[1]);
	if (texture.m_mipmapsCount == 0)
		return 1;

	SDL_Surface * surface = sdl.surface;
	swegl::mipmap_t & mipmap = texture.m_mipmaps[0];
	for (size_t j=0 ; j<mipmap.m_height ; j++)
		for (size_t i=0 ; i<mipmap.m_width ; i++)
			((unsigned int*)surface->pixels)[(int) ( j*surface->pitch/surface->format->BytesPerPixel + i)] = mipmap.m_bitmap[j*mipmap.m_width + i];
	sdl.update_frame();

	for (SDL_Event event;;)
	{
		while (SDL_PollEvent(&event))
			switch (event.type)
			{
				case SDL_KEYDOWN:
					if (event.key.keysym.sym == SDLK_ESCAPE)
						return -1;
					break;
				default:
					break;
			}
	}

	return 0;
}