

#pragma once

#include <iostream>
#include <thread>
#include <mutex>
#include <cmath>
#include <algorithm>
#include <freon/OnScopeExit.hpp>
#include <swegl/Projection/Vec2f.h>
#include <swegl/data/model.hpp>
#include <swegl/Projection/Camera.h>
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

void crude_line(viewport_t & m_viewport, int x1, int y1, int x2, int y2);

void fill_triangle(std::vector<vertex_idx> & indices,
                   vertex_idx i0,
                   vertex_idx i1,
                   vertex_idx i2,
                   std::vector<vertex_t> & vertices,
                   std::vector<Vec2f> & texture_mapping,
                   model_t & model,
                   viewport_t & vp,
                   float * zbuffer);
void fill_triangle_2(const std::vector<vertex_idx> & indices,
                     vertex_idx i0,
                     vertex_idx i1,
                     vertex_idx i2,
                     std::vector<vertex_t> & vertices,
                     std::vector<Vec2f> & texture_mapping,
                     model_t & model,
                     viewport_t & vp,
                     float * zbuffer);
void fill_half_triangle(int y, int y_end,
	                    line_side & side_left, line_side & side_right,
                        viewport_t & vp,
                        float * zbuffer,
                        pixel_shader_t & pixel_shader);

class renderer
{
public:
	scene_t & m_scene;
	Camera & m_camera;
	viewport_t & m_viewport;
	float *m_zbuffer;
	inline renderer(scene_t & scene, Camera & camera, viewport_t & viewport, float *zb)
		:m_scene(scene)
		,m_camera(camera)
		,m_viewport(viewport)
		,m_zbuffer(zb)
	{}

	inline void render(post_shader_t & post_shader)
	{
		//auto basic_vertice_transform_matrix = m_camera.m_projectionmatrix * m_camera.m_viewmatrix;

		std::fill(m_zbuffer, m_zbuffer+m_viewport.m_w*m_viewport.m_h, std::numeric_limits<std::remove_pointer<decltype(m_zbuffer)>::type>::max());

		static std::vector<vertex_t> vertices;
		static std::vector<normal_t> normals;

		for (auto & model : m_scene.models)
		{
			model.vertex_shader->shade(vertices, normals, model, m_scene, m_camera, m_viewport);

			model.pixel_shader->prepare_for_model(vertices, normals, model, m_scene, m_camera, m_viewport);

			// STRIPS
			for (triangle_strip & strip : model.mesh.triangle_strips)
			{
				model.pixel_shader->prepare_for_strip(strip);
				for (unsigned int i=2 ; i<strip.indices.size() ; i++)
					if ((i&0x1)==0)
						fill_triangle(strip.indices
									 ,i-2
						             ,i-1
						             ,i  
						             ,vertices
						             ,strip.texture_mapping
						             ,model, m_viewport, m_zbuffer);
					else
						fill_triangle(strip.indices
						             ,i-2
						             ,i  
						             ,i-1
						             ,vertices
						             ,strip.texture_mapping
						             ,model, m_viewport, m_zbuffer);
			}
			// FANS
			for (triangle_fan & fan : model.mesh.triangle_fans)
			{
				model.pixel_shader->prepare_for_fan(fan);
				for (unsigned int i=2 ; i<fan.indices.size() ; i++)
					fill_triangle(fan.indices
					             ,0  
					             ,i-1
					             ,i  
					             ,vertices
						         ,fan.texture_mapping
					             ,model, m_viewport, m_zbuffer);
			}
			// TRIs
			model.pixel_shader->prepare_for_triangle_list(model.mesh.triangle_list);
			for (unsigned int i=2 ; i<model.mesh.triangle_list.indices.size() ; i+= 3)
				fill_triangle(model.mesh.triangle_list.indices
				             ,i-2
				             ,i-1
				             ,i  
				             ,vertices
						     ,model.mesh.triangle_list.texture_mapping
				             ,model, m_viewport, m_zbuffer);
		}

		post_shader.shade(m_viewport, m_zbuffer);
	}
};

