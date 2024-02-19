
#include <memory.h>
#include <swegl/render/viewport.hpp>
#include <swegl/projection/points.hpp>

namespace swegl
{

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
	void viewport_t::flatten(const fraction_t & f, screen_t & front, screen_t & back)
	{
		auto pixel_front = front.pixel_iterator_at_line(m_h * f.numerator/f.denominator);
		auto pixel_back  = back .pixel_iterator_at_line(m_h * f.numerator/f.denominator);
		for (auto pixel_front_end = front.pixel_iterator_at_line(m_h*(f.numerator+1)/f.denominator) ; pixel_front != pixel_front_end ; )
			for (int i=0 ; i<m_w ; i++, pixel_front++, pixel_back++)
				if (pixel_front->o.a != 0)
					*pixel_back = blend(*pixel_back, *pixel_front);
	}

	void viewport_t::flatten(const fraction_t & f, screen_t & front)
	{
		auto pixel_front = front.pixel_iterator_at_line(m_h * f.numerator/f.denominator);
		int y_start = m_h* f.numerator   /f.denominator;
		int y_end   = m_h*(f.numerator+1)/f.denominator;
		auto screen_it = m_screen.iterator_at_line(y_start);
		if (m_got_transparency) // flatten from screen to 1st transparency layer
			for (int j = y_start ; j < y_end ; j++)
				for (int i=0 ; i<m_w ; i++, pixel_front++, screen_it++)
					if (pixel_front->o.a != 0)
						*pixel_front = blend(*screen_it, *pixel_front);
					else
						*pixel_front = *screen_it;
		else // just copy from screen to 1st transparency layer
			for (int j = y_start ; j < y_end  ; j++)
				for (int i=0 ; i<m_w ; i++, pixel_front++, screen_it++)
						*pixel_front = *screen_it;
	}

	void viewport_t::flatten(const fraction_t & f)
	{
		for (size_t i=1 ; i<m_transparency_layers.size() ; i++)
			flatten(f, m_transparency_layers[i], m_transparency_layers[0]);
		flatten(f, m_transparency_layers[0]);
	}

	void viewport_t::clear(const fraction_t & f)
	{
		int y_start = m_h* f.numerator   /f.denominator;
		int y_end   = m_h*(f.numerator+1)/f.denominator;

		auto it  = m_screen.iterator_at_line(y_start);
		auto end = m_screen.iterator_at_line(y_end  );

		if (m_x == 0 && m_w == m_screen.w())
		{
			// we can sweep a whole area with one memset call
			unsigned char * line1_ptr = (unsigned char *) &*it ;
			unsigned char * line2_ptr = (unsigned char *) &*end;
			memset(line1_ptr, 0, line2_ptr-line1_ptr);
		}
		else
		{
			unsigned char * line_ptr = (unsigned char *) &*it;
			int clear_width = m_w * m_screen.bytes_per_pixel();
			int line_width = m_screen.pitch();
			for (int j = y_start ; j < y_end ; j++, line_ptr+=line_width)
				memset(line_ptr, 0, clear_width);
		}

		//std::fill(m_zbuffer.get(), &m_zbuffer[m_w*m_h], std::numeric_limits<std::remove_pointer<typename decltype(m_zbuffer)::pointer>::type>::max());
		char * start_addr_here = (char*) it .z;
		char * start_addr_next = (char*) end.z;
		memset(start_addr_here ,0x7F ,start_addr_next-start_addr_here);
		for (auto & transparency_layer : m_transparency_layers)
		{
			start_addr_here = (char*) & transparency_layer.zbuffer()[m_w * m_h *  f.numerator   /f.denominator];
			start_addr_next = (char*) & transparency_layer.zbuffer()[m_w * m_h * (f.numerator+1)/f.denominator];
			memset(start_addr_here, 0x7F, start_addr_next-start_addr_here);

			start_addr_here = (char*) & transparency_layer.pixels()[m_w * m_h *  f.numerator   /f.denominator];
			start_addr_next = (char*) & transparency_layer.pixels()[m_w * m_h * (f.numerator+1)/f.denominator];
			memset(start_addr_here,    0, start_addr_next-start_addr_here);
		}
	}
	void viewport_t::clear()
	{
		if (m_x == 0 && m_w == m_screen.w())
		{
			// we can sweep a whole area with one memset call
			unsigned char * line1_ptr = (unsigned char *) &*m_screen.begin();
			unsigned char * line2_ptr = (unsigned char *) &*m_screen.end  ();
			memset(line1_ptr, 0, line2_ptr-line1_ptr);
		}
		else
		{
			unsigned char * line_ptr = (unsigned char *) &*m_screen.begin();
			int clear_width = m_w * m_screen.bytes_per_pixel();
			int line_width = m_screen.pitch();
			for (int j=m_y ; j<m_y+m_h ; j++, line_ptr+=line_width)
				memset(line_ptr, 0, clear_width);
		}

		//std::fill(m_zbuffer.get(), &m_zbuffer[m_w*m_h], std::numeric_limits<std::remove_pointer<typename decltype(m_zbuffer)::pointer>::type>::max());
		memset(m_screen.zbuffer(), 0x7F, 4 * m_w * m_h);
		for (auto & transparency_layer : m_transparency_layers)
		{
			memset(transparency_layer.zbuffer(), 0x7F, 4 * m_w * m_h);
			memset(transparency_layer.pixels (), 0, 4 * m_w * m_h);
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
