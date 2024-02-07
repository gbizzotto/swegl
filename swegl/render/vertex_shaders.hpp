
#pragma once

#include "swegl/data/model.hpp"
#include "swegl/projection/points.hpp"
#include "swegl/misc/lerp.hpp"
#include "swegl/misc/fraction.hpp"
#include "swegl/render/viewport.hpp"

#ifndef _GLIBCXX_PARALLEL
#define __gnu_parallel std
#endif

namespace swegl
{

struct new_vertex_shader_t
{
	static void original_to_world(new_scene_t & scene, node_t & node, const matrix44_t & vertex_parent_matrix, const matrix44_t & normal_parent_matrix);
	static void original_to_world(fraction_t thread_number, new_scene_t & scene);
	static void cut_triangle_if_needed(fraction_t thread_number, new_scene_t & scene, const viewport_t & viewport, new_triangle_t & triangle, bool face_normals, bool vertex_normals, const matrix44_t & matrix);
	static void world_to_screen(fraction_t thread_number, new_scene_t & scene, const viewport_t & viewport, bool face_normals, bool vertex_normals);
};

} // namespace
