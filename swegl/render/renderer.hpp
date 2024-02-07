

#pragma once

#include <iostream>
#include <thread>
#include <mutex>
#include <cmath>
#include <algorithm>

#include <utttil/perf.hpp>

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


void _render(const fraction_t & thread_number, new_scene_t & scene, viewport_t & viewport);

template<typename...T>
void _render(const fraction_t & thread_number, new_scene_t & scene, viewport_t & viewport, T&...t)
{
	_render(thread_number, scene, viewport);
	_render(thread_number, scene, t...);
}

template<typename...T>
void render(size_t thread_count, new_scene_t & scene, T&...t)
{
	scene.thread_local_extra_vertices .resize(thread_count);
	scene.thread_local_extra_triangles.resize(thread_count);

	std::vector<std::thread> threads;
	threads.reserve(thread_count);

	scene.extra_vertices .clear();
	scene.extra_triangles.clear();

	{		
		for (size_t i=0 ; i<thread_count ; i++)
			threads.emplace_back([i,thread_count,&scene,&t...]()
				{
					// Transform scene into world coordinates with vertex shader ONCE for all viewports
					new_vertex_shader_t::original_to_world(fraction_t{(int)i,(int)thread_count}, scene);
					_render(fraction_t{(int)i,(int)thread_count}, scene, t...);
				});
	}

	for (size_t i=0 ; i<thread_count ; i++)
		threads[i].join();	
}

} // namespace
