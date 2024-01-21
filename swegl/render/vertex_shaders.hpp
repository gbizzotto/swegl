
#pragma once

#include "swegl/data/model.hpp"
#include "swegl/projection/points.hpp"

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
			for (size_t i=0 ; i<model.mesh.vertices.size() ; i++)
			{
				model.mesh.vertices_world[i].v       = transform(model.mesh.vertices[i].v     , original_to_world_matrix);
				model.mesh.vertices_world[i].normal  = transform(model.mesh.vertices[i].normal, model.orientation       );
			}
		}
	}
	static inline void world_to_viewport(scene_t & scene, const viewport_t & viewport)
	{
		matrix44_t world_to_viewport_matrix = viewport.camera().m_projectionmatrix * viewport.camera().m_viewmatrix;
		for (auto & model : scene.models)
		{
			for (size_t i=0 ; i<model.mesh.vertices.size() ; i++)
			{
				model.mesh.vertices_viewport[i].v = transform(model.mesh.vertices_world[i].v, world_to_viewport_matrix);
				if (model.mesh.vertices_viewport[i].v.z() != 0)
				{
					model.mesh.vertices_viewport[i].v.x() /= fabs(model.mesh.vertices_viewport[i].v.z());
					model.mesh.vertices_viewport[i].v.y() /= fabs(model.mesh.vertices_viewport[i].v.z());
				}
				model.mesh.vertices_viewport[i].v = transform(model.mesh.vertices_viewport[i].v, viewport.m_viewportmatrix);
			}
		}
	}
	static inline vertex_t world_to_viewport(const vertex_t & v, const viewport_t & viewport)
	{
		matrix44_t world_to_viewport_matrix = viewport.camera().m_projectionmatrix * viewport.camera().m_viewmatrix;
		auto result = transform(v, world_to_viewport_matrix);
		if (result.z() != 0)
		{
			result.x() /= fabs(result.z());
			result.y() /= fabs(result.z());
		}
		return transform(result, viewport.m_viewportmatrix);
	}
};

} // namespace
