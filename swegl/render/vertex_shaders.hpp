
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
	static inline void original_to_world(scene_t & scene)
	{
		for (auto & model : scene.models)
		{
			matrix44_t original_to_world_matrix = model.orientation;
			original_to_world_matrix.translate(model.position.x(), model.position.y(), model.position.z());
			//__gnu_parallel::for_each(model.mesh.vertices.begin(), model.mesh.vertices.end(), [&](auto & mv)
			for (auto & primitive : model.primitives)
				for (auto & mv : primitive.vertices)
				{
					mv.v_world = transform(mv.v, original_to_world_matrix);
				}
			//);
		}
	}
	static inline void world_to_camera_or_frustum(scene_t & scene, const viewport_t & viewport)
	{
		for (auto & model : scene.models)
		{
			//__gnu_parallel::for_each(model.mesh.vertices.begin(), model.mesh.vertices.end(), [&](auto & mv)
			for (auto & primitive : model.primitives)
				for (auto & mv : primitive.vertices)
				{
					mv.yes = false;
					mv.v_viewport = transform(mv.v_world, viewport.camera().m_viewmatrix);
					//if (mv.v_viewport.z() >= 0.001)
						camera_to_frustum(mv, model, viewport);
				}
			//);
		}
	}

	static inline void world_to_viewport(mesh_vertex_t & mv, const node_t & model, const viewport_t & viewport)
	{
		mv.v_viewport = transform(mv.v_world, viewport.camera().m_viewmatrix);
		camera_to_frustum(mv, model, viewport);
		frustum_to_viewport(mv, viewport);
	}

	static inline void camera_to_frustum(mesh_vertex_t & mv, const node_t & model, const viewport_t & viewport)
	{
		mv.normal_world = rotate(mv.normal, model.orientation);

		mv.v_viewport = transform(mv.v_viewport, viewport.camera().m_projectionmatrix);
		if (mv.v_viewport.z() != 0)
		{
			mv.v_viewport.x() /= fabs(mv.v_viewport.z());
			mv.v_viewport.y() /= fabs(mv.v_viewport.z());
		}
	}
	static inline void frustum_to_viewport(node_t & model, const viewport_t & viewport)
	{
		//__gnu_parallel::for_each(model.mesh.vertices.begin(), model.mesh.vertices.end(), [&](auto & mv)
		for (auto & primitive : model.primitives)
			for (auto & mv : primitive.vertices)
			{
				if (mv.yes)
					viewport.transform(mv);
			}
		//);
	}
	static inline void frustum_to_viewport(mesh_vertex_t & mv, const viewport_t & viewport)
	{
		viewport.transform(mv);
	}
};

} // namespace
