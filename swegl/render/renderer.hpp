

#pragma once

#include <iostream>
#include <thread>
#include <mutex>
#include <cmath>
#include <algorithm>
#include <freon/OnScopeExit.hpp>
#include <swegl/projection/vec2f.hpp>
#include <swegl/data/model.hpp>
#include <swegl/projection/camera.hpp>
#include <swegl/render/vertex_shaders.hpp>
#include <swegl/render/pixel_shaders.hpp>
#include <swegl/render/post_shaders.hpp>
#include <swegl/render/viewport.hpp>
#include <swegl/render/interpolator.hpp>

namespace swegl
{

struct line_side
{
	interpolator_g<1> interpolator;
	float ratio;
	float x;
};

void crude_line(viewport_t & viewport, int x1, int y1, int x2, int y2);
bool do_triangle(const primitive_t & primitive, vertex_idx i0, vertex_idx i1, vertex_idx i2);
void fill_triangle(vertex_idx i0,
                   vertex_idx i1,
                   vertex_idx i2,
                   node_t & node,
                   primitive_t & primitive,
                   viewport_t & vp,
                   pixel_shader_t & pixel_shader);
void fill_triangle_2(vertex_idx i0,
                     vertex_idx i1,
                     vertex_idx i2,
                     primitive_t & primitive,
                     viewport_t & vp,
                     pixel_shader_t & pixel_shader);
void fill_half_triangle(int y, int y_end,
	                    line_side & side_left, line_side & side_right,
                        viewport_t & vp,
                        pixel_shader_t & pixel_shader);


struct transformed_scene_t
{
	scene_t * original;
	std::vector<node_t> world_view;
	std::vector<node_t> screen_view;
};

inline void _render(scene_t & scene, viewport_t & viewport)
{
	vertex_shader_t::world_to_camera_or_frustum(scene, viewport);
	viewport.clear();

	pixel_shader_t & pixel_shader = *viewport.m_pixel_shader;
	for (auto & node : scene.nodes)
	{
		// determine which vertices will be part of visible triangles and need more transformation 
		for (auto & primitive : node.primitives)
		{
			pixel_shader.prepare_for_primitive(primitive, scene, viewport);

			// STRIPS
			if (primitive.mode == primitive_t::index_mode_t::TRIANGLE_STRIP)
			{
				for (unsigned int i=2 ; i<primitive.indices.size() ; i++)
					if ((i&0x1)==0)
					{
						if (do_triangle(primitive, primitive.indices[i-2],primitive.indices[i-1],primitive.indices[i  ]))
						{
							primitive.vertices[primitive.indices[i-2]].yes = true;
							primitive.vertices[primitive.indices[i-1]].yes = true;
							primitive.vertices[primitive.indices[i  ]].yes = true;
						}
					}
					else
						if (do_triangle(primitive, primitive.indices[i-2],primitive.indices[i  ],primitive.indices[i-1]))
						{
							primitive.vertices[primitive.indices[i-2]].yes = true;
							primitive.vertices[primitive.indices[i-1]].yes = true;
							primitive.vertices[primitive.indices[i  ]].yes = true;
						}
			}
			// FANS
			if (primitive.mode == primitive_t::index_mode_t::TRIANGLE_FAN)
				for (unsigned int i=2 ; i<primitive.indices.size() ; i++)
					if (do_triangle(primitive, primitive.indices[0  ],primitive.indices[i-1],primitive.indices[i  ]))
					{
						primitive.vertices[primitive.indices[0  ]].yes = true;
						primitive.vertices[primitive.indices[i-1]].yes = true;
						primitive.vertices[primitive.indices[i  ]].yes = true;
					}
			// TRIs
			if (primitive.mode == primitive_t::index_mode_t::TRIANGLES)
				for (unsigned int i=2 ; i<primitive.indices.size() ; i+= 3)
					if (do_triangle(primitive
					               ,primitive.indices[i-2]
					               ,primitive.indices[i-1]
					               ,primitive.indices[i  ])
					    )
					{
						primitive.vertices[primitive.indices[i-2]].yes = true;
						primitive.vertices[primitive.indices[i-1]].yes = true;
						primitive.vertices[primitive.indices[i  ]].yes = true;
					}
		}

		// do the rest of the transformations to the vertices that are part of visible triangles
		vertex_shader_t::frustum_to_viewport(node, viewport);

		// do the painting
		for (auto & primitive : node.primitives)
		{
			pixel_shader.prepare_for_primitive(primitive, scene, viewport);

			// STRIPS
			if (primitive.mode == primitive_t::index_mode_t::TRIANGLE_STRIP)
			{
				for (unsigned int i=2 ; i<primitive.indices.size() ; i++)
					if ((i&0x1)==0)
						fill_triangle(primitive.indices[i-2]
						             ,primitive.indices[i-1]
						             ,primitive.indices[i  ]
						             ,node
						             ,primitive
						             ,viewport
						             ,pixel_shader
						             );
					else
						fill_triangle(primitive.indices[i-2]
						             ,primitive.indices[i  ]
						             ,primitive.indices[i-1]
						             ,node
						             ,primitive
						             ,viewport
						             ,pixel_shader
						             );
			}
			// FANSv
			if (primitive.mode == primitive_t::index_mode_t::TRIANGLE_FAN)
				for (unsigned int i=2 ; i<primitive.indices.size() ; i++)
					fill_triangle(primitive.indices[0  ]
					             ,primitive.indices[i-1]
					             ,primitive.indices[i  ]
					             ,node
					             ,primitive
					             ,viewport
					             ,pixel_shader
					             );
			// TRIs
			if (primitive.mode == primitive_t::index_mode_t::TRIANGLES)
				for (unsigned int i=2 ; i<primitive.indices.size() ; i+= 3)
					fill_triangle(primitive.indices[i-2]
					             ,primitive.indices[i-1]
					             ,primitive.indices[i  ]
					             ,node
					             ,primitive
					             ,viewport
					             ,pixel_shader
					             );
		}
	}

	viewport.m_post_shader->shade(viewport);
}

template<typename...T>
void _render(scene_t & scene, viewport_t & viewport, T&...t)
{
	_render(scene, viewport);
	_render(scene, t...);
}

template<typename...T>
void render(scene_t & scene, T&...t)
{
	// Transform scene into world coordinates with vertex shader ONCE for all viewports
	vertex_shader_t::original_to_world(scene);

	_render(scene, t...);
}


bool do_triangle(const primitive_t & primitive, vertex_idx i0, vertex_idx i1, vertex_idx i2)
{
	const vertex_t * v0 = &primitive.vertices[i0].v_viewport;
	const vertex_t * v1 = &primitive.vertices[i1].v_viewport;
	const vertex_t * v2 = &primitive.vertices[i2].v_viewport;

	// backface culling
	// z already inversed by viewmatrix (high Z = far)
	if ( cross((*v1-*v0),(*v2-*v0)).z() <= 0 )
	{
		return false;
	}

	// frustum clipping
	if (  (v0->x()  < -1    && v1->x()  < -1    && v2->x()  < -1   )
		||(v0->y()  < -1    && v1->y()  < -1    && v2->y()  < -1   )
		||(v0->x() >=  1    && v1->x() >=  1    && v2->x() >=  1   )
		||(v0->y() >=  1    && v1->y() >=  1    && v2->y() >=  1   )
		||(v0->z()  < 0.001 && v1->z()  < 0.001 && v2->z()  < 0.001)
	   )
	{
		return false;
	}
	return true;
}

void fill_triangle(vertex_idx i0,
                   vertex_idx i1,
                   vertex_idx i2,
                   node_t & node,
                   primitive_t & primitive,
                   viewport_t & vp,
                   pixel_shader_t & pixel_shader)
{
	if (  !primitive.vertices[i0].yes
	    ||!primitive.vertices[i1].yes
	    ||!primitive.vertices[i2].yes )
	{
	    return;
	}
	const vertex_t * v0 = &primitive.vertices[i0].v_viewport;
	const vertex_t * v1 = &primitive.vertices[i1].v_viewport;
	const vertex_t * v2 = &primitive.vertices[i2].v_viewport;

	// handle cases where 1 or 2 vertices have screen z values below zero (behind the camera)

	// sort by Z DESC
	if (v1->z() > v0->z()) {
		std::swap(v0, v1);
		std::swap(i0, i1);
	}
	if (v2->z() > v1->z()) {
		std::swap(v1, v2);
		std::swap(i1, i2);
	}
	if (v1->z() > v0->z()) {
		std::swap(v0, v1);
		std::swap(i0, i1);
	}

	//if (v0->z() < 0.001) // no vertex in front of the camera
	//	return; // Z-near clipping

	if (v1->z() < 0.001) // only v0 in front of the camera
	{
		float cut_1 = (v0->z()-0.001f) / (v0->z() - v1->z());
		mesh_vertex_t & new_vertex_1 = primitive.vertices.emplace_back();
		new_vertex_1.v_world      = primitive.vertices[i0].v_world      + (primitive.vertices[i1].v_world     -primitive.vertices[i0].v_world     )*cut_1;
		new_vertex_1.tex_coords   = primitive.vertices[i0].tex_coords   + (primitive.vertices[i1].tex_coords  -primitive.vertices[i0].tex_coords  )*cut_1;
		if (primitive.vertices[i1].normal == primitive.vertices[i0].normal)
			new_vertex_1.normal = primitive.vertices[i0].normal;
		else
			new_vertex_1.normal = primitive.vertices[i0].normal         + (primitive.vertices[i1].normal      -primitive.vertices[i0].normal      )*cut_1;
		vertex_shader_t::world_to_viewport(new_vertex_1, node, vp);

		float cut_2 = (v0->z()-0.001f) / (v0->z() - v2->z());
		mesh_vertex_t & new_vertex_2 = primitive.vertices.emplace_back();
		new_vertex_2.v_world      = primitive.vertices[i0].v_world      + (primitive.vertices[i2].v_world     -primitive.vertices[i0].v_world     )*cut_2;
		new_vertex_2.tex_coords   = primitive.vertices[i0].tex_coords   + (primitive.vertices[i2].tex_coords  -primitive.vertices[i0].tex_coords  )*cut_2;
		if (primitive.vertices[i2].normal == primitive.vertices[i0].normal)
			new_vertex_2.normal = primitive.vertices[i0].normal;
		else
			new_vertex_2.normal   = primitive.vertices[i0].normal       + (primitive.vertices[i2].normal      -primitive.vertices[i0].normal      )*cut_2;
		vertex_shader_t::world_to_viewport(new_vertex_2, node, vp);

		fill_triangle_2(i0, primitive.vertices.size()-2, primitive.vertices.size()-1, primitive, vp, pixel_shader);
		primitive.vertices.pop_back();
		primitive.vertices.pop_back();
		return;
	}

	if (v2->z() < 0.001) // only v2 is in the back of the camera
	{
		float cut_0 = (v0->z()-0.001f) / (v0->z() - v2->z());
		mesh_vertex_t & new_vertex_1 = primitive.vertices.emplace_back();
		new_vertex_1.v_world      = primitive.vertices[i0].v_world      + (primitive.vertices[i2].v_world     -primitive.vertices[i0].v_world     )*cut_0;
		new_vertex_1.tex_coords   = primitive.vertices[i0].tex_coords   + (primitive.vertices[i2].tex_coords  -primitive.vertices[i0].tex_coords  )*cut_0;
		if (primitive.vertices[i2].normal == primitive.vertices[i0].normal)
			new_vertex_1.normal = primitive.vertices[i0].normal;
		else
			new_vertex_1.normal   = primitive.vertices[i0].normal       + (primitive.vertices[i2].normal      -primitive.vertices[i0].normal      )*cut_0;
		vertex_shader_t::world_to_viewport(new_vertex_1, node, vp);

		float cut_1 = (v1->z()-0.001f) / (v1->z() - v2->z());
		mesh_vertex_t & new_vertex_2 = primitive.vertices.emplace_back();
		new_vertex_2.v_world      = primitive.vertices[i1].v_world      + (primitive.vertices[i2].v_world     -primitive.vertices[i1].v_world     )*cut_1;
		new_vertex_2.tex_coords   = primitive.vertices[i1].tex_coords   + (primitive.vertices[i2].tex_coords  -primitive.vertices[i1].tex_coords  )*cut_1;
		if (primitive.vertices[i2].normal == primitive.vertices[i1].normal)
			new_vertex_2.normal = primitive.vertices[i1].normal;
		else
			new_vertex_2.normal = primitive.vertices[i1].normal         + (primitive.vertices[i2].normal      -primitive.vertices[i1].normal      )*cut_1;
		vertex_shader_t::world_to_viewport(new_vertex_2, node, vp);

		fill_triangle_2(i1,                          i0, primitive.vertices.size()-2, primitive, vp, pixel_shader);
		fill_triangle_2(i1, primitive.vertices.size()-2, primitive.vertices.size()-1, primitive, vp, pixel_shader);
		primitive.vertices.pop_back();
		primitive.vertices.pop_back();
		return;
	}

	fill_triangle_2(i0, i1, i2, primitive, vp, pixel_shader);
}

void fill_triangle_2([[maybe_unused]] vertex_idx i0,
                     [[maybe_unused]] vertex_idx i1,
                     [[maybe_unused]] vertex_idx i2,
                     [[maybe_unused]] primitive_t & primitive,
                     [[maybe_unused]] viewport_t & vp,
                     [[maybe_unused]] pixel_shader_t & pixel_shader)
{
	const vertex_t * v0 = &primitive.vertices[i0].v_viewport;
	const vertex_t * v1 = &primitive.vertices[i1].v_viewport;
	const vertex_t * v2 = &primitive.vertices[i2].v_viewport;

	// Sort points by screen Y ASC
	if (v1->y() < v0->y()) {
		std::swap(v0, v1);
		std::swap(i0, i1);
	}
	if (v2->y() < v1->y()) {
		std::swap(v1, v2);
		std::swap(i1, i2);
	}
	if (v1->y() < v0->y()) {
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
	side_long.interpolator.InitSelf(v2->y() - v0->y(), v0->z(), v2->z()); // *t0, *t2
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
		side_short.interpolator.InitSelf(v1->y() - v0->y(), v0->z(), v1->z()); // *t0, *t1
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

		fill_half_triangle(y, y_end, side_left, side_right, vp, pixel_shader);
	}

	// lower half of the triangle
	if (y1 < vp.m_y + vp.m_h) // dont skip: at least some part is in the viewport
	{
		side_short.ratio = (v2->x()-v1->x()) / (v2->y()-v1->y()); // never div#0 because y0!=y2
		side_short.interpolator.InitSelf(v2->y() - v1->y(), v1->z(), v2->z()); // *t1, *t2
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

		pixel_shader.prepare_for_lower_triangle(long_line_on_right);

		fill_half_triangle(y, y_end, side_left, side_right, vp, pixel_shader);
	}
}

void fill_half_triangle(int y, int y_end,
	                    line_side & side_left, line_side & side_right,
                        viewport_t & vp,
                        pixel_shader_t & pixel_shader)
{
	for ( ; y < y_end ; y++)
	{
		int x1 = std::max((int)ceil(side_left .x), vp.m_x);
		int x2 = std::min((int)ceil(side_right.x), vp.m_x+vp.m_w);

		if (x1 < x2)
		{
			pixel_shader.prepare_for_scanline(side_left .interpolator.progress()
			                                 ,side_right.interpolator.progress());
			interpolator_g<1> qpixel;
			qpixel.InitSelf(side_right.x - side_left.x,
			            side_left .interpolator.value(0),
			            side_right.interpolator.value(0));
			qpixel.DisplaceStartingPoint(x1 - side_left.x);

			// fill_line
			unsigned int *video = &((unsigned int*)vp.m_screen->pixels)[(int) ( y*vp.m_screen->pitch/vp.m_screen->format->BytesPerPixel + x1)];
			float * zb = &vp.zbuffer()[(int) ( (y-vp.m_y)*vp.m_w + (x1-vp.m_x))];
			for ( ; x1 < x2 ; x1++ )
			{
				if (qpixel.value(0) < *zb && qpixel.value(0) > 0.001) // Ugly z-near clipping
				{
					int color = pixel_shader.shade(qpixel.progress());

					*video = color;
					*zb = qpixel.value(0);
				}
				video++;
				zb++;
				qpixel.Step();
			}
		}
		side_left .x += side_left .ratio;
		side_right.x += side_right.ratio;
		side_left .interpolator.Step();
		side_right.interpolator.Step();
	}
}

void crude_line(viewport_t & vp, int x1, int y1, int x2, int y2)
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
