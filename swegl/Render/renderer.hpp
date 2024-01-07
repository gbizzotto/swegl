

#pragma once

#include <iostream>
#include <thread>
#include <mutex>
#include <cmath>
#include <algorithm>
#include <swegl/Render/Filler.h>
#include <swegl/Projection/Vec2f.h>
#include <swegl/Data/model.hpp>
#include <swegl/Projection/Camera.h>
#include <swegl/Render/ViewPort.h>
#include <swegl/Render/ZInterpolator.h>

namespace swegl
{

struct line_side
{
	ZInterpolator interpolator;
	float ratio;
	float x;
};

void fill_flat_poly(const vertex_t * v0, const vertex_t * v1, const vertex_t * v2,
                    const Vec2f * t0, const Vec2f * t1, const Vec2f * t2,
                    float light,
                    const std::shared_ptr<Texture> & texture,
                    ViewPort & vp,
                    float * zbuffer,
                    const scene & scene);
void fill_half_tri(int y, int y_end,
	               line_side & side_left, line_side & side_right,
                   const std::shared_ptr<Texture> & t,
                   float light,
                   ViewPort & vp,
                   float * zbuffer);
void crude_line(ViewPort & m_viewport, int x1, int y1, int x2, int y2);

class renderer
{
public:
	scene & m_scene;
	Camera & m_camera;
	ViewPort & m_viewport;
	float *m_zbuffer;
	inline renderer(scene & scene, Camera & camera, ViewPort & viewport, float *zb)
		:m_scene(scene)
		,m_camera(camera)
		,m_viewport(viewport)
		,m_zbuffer(zb)
	{}

	inline void render_normals()
	{
	}

