
#include <memory.h>
#include <swegl/render/viewport.hpp>

namespace swegl
{

	viewport_t::viewport_t(int x, int y, int w, int h
	                      ,SDL_Surface *screen
	                      ,std::shared_ptr<swegl::vertex_shader_t> & vertex_shader
	                      ,std::shared_ptr<swegl:: pixel_shader_t> & pixel_shader_smooth
	                      ,std::shared_ptr<swegl:: pixel_shader_t> & pixel_shader_sharp
	                      ,std::shared_ptr<swegl::  post_shader_t> & post_shader
	                      )
		: m_x(x)
		, m_y(y)
		, m_w(w)
		, m_h(h)
		, m_screen(screen)
		, m_zbuffer(new float[w * h])
		, m_viewportmatrix(matrix44_t::Identity)
		, m_vertex_shader(vertex_shader)
		, m_pixel_shader_smooth(pixel_shader_smooth)
		, m_pixel_shader_sharp(pixel_shader_sharp)
		, m_post_shader(post_shader)
	{
		this->m_viewportmatrix[0][3] = x+w/2.0f;
		this->m_viewportmatrix[1][3] = y+h/2.0f;

		if (w>h)
			h = w;
		if (w<h)
			w = h;

		this->m_viewportmatrix[0][0] =  w/2.0f;
		this->m_viewportmatrix[1][1] = -h/2.0f;
		this->m_viewportmatrix[2][2] = 1.0f;
		this->m_viewportmatrix[3][3] = 1.0f;
	}

	void viewport_t::clear()
	{
		unsigned char * line_ptr = &((unsigned char*)m_screen->pixels)[(int) (m_y*m_screen->pitch) + m_x*m_screen->format->BytesPerPixel];
		int clear_width = m_w * m_screen->format->BytesPerPixel;
		int line_width = m_screen->pitch;
		for (int j=m_y ; j<m_y+m_h ; j++, line_ptr+=line_width)
			memset(line_ptr, 0, clear_width);

		std::fill(m_zbuffer.get(), &m_zbuffer[m_w*m_h], std::numeric_limits<std::remove_pointer<typename decltype(m_zbuffer)::pointer>::type>::max());
	}

}