void fill_triangle(std::vector<vertex_idx> & indices,
                   vertex_idx i0,
                   vertex_idx i1,
                   vertex_idx i2,
                   std::vector<vertex_t> & vertices,
                   std::vector<Vec2f> & texture_mapping,
                   model_t & model,
                   viewport_t & vp,
                   float * zbuffer)
{
	const vertex_t * v0 = &vertices[indices[i0]];
	const vertex_t * v1 = &vertices[indices[i1]];
	const vertex_t * v2 = &vertices[indices[i2]];

	//ON_SCOPE_EXIT([&](){ model.pixel_shader->next_triangle(); });

	// frustum clipping
	if (  (v0->x()  < vp.m_x        && v1->x()  < vp.m_x        && v2->x()  < vp.m_x       )
		||(v0->y()  < vp.m_y        && v1->y()  < vp.m_y        && v2->y()  < vp.m_y       )
		||(v0->x() >= vp.m_x+vp.m_w && v1->x() >= vp.m_x+vp.m_w && v2->x() >= vp.m_x+vp.m_w)
		||(v0->y() >= vp.m_y+vp.m_h && v1->y() >= vp.m_y+vp.m_h && v2->y() >= vp.m_y+vp.m_h)
	   )
	{
		model.pixel_shader->next_triangle();
		return;
	}

	// backface culling
	// z already inversed by viewmatrix (high Z = far)
	if ( Cross((*v1-*v0),(*v2-*v0)).z() >= 0 )
	{
		model.pixel_shader->next_triangle();
		return;
	}

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

	if (v0->z() < 0.001) // no vertex in front of the camera
	{
		model.pixel_shader->next_triangle();
		return; // Z-near clipping
	}

	if (v1->z() < 0.001) // only v0 in front of the camera
	{
		float cut_1 = (v0->z()-0.001f) / (v0->z() - v1->z());
		vertex_idx new_i1 = vertices.size();
		vertex_idx new_ii1 = indices.size();
		indices.push_back(new_i1);
		vertex_t new_vertex1 = model.mesh.vertices[indices[i0]] + (model.mesh.vertices[indices[i1]]-model.mesh.vertices[indices[i0]])*cut_1;
		model.pixel_shader->push_back_vertex_temporary(new_vertex1);
		vertices.push_back(model.vertex_shader->shade_one(new_vertex1));
		texture_mapping.push_back(texture_mapping[i0] + (texture_mapping[i1]-texture_mapping[i0])*cut_1);

		float cut_2 = (v0->z()-0.001f) / (v0->z() - v2->z());
		vertex_idx new_i2 = vertices.size();
		vertex_idx new_ii2 = indices.size();
		indices.push_back(new_i2);
		vertex_t new_vertex2 = model.mesh.vertices[indices[i0]] + (model.mesh.vertices[indices[i2]]-model.mesh.vertices[indices[i0]])*cut_2;
		model.pixel_shader->push_back_vertex_temporary(new_vertex2);
		vertices.push_back(model.vertex_shader->shade_one(new_vertex2));
		texture_mapping.push_back(texture_mapping[i0] + (texture_mapping[i2]-texture_mapping[i0])*cut_2);

		fill_triangle_2(indices, i0, new_ii1, new_ii2, vertices, texture_mapping, model, vp, zbuffer);
		texture_mapping.pop_back();
		texture_mapping.pop_back();
		indices.pop_back();
		indices.pop_back();
		model.pixel_shader->pop_back_vertex_temporary();
		model.pixel_shader->pop_back_vertex_temporary();

		model.pixel_shader->next_triangle();
		return;
	}

	if (v2->z() < 0.001) // only v2 is in the back of the camera
	{
		float cut_0 = (v0->z()-0.001f) / (v0->z() - v2->z());
		vertex_idx new_i0 = vertices.size();
		vertex_idx new_ii0 = indices.size();
		indices.push_back(new_i0);
		vertex_t new_vertex1 = model.mesh.vertices[indices[i0]] + (model.mesh.vertices[indices[i2]]-model.mesh.vertices[indices[i0]])*cut_0;
		model.pixel_shader->push_back_vertex_temporary(new_vertex1);
		vertices.push_back(model.vertex_shader->shade_one(new_vertex1));
		texture_mapping.push_back(texture_mapping[i0] + (texture_mapping[i2]-texture_mapping[i0])*cut_0);

		float cut_1 = (v1->z()-0.001f) / (v1->z() - v2->z());
		vertex_idx new_i1 = vertices.size();
		vertex_idx new_ii1 = indices.size();
		indices.push_back(new_i1);
		vertex_t new_vertex2 = model.mesh.vertices[indices[i1]] + (model.mesh.vertices[indices[i2]]-model.mesh.vertices[indices[i1]])*cut_1;
		model.pixel_shader->push_back_vertex_temporary(new_vertex2);
		vertices.push_back(model.vertex_shader->shade_one(new_vertex2));
		texture_mapping.push_back(texture_mapping[i1] + (texture_mapping[i2]-texture_mapping[i1])*cut_1);

		fill_triangle_2(indices, i1,      i0, new_ii0, vertices, texture_mapping, model, vp, zbuffer);
		fill_triangle_2(indices, i1, new_ii0, new_ii1, vertices, texture_mapping, model, vp, zbuffer);
		texture_mapping.pop_back();
		texture_mapping.pop_back();
		indices.pop_back();
		indices.pop_back();
		model.pixel_shader->pop_back_vertex_temporary();
		model.pixel_shader->pop_back_vertex_temporary();
		
		model.pixel_shader->next_triangle();
		return;
	}

	fill_triangle_2(indices, i0, i1, i2, vertices, texture_mapping, model, vp, zbuffer);
	model.pixel_shader->next_triangle();
}

