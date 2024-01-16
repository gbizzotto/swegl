
#pragma once

#include <numeric>
#include <cmath>

#include "swegl/data/model.hpp"
#include "swegl/projection/points.hpp"
#include "swegl/render/vertex_shaders.hpp"
#include "swegl/render/colors.hpp"

namespace swegl
{


class pixel_shader_t
{
public:
	virtual void prepare_for_model(std::vector<vertex_t> & vertices,
	                               std::vector<normal_t> & normals,
	                               const model_t & model,
	                               const scene_t & scene,
	                               const camera_t & camera,
	                               const viewport_t & viewport) {}
	virtual void push_back_vertex_temporary(vertex_t & v) {}
	virtual void pop_back_vertex_temporary() {}
	virtual void prepare_for_strip(const triangle_strip & strip) {}
	virtual void prepare_for_fan  (const triangle_fan & fan) {}
	virtual void prepare_for_triangle_list(const triangle_list_t & list) {}
	virtual void prepare_for_triangle(const std::vector<vertex_idx> & indices, vertex_idx i0, vertex_idx i1, vertex_idx i2) {}
	virtual void next_triangle() {}
	virtual void prepare_for_upper_triangle(bool long_line_on_right) {}
	virtual void prepare_for_lower_triangle(bool long_line_on_right) {}
	virtual void prepare_for_scanline(float progress_left, float progress_right) {}
	virtual int shade(float progress) { return 0; }
};

class pixel_shader_lights_flat : public pixel_shader_t
{
	vertex_shader_world vertex_shader;

	const model_t * model;
	const scene_t * scene;
	std::vector<vertex_t> vertices;
	std::vector<normal_t> * normals;
	int triangle_idx;
	const camera_t * camera;

	float light;

public:
	virtual void prepare_for_model(std::vector<vertex_t> & v,
	                               std::vector<normal_t> & n,
	                               const model_t & m,
	                               const scene_t & s,
	                               const camera_t & c,
	                               const viewport_t & vp)
	{
		model = &m;
		scene = &s;
		vertex_shader.shade(vertices, n, m, s, c, vp);
		normals = &n;
		camera = &c;
	}

	virtual void push_back_vertex_temporary(vertex_t & v) override
	{
		vertices.push_back(vertex_shader.shade_one(v));
	}
	virtual void pop_back_vertex_temporary() override
	{
		vertices.pop_back();
	}

	virtual void prepare_for_strip(const triangle_strip & strip) override
	{
		normals->clear();
		normals->reserve(strip.normals.size());
		for (const auto & n : strip.normals)
			normals->emplace_back(transform(n, model->orientation));

		triangle_idx = 0;
	}
	virtual void prepare_for_fan(const triangle_fan & fan) override
	{
		normals->clear();
		normals->reserve(fan.normals.size());
		for (const auto & n : fan.normals)
			normals->emplace_back(transform(n, model->orientation));

		triangle_idx = 0;
	}
	virtual void prepare_for_triangle_list(const triangle_list_t & list) override
	{
		normals->clear();
		normals->reserve(list.normals.size());
		for (const auto & n : list.normals)
			normals->emplace_back(transform(n, model->orientation));

		triangle_idx = 0;
	}
	virtual void prepare_for_triangle(const std::vector<vertex_idx> & indices, vertex_idx i0, vertex_idx i1, vertex_idx i2) override
	{
		float face_sun_intensity = -(*normals)[triangle_idx].dot(scene->sun_direction);
		if (face_sun_intensity < 0.0f)
			face_sun_intensity = 0.0f;
		else
			face_sun_intensity *= scene->sun_intensity;

		vertex_t center_vertex = (vertices[indices[i0]] + vertices[indices[i1]] + vertices[indices[i2]]) / 3;
		//vertex_t camera_position = vertex_t(-camera->m_viewmatrix[0][3], -camera->m_viewmatrix[1][3], -camera->m_viewmatrix[2][3]);
		//vertex_t camera_position = 
		vector_t camera_vector = camera->position() - center_vertex;
		camera_vector.normalize();

		float dynamic_lights_intensity = std::accumulate(scene->point_source_lights.begin(), scene->point_source_lights.end(), 0.0f,
			[&](float total, const auto & psl)
			{
				vector_t light_direction = center_vertex - psl.position;
				float light_distance_squared = light_direction.len_squared();
				float diffuse = psl.intensity / light_distance_squared;
				light_direction.normalize();
				vector_t & normal = (*normals)[triangle_idx];
				float alignment = -normal.dot(light_direction);
				if (alignment < 0.0f)
					return total;
				diffuse *= alignment;

				// specular
				vector_t reflection = light_direction + normal * alignment * 2;
				float specular = reflection.dot(camera_vector);
				if (specular > 0)
				{
					static const int p = 32;
					specular = pow(specular, p);
					specular = specular * p / 2; // make the integral[0,1] of specular 0.5 again so that no extra light is generated
					// should multiply by overall albedo, too so that some light is absorbed
					return total + diffuse + specular / light_distance_squared;
				}
				else
				{
					return total + diffuse;
				}
			});

		light = scene->ambient_light_intensity + face_sun_intensity + dynamic_lights_intensity;
		light *= 65536;
	}
	virtual int shade(float progress) override
	{
		return light;
	}
	virtual void next_triangle() override
	{
		triangle_idx++;
	}
};

class pixel_shader_lights_semiflat : public pixel_shader_t
{
	vertex_shader_world vertex_shader;

