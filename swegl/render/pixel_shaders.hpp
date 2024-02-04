
#pragma once

#include <numeric>
#include <cmath>

#include "swegl/data/model.hpp"
#include "swegl/projection/points.hpp"
#include "swegl/render/colors.hpp"
#include "swegl/render/viewport.hpp"

namespace swegl
{

struct pixel_shader_t
{
	const primitive_t * primitive;
	const scene_t * scene;
	const viewport_t * viewport;
	pixel_colors color;
	bool double_sided;

	virtual void prepare_for_primitive(const primitive_t & p, const scene_t & s, const viewport_t & vp);
	virtual void prepare_for_triangle(vertex_idx, vertex_idx, vertex_idx, bool) {}
	virtual void prepare_for_upper_triangle([[maybe_unused]] bool long_line_on_right) {}
	virtual void prepare_for_lower_triangle([[maybe_unused]] bool long_line_on_right) {}
	virtual void prepare_for_scanline([[maybe_unused]] float progress_left, [[maybe_unused]] float progress_right) {}
	virtual int shade([[maybe_unused]] float progress) { return color.to_int(); }
};

struct pixel_shader_lights_flat : pixel_shader_t
{
	float light;

	virtual void prepare_for_triangle(vertex_idx i0, vertex_idx i1, vertex_idx i2, bool inverted) override;
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

	virtual void prepare_for_triangle(vertex_idx i0, vertex_idx i1, vertex_idx i2, bool inverted) override;
	virtual void prepare_for_upper_triangle(bool long_line_on_right) override;
	virtual void prepare_for_lower_triangle(bool long_line_on_right) override;
	virtual void prepare_for_scanline(float progress_left, float progress_right) override;
	virtual int shade(float progress) override;
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

	unsigned int default_bitmap = pixel_colors(128,128,128,255).to_int();
	unsigned int *tbitmap;
	unsigned int twidth;
	unsigned int theight;

	virtual void prepare_for_primitive(const primitive_t & p, const scene_t & s, const viewport_t & vp) override;
	virtual void prepare_for_triangle(vertex_idx i0, vertex_idx i1, vertex_idx i2, bool) override;
	virtual void prepare_for_upper_triangle(bool long_line_on_right) override;
	virtual void prepare_for_lower_triangle(bool long_line_on_right) override;
	virtual void prepare_for_scanline(float progress_left, float progress_right) override;
	virtual int shade(float progress) override;
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

	unsigned int default_bitmap = pixel_colors(128,128,128,255).to_int();
	unsigned int *tbitmap;
	float twidth;
	float theight;

	virtual void prepare_for_primitive(const primitive_t & p, const scene_t & s, const viewport_t & vp) override;
	virtual void prepare_for_triangle(vertex_idx i0, vertex_idx i1, vertex_idx i2, bool) override;
	virtual void prepare_for_upper_triangle(bool long_line_on_right) override;
	virtual void prepare_for_lower_triangle(bool long_line_on_right) override;
	virtual void prepare_for_scanline(float progress_left, float progress_right) override;
	virtual int shade(float progress) override;
};


template<typename L, typename T>
struct pixel_shader_light_and_texture : pixel_shader_t
{
	L shader_flat_light;
	T shader_texture;

	virtual void prepare_for_primitive(const primitive_t & p,
	                                   const scene_t & s,
	                                   const viewport_t & vp) override
	{
		shader_flat_light.prepare_for_primitive(p, s, vp);
		shader_texture.prepare_for_primitive(p, s, vp);
	}

	virtual void prepare_for_triangle(vertex_idx i0, vertex_idx i1, vertex_idx i2, bool inverted) override
	{
		shader_flat_light.prepare_for_triangle(i0, i1, i2, inverted);
		shader_texture.prepare_for_triangle(i0, i1, i2, inverted);
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
