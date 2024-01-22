
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
			__gnu_parallel::for_each(model.mesh.vertices.begin(), model.mesh.vertices.end(), [&](auto & mv)
				{
					mv.v_world      = transform(mv.v     , original_to_world_matrix);
					mv.normal_world = rotate   (mv.normal, model.orientation       );
				});
		}
		
	}
	static inline void world_to_viewport(scene_t & scene, const viewport_t & viewport)
	{
		matrix44_t world_to_viewport_matrix = viewport.camera().m_projectionmatrix * viewport.camera().m_viewmatrix;
		for (auto & model : scene.models)
		{
			for (size_t i=0 ; i<model.mesh.vertices.size() ; i++)
			{
				model.mesh.vertices[i].v_viewport = transform(model.mesh.vertices[i].v_world, world_to_viewport_matrix);
				if (model.mesh.vertices[i].v_viewport.z() != 0)
				{
					model.mesh.vertices[i].v_viewport.x() /= fabs(model.mesh.vertices[i].v_viewport.z());
					model.mesh.vertices[i].v_viewport.y() /= fabs(model.mesh.vertices[i].v_viewport.z());
				}
				viewport.transform(model.mesh.vertices[i]);
			}
		}
	}
	static inline void world_to_viewport(mesh_vertex_t & mv, const viewport_t & viewport)
	{
		matrix44_t world_to_viewport_matrix = viewport.camera().m_projectionmatrix * viewport.camera().m_viewmatrix;
		mv.v_viewport = transform(mv.v_world, world_to_viewport_matrix);
		if (mv.v_viewport.z() != 0)
		{
			mv.v_viewport.x() /= fabs(mv.v_viewport.z());
			mv.v_viewport.y() /= fabs(mv.v_viewport.z());
		}
		viewport.transform(mv);
	}
};

} // namespace
