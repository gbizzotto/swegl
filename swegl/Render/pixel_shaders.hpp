
#pragma once

#include "swegl/Data/model.hpp"
#include "swegl/Projection/points.hpp"
#include "swegl/Render/vertex_shaders.hpp"

namespace swegl
{

class pixel_shader_t
{
public:
	virtual void prepare_for_model(std::vector<vertex_t> & vertices,
	                               std::vector<normal_t> & normals,
	                               const model_t & model,
	                               const scene_t & scene,
	                               const Camera & camera,
	                               const ViewPort & viewport) {}
	virtual void prepare_for_strip(const triangle_strip & strip) {}
	virtual void prepare_for_fan  (const triangle_fan & fan) {}
	virtual void prepare_for_triangle_list(const triangle_list_t & list) {}
	virtual void prepare_for_triangle(const std::vector<vertex_idx> & indices, vertex_idx i0, vertex_idx i1, vertex_idx i2) {}
	virtual void next_triangle() {}
	virtual void prepare_for_upper_triangle(bool long_line_on_right) {}
	virtual void prepare_for_lower_triangle() {}
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

	float ambient_light_intensity;
	float face_sun_intensity;

	vertex_t v0, v1, v2;
	bool long_line_on_right;
	vertex_t vleft, vright;
	vector_t vleftdir, vrightdir;
	vertex_t v;
	vector_t dir;

public:
	float light;

	virtual void prepare_for_model(std::vector<vertex_t> & v,
	                               std::vector<normal_t> & n,
	                               const model_t & m,
	                               const scene_t & s,
	                               const Camera & c,
	                               const ViewPort & vp)
	{
		model = &m;
		scene = &s;
		vertex_shader.shade(vertices, n, m, s, c, vp);
		normals = &n;

		ambient_light_intensity = s.ambient_light_intensity;
	}

	virtual void prepare_for_strip(const triangle_strip & strip) override
	{
		normals->clear();
		normals->reserve(strip.normals.size());
		for (const auto & n : strip.normals)
			normals->emplace_back(Transform(n, model->orientation));

		triangle_idx = 0;
	}
	virtual void prepare_for_fan(const triangle_fan & fan) override
	{
		normals->clear();
		normals->reserve(fan.normals.size());
		for (const auto & n : fan.normals)
			normals->emplace_back(Transform(n, model->orientation));

		triangle_idx = 0;
	}
	virtual void prepare_for_triangle_list(const triangle_list_t & list) override
	{
		normals->clear();
		normals->reserve(list.normals.size());
		for (const auto & n : list.normals)
			normals->emplace_back(Transform(n, model->orientation));

		triangle_idx = 0;
	}
	virtual void prepare_for_triangle(const std::vector<vertex_idx> & indices, vertex_idx i0, vertex_idx i1, vertex_idx i2) override
	{
		// add new fake vertices

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
		this->long_line_on_right = long_line_on_right;
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
	virtual void prepare_for_lower_triangle() override
	{
		if (long_line_on_right)
		{
			vleft = v1;
			vleftdir = v2-v1;
		}
		else
		{
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
		float dynamic_lights_intensity = 0.0f;
		for (const auto & psl : scene->point_source_lights)
		{
			vector_t light_direction = center_vertex - psl.position;
			float distance_squared = light_direction.len_squared();
			light_direction.normalize();
			float light_intensity = -(*normals)[triangle_idx].dot(light_direction);
			if (light_intensity > 0.0f)
			{
				// divide by square of distance to light
				light_intensity /= distance_squared;
				dynamic_lights_intensity += light_intensity * psl.intensity;
			}
		}

		light = ambient_light_intensity + face_sun_intensity + dynamic_lights_intensity;
		if (light > 1.0f)
			light = 1.0f;

		return light*256;
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
	const Camera * camera;
	const ViewPort * viewport;

	const std::vector<Vec2f> * texture_mapping;

	Vec2f t0;
	Vec2f t1;
	Vec2f t2;

	Vec2f side_long_t_dir;
	Vec2f side_short_t;
	Vec2f side_short_t_dir;

	bool long_line_on_right;	
	Vec2f t_left;
	Vec2f t_dir;

	unsigned int *tbitmap;
	unsigned int twidth;
	unsigned int theight;

public:
	virtual void prepare_for_model(std::vector<vertex_t> & v,
	                               std::vector<normal_t> & n,
	                               const model_t & m,
	                               const scene_t & s,
	                               const Camera & c,
	                               const ViewPort & vp) override
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

	virtual void prepare_for_lower_triangle() override
	{
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
		Vec2f t = t_left + t_dir * progress;
		int u = (int)t[0][0] % twidth;
		int v = (int)t[0][1] % theight;
		return tbitmap[v*twidth + u];
	}
};

class pixel_shader_standard : public pixel_shader_t
{
	pixel_shader_lights_flat shader_flat_light;
	pixel_shader_texture shader_texture;

	virtual void prepare_for_model(std::vector<vertex_t> & v,
	                               std::vector<normal_t> & n,
	                               const model_t & m,
	                               const scene_t & s,
	                               const Camera & c,
	                               const ViewPort & vp) override
	{
		shader_flat_light.prepare_for_model(v, n, m, s, c, vp);
		shader_texture.prepare_for_model(v, n, m, s, c, vp);
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

	virtual void prepare_for_lower_triangle() override
	{
		shader_flat_light.prepare_for_lower_triangle();
		shader_texture.prepare_for_lower_triangle();
	}

	virtual void prepare_for_scanline(float progress_left, float progress_right) override
	{
		shader_flat_light.prepare_for_scanline(progress_left, progress_right);
		shader_texture.prepare_for_scanline(progress_left, progress_right);
	}

	virtual int shade(float progress) override
	{
		int color = shader_texture.shade(progress);
		float light = shader_flat_light.shade(progress) / 256.0f;
		((unsigned char*)&color)[0] *= light;
		((unsigned char*)&color)[1] *= light;
		((unsigned char*)&color)[2] *= light;
		return color;
	}
};

} // namespace
