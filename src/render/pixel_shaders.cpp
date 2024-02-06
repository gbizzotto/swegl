
#include <cassert>

#include <swegl/render/pixel_shaders.hpp>
#include <swegl/render/viewport.hpp>
#include <swegl/render/colors.hpp>
#include <swegl/data/model.hpp>

#ifndef _GLIBCXX_PARALLEL
#define __gnu_parallel std
#endif

namespace swegl
{

void pixel_shader_t::prepare_for_scene(viewport_t & vp, new_scene_t & s)
{
	viewport = &vp;
	scene = &s;
}

void pixel_shader_lights_flat::prepare_for_triangle(new_triangle_t & triangle, const new_mesh_vertex_t *, const new_mesh_vertex_t *, const new_mesh_vertex_t *)
{
	float face_sun_intensity = - triangle.normal_world.dot(scene->sun_direction);
	if (face_sun_intensity < 0.0f)
		face_sun_intensity = 0.0f;
	else
		face_sun_intensity *= scene->sun_intensity;

	vertex_t center_vertex = (scene->vertices[triangle.i0].v_world + scene->vertices[triangle.i1].v_world + scene->vertices[triangle.i2].v_world) / 3;
	vector_t camera_vector = viewport->camera().position() - center_vertex;
	camera_vector.normalize();

	float dynamic_lights_intensity = 0.0f;
	__gnu_parallel::for_each(scene->point_source_lights.begin(), scene->point_source_lights.end(),
		[&](const auto & psl)
		{
			vector_t light_direction = center_vertex - psl.position;
			float light_distance_squared = light_direction.len_squared();
			float diffuse = psl.intensity / light_distance_squared;
			if (diffuse < 0.05)
				return;
			light_direction.normalize();
			float alignment = -triangle.normal_world.dot(light_direction);
			if (alignment < 0.0f)
				return;
			diffuse *= alignment;

			// specular
			vector_t reflection = light_direction + triangle.normal_world * (alignment * 2);
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

void pixel_shader_lights_phong::prepare_for_triangle(new_triangle_t & triangle, const new_mesh_vertex_t * _v0, const new_mesh_vertex_t * _v1, const new_mesh_vertex_t * _v2)
{
	v0 = _v0->v_world;
	v1 = _v1->v_world;
	v2 = _v2->v_world;
	if ( ! triangle.backface)
	{
		n0 = _v0->normal_world;
		n1 = _v1->normal_world;
		n2 = _v2->normal_world;
	}
	else
	{
		n0 = - _v0->normal_world;
		n1 = - _v1->normal_world;
		n2 = - _v2->normal_world;
	}
}
void pixel_shader_lights_phong::prepare_for_upper_triangle(bool long_line_on_right)
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
void pixel_shader_lights_phong::prepare_for_lower_triangle(bool long_line_on_right)
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
void pixel_shader_lights_phong::prepare_for_scanline(float progress_left, float progress_right)
{
	v = vleft + vleftdir*progress_left;
	vdir = vright + vrightdir*progress_right - v;
	n = nleft + nleftdir*progress_left;
	ndir = nright + nrightdir*progress_right - n;
}
int pixel_shader_lights_phong::shade(float progress)
{
	vertex_t center_vertex = v + vdir*progress;
	normal_t normal        = n + ndir*progress;
	vector_t camera_vector = viewport->camera().position() - center_vertex;
	camera_vector.normalize();

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
			float light_distance_squared = light_direction.len_squared();
			float diffuse = psl.intensity / light_distance_squared;
			if (diffuse < 0.05)
				return;
			light_direction.normalize();
			float alignment = - normal.dot(light_direction);
			if (alignment < 0.0f)
				return;
			diffuse *= alignment;

			// specular
			vector_t reflection = light_direction + normal * (alignment * 2);
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

	return 65536 * (scene->ambient_light_intensity + face_sun_intensity + dynamic_lights_intensity);
}


void pixel_shader_texture::prepare_for_triangle(new_triangle_t & triangle, const new_mesh_vertex_t * v0, const new_mesh_vertex_t * v1, const new_mesh_vertex_t * v2)
{
	// TODO: select LOD / mipmap according to distance from camera

	if (triangle.material_idx == -1)
	{
		default_bitmap = scene->default_material.color.to_int();
		tbitmap = &default_bitmap;
		twidth  = 1;
		theight = 1;
	}
	else if (scene->materials[triangle.material_idx].texture_idx == -1)
	{
		default_bitmap = scene->materials[triangle.material_idx].color.to_int();
		tbitmap = &default_bitmap;
		twidth  = 1;
		theight = 1;
	}
	else
	{
		int texture_id = scene->materials[triangle.material_idx].texture_idx;
		tbitmap = scene->images[texture_id].m_mipmaps[0]->m_bitmap;
		twidth  = scene->images[texture_id].m_mipmaps[0]->m_width;
		theight = scene->images[texture_id].m_mipmaps[0]->m_height;
	}

	t0 = v0->tex_coords;
	t1 = v1->tex_coords;
	t2 = v2->tex_coords;

	t0.x() *= twidth;
	t0.y() *= theight;
	t1.x() *= twidth;
	t1.y() *= theight;
	t2.x() *= twidth;
	t2.y() *= theight;

	side_long_t_dir = t2 - t0;
}

void pixel_shader_texture::prepare_for_upper_triangle(bool long_line_on_right)
{
	this->long_line_on_right = long_line_on_right;
	side_short_t = t0;
	side_short_t_dir = t1 - t0;
}

void pixel_shader_texture::prepare_for_lower_triangle(bool long_line_on_right)
{
	this->long_line_on_right = long_line_on_right;
	side_short_t = t1;
	side_short_t_dir = t2 - t1;
}

void pixel_shader_texture::prepare_for_scanline(float progress_left, float progress_right)
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

int pixel_shader_texture::shade(float progress)
{
	vec2f_t t = t_left + t_dir * progress;
	int u = (int)t.x() % twidth;
	int v = (int)t.y() % theight;
	return tbitmap[v*twidth + u];
}


void pixel_shader_texture_bilinear::prepare_for_triangle(new_triangle_t & triangle, const new_mesh_vertex_t * v0, const new_mesh_vertex_t * v1, const new_mesh_vertex_t * v2)
{
	if (triangle.material_idx == -1)
	{
		default_bitmap = scene->default_material.color.to_int();
		tbitmap = &default_bitmap;
		twidth  = 1;
		theight = 1;
	}
	else if (scene->materials[triangle.material_idx].texture_idx == -1)
	{
		default_bitmap = scene->materials[triangle.material_idx].color.to_int();
		tbitmap = &default_bitmap;
		twidth  = 1;
		theight = 1;
	}
	else
	{
		int texture_id = scene->materials[triangle.material_idx].texture_idx;
		tbitmap = scene->images[texture_id].m_mipmaps[0]->m_bitmap;
		twidth  = scene->images[texture_id].m_mipmaps[0]->m_width;
		theight = scene->images[texture_id].m_mipmaps[0]->m_height;
	}

	t0 = v0->tex_coords;
	t1 = v1->tex_coords;
	t2 = v2->tex_coords;

	t0.x() *= twidth;
	t0.y() *= theight;
	t1.x() *= twidth;
	t1.y() *= theight;
	t2.x() *= twidth;
	t2.y() *= theight;

	side_long_t_dir = t2 - t0;
}

void pixel_shader_texture_bilinear::prepare_for_upper_triangle(bool long_line_on_right)
{
	this->long_line_on_right = long_line_on_right;
	side_short_t = t0;
	side_short_t_dir = t1 - t0;
}

void pixel_shader_texture_bilinear::prepare_for_lower_triangle(bool long_line_on_right)
{
	this->long_line_on_right = long_line_on_right;
	side_short_t = t1;
	side_short_t_dir = t2 - t1;
}

void pixel_shader_texture_bilinear::prepare_for_scanline(float progress_left, float progress_right)
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

int pixel_shader_texture_bilinear::shade(float progress)
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

	int v1m = ((int)v1) % (int)theight;
	while (v1m < 0)
		v1m += theight;
	int v2m = v1m + 1;
	while (v2m == theight)
		v2m = 0;
	int u1m = ((int)u1) % (int)twidth;
	while (u1m < 0)
		u1m += twidth;
	int u2m = u1m + 1;
	while (u2m == twidth)
		u2m = 0;
	
	assert(u1m >= 0);
	assert(u1m <  twidth);
	assert(u2m >= 0);
	assert(u2m <  twidth);
	assert(v1m >= 0);
	assert(v1m <  theight);
	assert(v2m >= 0);
	assert(v2m <  theight);

	v1m *= twidth;
	v2m *= twidth;
	
	return ( (pc[v1m + u1m] * ((u - u1) * (v - v1)))
	        +(pc[v2m + u1m] * ((u - u1) * (v2 - v)))
	        +(pc[v1m + u2m] * ((u2 - u) * (v - v1)))
	        +(pc[v2m + u2m] * ((u2 - u) * (v2 - v)))
	       ).to_int();
}

} // namespace
