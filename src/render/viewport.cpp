
#include <memory.h>
#include <swegl/render/viewport.hpp>
#include <swegl/projection/points.hpp>

namespace swegl
{

	viewport_t::viewport_t(int x, int y, int w, int h
	                      ,SDL_Surface *screen
	                      ,std::shared_ptr<swegl:: pixel_shader_t> & pixel_shader
	                      )
		: m_x(x)
		, m_y(y)
		, m_w(w)
		, m_h(h)
		, m_screen(screen)
		, m_zbuffer(new float[w * h])
		, m_viewportmatrix(matrix44_t::Identity)
		, m_camera(1.0*w/h)
		, m_pixel_shader(pixel_shader)
	{
		this->m_viewportmatrix[0][3] = x+w/2.0f;
		this->m_viewportmatrix[1][3] = y+h/2.0f;
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

	vertex_t viewport_t::transform(const vertex_t & v) const
	{
		const auto & m = m_viewportmatrix;
		return vertex_t(m[0][0]*v.x() + m[0][3],
		                m[1][1]*v.y() + m[1][3],
		                        v.z()          );
	}

	void viewport_t::transform(mesh_vertex_t & mv) const
	{
		const auto & m = m_viewportmatrix;
		mv.v_viewport.x() = m[0][0]*mv.v_viewport.x() + m[0][3];
		mv.v_viewport.y() = m[1][1]*mv.v_viewport.y() + m[1][3];
		mv.v_viewport.z() =         mv.v_viewport.z()          ;

	}

}
