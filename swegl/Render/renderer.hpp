

#pragma once

#include <iostream>
#include <thread>
#include <mutex>
#include <cmath>
#include <algorithm>
#include <freon/OnScopeExit.hpp>
#include <swegl/Projection/Vec2f.h>
#include <swegl/Data/model.hpp>
#include <swegl/Projection/Camera.h>
#include <swegl/Render/vertex_shaders.hpp>
#include <swegl/Render/pixel_shaders.hpp>
#include <swegl/Render/ViewPort.h>
#include <swegl/Render/interpolator.hpp>

namespace swegl
{

struct line_side
{
	interpolator_t interpolator;
	float ratio;
	float x;
};

void crude_line(ViewPort & m_viewport, int x1, int y1, int x2, int y2);

void fill_triangle(const std::vector<vertex_idx> & indices,
                   vertex_idx i0,
                   vertex_idx i1,
                   vertex_idx i2,
                   std::vector<vertex_t> & vertices,
                   pixel_shader_t & pixel_shader,
                   ViewPort & vp,
                   float * zbuffer);
void fill_half_triangle(int y, int y_end,
	                    line_side & side_left, line_side & side_right,
                        ViewPort & vp,
                        float * zbuffer,
                        pixel_shader_t & pixel_shader);

class renderer
{
public:
	scene_t & m_scene;
	Camera & m_camera;
	ViewPort & m_viewport;
	float *m_zbuffer;
	inline renderer(scene_t & scene, Camera & camera, ViewPort & viewport, float *zb)
		:m_scene(scene)
		,m_camera(camera)
		,m_viewport(viewport)
		,m_zbuffer(zb)
	{}

