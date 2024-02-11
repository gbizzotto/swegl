
#pragma once

#include <cassert>
#include <memory>
#include <SDL2/SDL.h>
#include <swegl/render/colors.hpp>

namespace swegl
{

class screen_t
{
	float        * zbuffer;
	pixel_colors * pixels;
	int w_, h_;
	int x_gap;
	bool owns_zbuffer;
	bool owns_pixels ;

public:
	inline screen_t(int w, int h)
		: zbuffer(new float[w*h])
		, pixels (new pixel_colors[w*h])
		, w_(w)
		, h_(h)
		, x_gap(0)
		, owns_zbuffer(true)
		, owns_pixels(true)
	{}
	inline screen_t(SDL_Surface * sdl_surface)
		: zbuffer(new float[sdl_surface->w*sdl_surface->h])
		, pixels ((pixel_colors*)sdl_surface->pixels)
		, w_(sdl_surface->w)
		, h_(sdl_surface->h)
		, x_gap(sdl_surface->pitch/sdl_surface->format->BytesPerPixel - sdl_surface->w)
		, owns_zbuffer(true)
		, owns_pixels(false)
	{}
	inline ~screen_t()
	{
		if (owns_zbuffer)
			delete[] zbuffer;
		if (owns_pixels )
			delete[] pixels;
	}

	int w() const { return w_; }
	int h() const { return h_; }
	int gap() const { return x_gap; }
	int bytes_per_pixel() const { return 4; }
	int pitch() const { return (w_ + x_gap) * bytes_per_pixel(); }
	      float * z_buffer()       { return zbuffer; }
	const float * z_buffer() const { return zbuffer; }

	template<typename F>
	void for_each(F && f)
	{
		float * z = zbuffer;
		int x = 0;
		int y = 0;
		for (pixel_colors *p=pixels, *p_end=pixels+((w_+x_gap)*h_) ; p < p_end ; p += x_gap, ++y)
			for (pixel_colors *line_end=p+w_ ; p<line_end ; ++p, ++z, ++x)
				f(*z, *p, x, y);
	}

	friend class iterator;
	class iterator
	{
	public:
		float * z;
		int x;
		int y;
		const int y_end;
		const int x_gap;
		const int w;
		pixel_colors * p;
		pixel_colors * p_line_end;

		inline iterator(const screen_t & s, size_t pixel_idx)
			: z( & s.zbuffer[pixel_idx])
			, x(pixel_idx % s.w())
			, y(pixel_idx / s.w())
			, y_end(s.h())
			, x_gap(s.x_gap)
			, w(s.w())
			, p         ( & s.pixels[(pixel_idx / s.w()    ) * (s.w()+s.gap()) + pixel_idx % s.w()])
			, p_line_end( & s.pixels[(pixel_idx / s.w() + 1) * (s.w()+s.gap())                    ])
		{
			assert(pixel_idx <= s.w() * s.h());
		}

		inline void next_line()
		{
			if (y == y_end)
				return;
			z += w;
			++y;
			if (y == y_end)
			{
				p += w - x;
				x = 0;
			}
			else
			{
				p += w;
			}
			p_line_end += w;
		}

		inline iterator() = default;
		inline iterator(const iterator & ) = default;
		inline bool operator==(const iterator & other) { return z == other.z; }
		inline bool operator!=(const iterator & other) { return z != other.z; }
		inline        pixel_colors & operator *()       { return *p; }
		inline const  pixel_colors & operator *() const { return *p; }
		inline        pixel_colors * operator->()       { return  p; }
		inline const  pixel_colors * operator->() const { return  p; }
		inline iterator& operator++() //prefix increment
		{
			if (y == y_end)
				return *this;
			++z;
			++p;
			if (p == p_line_end)
			{
				p += x_gap;
				y++;
				x = 0;
			}
			else
			{
				x++;
			}
			return *this;
		}
		inline iterator operator++(int) // postfix increment
		{
			auto old = *this;
			++(*this);
			return old;
		}
	};

	inline iterator begin()
	{
		return iterator(*this, 0);
	}
	inline iterator end()
	{
		return iterator(*this, w()*h());
	}

	inline iterator iterator_at_line(int y)
	{
		return iterator(*this, y * w());
	}
};
	
} // namespace
