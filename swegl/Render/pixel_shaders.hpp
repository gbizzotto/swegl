
#pragma once

#include "swegl/Data/model.hpp"
#include "swegl/Projection/points.hpp"

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
	                               const ViewPort & viewport) = 0;
	virtual void prepare_for_strip(const triangle_strip & strip) = 0;
	virtual void prepare_for_fan  (const triangle_fan & fan) = 0;
	virtual void prepare_for_triangle_list(const triangle_list_t & list) = 0;
	virtual void prepare_for_triangle(vertex_idx i0, vertex_idx i1, vertex_idx i2) = 0;
	virtual void next_triangle() = 0;
	virtual void prepare_for_upper_triangle(bool long_line_on_right) = 0;
	virtual void prepare_for_lower_triangle() = 0;
	virtual void prepare_for_scanline(float progress_left, float progress_right) = 0;
	virtual int shade(float progress) = 0;
};

struct pixel_shader_standard : public pixel_shader_t
{
	const model_t * model;
	const scene_t * scene;
	const Camera * camera;
	const ViewPort * viewport;

	std::vector<vertex_t> * vertices;
	std::vector<normal_t> * normals;
	const std::vector<Vec2f> * texture_mapping;

	Vec2f t0;
	Vec2f t1;
	Vec2f t2;
	float light;

	int triangle_idx;

	Vec2f side_long_t_dir;
	Vec2f side_short_t;
	Vec2f side_short_t_dir;

	bool long_line_on_right;	
	Vec2f t_left;
	Vec2f t_dir;

	unsigned int *tbitmap;
	unsigned int twidth;
	unsigned int theight;

	virtual void prepare_for_model(std::vector<vertex_t> & v,
	                               std::vector<normal_t> & n,
	                               const model_t & m,
	                               const scene_t & s,
	                               const Camera & c,
	                               const ViewPort & vp) override
	{
		vertices = & v;
		normals  = & n;
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
		normals->clear();
		normals->reserve(strip.normals.size());
		for (const auto & n : strip.normals)
			normals->emplace_back(Transform(n, model->orientation));

		texture_mapping = &strip.texture_mapping;

		triangle_idx = 0;
	}
	virtual void prepare_for_fan(const triangle_fan & fan) override
	{
		normals->clear();
		normals->reserve(fan.normals.size());
		for (const auto & n : fan.normals)
			normals->emplace_back(Transform(n, model->orientation));

		texture_mapping = &fan.texture_mapping;

		triangle_idx = 0;
	}
	virtual void prepare_for_triangle_list(const triangle_list_t & list) override
	{
		normals->clear();
		normals->reserve(list.normals.size());
		for (const auto & n : list.normals)
			normals->emplace_back(Transform(n, model->orientation));

		texture_mapping = &list.texture_mapping;

		triangle_idx = 0;
	}

	virtual void prepare_for_triangle(vertex_idx i0, vertex_idx i1, vertex_idx i2) override
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

		float face_sun_intensity = -(*normals)[triangle_idx].dot(scene->sun_direction);
		if (face_sun_intensity < 0.0f)
			face_sun_intensity = 0.0f;
		light = scene->ambient_light_intensity + face_sun_intensity*scene->sun_intensity;
		if (light > 1.0f)
			light = 1.0f;
	}
	
	virtual void next_triangle() override
	{
		triangle_idx++;
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
		int color = tbitmap[v*twidth + u];
		((unsigned char*)&color)[0] *= light;
		((unsigned char*)&color)[1] *= light;
		((unsigned char*)&color)[2] *= light;
		return color;
	}
};

} // namespace