	const model_t * model;
	const scene_t * scene;
	std::vector<vertex_t> vertices;
	std::vector<normal_t> * normals;
	int triangle_idx;

	float face_sun_intensity;

	vertex_t v0, v1, v2;
	vertex_t vleft, vright;
	vector_t vleftdir, vrightdir;
	vertex_t v;
	vector_t dir;

public:
	virtual void prepare_for_model(std::vector<vertex_t> & v,
	                               std::vector<normal_t> & n,
	                               const model_t & m,
	                               const scene_t & s,
	                               const camera_t & c,
	                               const viewport_t & vp)
	{
		model = &m;
		scene = &s;
		vertex_shader.shade(vertices, n, m, s, c, vp);
		normals = &n;
	}

	virtual void push_back_vertex_temporary(vertex_t & v) override
	{
		vertices.push_back(vertex_shader.shade_one(v));
	}
	virtual void pop_back_vertex_temporary() override
	{
		vertices.pop_back();
	}

	virtual void prepare_for_strip(const triangle_strip & strip) override
	{
		normals->clear();
		normals->reserve(strip.normals.size());
		for (const auto & n : strip.normals)
			normals->emplace_back(transform(n, model->orientation));

		triangle_idx = 0;
	}
	virtual void prepare_for_fan(const triangle_fan & fan) override
	{
		normals->clear();
		normals->reserve(fan.normals.size());
		for (const auto & n : fan.normals)
			normals->emplace_back(transform(n, model->orientation));

		triangle_idx = 0;
	}
	virtual void prepare_for_triangle_list(const triangle_list_t & list) override
	{
		normals->clear();
		normals->reserve(list.normals.size());
		for (const auto & n : list.normals)
			normals->emplace_back(transform(n, model->orientation));

		triangle_idx = 0;
	}
	virtual void prepare_for_triangle(const std::vector<vertex_idx> & indices, vertex_idx i0, vertex_idx i1, vertex_idx i2) override
	{
		v0 = vertices[indices[i0]];
		v1 = vertices[indices[i1]];
		v2 = vertices[indices[i2]];

		face_sun_intensity = -(*normals)[triangle_idx].dot(scene->sun_direction);
		if (face_sun_intensity < 0.0f)
			face_sun_intensity = 0.0f;
		else
			face_sun_intensity *= scene->sun_intensity;
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
	}
	virtual void prepare_for_lower_triangle(bool long_line_on_right) override
	{
		if (long_line_on_right)
		{
			vright = v0;
			vrightdir = v2-v0;
			vleft = v1;
			vleftdir = v2-v1;
		}
		else
		{
			vleft = v0;
			vleftdir = v2-v0;
			vright = v1;
			vrightdir = v2-v1;	
		}
	}
	virtual void prepare_for_scanline(float progress_left, float progress_right) override
	{
		v = vleft + vleftdir*progress_left;
		dir = vright + vrightdir*progress_right - v;
	}
	virtual int shade(float progress) override
	{
		vertex_t center_vertex = v + dir*progress;
		float dynamic_lights_intensity = std::accumulate(scene->point_source_lights.begin(), scene->point_source_lights.end(), 0.0f,
			[&](float total, const auto & psl)
			{
				vector_t light_direction = center_vertex - psl.position;
				float intensity = psl.intensity / light_direction.len_squared();
				if (intensity < 0.05)
					return total;
				light_direction.normalize();
				float alignment = -(*normals)[triangle_idx].dot(light_direction);
				if (alignment < 0.0f)
					return total;
				return total + alignment * intensity;
			});

		return 65536 * scene->ambient_light_intensity + face_sun_intensity + dynamic_lights_intensity;
	}
	virtual void next_triangle() override
	{
		triangle_idx++;
	}
};

class pixel_shader_texture : public pixel_shader_t
{
	const model_t * model;
	const scene_t * scene;
	const camera_t * camera;
	const viewport_t * viewport;

