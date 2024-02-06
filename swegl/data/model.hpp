
#pragma once

#include <swegl/projection/camera.hpp>
#include <swegl/projection/vec2f.hpp>
#include <swegl/projection/points.hpp>
#include <swegl/projection/matrix44.hpp>
#include <swegl/data/texture.hpp>
#include <swegl/render/colors.hpp>


namespace swegl
{

using vertex_idx = std::uint32_t;

struct new_mesh_vertex_t
{
	vertex_t v;
	vertex_t v_world;    // after transformations into world coordinates
	vertex_t v_viewport; // after transformations into viewport coordinates (pixel x,y + z depth
	vec2f_t tex_coords;
	normal_t normal;
	normal_t normal_world;
	int node_idx;        // index into scene.nodes, which contains the vertex' world matrix
	bool yes = false;
};
bool operator==(const new_mesh_vertex_t & left, const new_mesh_vertex_t & right);
bool operator<(const new_mesh_vertex_t & left, const new_mesh_vertex_t & right);

struct new_triangle_t
{
	vertex_idx i0, i1, i2; // indices into scene.vertices
	normal_t normal;
	normal_t normal_world;
	int material_idx;      // index into scene.materials
	int node_idx;
	bool yes;
	bool backface;
};

struct node_t
{
	vertex_t scale = vertex_t(1,1,1);
	matrix44_t rotation = matrix44_t::Identity;
	vertex_t translation = vertex_t(0,0,0);

	inline matrix44_t get_local_vertex_world_matrix() const
	{
		matrix44_t m = swegl::scale(rotation, scale);
		m.translate(translation.x(), translation.y(), translation.z());
		return m;
	}
	inline matrix44_t get_local_normal_world_matrix() const
	{
		return swegl::scale(rotation, scale);
	}

	// final transformation matrix, including parents
	matrix44_t vertex_original_to_world_matrix = matrix44_t::Identity;
	matrix44_t normal_original_to_world_matrix = matrix44_t::Identity;

	//std::vector<primitive_t> primitives;

	std::vector<int> children_idx;

	bool root = true;

	normal_t transform(const normal_t & n);
};


struct point_source_light
{
	vertex_t position;
	float intensity;
};

struct material_t
{
	pixel_colors color = pixel_colors(255,255,255,255);
	float metallic = 1;
	float roughness = 1;
	int texture_idx = -1; // -1 means none
	bool double_sided = false;
};


struct animation_step_t
{
	float time;
	vec4f_t value;
};
struct animation_channel_t
{
	enum path_t
	{
		SCALE       = 0,
		ROTATION    = 1,
		TRANSLATION = 2,
		WEIGHTS     = 3,
		NONE        = 3,
	};
	int node_idx;
	path_t path;
	std::vector<animation_step_t> steps;

	std::tuple<animation_step_t&,animation_step_t&> get_steps(float time);
};
struct animation_t
{
	float end_time = 0.0f;
	std::vector<animation_channel_t> channels;
};

struct new_scene_t
{
	std::vector<node_t> nodes;
	std::vector<int> root_nodes;

	float ambient_light_intensity;

	normal_t sun_direction;
	float sun_intensity;

	material_t default_material;

	std::vector<point_source_light> point_source_lights;

	std::vector<material_t> materials;
	std::vector<texture_t> images;
	std::vector<animation_t> animations;

	std::vector<new_mesh_vertex_t> vertices;
	std::vector<new_triangle_t   > triangles;

	std::vector<std::vector<new_mesh_vertex_t>> thread_local_extra_vertices;
	std::vector<std::vector<new_triangle_t   >> thread_local_extra_triangles;

	void animate(const float elapsed_seconds);
};
/*
struct scene_t
{
	std::vector<node_t> nodes;
	std::vector<int> root_nodes;

	float ambient_light_intensity;

	normal_t sun_direction;
	float sun_intensity;

	material_t default_material;

	std::vector<point_source_light> point_source_lights;

	std::vector<material_t> materials;
	std::vector<texture_t> images;
	std::vector<animation_t> animations;

	inline void animate(const float elapsed_seconds)
	{
		for (auto & animation : animations)
		{
			float relative_seconds = fmod(elapsed_seconds, animation.end_time);

			for (auto & channel : animation.channels)
			{
				// calculate frame with linear interpolation
				const auto & [before_step, after_step] = channel.get_steps(relative_seconds);
				vec4f_t frame = [&]()
					{
						if (after_step.time == before_step.time)
							return before_step.value;
						else
							return (before_step.value * ((after_step.time  - relative_seconds) / (after_step.time - before_step.time)))
							     + ( after_step.value * ((relative_seconds - before_step.time) / (after_step.time - before_step.time)));
					}();

				node_t & node = nodes[channel.node_idx];
				if (channel.path == animation_channel_t::path_t::ROTATION)
				{
					frame.normalize();
					node.rotation = matrix44_t::from_quaternion(frame.x(), frame.y(), frame.z(), frame.w());
				}
				else if (channel.path == animation_channel_t::path_t::TRANSLATION)
					node.translation = vertex_t(frame.x(), frame.y(), frame.z());
				else if (channel.path == animation_channel_t::path_t::SCALE)
					node.scale = vertex_t(frame.x(), frame.y(), frame.z());
			}
		}
	}
};
*/

//void calculate_face_normals(primitive_t & primitive);node_t make_cube(float size, int material_idx);
//node_t make_tore(unsigned int precision, int material_idx);
//node_t make_tri(float size, int material_idx);
//node_t make_cube(float size, int material_idx);
//node_t make_sphere(unsigned int precision, float radius, int material_idx);

} // namespace
