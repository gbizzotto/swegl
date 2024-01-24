
#pragma once

#include <numeric>
#include <cmath>

#include "swegl/data/model.hpp"
#include "swegl/projection/points.hpp"
#include "swegl/render/vertex_shaders.hpp"
#include "swegl/render/colors.hpp"

#ifndef _GLIBCXX_PARALLEL
#define __gnu_parallel std
#endif

namespace swegl
{

struct pixel_shader_t
{
	const model_t * model;
	const scene_t * scene;
	const viewport_t * viewport;

	virtual void prepare_for_model(const model_t & m,
	                               const scene_t & s,
	                               const viewport_t & vp)
	{
		model = &m;
		scene = &s;
		viewport = &vp;
	}
	virtual void prepare_for_triangle(vertex_idx, vertex_idx, vertex_idx) {}
	virtual void prepare_for_upper_triangle([[maybe_unused]] bool long_line_on_right) {}
	virtual void prepare_for_lower_triangle([[maybe_unused]] bool long_line_on_right) {}
	virtual void prepare_for_scanline([[maybe_unused]] float progress_left, [[maybe_unused]] float progress_right) {}
	virtual int shade([[maybe_unused]] float progress) { return pixel_colors(128,128,128).to_int(); }
};

struct pixel_shader_lights_flat : pixel_shader_t
{
	vertex_shader_t vertex_shader;

	float light;

	virtual void prepare_for_triangle(vertex_idx i0, vertex_idx i1, vertex_idx i2) override
	{
		vector_t normal_viewport = cross(model->mesh.vertices[i1].v_viewport - model->mesh.vertices[i0].v_viewport
		                                ,model->mesh.vertices[i2].v_viewport - model->mesh.vertices[i0].v_viewport);
		normal_t normal_world(cross(model->mesh.vertices[i1].v_world - model->mesh.vertices[i0].v_world
		                           ,model->mesh.vertices[i2].v_world - model->mesh.vertices[i0].v_world));
		if (normal_viewport.z() > 0)
			normal_world = -normal_world;

		float face_sun_intensity = - normal_world.dot(scene->sun_direction);
		if (face_sun_intensity < 0.0f)
			face_sun_intensity = 0.0f;
		else
			face_sun_intensity *= scene->sun_intensity;

		vertex_t center_vertex = (model->mesh.vertices[i0].v_world + model->mesh.vertices[i1].v_world + model->mesh.vertices[i2].v_world) / 3;
		vector_t camera_vector = viewport->camera().position() - center_vertex;
		camera_vector.normalize();

		float dynamic_lights_intensity = 0.0f;
		__gnu_parallel::for_each(scene->point_source_lights.begin(), scene->point_source_lights.end(),
			[&](const auto & psl)
			{
				vector_t light_direction = center_vertex - psl.position;
				float light_distance_squared = light_direction.len_squared();
				float diffuse = psl.intensity / light_distance_squared;
				light_direction.normalize();
				float alignment = -normal_world.dot(light_direction);
				if (alignment < 0.0f)
					return;
				diffuse *= alignment;

				// specular
				vector_t reflection = light_direction + normal_world * (alignment * 2);
				float specular = reflection.dot(camera_vector);
				if (specular > 0)
				{
					static const int p = 32;
					specular = pow(specular, p);
					specular = specular * p / 2; // make the integral[0,1] of specular 0.5 again so that no extra light is generated
					// should multiply by overall albedo, too so that some light is absorbed
					dynamic_lights_intensity += diffuse + specular / light_distance_squared;
				}
				else
				{
					dynamic_lights_intensity += diffuse;
				}
			});

		light = scene->ambient_light_intensity + face_sun_intensity + dynamic_lights_intensity;
		light *= 65536;
	}
	virtual int shade([[maybe_unused]] float progress) override
	{
		return light;
	}
};

struct pixel_shader_lights_phong : pixel_shader_t
{
	float face_sun_intensity;

	vertex_t v0, v1, v2;
	vertex_t vleft, vright;
	vector_t vleftdir, vrightdir;
	vertex_t v;
	vector_t vdir;