	inline void render()
	{
		//auto basic_vertice_transform_matrix = m_camera.m_projectionmatrix * m_camera.m_viewmatrix;

		static std::vector<vertex_t> vertices;
		static std::vector<normal_t> normals;

		for (const auto & model : m_scene.models)
		{
			model.vertex_shader->shade(vertices, normals, model, m_scene, m_camera, m_viewport);

			model.pixel_shader->prepare_for_model(vertices, normals, model, m_scene, m_camera, m_viewport);

			// STRIPS
			for (const triangle_strip & strip : model.mesh.triangle_strips)
			{
				model.pixel_shader->prepare_for_strip(strip);
				for (unsigned int i=2 ; i<strip.indices.size() ; i++)
					if ((i&0x1)==0)
						fill_triangle(strip.indices, i-2, i-1, i  , vertices, *model.pixel_shader, m_viewport, m_zbuffer);
					else
						fill_triangle(strip.indices, i-2, i  , i-1, vertices, *model.pixel_shader, m_viewport, m_zbuffer);
			}
			// FANS
			for (const triangle_fan & fan : model.mesh.triangle_fans)
			{
				model.pixel_shader->prepare_for_fan(fan);
				for (unsigned int i=2 ; i<fan.indices.size() ; i++)
					fill_triangle(fan.indices, 0, i-1, i, vertices, *model.pixel_shader, m_viewport, m_zbuffer);
			}
			// TRIs
			model.pixel_shader->prepare_for_triangle_list(model.mesh.triangle_list);
			for (unsigned int i=2 ; i<model.mesh.triangle_list.indices.size() ; i+= 3)
				fill_triangle(model.mesh.triangle_list.indices, i-2, i-1, i, vertices, *model.pixel_shader, m_viewport, m_zbuffer);
		}
	}
};

void fill_triangle(const std::vector<vertex_idx> & indices,
                   vertex_idx i0,
                   vertex_idx i1,
                   vertex_idx i2,
                   std::vector<vertex_t> & vertices,
                   pixel_shader_t & pixel_shader,
                   ViewPort & vp,
                   float * zbuffer)
{
	const vertex_t * v0 = &vertices[indices[i0]];
	const vertex_t * v1 = &vertices[indices[i1]];
	const vertex_t * v2 = &vertices[indices[i2]];

	ON_SCOPE_EXIT([&](){pixel_shader.next_triangle();});

	// frustum clipping
	if (  (v0->x()  < vp.m_x        && v1->x()  < vp.m_x        && v2->x()  < vp.m_x               )
		||(v0->y()  < vp.m_y        && v1->y()  < vp.m_y        && v2->y()  < vp.m_y               )
		||(v0->x() >= vp.m_x+vp.m_w && v1->x() >= vp.m_x+vp.m_w && v2->x() >= vp.m_x+vp.m_w)
		||(v0->y() >= vp.m_y+vp.m_h && v1->y() >= vp.m_y+vp.m_h && v2->y() >= vp.m_y+vp.m_h)
	   )
	{
		return;
	}

	// backface culling
	if ( Cross((*v1-*v0),(*v2-*v0)).z() >= 0 )
		return;

	// Z-near clipping
	if ((*v0).z() < 0.001 && (*v1).z() < 0.001 && (*v2).z() < 0.001)
		return;

	// Sort points
	if (v1->y() <= v0->y()) {
		std::swap(v0, v1);
		std::swap(i0, i1);
	}
	if (v2->y() <= v1->y()) {
		std::swap(v1, v2);
		std::swap(i1, i2);
	}
	if (v1->y() <= v0->y()) {
		std::swap(v0, v1);
		std::swap(i0, i1);
	}

	// get pixel limits
	int y0 = (int) ceil(v0->y());
	int y1 = (int) ceil(v1->y());
	int y2 = (int) ceil(v2->y());

	if (y0==y2) return; // All on 1 scanline, not worth drawing

	line_side side_long;
	line_side side_short;

	// Init long line
	side_long.ratio = (v2->x()-v0->x()) / (v2->y()-v0->y()); // never div#0 because y0!=y2
	side_long.interpolator.Init(v2->y() - v0->y(), v0->z(), v2->z()); // *t0, *t2
	if (y0 < vp.m_y) {
		// start at first scanline of viewport
		side_long.interpolator.DisplaceStartingPoint(vp.m_y-v0->y());
		side_long.x = v0->x() + side_long.ratio*(vp.m_y-v0->y());
	} else {
		side_long.interpolator.DisplaceStartingPoint(y0-v0->y());
		side_long.x = v0->x() + side_long.ratio*(y0-v0->y());
	}

	int y, y_end; // scanlines upper and lower bound of whole triangle

	pixel_shader.prepare_for_triangle(i0, i1, i2);

	// upper half of the triangle
	if (y1 >= vp.m_y) // dont skip: at least some part is in the viewport
	{
		side_short.ratio = (v1->x()-v0->x()) / (v1->y()-v0->y()); // never div#0 because y0!=y2
		side_short.interpolator.Init(v1->y() - v0->y(), v0->z(), v1->z()); // *t0, *t1
		y = std::max(y0, vp.m_y);
		y_end = std::min(y1, vp.m_y + vp.m_h);
		side_short.interpolator.DisplaceStartingPoint(y-v0->y());
		side_short.x = v0->x() + side_short.ratio*(y-v0->y());

		bool long_line_on_right = side_long.ratio > side_short.ratio;
		auto [side_left, side_right] = [&]() -> std::tuple<line_side&,line_side&> {
				if (long_line_on_right)
					return {side_short, side_long};
				else
					return {side_long, side_short};
			}();

		pixel_shader.prepare_for_upper_triangle(long_line_on_right);

		fill_half_triangle(y, y_end, side_left, side_right, vp, zbuffer, pixel_shader);
	}

	// lower half of the triangle
	if (y1 < vp.m_y + vp.m_h) // dont skip: at least some part is in the viewport
	{
		side_short.ratio = (v2->x()-v1->x()) / (v2->y()-v1->y()); // never div#0 because y0!=y2
		side_short.interpolator.Init(v2->y() - v1->y(), v1->z(), v2->z()); // *t1, *t2
		y = std::max(y1, vp.m_y);
		y_end = std::min(y2, vp.m_y + vp.m_h);
		side_short.interpolator.DisplaceStartingPoint(y-v1->y());
		side_short.x = v1->x() + side_short.ratio*(y-v1->y());

		bool long_line_on_right = side_long.ratio < side_short.ratio;
		auto [side_left, side_right] = [&]() -> std::tuple<line_side&,line_side&> {
				if (long_line_on_right)
					return {side_short, side_long};
				else
					return {side_long, side_short};
			}();

		pixel_shader.prepare_for_lower_triangle();

		fill_half_triangle(y, y_end, side_left, side_right, vp, zbuffer, pixel_shader);
	}
}

void fill_half_triangle(int y, int y_end,
	                    line_side & side_left, line_side & side_right,
                        ViewPort & vp,
                        float * zbuffer,
                        pixel_shader_t & pixel_shader)
{
	for ( ; y < y_end ; y++)
	{
		int x1 = std::max((int)ceil(side_left .x), vp.m_x);
		int x2 = std::min((int)ceil(side_right.x), vp.m_x+vp.m_w);
		interpolator_t qpixel;

		pixel_shader.prepare_for_scanline(side_left .interpolator.progress()
		                                 ,side_right.interpolator.progress());


		qpixel.Init(side_right.x - side_left.x,
		            side_left .interpolator.ualpha[0][1],
		            side_right.interpolator.ualpha[0][1]);
			        //Vec2f{side_left .interpolator.ualpha[0][0],side_left .interpolator.ualpha[0][1]},
		            //Vec2f{side_right.interpolator.ualpha[0][0],side_right.interpolator.ualpha[0][1]});
		qpixel.DisplaceStartingPoint(x1 - side_left.x);

		// fill_line
		unsigned int *video = &((unsigned int*)vp.m_screen->pixels)[(int) ( y*vp.m_screen->pitch/4 + x1)];
		float * zb = &zbuffer[(int) ( y*vp.m_w + x1)];
		for ( ; x1 < x2 ; x1++ )
		{
			//int u, v;

			if (qpixel.ualpha[0][1] < *zb && qpixel.ualpha[0][1] > 0.001) // Ugly z-near culling
			{
				int color = pixel_shader.shade(qpixel.progress());

				*video = color;
				*zb = qpixel.ualpha[0][1];
			}
			video++;
			zb++;

			qpixel.Step();
		}

		side_left .x += side_left .ratio;
		side_right.x += side_right.ratio;
		side_left .interpolator.Step();
		side_right.interpolator.Step();
	}
}

void crude_line(ViewPort & vp, int x1, int y1, int x2, int y2)
{
	if ((x2-x1) == 0)
		return;
	if (x2<x1)
		return crude_line(vp, x2, y2, x1, y1);
	for (int x=x1 ; x<=x2 ; x++)
	{
		int y = y1 + (y2-y1)*(x-x1)/(x2-x1);
		unsigned int *video = &((unsigned int*)vp.m_screen->pixels)[(int) ( y*vp.m_screen->pitch/4 + x)];
		if (x<0 || y<0 || x>vp.m_screen->pitch/4 || y>479)
			continue;
		*video = 0xFFFF0000;
	}
}

} // namespace
