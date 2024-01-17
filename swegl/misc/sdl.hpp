
#pragma once

#include <stdlib.h>
#include <string>
#include <SDL2/SDL.h>

namespace swegl
{

struct sdl_t
{
	char keys[256];
	SDL_Window *window;
	SDL_Renderer *renderer;
	SDL_Surface *surface;
	int l, t;
	int w, h;
	std::string n;

	inline sdl_t(int left, int top, int width, int height, std::string name)
		: keys{0}
		, l(left)
		, t(top)
		, w(width)
		, h(height)
		, n(name)
	{
		// Initialize SDL's subsystems
		if (SDL_Init(SDL_INIT_VIDEO) < 0)
		{
			fprintf(stderr, "Unable to init SDL: %s\n", SDL_GetError());
			throw;
		}

		SDL_SetRelativeMouseMode(SDL_FALSE);
		SDL_ShowCursor(0);

		window = SDL_CreateWindow(n.c_str(), l, t, w, h, 0);
		if (window == nullptr)
			throw;

		renderer = SDL_CreateRenderer(window, -1, 0);
		if (renderer == nullptr)
			throw;
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

		surface = SDL_GetWindowSurface(window);
		//surface = SDL_CreateRGBSurface(0, SCR_WIDTH, SCR_HEIGHT, 32, 0, 0, 0, 0);
		if (surface == nullptr)
			throw;
	}
	inline ~sdl_t()
	{
		SDL_Quit();
	}

	inline void update_frame()
	{
		SDL_UpdateWindowSurface(window);
	}

	inline SDL_Surface * make_surface()
	{
		return SDL_CreateRGBSurface(0, w, h, 32, 0, 0, 0, 0);
	}
};


} // namespace