	vector_t n0, n1, n2;
	vector_t nleft, nright;
	vector_t nleftdir, nrightdir;
	vector_t n;
	vector_t ndir;

	virtual void prepare_for_triangle(vertex_idx i0, vertex_idx i1, vertex_idx i2) override
	{
		n0 = (vector_t)model->mesh.vertices[i0].normal_world;
		n1 = (vector_t)model->mesh.vertices[i1].normal_world;
		n2 = (vector_t)model->mesh.vertices[i2].normal_world;
		v0 = model->mesh.vertices[i0].v_world;
		v1 = model->mesh.vertices[i1].v_world;
		v2 = model->mesh.vertices[i2].v_world;
	}
	virtual void prepare_for_upper_triangle(bool long_line_on_right) override
	{
		vleft = v0;
		vright = v0;
		if (long_line_on_right) {
			vleftdir = v1-v0;
			vrightdir = v2-v0;
		} else {
			vleftdir = v2-v0;
			vrightdir = v1-v0;
		}
		nleft = n0;
		nright = n0;
		if (long_line_on_right) {
			nleftdir = n1-n0;
			nrightdir = n2-n0;
		} else {
			nleftdir = n2-n0;
			nrightdir = n1-n0;
		}
	}
	virtual void prepare_for_lower_triangle(bool long_line_on_right) override
	{
		if (long_line_on_right)
		{
			vright = v0;
			vrightdir = v2-v0;
			vleft = v1;
			vleftdir = v2-v1;
			nright = n0;
			nrightdir = n2-n0;
			nleft = n1;
			nleftdir = n2-n1;
		}
		else
		{
			vleft = v0;
			vleftdir = v2-v0;
			vright = v1;
			vrightdir = v2-v1;
			nleft = n0;
			nleftdir = n2-n0;
			nright = n1;
			nrightdir = n2-n1;
		}
	}
	virtual void prepare_for_scanline(float progress_left, float progress_right) override
	{
		v = vleft + vleftdir*progress_left;
		vdir = vright + vrightdir*progress_right - v;
		n = nleft + nleftdir*progress_left;
		ndir = nright + nrightdir*progress_right - n;
	}
	virtual int shade(float progress) override
	{
		vertex_t center_vertex = v + vdir*progress;
		normal_t normal        = n + ndir*progress;
		
		float face_sun_intensity = - normal.dot(scene->sun_direction);
		if (face_sun_intensity < 0.0f)
			face_sun_intensity = 0.0f;
		else
			face_sun_intensity *= scene->sun_intensity;

		float dynamic_lights_intensity = 0.0f;
		__gnu_parallel::for_each(scene->point_source_lights.begin(), scene->point_source_lights.end(),
			[&](const auto & psl)
			{
				vector_t light_direction = center_vertex - psl.position;
				float intensity = psl.intensity / light_direction.len_squared();
				if (intensity < 0.05)
					return;
				light_direction.normalize();
				float alignment = - normal.dot(light_direction);
				if (alignment < 0.0f)
					return;
				dynamic_lights_intensity += alignment * intensity;
			});

		return 65536 * (scene->ambient_light_intensity + face_sun_intensity + dynamic_lights_intensity);
	}
};

struct pixel_shader_texture : pixel_shader_t
{
	vec2f_t t0;
	vec2f_t t1;
	vec2f_t t2;

	vec2f_t side_long_t_dir;
	vec2f_t side_short_t;
	vec2f_t side_short_t_dir;

	bool long_line_on_right;	
	vec2f_t t_left;
	vec2f_t t_dir;

	unsigned int *tbitmap;
	unsigned int twidth;
	unsigned int theight;

	virtual void prepare_for_model([[maybe_unused]] const model_t & m,
	                               [[maybe_unused]] const scene_t & s,
	                               [[maybe_unused]] const viewport_t & vp) override
	{
		pixel_shader_t::prepare_for_model(m, s, vp);

		// TODO: select LOD / mipmap according to distance from camera

		tbitmap = m.mesh.textures[0]->m_mipmaps.get()[0].m_bitmap;
		twidth  = m.mesh.textures[0]->m_mipmaps.get()[0].m_width;
		theight = m.mesh.textures[0]->m_mipmaps.get()[0].m_height;
	}

