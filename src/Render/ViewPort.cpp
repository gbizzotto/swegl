
#include <swegl/Render/ViewPort.h>

namespace swegl
{

	ViewPort::ViewPort(int x, int y, int w, int h, SDL_Surface *screen)
	{
		this->m_x = x;
		this->m_y = y;
		this->m_w = w;
		this->m_h = h;
		this->m_screen = screen;

		this->m_viewportmatrix.m_data[0] = w/2.0f;
		this->m_viewportmatrix.m_data[3] = x+w/2.0f;
		this->m_viewportmatrix.m_data[5] = -h/2.0f;
		this->m_viewportmatrix.m_data[7] = y+h/2.0f;
		this->m_viewportmatrix.m_data[10] = 1.0f;
		this->m_viewportmatrix.m_data[15] = 1.0f;
	}


	Vec3f ViewPort::ToPixel(Vec3f &v)
	{
		return v * m_viewportmatrix;;
	}
	/*
	inline void ViewPort::ShowPoint(int x, int y, unsigned char shade)
	{
		if (x<m_x || x>=m_x+m_w)
			return;
		if (y<m_y || y>=m_y+m_h)
			return;

		((unsigned int*)m_screen->pixels)[(int) ( y*m_screen->pitch/4 + x)] = shade<<16|shade<<8|shade;
	}
	*/
	void ViewPort::HLine(int x0, int x1, int y, unsigned char shade)
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

	void ViewPort::Clear()
	{
		for (int j=m_y ; j<m_y+m_h ; j++)
			memset(&((unsigned char*)m_screen->pixels)[(int) (j*m_screen->pitch)], 0, m_w*4);
		//	for (int i=m_x ; i<m_x+m_w ; i++)
		//		((unsigned int*)m_screen->pixels)[(int) (j*m_screen->pitch/4 + i)] = 0;
	}

}