	inline void render()
	{
		//auto basic_vertice_transform_matrix = m_camera.m_projectionmatrix * m_camera.m_viewmatrix;

		static std::vector<vertex_t> vertices;
		static std::vector<normal_t> normals;

		for (const auto & model : m_scene.models)
		{
			auto model_matrix = model.orientation;
			model_matrix.Translate(model.position.x(), model.position.y(), model.position.z());
			auto world_matrix = m_camera.m_viewmatrix * model_matrix;
			auto vertice_transform_matrix = m_camera.m_projectionmatrix * world_matrix;

			vertices.clear();
			vertices.reserve(model.mesh.vertices.size());
			for (const vertex_t & v : model.mesh.vertices)
			{
				vertex_t vec = Transform(v, vertice_transform_matrix);
				vec.x() /= fabs(vec.z());
				vec.y() /= fabs(vec.z());
				vertices.emplace_back(Transform(vec, m_viewport.m_viewportmatrix));
			}

			
			// Reading index buffer and drawing lines
			// STRIPS
			for (const triangle_strip & strip : model.mesh.triangle_strips)
			{
				normals.clear();
				normals.reserve(strip.normals.size());
				for (const auto & n : strip.normals)
					normals.emplace_back(Transform(n, model.orientation));

				const vertex_t * v2;
				const vertex_t * v0 = &vertices[strip.indices[0]];
				const vertex_t * v1 = &vertices[strip.indices[1]];
				Vec2f t2;
				Vec2f t0 = strip.texture_mapping[0];
				Vec2f t1 = strip.texture_mapping[1];

				t0[0][0] *= model.mesh.textures[0]->m_mipmaps[0].m_width;
				t0[0][1] *= model.mesh.textures[0]->m_mipmaps[0].m_height;
				t1[0][0] *= model.mesh.textures[0]->m_mipmaps[0].m_width;
				t1[0][1] *= model.mesh.textures[0]->m_mipmaps[0].m_height;

				for (unsigned int i=2 ; i<strip.indices.size() ; i++, v0=v1, v1=v2, t0=t1, t1=t2)
				{
					t2 = strip.texture_mapping[i];
					t2[0][0] *= model.mesh.textures[0]->m_mipmaps[0].m_width;
					t2[0][1] *= model.mesh.textures[0]->m_mipmaps[0].m_height;
					v2 = &vertices[strip.indices[i]];

					// frustum clipping
					if (      (v0->x()  < m_viewport.m_x                && v1->x()  < m_viewport.m_x                && v2->x()  < m_viewport.m_x               )
							||(v0->y()  < m_viewport.m_y                && v1->y()  < m_viewport.m_y                && v2->y()  < m_viewport.m_y               )
							||(v0->x() >= m_viewport.m_x+m_viewport.m_w && v1->x() >= m_viewport.m_x+m_viewport.m_w && v2->x() >= m_viewport.m_x+m_viewport.m_w)
							||(v0->y() >= m_viewport.m_y+m_viewport.m_h && v1->y() >= m_viewport.m_y+m_viewport.m_h && v2->y() >= m_viewport.m_y+m_viewport.m_h)
					   )
					{
						continue;
					}

					// backface culling
					if ((i&0x1)==0) {
						if ( Cross((*v1-*v0),(*v2-*v0)).z() >= 0 )
							continue;
					} else {
						if ( Cross((*v2-*v0),(*v1-*v0)).z() >= 0 )
							continue;
					}

					// Z-near clipping
					if ((*v0).z() < 0.001 && (*v1).z() < 0.001 && (*v2).z() < 0.001)
						continue;

					float face_sun_intensity = -normals[i-2].Dot(m_scene.sun_direction);
					if (face_sun_intensity < 0.0f)
						face_sun_intensity = 0.0f;
					float light = m_scene.ambient_light_intensity + face_sun_intensity*m_scene.sun_intensity;
					if (light > 1.0f)
						light = 1.0f;

					// Fill poly
					fill_flat_poly( v0,  v1,  v2,
					               &t0, &t1, &t2,
					               light,
					               model.mesh.textures[0], m_viewport, m_zbuffer,
					               m_scene);
				}
			}
			// FANS
			for (const triangle_fan & fan : model.mesh.triangle_fans)
			{
				normals.clear();
				normals.reserve(fan.normals.size());
				for (const auto & n : fan.normals)
					normals.emplace_back(Transform(n, model.orientation));

				const vertex_t * v2;
				const vertex_t * v0 = &vertices[fan.indices[0]];
				const vertex_t * v1 = &vertices[fan.indices[1]];
				Vec2f t2;
				Vec2f t0 = fan.texture_mapping[0];
				Vec2f t1 = fan.texture_mapping[1];
				t0[0][0] *= model.mesh.textures[0]->m_mipmaps[0].m_width;
				t0[0][1] *= model.mesh.textures[0]->m_mipmaps[0].m_height;
				t1[0][0] *= model.mesh.textures[0]->m_mipmaps[0].m_width;
				t1[0][1] *= model.mesh.textures[0]->m_mipmaps[0].m_height;

				for (unsigned int i=2 ; i<fan.indices.size() ; i++, v1=v2, t1=t2)
				{
					t2 = fan.texture_mapping[i];
					t2[0][0] *= model.mesh.textures[0]->m_mipmaps[0].m_width;
					t2[0][1] *= model.mesh.textures[0]->m_mipmaps[0].m_height;
					v2 = &vertices[fan.indices[i]];

					// frustum clipping
					if (      (v0->x()  < m_viewport.m_x                && v1->x()  < m_viewport.m_x                && v2->x()  < m_viewport.m_x               )
							||(v0->y()  < m_viewport.m_y                && v1->y()  < m_viewport.m_y                && v2->y()  < m_viewport.m_y               )
							||(v0->x() >= m_viewport.m_x+m_viewport.m_w && v1->x() >= m_viewport.m_x+m_viewport.m_w && v2->x() >= m_viewport.m_x+m_viewport.m_w)
							||(v0->y() >= m_viewport.m_y+m_viewport.m_h && v1->y() >= m_viewport.m_y+m_viewport.m_h && v2->y() >= m_viewport.m_y+m_viewport.m_h)
					   )
					{
						continue;
					}

					// backface culling
					if ( Cross((*v1-*v0),(*v2-*v0)).z() >= 0 )
						continue;

					// Z-near clipping
					if ((*v0).z() < 0.001 && (*v1).z() < 0.001 && (*v2).z() < 0.001)
						continue;

					float face_sun_intensity = -normals[i-2].Dot(m_scene.sun_direction);
					if (face_sun_intensity < 0.0f)
						face_sun_intensity = 0.0f;
					float light = m_scene.ambient_light_intensity + face_sun_intensity*m_scene.sun_intensity;
					if (light > 1.0f)
						light = 1.0f;

					// Fill poly
					fill_flat_poly( v0,  v1,  v2,
					               &t0, &t1, &t2,
					               light,
					               model.mesh.textures[0], m_viewport, m_zbuffer,
					               m_scene);
				}
			}
			// TRIs
			for (const triangle_solo & tri : model.mesh.triangle_list)
			{
				auto normal = Transform(tri.normal, model.orientation);

				vertex_t v0 = vertices[tri.v0];
				vertex_t v1 = vertices[tri.v1];
				vertex_t v2 = vertices[tri.v2];
				Vec2f t0 = tri.t0;
				Vec2f t1 = tri.t1;
				Vec2f t2 = tri.t2;
				t0[0][0] *= model.mesh.textures[0]->m_mipmaps[0].m_width;
				t0[0][1] *= model.mesh.textures[0]->m_mipmaps[0].m_height;
				t1[0][0] *= model.mesh.textures[0]->m_mipmaps[0].m_width;
				t1[0][1] *= model.mesh.textures[0]->m_mipmaps[0].m_height;
				t2[0][0] *= model.mesh.textures[0]->m_mipmaps[0].m_width;
				t2[0][1] *= model.mesh.textures[0]->m_mipmaps[0].m_height;

				// backface culling
				if ( Cross((v1-v0),(v2-v0))[0][2] >= 0 )
					continue;

				// frustum clipping
				if (      (v0.x()  < m_viewport.m_x                && v1.x()  < m_viewport.m_x                && v2.x()  < m_viewport.m_x               )
						||(v0.y()  < m_viewport.m_y                && v1.y()  < m_viewport.m_y                && v2.y()  < m_viewport.m_y               )
						||(v0.x() >= m_viewport.m_x+m_viewport.m_w && v1.x() >= m_viewport.m_x+m_viewport.m_w && v2.x() >= m_viewport.m_x+m_viewport.m_w)
						||(v0.y() >= m_viewport.m_y+m_viewport.m_h && v1.y() >= m_viewport.m_y+m_viewport.m_h && v2.y() >= m_viewport.m_y+m_viewport.m_h)
				   )
				{
					continue;
				}

				// Z-near clipping
				if (v0.z() < 0.001 && v1.z() < 0.001 && v2.z() < 0.001)
					continue;

				float face_sun_intensity = -normal.Dot(m_scene.sun_direction);
				if (face_sun_intensity < 0.0f)
					face_sun_intensity = 0.0f;
				float light = m_scene.ambient_light_intensity + face_sun_intensity*m_scene.sun_intensity;
				if (light > 1.0f)
					light = 1.0f;

				// Fill poly
				fill_flat_poly(&v0, &v1, &v2,
				               &t0, &t1, &t2,
				               light,
				               model.mesh.textures[0], m_viewport, m_zbuffer,
				               m_scene);
			}
			/*
			// print vertices and normals
			normal_idx = 0;
			static bool done = false;
			if (!done)
			{
				done = true;
				for (int x=0 ; x<flex_model.common->triangle_fans.size() ; x++)
				{
					auto & fan = flex_model.common->triangle_fans[x];
					std::cout << "Fan " << x << ":" << std::endl;
					for (unsigned int i=2 ; i<fan.indices.size() ; i++)
					{
						std::cout << "  Triangle " << i-2 << std::endl;
						{
							std::cout << "  vertex 0: indices[" << fan.indices[0] << "]" << std::endl;
							std::cout << "  normal_idx:" << normal_idx << std::endl;
							auto n = flex_model.get_vertices()[fan.indices[0]];
							auto nn = n + flex_model.get_normals()[normal_idx];
							std::cout << "    v: " << n << std::endl;
							std::cout << "    normal: " << flex_model.get_normals()[normal_idx] << std::endl;
							std::cout << "    n: " << nn << std::endl;
							Vec3f vec1 = Transform(Vec3f(n[0][0], n[0][1], n[0][2]), vertice_transform_matrix);
							vec1[0][0] /= fabs(vec1[0][2]);
							vec1[0][1] /= fabs(vec1[0][2]);
							vec1 = Transform(vec1, m_viewport.m_viewportmatrix);
							Vec3f vec2 = Transform(Vec3f(nn[0][0], nn[0][1], nn[0][2]), vertice_transform_matrix);
							vec2[0][0] /= fabs(vec2[0][2]);
							vec2[0][1] /= fabs(vec2[0][2]);
							vec2 = Transform(vec2, m_viewport.m_viewportmatrix);
							std::cout << "    transformed v: " << vec1 << std::endl;
							std::cout << "    transformed n: " << vec2 << std::endl;
							crude_line(m_viewport, vec1[0][0], vec1[0][1], vec2[0][0], vec2[0][1]);
						}
						{
							std::cout << "  vertex " << i-1 << " indices[" << fan.indices[i-1] << "]:" << std::endl;
							std::cout << "  normal_idx:" << normal_idx << std::endl;
							auto n = flex_model.get_vertices()[fan.indices[i-1]];
							auto nn = n + flex_model.get_normals()[normal_idx];
							std::cout << "    v: " << n << std::endl;
							std::cout << "    normal: " << flex_model.get_normals()[normal_idx] << std::endl;
							std::cout << "    n: " << nn << std::endl;
							Vec3f vec1 = Transform(Vec3f(n[0][0], n[0][1], n[0][2]), vertice_transform_matrix);
							vec1[0][0] /= fabs(vec1[0][2]);
							vec1[0][1] /= fabs(vec1[0][2]);
							vec1 = Transform(vec1, m_viewport.m_viewportmatrix);
							Vec3f vec2 = Transform(Vec3f(nn[0][0], nn[0][1], nn[0][2]), vertice_transform_matrix);
							vec2[0][0] /= fabs(vec2[0][2]);
							vec2[0][1] /= fabs(vec2[0][2]);
							vec2 = Transform(vec2, m_viewport.m_viewportmatrix);
							std::cout << "    transformed v: " << vec1 << std::endl;
							std::cout << "    transformed n: " << vec2 << std::endl;
							crude_line(m_viewport, vec1[0][0], vec1[0][1], vec2[0][0], vec2[0][1]);
						}
						{
							std::cout << "  vertex " << i << " indices[" << fan.indices[i] << "]:" << std::endl;
							std::cout << "  normal_idx:" << normal_idx << std::endl;
							auto n = flex_model.get_vertices()[fan.indices[i]];
							auto nn = n + flex_model.get_normals()[normal_idx];
							std::cout << "    v: " << n << std::endl;
							std::cout << "    normal: " << flex_model.get_normals()[normal_idx] << std::endl;
							std::cout << "    n: " << nn << std::endl;
							Vec3f vec1 = Transform(Vec3f(n[0][0], n[0][1], n[0][2]), vertice_transform_matrix);
							vec1[0][0] /= fabs(vec1[0][2]);
							vec1[0][1] /= fabs(vec1[0][2]);
							vec1 = Transform(vec1, m_viewport.m_viewportmatrix);
							Vec3f vec2 = Transform(Vec3f(nn[0][0], nn[0][1], nn[0][2]), vertice_transform_matrix);
							vec2[0][0] /= fabs(vec2[0][2]);
							vec2[0][1] /= fabs(vec2[0][2]);
							vec2 = Transform(vec2, m_viewport.m_viewportmatrix);
							std::cout << "    transformed v: " << vec1 << std::endl;
							std::cout << "    transformed n: " << vec2 << std::endl;
							crude_line(m_viewport, vec1[0][0], vec1[0][1], vec2[0][0], vec2[0][1]);
						}
						normal_idx++;
					}
				}
			}
			//*/

			//*
			// draw normals
			//for (const triangle_strip & strip : model.mesh.triangle_strips)
			//{
			//	for (unsigned int i=0 ; i<strip.indices.size()-2 ; i++)
			//	{
			//		for (int j=0 ; j<1 ; j++)
			//		{
			//			auto n = model.mesh.vertices[strip.indices[i+j]];
			//			auto nn = n + model.mesh.normals[normal_idx];
			//			Vec3f vec1 = Transform(Vec3f(n[0][0], n[0][1], n[0][2]), vertice_transform_matrix);
			//			vec1[0][0] /= fabs(vec1[0][2]);
			//			vec1[0][1] /= fabs(vec1[0][2]);
			//			vec1 = Transform(vec1, m_viewport.m_viewportmatrix);
			//			Vec3f vec2 = Transform(Vec3f(nn[0][0], nn[0][1], nn[0][2]), vertice_transform_matrix);
			//			vec2[0][0] /= fabs(vec2[0][2]);
			//			vec2[0][1] /= fabs(vec2[0][2]);
			//			vec2 = Transform(vec2, m_viewport.m_viewportmatrix);
			//			crude_line(m_viewport, vec1[0][0], vec1[0][1], vec2[0][0], vec2[0][1]);
			//			crude_line(m_viewport, n[0][0], n[0][1], nn[0][0], nn[0][1]);
			//		}
			//		normal_idx++;
			//	}
			//}
			//for (const triangle_fan & fan : model.mesh.triangle_fans)
			//{
			//	for (unsigned int i=2 ; i<fan.indices.size() ; i++)
			//	{
			//		{
			//			auto n = model.mesh.vertices[fan.indices[0]];
			//			auto nn = n + model.mesh.normal_points[normal_idx];
			//			Vec3f vec1 = Transform(Vec3f(n[0][0], n[0][1], n[0][2]), vertice_transform_matrix);
			//			vec1[0][0] /= fabs(vec1[0][2]);
			//			vec1[0][1] /= fabs(vec1[0][2]);
			//			vec1 = Transform(vec1, m_viewport.m_viewportmatrix);
			//			Vec3f vec2 = Transform(Vec3f(nn[0][0], nn[0][1], nn[0][2]), vertice_transform_matrix);
			//			vec2[0][0] /= fabs(vec2[0][2]);
			//			vec2[0][1] /= fabs(vec2[0][2]);
			//			vec2 = Transform(vec2, m_viewport.m_viewportmatrix);
			//			crude_line(m_viewport, vec1[0][0], vec1[0][1], vec2[0][0], vec2[0][1]);
			//		}
			//		{
			//			auto n = model.mesh.vertices[fan.indices[i-1]];
			//			auto nn = n + model.mesh.normal_points[normal_idx];
			//			Vec3f vec1 = Transform(Vec3f(n[0][0], n[0][1], n[0][2]), vertice_transform_matrix);
			//			vec1[0][0] /= fabs(vec1[0][2]);
			//			vec1[0][1] /= fabs(vec1[0][2]);
			//			vec1 = Transform(vec1, m_viewport.m_viewportmatrix);
			//			Vec3f vec2 = Transform(Vec3f(nn[0][0], nn[0][1], nn[0][2]), vertice_transform_matrix);
			//			vec2[0][0] /= fabs(vec2[0][2]);
			//			vec2[0][1] /= fabs(vec2[0][2]);
			//			vec2 = Transform(vec2, m_viewport.m_viewportmatrix);
			//			crude_line(m_viewport, vec1[0][0], vec1[0][1], vec2[0][0], vec2[0][1]);
			//		}
			//		{
			//			auto n = model.mesh.vertices[fan.indices[i]];
			//			auto nn = n + model.mesh.normal_points[normal_idx];
			//			Vec3f vec1 = Transform(Vec3f(n[0][0], n[0][1], n[0][2]), vertice_transform_matrix);
			//			vec1[0][0] /= fabs(vec1[0][2]);
			//			vec1[0][1] /= fabs(vec1[0][2]);
			//			vec1 = Transform(vec1, m_viewport.m_viewportmatrix);
			//			Vec3f vec2 = Transform(Vec3f(nn[0][0], nn[0][1], nn[0][2]), vertice_transform_matrix);
			//			vec2[0][0] /= fabs(vec2[0][2]);
			//			vec2[0][1] /= fabs(vec2[0][2]);
			//			vec2 = Transform(vec2, m_viewport.m_viewportmatrix);
			//			crude_line(m_viewport, vec1[0][0], vec1[0][1], vec2[0][0], vec2[0][1]);
			//		}
			//		normal_idx++;
			//	}
			//}
			//*/
		}
	}
};

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

void fill_flat_poly(const vertex_t * v0, const vertex_t * v1, const vertex_t * v2,
                    const Vec2f * t0, const Vec2f * t1, const Vec2f * t2,
                    float light,
                    const std::shared_ptr<Texture> & texture,
                    ViewPort & vp,
                    float * zbuffer,
                    const scene & scene)
{
	// Sort points
	if ((*v1)[0][1] <= (*v0).y()) {
		std::swap(v0, v1);
		std::swap(t0, t1);
	}
	if ((*v2)[0][1] <= (*v1).y()) {
		std::swap(v1, v2);
		std::swap(t1, t2);
	}
	if ((*v1)[0][1] <= (*v0).y()) {
		std::swap(v0, v1);
		std::swap(t0, t1);
	}

	// get pixel limits
	int y0 = (int) ceil((*v0).y());
	int y1 = (int) ceil((*v1).y());
	int y2 = (int) ceil((*v2).y());

	if (y0==y2) return; // All on 1 scanline, not worth drawing

	line_side side_long;
	line_side side_short;

	// Init long line
	side_long.ratio = ((*v2).x()-(*v0).x()) / ((*v2).y()-(*v0).y()); // never div#0 because y0!=y2
	side_long.interpolator.Init(ZInterpolator::VERTICAL, *v0, *v2, *t0, *t2);
	if (y0 < vp.m_y) {
		// start at first scanline of viewport
		side_long.interpolator.DisplaceStartingPoint(vp.m_y-(*v0).y());
		side_long.x = v0->x() + side_long.ratio*(vp.m_y-(*v0).y());
	} else {
		side_long.interpolator.DisplaceStartingPoint(y0-(*v0).y());
		side_long.x = v0->x() + side_long.ratio*(y0-(*v0).y());
	}

	int y, y_end; // scanlines upper and lower bound of whole triangle

	// upper half of the triangle
	if (y1 >= vp.m_y) // dont skip: at least some part is in the viewport
	{
		side_short.ratio = (v1->x()-v0->x()) / (v1->y()-v0->y()); // never div#0 because y0!=y2
		side_short.interpolator.Init(ZInterpolator::VERTICAL, *v0, *v1, *t0, *t1);
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

		fill_half_tri(y, y_end, side_left, side_right, texture, light, vp, zbuffer);
	}

	// lower half of the triangle
	if (y1 < vp.m_y + vp.m_h) // dont skip: at least some part is in the viewport
	{
		side_short.ratio = ((*v2).x()-(*v1).x()) / ((*v2).y()-(*v1).y()); // never div#0 because y0!=y2
		side_short.interpolator.Init(ZInterpolator::VERTICAL, *v1, *v2, *t1, *t2);
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

		fill_half_tri(y, y_end, side_left, side_right, texture, light, vp, zbuffer);
	}
}

void fill_half_tri(int y, int y_end,
	               line_side & side_left, line_side & side_right,
                   const std::shared_ptr<Texture> & t,
                   float light,
                   ViewPort & vp,
                   float * zbuffer)
{
	// determine mipmap (hardcoded for now)
	unsigned int *tbitmap = t->m_mipmaps[0].m_bitmap;
	unsigned int twidth   = t->m_mipmaps[0].m_width;
	unsigned int theight  = t->m_mipmaps[0].m_height;

	for ( ; y < y_end ; y++)
	{
		int x1 = std::max((int)ceil(side_left .x), vp.m_x);
		int x2 = std::min((int)ceil(side_right.x), vp.m_x+vp.m_w);
		ZInterpolator qpixel;
		Vec3f linev0(side_left .x, 0, side_left .interpolator.ualpha[0][2]);
		Vec3f linev1(side_right.x, 0, side_right.interpolator.ualpha[0][2]);
		qpixel.Init(ZInterpolator::HORIZONTAL, linev0, linev1, Vec2f{side_left .interpolator.ualpha[0][0],side_left .interpolator.ualpha[0][1]},
		                                                       Vec2f{side_right.interpolator.ualpha[0][0],side_right.interpolator.ualpha[0][1]});
		qpixel.DisplaceStartingPoint(x1 - side_left.x);

		// fill_line
		unsigned int *video = &((unsigned int*)vp.m_screen->pixels)[(int) ( y*vp.m_screen->pitch/4 + x1)];
		float * zb = &zbuffer[(int) ( y*vp.m_w + x1)];
		for ( ; x1 < x2 ; x1++ )
		{
			int u, v;

			if (qpixel.ualpha[0][2] < *zb && qpixel.ualpha[0][2] > 0.001) // Ugly z-near culling
			{
				u = ((int)qpixel.ualpha[0][0]) % twidth;
				v = ((int)qpixel.ualpha[0][1]) % theight;
				*video = tbitmap[v*twidth + u];
				((unsigned char*)video)[0] *= light;
				((unsigned char*)video)[1] *= light;
				((unsigned char*)video)[2] *= light;
				*zb    = qpixel.ualpha[0][2];
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

} // namespace