	const std::vector<vec2f_t> * texture_mapping;

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

public:
	virtual void prepare_for_model(std::vector<vertex_t> & v,
	                               std::vector<normal_t> & n,
	                               const model_t & m,
	                               const scene_t & s,
	                               const camera_t & c,
	                               const viewport_t & vp) override
	{
		model    = & m;
		scene    = & s;
		camera   = & c;
		viewport = & vp;

		tbitmap = m.mesh.textures[0]->m_mipmaps[0].m_bitmap;
		twidth  = m.mesh.textures[0]->m_mipmaps[0].m_width;
		theight = m.mesh.textures[0]->m_mipmaps[0].m_height;
	}

	virtual void prepare_for_strip(const triangle_strip & strip) override
	{
		texture_mapping = &strip.texture_mapping;
	}
	virtual void prepare_for_fan(const triangle_fan & fan) override
	{
		texture_mapping = &fan.texture_mapping;
	}
	virtual void prepare_for_triangle_list(const triangle_list_t & list) override
	{
		texture_mapping = &list.texture_mapping;
	}

	virtual void prepare_for_triangle(const std::vector<vertex_idx> & indices, vertex_idx i0, vertex_idx i1, vertex_idx i2) override
	{
		t0 = (*texture_mapping)[i0];
		t1 = (*texture_mapping)[i1];
		t2 = (*texture_mapping)[i2];

		t0[0][0] *= model->mesh.textures[0]->m_mipmaps[0].m_width;
		t0[0][1] *= model->mesh.textures[0]->m_mipmaps[0].m_height;
		t1[0][0] *= model->mesh.textures[0]->m_mipmaps[0].m_width;
		t1[0][1] *= model->mesh.textures[0]->m_mipmaps[0].m_height;
		t2[0][0] *= model->mesh.textures[0]->m_mipmaps[0].m_width;
		t2[0][1] *= model->mesh.textures[0]->m_mipmaps[0].m_height;

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
		int u = (int)t[0][0] % twidth;
		int v = (int)t[0][1] % theight;
		return tbitmap[v*twidth + u];
	}
};


class pixel_shader_texture_bilinear : public pixel_shader_t
{
	const model_t * model;
	const scene_t * scene;
	const camera_t * camera;
	const viewport_t * viewport;

	const std::vector<vec2f_t> * texture_mapping;

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

public:
	virtual void prepare_for_model(std::vector<vertex_t> & v,
	                               std::vector<normal_t> & n,
	                               const model_t & m,
	                               const scene_t & s,
	                               const camera_t & c,
	                               const viewport_t & vp) override
	{
		model    = & m;
		scene    = & s;
		camera   = & c;
		viewport = & vp;

		tbitmap = m.mesh.textures[0]->m_mipmaps[0].m_bitmap;
		twidth  = m.mesh.textures[0]->m_mipmaps[0].m_width;
		theight = m.mesh.textures[0]->m_mipmaps[0].m_height;
	}

