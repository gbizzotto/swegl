

#pragma once

#include <iostream>
#include <thread>
#include <mutex>
#include <cmath>
#include <algorithm>

#include <swegl/render/viewport.hpp>
#include <swegl/data/model.hpp>
#include <swegl/render/vertex_shaders.hpp>

namespace swegl
{

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

} // namespace
