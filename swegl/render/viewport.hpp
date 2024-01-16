
#pragma once

#include <SDL.h>

#include <swegl/projection/Matrix4x4.h>

namespace swegl
{

	class viewport_t
	{
	public:
		int m_x, m_y;
		int m_w, m_h;
		Matrix4x4 m_viewportmatrix;
		SDL_Surface *m_screen;

		viewport_t(int x, int y, int w, int h, SDL_Surface *screen);

		inline void ShowPoint(int x, int y, unsigned char shade)
		{
			if (x<0 || x>=m_w || y<0 || y>=m_h) return;
			((unsigned int*)m_screen->pixels)[(int) ( y*m_screen->pitch/4 + x)] = shade<<16|shade<<8|shade;
		}
		inline void ShowPoint(int x, int y, unsigned int shade)
		{
			((unsigned int*)m_screen->pixels)[(int) ( y*m_screen->pitch/4 + x)] = shade;
		}
		void HLine(int x0, int x1, int y, unsigned char shade);
		void Clear();
	};

}
