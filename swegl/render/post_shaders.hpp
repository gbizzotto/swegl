
#pragma once

#include <thread>
#include <memory>

#include <SDL2/SDL.h>

#include <swegl/render/colors.hpp>
#include <swegl/misc/lerp.hpp>
#include <swegl/misc/range.hpp>

namespace swegl
{

struct post_shader_t
{
	virtual void shade(viewport_t &) {}
};


struct post_shader_depth_box : public post_shader_t
{
	float focal_distance;
	float focal_depth;
	int hardware_concurrency;
	std::unique_ptr<pixel_colors[]> temp_buffer;

	post_shader_depth_box(float dist, float depth, viewport_t & vp)
		: focal_distance(dist)
		, focal_depth(depth)
		, hardware_concurrency(std::thread::hardware_concurrency())
		, temp_buffer(std::make_unique<pixel_colors[]>(vp.m_h * vp.m_w))
	{}

	void translate_z_to_blur_factor(int y_begin, int y_end, viewport_t & vp)
	{
		for (int y=y_begin ; y<y_end ; y++)
			for (int x=0 ; x<vp.m_w ; x++)
			{
				float & z = vp.zbuffer()[y*vp.m_w + x];
				z = remap_clipped(1.0f, focal_depth, 0.0f, 5.0f, abs(focal_distance-z));
			}
	}

	void do_blur(int y_begin, int y_end, viewport_t & vp)
	{
		pixel_colors * frame_buffer = (pixel_colors *) vp.m_screen->pixels;
		for (int y=y_begin ; y<y_end ; y++)
			for (int x=0 ; x<vp.m_w ; x++)
			{
				//__builtin_prefetch(&frame_buffer[(std::max(0,y-5)+vp.m_y)*vp.m_screen->pitch/4 + std::max(0,x-5)+vp.m_x]);
				int radius = vp.zbuffer()[y*vp.m_w + x];
				int b=0, g=0, r=0;
				int count = 0;
				if (radius == 0)
					temp_buffer[y*vp.m_w + x] = frame_buffer[y*vp.m_screen->pitch/4 + x];
				else
				{
					for (int j=std::max(0,y-radius) ; j<std::min(vp.m_h,y+radius) ; j++)
					{
						for (int i=std::max(0,x-radius) ; i<std::min(vp.m_w,x+radius) ; i++)
						{
							count++;
							pixel_colors & p = frame_buffer[(j+vp.m_y)*vp.m_screen->pitch/4 + i+vp.m_x];
							b += p.o.b;
							g += p.o.g;
							r += p.o.r;
						}
					}
					if (count)
						temp_buffer[y*vp.m_w + x] = pixel_colors(b/count,g/count,r/count);
				}
			}
	};

	void copy_back_to_frame_buffer(int y_begin, int y_end, viewport_t & vp)
	{
		pixel_colors * frame_buffer = (pixel_colors *) vp.m_screen->pixels;
		for (int y=y_begin ; y<y_end ; y++)
			for (int x=0 ; x<vp.m_w ; x++)
				frame_buffer[(y+vp.m_y)*vp.m_screen->pitch/4 + x+vp.m_x] = temp_buffer[y*vp.m_w + x];
	}

	virtual void shade(viewport_t & vp) override
	{
		std::vector<std::thread> vt;
		vt.reserve(hardware_concurrency);

		// translate z-buffer into blur factor
		vt.clear();
		for (int i=0 ; i<hardware_concurrency ; i++)
			vt.emplace_back([&vp,i,this](){ translate_z_to_blur_factor(i*vp.m_h/hardware_concurrency, (i+1)*vp.m_h/hardware_concurrency, vp); });
		for (auto & t : vt)
			t.join();

		// render blurred image into temporary buffer
		vt.clear();
		for (int i=0 ; i<hardware_concurrency ; i++)
			vt.emplace_back([&vp,i,this](){ do_blur(i*vp.m_h/hardware_concurrency, (i+1)*vp.m_h/hardware_concurrency, vp); });
		for (auto & t : vt)
			t.join();

		// copy temporary buffer back to frame_buffer
		vt.clear();
		for (int i=0 ; i<hardware_concurrency ; i++)
			vt.emplace_back([&vp,i,this](){ copy_back_to_frame_buffer(i*vp.m_h/hardware_concurrency, (i+1)*vp.m_h/hardware_concurrency, vp); });
		for (auto & t : vt)
			t.join();
	}
};

} // namespace