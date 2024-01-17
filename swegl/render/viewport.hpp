
#pragma once

#include <SDL.h>

#include <swegl/projection/matrix44.hpp>

namespace swegl
{

	class viewport_t
	{
	public:
		int m_x, m_y;
		int m_w, m_h;
		SDL_Surface *m_screen;
		matrix44_t m_viewportmatrix;

		viewport_t(int x, int y, int w, int h, SDL_Surface *screen);

		void clear();
	};

}
