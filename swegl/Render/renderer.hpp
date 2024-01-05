

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

void fill_flat_poly(const Vec3f & _v0, const Vec3f & _v1, const Vec3f & _v2,
                    const Vec2f & _t0, const Vec2f & _t1, const Vec2f & _t2,
                    float light,
                    const std::shared_ptr<Texture> & t, ViewPort & vp, float * zbuffer,
                    const scene & scene);

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

		static std::vector<Vec3f> vertices;
		static std::vector<Vec3f> normals;

		for (const auto & model : m_scene.models)
		{
			auto model_matrix = model.orientation;
			model_matrix.Translate(model.position[0][0], model.position[0][1], model.position[0][2]);
			auto world_matrix = m_camera.m_viewmatrix * model_matrix;
			auto vertice_transform_matrix = m_camera.m_projectionmatrix * world_matrix;

			vertices.clear();
			vertices.reserve(model.mesh.vertices.size());
			for (const Vec3f & v : model.mesh.vertices)
			{
				Vec3f vec = Transform(v, vertice_transform_matrix);
				vec[0][0] /= fabs(vec[0][2]);
				vec[0][1] /= fabs(vec[0][2]);
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

				const Vec3f * v2;
				const Vec3f * v0 = &vertices[strip.indices[0]];
				const Vec3f * v1 = &vertices[strip.indices[1]];
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

					// frustum culling
					if (      ((*v0)[0][0]  < m_viewport.m_x                && (*v1)[0][0]  < m_viewport.m_x                && (*v2)[0][0]  < m_viewport.m_x               )
							||((*v0)[0][1]  < m_viewport.m_y                && (*v1)[0][1]  < m_viewport.m_y                && (*v2)[0][1]  < m_viewport.m_y               )
							||((*v0)[0][0] >= m_viewport.m_x+m_viewport.m_w && (*v1)[0][0] >= m_viewport.m_x+m_viewport.m_w && (*v2)[0][0] >= m_viewport.m_x+m_viewport.m_w)
							||((*v0)[0][1] >= m_viewport.m_y+m_viewport.m_h && (*v1)[0][1] >= m_viewport.m_y+m_viewport.m_h && (*v2)[0][1] >= m_viewport.m_y+m_viewport.m_h)
					   )
					{
						continue;
					}

					// backface culling
					if ((i&0x1)==0) {
						if ( Cross((*v1-*v0),(*v2-*v0))[0][2] >= 0 )
							continue;
					} else {
						if ( Cross((*v2-*v0),(*v1-*v0))[0][2] >= 0 )
							continue;
					}

					// Z-near clipping
					if ((*v0)[0][2] < 0.001 && (*v1)[0][2] < 0.001 && (*v2)[0][2] < 0.001)
						continue;

					float face_sun_intensity = -normals[i-2].Dot(m_scene.sun_direction);
					if (face_sun_intensity < 0.0f)
						face_sun_intensity = 0.0f;
					float light = m_scene.ambient_light_intensity + face_sun_intensity*m_scene.sun_intensity;
					if (light > 1.0f)
						light = 1.0f;

					// Fill poly
					fill_flat_poly(*v0, *v1, *v2,
					                t0,  t1,  t2,
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

				const Vec3f * v2;
				const Vec3f * v0 = &vertices[fan.indices[0]];
				const Vec3f * v1 = &vertices[fan.indices[1]];
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

					// frustum culling
					if (  ((*v0)[0][0] <  m_viewport.m_x                && (*v1)[0][0] <  m_viewport.m_x                && (*v2)[0][0] <  m_viewport.m_x)
						||((*v0)[0][1] <  m_viewport.m_y                && (*v1)[0][1] <  m_viewport.m_y                && (*v2)[0][1] <  m_viewport.m_y)
						||((*v0)[0][0] >= m_viewport.m_x+m_viewport.m_w && (*v1)[0][0] >= m_viewport.m_x+m_viewport.m_w && (*v2)[0][0] >= m_viewport.m_x+m_viewport.m_w)
						||((*v0)[0][1] >= m_viewport.m_y+m_viewport.m_h && (*v1)[0][1] >= m_viewport.m_y+m_viewport.m_h && (*v2)[0][1] >= m_viewport.m_y+m_viewport.m_h)
					   )
					{
						continue;
					}

					// backface culling
					if ( Cross((*v1-*v0),(*v2-*v0))[0][2] >= 0 )
						continue;

					// Z-near clipping
					if ((*v0)[0][2] < 0.001 && (*v1)[0][2] < 0.001 && (*v2)[0][2] < 0.001)
						continue;

					float face_sun_intensity = -normals[i-2].Dot(m_scene.sun_direction);
					if (face_sun_intensity < 0.0f)
						face_sun_intensity = 0.0f;
					float light = m_scene.ambient_light_intensity + face_sun_intensity*m_scene.sun_intensity;
					if (light > 1.0f)
						light = 1.0f;

					// Fill poly
					fill_flat_poly(*v0, *v1, *v2,
					                t0,  t1,  t2,
					               light,
					               model.mesh.textures[0], m_viewport, m_zbuffer,
					               m_scene);
				}
			}
			// TRIs
			for (const triangle_solo & tri : model.mesh.triangle_list)
			{
				auto normal = Transform(tri.normal, model.orientation);

				Vec3f v0 = vertices[tri.v0];
				Vec3f v1 = vertices[tri.v1];
				Vec3f v2 = vertices[tri.v2];
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

				// frustum culling
				if (      ((v0)[0][0]  < m_viewport.m_x                && (v1)[0][0]  < m_viewport.m_x                && (v2)[0][0]  < m_viewport.m_x               )
						||((v0)[0][1]  < m_viewport.m_y                && (v1)[0][1]  < m_viewport.m_y                && (v2)[0][1]  < m_viewport.m_y               )
						||((v0)[0][0] >= m_viewport.m_x+m_viewport.m_w && (v1)[0][0] >= m_viewport.m_x+m_viewport.m_w && (v2)[0][0] >= m_viewport.m_x+m_viewport.m_w)
						||((v0)[0][1] >= m_viewport.m_y+m_viewport.m_h && (v1)[0][1] >= m_viewport.m_y+m_viewport.m_h && (v2)[0][1] >= m_viewport.m_y+m_viewport.m_h)
				   )
				{
					continue;
				}

				// Z-near clipping
				if ((v0)[0][2] < 0.001 && (v1)[0][2] < 0.001 && (v2)[0][2] < 0.001)
					continue;

				float face_sun_intensity = -normal.Dot(m_scene.sun_direction);
				if (face_sun_intensity < 0.0f)
					face_sun_intensity = 0.0f;
				float light = m_scene.ambient_light_intensity + face_sun_intensity*m_scene.sun_intensity;
				if (light > 1.0f)
					light = 1.0f;

				// Fill poly
				fill_flat_poly(v0, v1, v2,
				               t0, t1, t2,
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


void fill_flat_poly(const Vec3f & _v0, const Vec3f & _v1, const Vec3f & _v2,
                    const Vec2f & _t0, const Vec2f & _t1, const Vec2f & _t2,
                    float light,
                    const std::shared_ptr<Texture> & t, ViewPort & vp, float * zbuffer,
                    const scene & scene)
{
	const Vec3f *v0, *v1, *v2;
	const Vec2f *t0, *t1, *t2;

	if (_v0[0][1] <= _v1[0][1] && _v1[0][1] <= _v2[0][1]) {
		v0 = &_v0;
		v1 = &_v1;
		v2 = &_v2;
		t0 = &_t0;
		t1 = &_t1;
		t2 = &_t2;
	} else if (_v0[0][1] <= _v2[0][1] && _v2[0][1] <= _v1[0][1]) {
		v0 = &_v0;
		v1 = &_v2;
		v2 = &_v1;
		t0 = &_t0;
		t1 = &_t2;
		t2 = &_t1;
	} else if (_v1[0][1] <= _v0[0][1] && _v0[0][1] <= _v2[0][1]) {
		v0 = &_v1;
		v1 = &_v0;
		v2 = &_v2;
		t0 = &_t1;
		t1 = &_t0;
		t2 = &_t2;
	} else if (_v1[0][1] <= _v2[0][1] && _v2[0][1] <= _v0[0][1]) {
		v0 = &_v1;
		v1 = &_v2;
		v2 = &_v0;
		t0 = &_t1;
		t1 = &_t2;
		t2 = &_t0;
	} else if (_v2[0][1] <= _v0[0][1] && _v0[0][1] <= _v1[0][1]) {
		v0 = &_v2;
		v1 = &_v0;
		v2 = &_v1;
		t0 = &_t2;
		t1 = &_t0;
		t2 = &_t1;
	} else if (_v2[0][1] <= _v1[0][1] && _v1[0][1] <= _v0[0][1]) {
		v0 = &_v2;
		v1 = &_v1;
		v2 = &_v0;
		t0 = &_t2;
		t1 = &_t1;
		t2 = &_t0;
	}

	int y0 = (int) floor((*v0)[0][1]);
	int y1 = (int) floor((*v1)[0][1]);
	int y2 = (int) floor((*v2)[0][1]);
	int xa = (int) floor((*v0)[0][0]);
	int xb = (int) floor((*v1)[0][0]);
	int xc = (int) floor((*v2)[0][0]);
	bool line2_on_right; // true if line2 is on the right and we must choose its highest x value

	if (y0==y2) return; // All on 1 scanline, not worth drawing

	int y, ymax;
	float x1f, x2f; // reference points for the (u,v)s
	float ratio1, ratio2;
	ZInterpolator zi1, zi2;
	ZInterpolator qpixel;
	float displacement;

	unsigned int *tbitmap;
	unsigned int twidth;
	unsigned int theight;

	auto fill_line = [&](int y, int x1, int x2, unsigned int *video, float * zb)
		{
			for ( ; x1 <= x2 ; x1++ )
			{
				int u, v;

				if (qpixel.ualpha[0][2] < *zb && qpixel.ualpha[0][2] > 0.001) // Ugly z-near culling
				{
					u = ((int)qpixel.ualpha[0][0]) % twidth;
					v = ((int)qpixel.ualpha[0][1]) % theight;
					*video = tbitmap[v*twidth + u];
					//video[0] *= scene.ambient_light_intensity;
					((unsigned char*)video)[0] *= light;
					((unsigned char*)video)[1] *= light;
					((unsigned char*)video)[2] *= light;
					//((unsigned char*)video)[3] *= scene.ambient_light_intensity;
					*zb    = qpixel.ualpha[0][2];
				}
				video++;
				zb++;

				qpixel.Step();
			}
		};

	// Init lines
	displacement = y0+0.5f - (*v0)[0][1];
	ratio2 = ((*v2)[0][0]-(*v0)[0][0]) / ((*v2)[0][1]-(*v0)[0][1]); // never div#0 because y0!=y2
	zi2.Init(ZInterpolator::VERTICAL, *v0, *v2, *t0, *t2);
	zi2.DisplaceStartingPoint(displacement);
	x2f = (*v0)[0][0] + ratio2*displacement;

	if (y1 <= vp.m_y || y0 == y1) {
		// Start drawing at y1
		if (y1 <= vp.m_y) {
			// Skip first half of triangle
			zi2.DisplaceStartingPoint((float)(y1-y0));
			x2f = (*v0)[0][0] + ratio2*(y1-y0);
		}
		displacement = y1+0.5f - (*v1)[0][1];
		ratio1 = ((*v2)[0][0]-(*v1)[0][0]) / ((*v2)[0][1]-(*v1)[0][1]); // never div#0 because y1!=y2
		zi1.Init(ZInterpolator::VERTICAL, *v1, *v2, *t1, *t2);
		zi1.DisplaceStartingPoint(displacement);
		x1f = (*v1)[0][0] + ratio1*displacement;
		line2_on_right = ratio2 < ratio1;
		y = y1;
		ymax = y2;
	} else {
		// Normal case (y0 != y1)
		ratio1 = ((*v1)[0][0]-(*v0)[0][0]) / ((*v1)[0][1]-(*v0)[0][1]); // never div#0 because y1!=y0
		zi1.Init(ZInterpolator::VERTICAL, *v0, *v1, *t0, *t1);
		zi1.DisplaceStartingPoint(displacement);
		x1f = (*v0)[0][0] + ratio1*displacement;
		line2_on_right = ratio2 > ratio1;
		y = y0;
		ymax = y1;
	}

	//if (ratio1-ratio2 < 0.01 && ratio1-ratio2 > -0.01) {
	//	// Triangle too small, not worth drawing;
	//	return;
	//}

	if (y < vp.m_y) {
		zi1.DisplaceStartingPoint((float)(vp.m_y-y));
		zi2.DisplaceStartingPoint((float)(vp.m_y-y));
		x1f += ratio1 * (vp.m_y-y);
		x2f += ratio2 * (vp.m_y-y);
		y = vp.m_y;
	}

	tbitmap = t->m_mipmaps[0].m_bitmap;
	twidth  = t->m_mipmaps[0].m_width;
	theight = t->m_mipmaps[0].m_height;

	if (vp.m_y + vp.m_h < ymax)
		ymax = vp.m_y + vp.m_h;
	while (y <= ymax)
	{
		int x1, x2;

		//int ualphastep_x;

		if (line2_on_right) {
			if (ratio2>0) {
				if (y == y2) {
					x2 = xc;
				} else {
					x2 = (int) (x2f + ratio2*0.5f);
				}
			} else {
				if (y == y0) {
					x2 = xa;
				} else {
					x2 = (int) (x2f - ratio2*0.5f);
				}
			}
			if (ratio1<0) {
				if (y == y1) {
					x1 = xb;
				} else if (y == y2) {
					x1 = xc;
				} else {
					x1 = (int) (x1f + ratio1*0.5f);
				}
			} else {
				if (y == y0) {
					x1 = xa;
				} else if (y == y1) {
					x1 = xb;
				} else {
					x1 = (int) (x1f - ratio1*0.5f);
				}
			}
		} else {
			if (ratio2<0) {
				if (y == y2) {
					x1 = xc;
				} else {
					x1 = (int) (x2f + ratio2*0.5f);
				}
			} else {
				if (y == y0) {
					x1 = xa;
				} else {
					x1 = (int) (x2f - ratio2*0.5f);
				}
			}
			if (ratio1>0) {
				if (y == y1) {
					x2 = xb;
				} else if (y == y2) {
					x2 = xc;
				} else {
					x2 = (int) (x1f + ratio1*0.5f);
				}
			} else {
				if (y == y0) {
					x2 = xa;
				} else if (y == y1) {
					x2 = xb;
				} else {
					x2 = (int) (x1f - ratio1*0.5f);
				}
			}
		}

		Vec3f linev0(x1f, 0, zi1.ualpha[0][2]);
		Vec3f linev1(x2f, 0, zi2.ualpha[0][2]);
		if (x1f <= x2f) {
			qpixel.Init(ZInterpolator::HORIZONTAL, linev0, linev1, {zi1.ualpha[0][0],zi1.ualpha[0][1]}, {zi2.ualpha[0][0],zi2.ualpha[0][1]});
			displacement = x1+0.5f - linev0[0][0];
		} else {
			qpixel.Init(ZInterpolator::HORIZONTAL, linev1, linev0, {zi2.ualpha[0][0],zi2.ualpha[0][1]}, {zi1.ualpha[0][0],zi1.ualpha[0][1]});
			displacement = x1+0.5f - linev1[0][0];
		}
		if (x1 < vp.m_x) {
			displacement += (float)(vp.m_x - x1);
			x1 = vp.m_x;
		}
		qpixel.DisplaceStartingPoint(displacement);

		if (x2 >= vp.m_x + vp.m_w) {
			x2 = vp.m_x + vp.m_w - 1;
		}

		//ASSERT(x1 <= x2 || x1 >= vp.m_x + vp.m_w);
		//ASSERT(x1 >= vp.m_x);

		//scanlines.emplace_back(y, x1, x2, qpixel);
		unsigned int *video = &((unsigned int*)vp.m_screen->pixels)[(int) ( y*vp.m_screen->pitch/4 + x1)];
		float * zb = &zbuffer[(int) ( y*vp.m_w + x1)];
		fill_line(y, x1, x2, video, zb);


		x1f += ratio1;
		x2f += ratio2;
		zi1.Step();
		zi2.Step();
		y++;
		if (y == y1 && y1 != y2) {
			displacement = y1+0.5f - (*v1)[0][1];
			ratio1 = ((*v2)[0][0]-(*v1)[0][0]) / ((*v2)[0][1]-(*v1)[0][1]); // never div#0 because y1!=y2
			zi1.Init(ZInterpolator::VERTICAL, *v1, *v2, *t1, *t2);
			zi1.DisplaceStartingPoint(displacement);
			x1f = (*v1)[0][0] + ratio1*displacement;
			ymax = y2;
			if (vp.m_y + vp.m_h < ymax)
				ymax = vp.m_y + vp.m_h;
		}
	}
}


} // namespace
