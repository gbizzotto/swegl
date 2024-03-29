
#pragma once

#include "swegl/data/model.hpp"
#include "swegl/projection/points.hpp"

#ifndef _GLIBCXX_PARALLEL
#define __gnu_parallel std
#endif

namespace swegl
{

struct vertex_shader_t
{
	static inline void original_to_world(scene_t & scene, node_t & node, const matrix44_t & parent_matrix)
	{
		node.original_to_world_matrix = parent_matrix * node.get_local_world_matrix();
		//__gnu_parallel::for_each(node.mesh.vertices.begin(), node.mesh.vertices.end(), [&](auto & mv)
		for (auto & primitive : node.primitives)
		{
			for (auto & mv : primitive.vertices)
				mv.v_world = transform(mv.v, node.original_to_world_matrix);
		}
		for (auto child_idx : node.children_idx)
			original_to_world(scene, scene.nodes[child_idx], node.original_to_world_matrix);
		//);
	}
	static inline void original_to_world(scene_t & scene)
	{
		for (auto node_idx : scene.root_nodes)
			original_to_world(scene, scene.nodes[node_idx], matrix44_t::Identity);
	}

	static inline void world_to_camera_or_frustum(node_t & node, const viewport_t & viewport)	
	{
		//__gnu_parallel::for_each(node.mesh.vertices.begin(), node.mesh.vertices.end(), [&](auto & mv)
		for (auto & primitive : node.primitives)
			for (auto & mv : primitive.vertices)
			{
				mv.yes = false;
				mv.v_viewport = transform(mv.v_world, viewport.camera().m_viewmatrix);
				//if (mv.v_viewport.z() >= 0.001)
					camera_to_frustum(mv, node, viewport);
			}
		//);
	}
	static inline void world_to_camera_or_frustum(scene_t & scene, const viewport_t & viewport)
	{
		for (auto & node : scene.nodes)
			world_to_camera_or_frustum(node, viewport);
	}

	static inline void world_to_viewport(mesh_vertex_t & mv, const node_t & node, const viewport_t & viewport)
	{
		mv.v_viewport = transform(mv.v_world, viewport.camera().m_viewmatrix);
		camera_to_frustum(mv, node, viewport);
		frustum_to_viewport(mv, viewport);
	}

	static inline void camera_to_frustum(mesh_vertex_t & mv, const node_t & node, const viewport_t & viewport)
	{
		mv.normal_world = rotate(mv.normal, scale(node.rotation, node.scale)).normalize();

		mv.v_viewport = transform(mv.v_viewport, viewport.camera().m_projectionmatrix);
		if (mv.v_viewport.z() != 0)
		{
			mv.v_viewport.x() /= fabs(mv.v_viewport.z());
			mv.v_viewport.y() /= fabs(mv.v_viewport.z());
		}
	}
	static inline void frustum_to_viewport(node_t & node, const viewport_t & viewport)
	{
		//__gnu_parallel::for_each(node.primitives.begin(), node.primitives.end(), [&](auto & primitive) {
		for (auto & primitive : node.primitives)
			//__gnu_parallel::for_each(primitive.vertices.begin(), primitive.vertices.end(), [&](auto & mv) {
				for (auto & mv : primitive.vertices)
				{
					if (mv.yes)
						viewport.transform(mv);
				}
			//});
		//});
	}
	static inline void frustum_to_viewport(mesh_vertex_t & mv, const viewport_t & viewport)
	{
		viewport.transform(mv);
	}
};

} // namespace
