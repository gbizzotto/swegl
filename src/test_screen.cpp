
#include "headers.hpp"

#include <swegl/misc/sdl.hpp>
#include <swegl/misc/screen.hpp>

int main(int argc, char *argv[])
{
	swegl::sdl_t sdl(400, 1600, 800, 600, "test_1");
	SDL_Surface * surface = sdl.surface;

	swegl::screen_t screen(800, 600);
	screen.for_each([](float & z, swegl::pixel_colors & c, int x, int y)
		{
			c = swegl::pixel_colors(128,0,0,255);
		});

	swegl::screen_t screen_2(sdl.surface);
	screen_2.for_each([](float & z, swegl::pixel_colors & c, int x, int y)
		{
			c = swegl::pixel_colors(0,0,128,255);
		});

	for (swegl::screen_t::iterator it_screen = screen.begin()
		                          ,it_end    = screen.end()
		                          ,it_screen_2 = screen_2.begin()
		;it_screen != it_end
		;++it_screen,++it_screen_2)
	{
		*it_screen_2 = *it_screen_2 + *it_screen;
	}

	for (auto it=screen_2.iterator_at_line(100), end=screen_2.iterator_at_line(200)
		;it!=end
		;it++)
	{
		*it.z = 123.456;
		*it = swegl::pixel_colors(0,128,128,255);
	}

	
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