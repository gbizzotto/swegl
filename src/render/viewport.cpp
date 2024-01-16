
#include <memory.h>
#include <swegl/render/viewport.hpp>

namespace swegl
{

	viewport_t::viewport_t(int x, int y, int w, int h, SDL_Surface *screen)
		:m_viewportmatrix(matrix44_t::Identity)
	{
		this->m_x = x;
		this->m_y = y;
		this->m_w = w;
		this->m_h = h;
		this->m_screen = screen;

		this->m_viewportmatrix[0][0] = w/2.0f;
		this->m_viewportmatrix[0][3] = x+w/2.0f;
		this->m_viewportmatrix[1][1] = -h/2.0f;
		this->m_viewportmatrix[1][3] = y+h/2.0f;
		this->m_viewportmatrix[2][2] = 1.0f;
		this->m_viewportmatrix[3][3] = 1.0f;
	}


	/*
	inline void viewport_t::ShowPoint(int x, int y, unsigned char shade)
	{
		if (x<m_x || x>=m_x+m_w)
			return;
		if (y<m_y || y>=m_y+m_h)
			return;

		((unsigned int*)m_screen->pixels)[(int) ( y*m_screen->pitch/4 + x)] = shade<<16|shade<<8|shade;
	}
	*/
	void viewport_t::HLine(int x0, int x1, int y, unsigned char shade)
	{
		if (x1<m_x || x0>=m_x+this->m_w)
			return;
		if (y <m_y || y >=m_y+this->m_h)
			return;
		if (x0<m_x) x0=m_x;
		if (x1>=m_x+this->m_w) x1 = m_x+this->m_w-1;

		memset(&(((unsigned int*)m_screen->pixels)[(int) ( y*m_screen->pitch/4 + x0)]),
			   shade<<16|shade<<8|shade,
			   sizeof(int)*(x1-x0));
	}

	void viewport_t::Clear()
	{
		for (int j=m_y ; j<m_y+m_h ; j++)
			memset(&((unsigned char*)m_screen->pixels)[(int) (j*m_screen->pitch)], 0, m_w*4);
		//	for (int i=m_x ; i<m_x+m_w ; i++)
		//		((unsigned int*)m_screen->pixels)[(int) (j*m_screen->pitch/4 + i)] = 0;
	}

}
