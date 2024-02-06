
#include <memory.h>
#include <swegl/render/viewport.hpp>
#include <swegl/projection/points.hpp>

namespace swegl
{

	transparency_layer_t::transparency_layer_t(int w, int h)
		: m_colors(std::make_unique<pixel_colors[]>(w*h))
		, m_zbuffer(std::make_unique<float[]>(w*h))
	{}

	viewport_t::viewport_t(int x, int y, int w, int h
	                      ,SDL_Surface *screen
	                      ,std::shared_ptr<swegl:: pixel_shader_t> & pixel_shader
	                      ,int transparency_layer_count
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
		, m_got_transparency(transparency_layer_count > 0)
	{
		this->m_viewportmatrix[0][3] = x+w/2.0f;
		this->m_viewportmatrix[1][3] = y+h/2.0f;
		this->m_viewportmatrix[0][0] =  w/2.0f;
		this->m_viewportmatrix[1][1] = -h/2.0f;
		this->m_viewportmatrix[2][2] = 1.0f;
		this->m_viewportmatrix[3][3] = 1.0f;

		m_transparency_layers.reserve(transparency_layer_count);
		for (int i=0 ; i<std::max(1,transparency_layer_count) ; i++)
			m_transparency_layers.emplace_back(w, h);
	}

	// merge 2 transparency layers
	void viewport_t::flatten(transparency_layer_t & front, transparency_layer_t & back)
	{
		pixel_colors * pixel_front = &front.m_colors[0];
		pixel_colors * pixel_back  = &back .m_colors[0];
		for (int j=0 ; j<m_h ; j++)
			for (int i=0 ; i<m_w ; i++, pixel_front++, pixel_back++)
				if (pixel_front->o.a != 0)
					*pixel_back = blend(*pixel_back, *pixel_front);
	}

	void viewport_t::flatten(transparency_layer_t & front)
	{
		pixel_colors * pixel_front = &front.m_colors[0];
		if (m_got_transparency)
		{
			// flatten from screen to 1st transparency layer
			for (int j=0 ; j<m_h ; j++)
			{
				pixel_colors * pixel_back = &((pixel_colors*)m_screen->pixels)[(int)(j*m_screen->pitch/m_screen->format->BytesPerPixel)];
				for (int i=0 ; i<m_w ; i++, pixel_front++, pixel_back++)
					if (pixel_front->o.a != 0)
						*pixel_front = blend(*pixel_back, *pixel_front);
					else
						*pixel_front = *pixel_back;
			}
		}
		else
		{
			// just copy from screen to 1st transparency layer
			for (int j=0 ; j<m_h ; j++)
			{
				pixel_colors * pixel_back = &((pixel_colors*)m_screen->pixels)[(int)(j*m_screen->pitch/m_screen->format->BytesPerPixel)];
				for (int i=0 ; i<m_w ; i++, pixel_front++, pixel_back++)
						*pixel_front = *pixel_back;
			}
		}
	}

	void viewport_t::flatten()
	{
		for (size_t i=1 ; i<m_transparency_layers.size() ; i++)
			flatten(m_transparency_layers[i], m_transparency_layers[0]);
		flatten(m_transparency_layers[0]);
	}

	void viewport_t::clear(const fraction_t & f)
	{
		clear();
	}
	void viewport_t::clear()
	{
		if (m_x == 0 && m_w == m_screen->w)
		{
			// we can sweep a whole area with one memset call
			unsigned char * line1_ptr = &((unsigned char*)m_screen->pixels)[(int) (m_y*m_screen->pitch)];
			unsigned char * line2_ptr = &((unsigned char*)m_screen->pixels)[(int) ((m_y+m_h)*m_screen->pitch)];
			memset(line1_ptr, 0, line2_ptr-line1_ptr);
		}
		else
		{
			unsigned char * line_ptr = &((unsigned char*)m_screen->pixels)[(int) (m_y*m_screen->pitch) + m_x*m_screen->format->BytesPerPixel];
			int clear_width = m_w * m_screen->format->BytesPerPixel;
			int line_width = m_screen->pitch;
			for (int j=m_y ; j<m_y+m_h ; j++, line_ptr+=line_width)
				memset(line_ptr, 0, clear_width);
		}

		//std::fill(m_zbuffer.get(), &m_zbuffer[m_w*m_h], std::numeric_limits<std::remove_pointer<typename decltype(m_zbuffer)::pointer>::type>::max());
		memset(m_zbuffer.get(), 0x7F, 4 * m_w * m_h);
		for (auto & transparency_layer : m_transparency_layers)
		{
			memset(transparency_layer.m_zbuffer.get(), 0x7F, 4 * m_w * m_h);
			memset(transparency_layer.m_colors .get(), 0, 4 * m_w * m_h);
		}
	}

	vertex_t viewport_t::transform(const vertex_t & v) const
	{
		const auto & m = m_viewportmatrix;
		return vertex_t(m[0][0]*v.x() + m[0][3],
		                m[1][1]*v.y() + m[1][3],
		                        v.z()          );
	}

	void viewport_t::transform(new_mesh_vertex_t & mv) const
	{
		mv.v_viewport = transform(mv.v_viewport);
	}

}
