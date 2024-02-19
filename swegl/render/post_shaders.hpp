
#pragma once

#include <thread>
#include <memory>

#include <SDL2/SDL.h>

#include <swegl/render/colors.hpp>
#include <swegl/misc/lerp.hpp>
#include <swegl/misc/fraction.hpp>

namespace swegl
{

struct post_shader_t
{
	void copy_first_transparency_layer_to_screen(int y_begin, int y_end, viewport_t & vp)
	{
		int * pixel = (int*)&vp.m_transparency_layers[0].pixels()[y_begin*vp.m_w];
		for (auto screen_it = vp.m_screen.iterator_at_line(y_begin), screen_end = vp.m_screen.iterator_at_line(y_end)
			;screen_it!=screen_end
			;++screen_it,++pixel)
		{
			*screen_it = *pixel;
		}
	}

	virtual void shade(const fraction_t & f, viewport_t & vp)
	{
		if (vp.m_got_transparency == false)
			return;
		copy_first_transparency_layer_to_screen(vp.m_h* f.numerator   /f.denominator
		                                       ,vp.m_h*(f.numerator+1)/f.denominator
		                                       ,vp);
	}
};


struct post_shader_depth_box : public post_shader_t
{
	float focal_distance;
	float focal_depth;
	std::unique_ptr<pixel_colors[]> temp_buffer;

	post_shader_depth_box(float dist, float depth, viewport_t & vp)
		: focal_distance(dist)
		, focal_depth(depth)
		, temp_buffer(std::make_unique<pixel_colors[]>(vp.m_h * vp.m_w))
	{}

	void translate_z_to_blur_factor(int y_begin, int y_end, viewport_t & vp)
	{
		float * z = &vp.m_transparency_layers[0].zbuffer()[y_begin*vp.m_w];
		
		for (int y=y_begin ; y<y_end ; y++)
			for (int x=0 ; x<vp.m_w ; x++,z++)
				*z = remap_clipped(1.0f, focal_depth, 0.0f, 5.0f, abs(focal_distance-*z));
	}

	void do_blur(int y_begin, int y_end, viewport_t & vp)
	{
		// By now zbuffer (actually 1st transparency layer's zbuffer) has been translated from Z coord to blur factor

		pixel_colors * source_colors  = (pixel_colors*) &vp.m_transparency_layers[0].pixels ()[0];
		float        * blur_factor =                    &vp.m_transparency_layers[0].zbuffer()[0];
		float        * blur_factor_local = &blur_factor[y_begin*vp.m_w];
		auto screen_it = vp.m_screen.iterator_at_line(y_begin);
		for (int y=y_begin ; y<y_end ; y++)
		{
			//pixel_colors * dest_colors = & ((pixel_colors *) vp.m_screen->pixels)[(int)(y*vp.m_screen->pitch/vp.m_screen->format->BytesPerPixel)];
			for (int x=0 ; x<vp.m_w ; x++, ++source_colors, ++screen_it, ++blur_factor_local)
			{
				int radius = *blur_factor_local;
				if (radius == 0)
					*screen_it = *source_colors;
				else
				{
					int b=0, g=0, r=0;
					int count = 0;
					for (int j=std::max(0,y-radius) ; j<std::min(vp.m_h,y+radius) ; j++)
					{
						for (int i=std::max(0,x-radius) ; i<std::min(vp.m_w,x+radius) ; i++)
						{
							bool focused = blur_factor[j*vp.m_w + i] == 0;
							if (! focused)
							{
								count++;
								pixel_colors & p = source_colors[(j+vp.m_y)*vp.m_screen.pitch()/4 + i+vp.m_x];
								b += p.o.b;
								g += p.o.g;
								r += p.o.r;
							}
						}
					}
					if (count)
						*screen_it = pixel_colors(b/count,g/count,r/count,255);
				}
			}
		}
	};

	virtual void shade(const fraction_t & f, viewport_t & vp) override
	{
		// translate z-buffer into blur factor
		translate_z_to_blur_factor(vp.m_h*f.numerator/f.denominator, vp.m_h*(f.numerator+1)/f.denominator, vp);

		// render blurred image into temporary buffer
		do_blur(vp.m_h*f.numerator/f.denominator, vp.m_h*(f.numerator+1)/f.denominator, vp);
	}
};

} // namespace