	virtual void prepare_for_triangle(vertex_idx i0, vertex_idx i1, vertex_idx i2) override
	{
		t0 = model->mesh.vertices[i0].tex_coords;
		t1 = model->mesh.vertices[i1].tex_coords;
		t2 = model->mesh.vertices[i2].tex_coords;

		t0.x() *= model->mesh.textures[0]->m_mipmaps[0].m_width;
		t0.y() *= model->mesh.textures[0]->m_mipmaps[0].m_height;
		t1.x() *= model->mesh.textures[0]->m_mipmaps[0].m_width;
		t1.y() *= model->mesh.textures[0]->m_mipmaps[0].m_height;
		t2.x() *= model->mesh.textures[0]->m_mipmaps[0].m_width;
		t2.y() *= model->mesh.textures[0]->m_mipmaps[0].m_height;

		side_long_t_dir = t2 - t0;
	}

	virtual void prepare_for_upper_triangle(bool long_line_on_right) override
	{
		this->long_line_on_right = long_line_on_right;
		side_short_t = t0;
		side_short_t_dir = t1 - t0;
	}

	virtual void prepare_for_lower_triangle(bool long_line_on_right) override
	{
		this->long_line_on_right = long_line_on_right;
		side_short_t = t1;
		side_short_t_dir = t2 - t1;
	}

	virtual void prepare_for_scanline(float progress_left, float progress_right) override
	{
		if (long_line_on_right)
		{
			t_left  = side_short_t + side_short_t_dir * progress_left ;
			t_dir = t0 + side_long_t_dir  * progress_right - t_left;
		}
		else
		{
			t_left  =           t0 + side_long_t_dir  * progress_left;
			t_dir = side_short_t + side_short_t_dir * progress_right - t_left;
		}
	}

	virtual int shade(float progress) override
	{
		vec2f_t t = t_left + t_dir * progress;
		int u = (int)t.x() % twidth;
		int v = (int)t.y() % theight;
		return tbitmap[v*twidth + u];
	}
};


struct pixel_shader_texture_bilinear : pixel_shader_t
{
	vec2f_t t0;
	vec2f_t t1;
	vec2f_t t2;

	vec2f_t side_long_t_dir;
	vec2f_t side_short_t;
	vec2f_t side_short_t_dir;

	bool long_line_on_right;	
	vec2f_t t_left;
	vec2f_t t_dir;

	unsigned int *tbitmap;
	float twidth;
	float theight;

	virtual void prepare_for_model([[maybe_unused]] const model_t & m,
	                               [[maybe_unused]] const scene_t & s,
	                               [[maybe_unused]] const viewport_t & vp) override
	{
		pixel_shader_t::prepare_for_model(m, s, vp);

		tbitmap = m.mesh.textures[0]->m_mipmaps[0].m_bitmap;
		twidth  = m.mesh.textures[0]->m_mipmaps[0].m_width;
		theight = m.mesh.textures[0]->m_mipmaps[0].m_height;
	}

	virtual void prepare_for_triangle(vertex_idx i0, vertex_idx i1, vertex_idx i2) override
	{
		t0 = model->mesh.vertices[i0].tex_coords;
		t1 = model->mesh.vertices[i1].tex_coords;
		t2 = model->mesh.vertices[i2].tex_coords;

		t0.x() *= model->mesh.textures[0]->m_mipmaps[0].m_width;
		t0.y() *= model->mesh.textures[0]->m_mipmaps[0].m_height;
		t1.x() *= model->mesh.textures[0]->m_mipmaps[0].m_width;
		t1.y() *= model->mesh.textures[0]->m_mipmaps[0].m_height;
		t2.x() *= model->mesh.textures[0]->m_mipmaps[0].m_width;
		t2.y() *= model->mesh.textures[0]->m_mipmaps[0].m_height;

		side_long_t_dir = t2 - t0;
	}

	virtual void prepare_for_upper_triangle(bool long_line_on_right) override
	{
		this->long_line_on_right = long_line_on_right;
		side_short_t = t0;
		side_short_t_dir = t1 - t0;
	}

