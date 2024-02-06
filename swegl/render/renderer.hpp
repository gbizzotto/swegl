

#pragma once

#include <iostream>
#include <thread>
#include <mutex>
#include <cmath>
#include <algorithm>

#include <swegl/render/viewport.hpp>
#include <swegl/data/model.hpp>
#include <swegl/render/vertex_shaders.hpp>
#include <swegl/misc/fraction.hpp>

namespace swegl
{

/*
void _render(scene_t & scene, viewport_t & viewport);

template<typename...T>
void _render(scene_t & scene, viewport_t & viewport, T&...t)
{
	_render(scene, viewport);
	_render(scene, t...);
}

template<typename...T>
void render(scene_t & scene, T&...t)
{
	// Transform scene into world coordinates with vertex shader ONCE for all viewports
	vertex_shader_t::original_to_world(scene);

	_render(scene, t...);
}
*/


void _render(fraction_t thread_number, new_scene_t & scene, viewport_t & viewport);

template<typename...T>
void _render(fraction_t thread_number, new_scene_t & scene, viewport_t & viewport, T&...t)
{
	_render(thread_number, scene, viewport);
	_render(thread_number, scene, t...);
}

template<typename...T>
void render(size_t thread_count, new_scene_t & scene, T&...t)
{
	scene.thread_local_extra_vertices .resize(thread_count);
	scene.thread_local_extra_triangles.resize(thread_count);

	// Transform scene into world coordinates with vertex shader ONCE for all viewports
	new_vertex_shader_t::original_to_world(fraction_t{0,1}, scene);

	_render(fraction_t{0,1}, scene, t...);
}

} // namespace