void fill_triangle_2(const std::vector<vertex_idx> & indices,
                     vertex_idx i0,
                     vertex_idx i1,
                     vertex_idx i2,
                     std::vector<vertex_t> & vertices,
                     std::vector<Vec2f> & texture_mapping,
                     model_t & model,
                     viewport_t & vp,
                     float * zbuffer)
{
	const vertex_t * v0 = &vertices[indices[i0]];
	const vertex_t * v1 = &vertices[indices[i1]];
	const vertex_t * v2 = &vertices[indices[i2]];

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

	model.pixel_shader->prepare_for_triangle(indices, i0, i1, i2);

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

		model.pixel_shader->prepare_for_upper_triangle(long_line_on_right);

		fill_half_triangle(y, y_end, side_left, side_right, vp, zbuffer, *model.pixel_shader);
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

		model.pixel_shader->prepare_for_lower_triangle(long_line_on_right);

		fill_half_triangle(y, y_end, side_left, side_right, vp, zbuffer, *model.pixel_shader);
	}
}

void fill_half_triangle(int y, int y_end,
	                    line_side & side_left, line_side & side_right,
                        viewport_t & vp,
                        float * zbuffer,
                        pixel_shader_t & pixel_shader)
{
	for ( ; y < y_end ; y++)
	{
		int x1 = std::max((int)ceil(side_left .x), vp.m_x);
		int x2 = std::min((int)ceil(side_right.x), vp.m_x+vp.m_w);
		interpolator_g<1> qpixel;

		pixel_shader.prepare_for_scanline(side_left .interpolator.progress()
		                                 ,side_right.interpolator.progress());

		qpixel.InitSelf(side_right.x - side_left.x,
		            side_left .interpolator.value(0),
		            side_right.interpolator.value(0));
		qpixel.DisplaceStartingPoint(x1 - side_left.x);

		// fill_line
		unsigned int *video = &((unsigned int*)vp.m_screen->pixels)[(int) ( y*vp.m_screen->pitch/4 + x1)];
		float * zb = &zbuffer[(int) ( y*vp.m_w + x1)];
		for ( ; x1 < x2 ; x1++ )
		{
			//int u, v;

			if (qpixel.value(0) < *zb && qpixel.value(0) > 0.001) // Ugly z-near culling
			{
				int color = pixel_shader.shade(qpixel.progress());

				*video = color;
				*zb = qpixel.value(0);
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