	virtual void prepare_for_lower_triangle(bool long_line_on_right) override
	{
		this->long_line_on_right = long_line_on_right;
		side_short_t = t1;
		side_short_t_dir = t2 - t1;
	}

	virtual void prepare_for_scanline(float progress_left, float progress_right) override
	{
		if (long_line_on_right)
		{
			t_left  = side_short_t + side_short_t_dir * progress_left ;
			t_dir = t0 + side_long_t_dir  * progress_right - t_left;
		}
		else
		{
			t_left  =           t0 + side_long_t_dir  * progress_left;
			t_dir = side_short_t + side_short_t_dir * progress_right - t_left;
		}
	}

	virtual int shade(float progress) override
	{
		const pixel_colors * pc = (pixel_colors*)tbitmap;

		vec2f_t t = t_left + t_dir * progress;

		float v = t.x();
		float u = t.y();
		float u1 = u-0.5;
		float u2 = u+0.5;
		float v1 = v-0.5;
		float v2 = v+0.5;

		u = floor(u2);
		v = floor(v2);

		unsigned int v1m = (unsigned int) (((int)v1) % (int)theight);
		unsigned int v2m = v1m + 1;
		if (v2m == theight)
			v2m = 0;
		v1m *= twidth;
		v2m *= twidth;
		unsigned int u1m = (unsigned int) (((int)u1) % (int)twidth);
		unsigned int u2m = u1m + 1;
		if (u2m == twidth)
			u2m = 0;
		
		return ( (pc[v1m + u1m] * ((u - u1) * (v - v1)))
		        +(pc[v2m + u1m] * ((u - u1) * (v2 - v)))
		        +(pc[v1m + u2m] * ((u2 - u) * (v - v1)))
		        +(pc[v2m + u2m] * ((u2 - u) * (v2 - v)))
		       ).to_int();
	}
};


template<typename L, typename T>
struct pixel_shader_light_and_texture : pixel_shader_t
{
	L shader_flat_light;
	T shader_texture;

	virtual void prepare_for_model(const model_t & m,
	                               const scene_t & s,
	                               const viewport_t & vp) override
	{
		shader_flat_light.prepare_for_model(m, s, vp);
		shader_texture.prepare_for_model(m, s, vp);
	}

	virtual void prepare_for_triangle(vertex_idx i0, vertex_idx i1, vertex_idx i2) override
	{
		shader_flat_light.prepare_for_triangle(i0, i1, i2);
		shader_texture.prepare_for_triangle(i0, i1, i2);
	}

	virtual void prepare_for_upper_triangle(bool long_line_on_right) override
	{
		shader_flat_light.prepare_for_upper_triangle(long_line_on_right);
		shader_texture.prepare_for_upper_triangle(long_line_on_right);
	}

	virtual void prepare_for_lower_triangle(bool long_line_on_right) override
	{
		shader_flat_light.prepare_for_lower_triangle(long_line_on_right);
		shader_texture.prepare_for_lower_triangle(long_line_on_right);
	}

	virtual void prepare_for_scanline(float progress_left, float progress_right) override
	{
		shader_flat_light.prepare_for_scanline(progress_left, progress_right);
		shader_texture.prepare_for_scanline(progress_left, progress_right);
	}

	virtual int shade(float progress) override
	{
		int color = shader_texture.shade(progress);
		float light = shader_flat_light.shade(progress) / 65536.0;
		if (light < 1)
		{
			((unsigned char*)&color)[0] = ((unsigned char*)&color)[0] * light;
			((unsigned char*)&color)[1] = ((unsigned char*)&color)[1] * light;
			((unsigned char*)&color)[2] = ((unsigned char*)&color)[2] * light;
		}
		else
		{
			light = sqrt(light);
			light = sqrt(light);
			((unsigned char*)&color)[0] = 255 - (unsigned char) ((255 - ((unsigned char*)&color)[0]) / light);
			((unsigned char*)&color)[1] = 255 - (unsigned char) ((255 - ((unsigned char*)&color)[1]) / light);
			((unsigned char*)&color)[2] = 255 - (unsigned char) ((255 - ((unsigned char*)&color)[2]) / light);
		}
		return color;
	}
};

} // namespace