	virtual void prepare_for_strip(const triangle_strip & strip) override
	{
		texture_mapping = &strip.texture_mapping;
	}
	virtual void prepare_for_fan(const triangle_fan & fan) override
	{
		texture_mapping = &fan.texture_mapping;
	}
	virtual void prepare_for_triangle_list(const triangle_list_t & list) override
	{
		texture_mapping = &list.texture_mapping;
	}

	virtual void prepare_for_triangle(const std::vector<vertex_idx> & indices, vertex_idx i0, vertex_idx i1, vertex_idx i2) override
	{
		t0 = (*texture_mapping)[i0];
		t1 = (*texture_mapping)[i1];
		t2 = (*texture_mapping)[i2];

		t0[0][0] *= model->mesh.textures[0]->m_mipmaps[0].m_width;
		t0[0][1] *= model->mesh.textures[0]->m_mipmaps[0].m_height;
		t1[0][0] *= model->mesh.textures[0]->m_mipmaps[0].m_width;
		t1[0][1] *= model->mesh.textures[0]->m_mipmaps[0].m_height;
		t2[0][0] *= model->mesh.textures[0]->m_mipmaps[0].m_width;
		t2[0][1] *= model->mesh.textures[0]->m_mipmaps[0].m_height;

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

		float v = t[0][0];
		float u = t[0][1];
		float u1 = u-0.5;
		float u2 = u+0.5;
		float v1 = v-0.5;
		float v2 = v+0.5;

		u = floor(u2);
		v = floor(v2);

		unsigned int v1m = (unsigned int) (((int)v1) % (int)theight);
		unsigned int v2m = v1m + 1;
		if (v2m > theight)
			v2m -= theight;
		v1m *= twidth;
		v2m *= twidth;
		unsigned int u1m = (unsigned int) (((int)u1) % (int)twidth);
		unsigned int u2m = u1m + 1;
		if (u2m > twidth)
			u2m -= twidth;
		
		return ( (pc[v1m + u1m] * ((u - u1) * (v - v1)))
		        +(pc[v2m + u1m] * ((u - u1) * (v2 - v)))
		        +(pc[v1m + u2m] * ((u2 - u) * (v - v1)))
		        +(pc[v2m + u2m] * ((u2 - u) * (v2 - v)))
		       ).to_int();
	}
};


template<typename L, typename T>
class pixel_shader_light_and_texture : public pixel_shader_t
{
	L shader_flat_light;
	T shader_texture;

	virtual void prepare_for_model(std::vector<vertex_t> & v,
	                               std::vector<normal_t> & n,
	                               const model_t & m,
	                               const scene_t & s,
	                               const camera_t & c,
	                               const viewport_t & vp) override
	{
		shader_flat_light.prepare_for_model(v, n, m, s, c, vp);
		shader_texture.prepare_for_model(v, n, m, s, c, vp);
	}

	virtual void push_back_vertex_temporary(vertex_t & v) override
	{
		shader_flat_light.push_back_vertex_temporary(v);
		shader_texture.push_back_vertex_temporary(v);
	}
	virtual void pop_back_vertex_temporary() override
	{
		shader_flat_light.pop_back_vertex_temporary();
		shader_texture.pop_back_vertex_temporary();
	}

	virtual void prepare_for_strip(const triangle_strip & strip) override
	{
		shader_flat_light.prepare_for_strip(strip);
		shader_texture.prepare_for_strip(strip);
	}
	virtual void prepare_for_fan(const triangle_fan & fan) override
	{
		shader_flat_light.prepare_for_fan(fan);
		shader_texture.prepare_for_fan(fan);
	}
	virtual void prepare_for_triangle_list(const triangle_list_t & list) override
	{
		shader_flat_light.prepare_for_triangle_list(list);
		shader_texture.prepare_for_triangle_list(list);
	}

	virtual void prepare_for_triangle(const std::vector<vertex_idx> & indices, vertex_idx i0, vertex_idx i1, vertex_idx i2) override
	{
		shader_flat_light.prepare_for_triangle(indices, i0, i1, i2);
		shader_texture.prepare_for_triangle(indices, i0, i1, i2);
	}
	virtual void next_triangle() override
	{
		shader_flat_light.next_triangle();
		shader_texture.next_triangle();
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
