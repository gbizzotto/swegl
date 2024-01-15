
#pragma once

#include <swegl/Render/colors.hpp>
#include <swegl/misc/lerp.hpp>

namespace swegl
{

struct post_shader_t
{
	virtual void shade(ViewPort & m_viewport, float *m_zbuffer) {}
};

struct post_shader_depth : public post_shader_t
{
	float focal_distance;
	float focal_depth;

	post_shader_depth(float dist, float depth)
		: focal_distance(dist)
		, focal_depth(depth)
	{}

	virtual void shade(ViewPort & vp, float * zb) override
	{
		pixel_colors * frame_buffer = (pixel_colors *) vp.m_screen->pixels;

		for (int y=0 ; y<vp.m_h ; y++)
			for (int x=0 ; x<vp.m_w ; x++)
			{
				float z = zb[y*vp.m_w + x];
				if (z == std::numeric_limits<std::remove_pointer<decltype(zb)>::type>::max())
					continue;
				int blur = remap_clipped(0.0f, focal_depth, 0.0f, 5.0f, abs(focal_distance-z));
				int a=0, b=0, c=0, d=0;
				int count = 0;
				for (int j=std::max(0,y-blur) ; j<std::min(vp.m_h,y+blur) ; j++)
					for (int i=std::max(0,x-blur) ; i<std::min(vp.m_w,x+blur) ; i++)
					{
						count++;
						pixel_colors & p = frame_buffer[(j+vp.m_y)*vp.m_screen->pitch/4 + i+vp.m_x];
						a += p.o.a;
						b += p.o.b;
						c += p.o.c;
						d += p.o.d;
					}
				if (count)
					frame_buffer[(y+vp.m_y)*vp.m_screen->pitch/4 + x+vp.m_x] = pixel_colors(a/count,b/count,c/count,d/count);
			}
	}
};

} // namespace