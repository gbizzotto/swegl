
#pragma once

#include <thread>
#include <memory>

#include <SDL2/SDL.h>

#include <swegl/render/colors.hpp>
#include <swegl/misc/lerp.hpp>
#include <swegl/misc/fraction.hpp>
#include <swegl/misc/sync.hpp>
#include <swegl/render/viewport.hpp>

namespace swegl
{

struct post_shader_t
{
	virtual void shade(const fraction_t &, screen_t &, viewport_t &) {}
	virtual bool does_copy() { return false; }
};


struct post_shader_depth_box : public post_shader_t
{
	float focal_distance;
	float focal_depth;

	virtual bool does_copy() override { return true; }

	post_shader_depth_box(float dist, float depth)
		: focal_distance(dist)
		, focal_depth(depth)
	{}

	void translate_z_to_blur_factor(int y_begin, int y_end, screen_t & screen)
	{
		for (auto it=screen.z_iterator_at_line(y_begin)
			    ,end=screen.z_iterator_at_line(y_end  )
			; it != end
			; ++it)
		{
			*it.z = remap_clipped(1.0f, focal_depth, 0.0f, 5.0f, abs(focal_distance-*it.z));
		}
	}

	void do_blur(int y_begin, int y_end, screen_t & in, viewport_t & out)
	{
		// By now zbuffer (actually 1st transparency layer's zbuffer) has been translated from Z coord to blur factor

		pixel_colors * source_colors = & in.pixels ()[0];
		float        * blur_factor   = & in.zbuffer()[0];

		auto dst_it  = out.output_screen().iterator_at_line(y_begin + out.m_y);
		auto src_it  = in.iterator_at_line(y_begin);
		auto src_end = in.iterator_at_line(y_end  );
		for ( ; src_it != src_end ; ++src_it,++dst_it)
		{
			int radius = *src_it.z;
			if (radius == 0)
				*dst_it = *src_it;
			else
			{
				int b=0, g=0, r=0;
				int count = 0;
				for (int j=std::max(0,src_it.y-radius) ; j<std::min(in.h(),src_it.y+radius) ; j++)
					for (int i=std::max(0,src_it.x-radius) ; i<std::min(in.w(),src_it.x+radius) ; i++)
					{
						bool focused = blur_factor[j*in.w() + i] == 0; // TODO optimize this out of the loop
						if (! focused)
						{
							count++;
							pixel_colors & p = source_colors[j*in.w() + i]; // TODO optimize this out of the loop
							b += p.o.b;
							g += p.o.g;
							r += p.o.r;
						}
					}
				if (count)
					*dst_it = pixel_colors(b/count,g/count,r/count,255);
			}
		}
	};

	virtual void shade(const fraction_t & f, screen_t & in, viewport_t & out) override
	{
		// translate z-buffer into blur factor
		translate_z_to_blur_factor(out.m_h*f.numerator/f.denominator, out.m_h*(f.numerator+1)/f.denominator, in);

		// wait for all threads to finish up testing triangles
		static sync_point_t sync_point_1;
		sync_point_1.sync(f.denominator);

		// render blurred image into temporary buffer
		do_blur(out.m_h*f.numerator/f.denominator, out.m_h*(f.numerator+1)/f.denominator, in, out);
	}
};

} // namespace