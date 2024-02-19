
#pragma once

#include <cassert>
#include <memory>
#include <SDL2/SDL.h>
#include <swegl/render/colors.hpp>

namespace swegl
{

class screen_t
{
	float        * zbuffer_;
	pixel_colors *  pixels_;
	int w_, h_;
	int x_gap;
	bool owns_zbuffer;
	bool owns_pixels ;

public:
	inline screen_t(int w, int h)
		: zbuffer_(new float[w*h])
		,  pixels_(new pixel_colors[w*h])
		, w_(w)
		, h_(h)
		, x_gap(0)
		, owns_zbuffer(true)
		, owns_pixels(true)
	{}
	inline screen_t(SDL_Surface * sdl_surface)
		: zbuffer_(new float[sdl_surface->w*sdl_surface->h])
		,  pixels_((pixel_colors*)sdl_surface->pixels)
		, w_(sdl_surface->w)
		, h_(sdl_surface->h)
		, x_gap(sdl_surface->pitch/sdl_surface->format->BytesPerPixel - sdl_surface->w)
		, owns_zbuffer(true)
		, owns_pixels(false)
	{}
	inline ~screen_t()
	{
		if (owns_zbuffer)
			delete[] zbuffer_;
		if (owns_pixels )
			delete[] pixels_;
	}

	int w() const { return w_; }
	int h() const { return h_; }
	int gap() const { return x_gap; }
	int bytes_per_pixel() const { return 4; }
	int pitch() const { return (w_ + x_gap) * bytes_per_pixel(); }
	      float * zbuffer()       { return zbuffer_; }
	const float * zbuffer() const { return zbuffer_; }
	      pixel_colors *  pixels()       { return pixels_; }
	const pixel_colors *  pixels() const { return pixels_; }

	template<typename F>
	void for_each(F && f)
	{
		float * z = zbuffer_;
		int x = 0;
		int y = 0;
		for (pixel_colors *p=pixels_, *p_end=pixels_+((w_+x_gap)*h_) ; p < p_end ; p += x_gap, ++y)
			for (pixel_colors *line_end=p+w_ ; p<line_end ; ++p, ++z, ++x)
				f(*z, *p, x, y);
	}
	template<typename F>
	void for_each_pixel(F && f)
	{
		int x = 0;
		int y = 0;
		for (pixel_colors *p=pixels_, *p_end=pixels_+((w_+x_gap)*h_) ; p < p_end ; p += x_gap, ++y)
			for (pixel_colors *line_end=p+w_ ; p<line_end ; ++p, ++x)
				f(*p, x, y);
	}
	template<typename F>
	void for_each_z(F && f)
	{
		int x = 0;
		int y = 0;
		for (float *z = zbuffer_, *z_end = &zbuffer_[w_*h_] ; z < z_end ; ++y)
			for (float *z_line_end = &zbuffer_[w_] ; z<z_line_end ; ++z, ++x)
				f(*z, x, y);
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

		inline iterator(const screen_t & s, size_t pixel_idx)
			: z( & s.zbuffer_[pixel_idx])
			, x(pixel_idx % s.w())
			, y(pixel_idx / s.w())
			, y_end(s.h())
			, x_gap(s.x_gap)
			, w(s.w())
			, p( & s.pixels_[(pixel_idx / s.w()) * (s.w()+s.gap()) + pixel_idx % s.w()])
		{
			assert(pixel_idx <= s.w() * s.h());
		}

		inline void next_line()
		{
			if (y == y_end)
				return;
			++y;
			p += w;
			z += w;
			if (y == y_end)
			{
				// move to end() position
				z -= x;
				p -= x;
				x = 0;
			}
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
			++x;
			if (x == w)
			{
				p += x_gap;
				++y;
				x = 0;
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

	friend class pixel_iterator;
	class pixel_iterator
	{
	public:
		int x;
		int y;
		const int y_end;
		const int x_gap;
		const int w;
		pixel_colors * p;

		inline pixel_iterator(const screen_t & s, size_t pixel_idx)
			: x(pixel_idx % s.w())
			, y(pixel_idx / s.w())
			, y_end(s.h())
			, x_gap(s.x_gap)
			, w(s.w())
			, p( & s.pixels_[(pixel_idx / s.w()) * (s.w()+s.gap()) + pixel_idx % s.w()])
		{
			assert(pixel_idx <= s.w() * s.h());
		}

		inline void next_line()
		{
			if (y == y_end)
				return;
			++y;
			p += w;
			if (y == y_end)
			{
				// move to end() position
				p -= x;
				x = 0;
			}
		}

		inline pixel_iterator() = default;
		inline pixel_iterator(const pixel_iterator & ) = default;
		inline bool operator==(const pixel_iterator & other) { return p == other.p; }
		inline bool operator!=(const pixel_iterator & other) { return p != other.p; }
		inline        pixel_colors & operator *()       { return *p; }
		inline const  pixel_colors & operator *() const { return *p; }
		inline        pixel_colors * operator->()       { return  p; }
		inline const  pixel_colors * operator->() const { return  p; }
		inline pixel_iterator& operator++() //prefix increment
		{
			if (y == y_end)
				return *this;
			++p;
			++x;
			if (x == w)
			{
				p += x_gap;
				++y;
				x = 0;
			}
			return *this;
		}
		inline pixel_iterator operator++(int) // postfix increment
		{
			auto old = *this;
			++(*this);
			return old;
		}
	};

	inline pixel_iterator pixel_begin()
	{
		return pixel_iterator(*this, 0);
	}
	inline pixel_iterator pixel_end()
	{
		return pixel_iterator(*this, w()*h());
	}

	inline pixel_iterator pixel_iterator_at_line(int y)
	{
		return pixel_iterator(*this, y * w());
	}


	friend class z_iterator;
	class z_iterator
	{
	public:
		float * z;
		int x;
		int y;
		const int y_end;
		const int w;

		inline z_iterator(const screen_t & s, size_t pixel_idx)
			: z( & s.zbuffer_[pixel_idx])
			, x(pixel_idx % s.w())
			, y(pixel_idx / s.w())
			, y_end(s.h())
			, w(s.w())
		{
			assert(pixel_idx <= s.w() * s.h());
		}

		inline void next_line()
		{
			if (y == y_end)
				return;
			++y;
			z += w;
			if (y == y_end)
			{
				// move to end() position
				z -= x;
				x = 0;
			}
		}

		inline z_iterator() = default;
		inline z_iterator(const z_iterator & ) = default;
		inline bool operator==(const z_iterator & other) { return z == other.z; }
		inline bool operator!=(const z_iterator & other) { return z != other.z; }
		inline        float & operator *()       { return *z; }
		inline const  float & operator *() const { return *z; }
		inline        float * operator->()       { return  z; }
		inline const  float * operator->() const { return  z; }
		inline z_iterator& operator++() //prefix increment
		{
			if (y == y_end)
				return *this;
			++z;
			++x;
			if (x == w)
			{
				y++;
				x = 0;
			}
			return *this;
		}
		inline z_iterator operator++(int) // postfix increment
		{
			auto old = *this;
			++(*this);
			return old;
		}
	};

	inline z_iterator z_begin()
	{
		return z_iterator(*this, 0);
	}
	inline z_iterator z_end()
	{
		return z_iterator(*this, w()*h());
	}

	inline z_iterator z_iterator_at_line(int y)
	{
		return z_iterator(*this, y * w());
	}
};
	
} // namespace
