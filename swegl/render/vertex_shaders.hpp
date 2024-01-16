
#pragma once

#include "swegl/data/model.hpp"
#include "swegl/projection/points.hpp"

namespace swegl
{

class vertex_shader_t
{	
public:
	Matrix4x4 vertice_transform_matrix;

	virtual void shade(std::vector<vertex_t> & vertices,
	                   std::vector<normal_t> & normals,
	                   const model_t & model,
	                   const scene_t & scene,
	                   const camera_t & camera,
	                   const viewport_t & viewport) = 0;
	virtual vertex_t shade_one(vertex_t v) = 0;
};

struct vertex_shader_standard : public vertex_shader_t
{
	const Matrix4x4 * viewportmatrix;
	virtual void shade(std::vector<vertex_t> & vertices,
	                   std::vector<normal_t> & normals,
	                   const model_t & model,
	                   const scene_t & scene,
	                   const camera_t & camera,
	                   const viewport_t & viewport) override
	{
		auto model_matrix = model.orientation;
		model_matrix.Translate(model.position.x(), model.position.y(), model.position.z());
		//auto world_matrix = camera.m_viewmatrix * model_matrix;
		auto world_matrix = camera.m_viewmatrix * model_matrix;
		vertice_transform_matrix = camera.m_projectionmatrix * world_matrix;
		viewportmatrix = &viewport.m_viewportmatrix;

		vertices.clear();
		vertices.reserve(model.mesh.vertices.size());
		for (const vertex_t & v : model.mesh.vertices)
		{
			vertex_t vec = Transform(v, vertice_transform_matrix);
			vec.x() /= fabs(vec.z());
			vec.y() /= fabs(vec.z());
			vertices.emplace_back(Transform(vec, *viewportmatrix));
		}
	}

	virtual vertex_t shade_one(vertex_t v) override
	{
		v = Transform(v, vertice_transform_matrix);
		v.x() /= fabs(v.z());
		v.y() /= fabs(v.z());
		return Transform(v, *viewportmatrix);
	}
};

struct vertex_shader_world : public vertex_shader_t
{
	virtual void shade(std::vector<vertex_t> & vertices,
	                   std::vector<normal_t> & normals,
	                   const model_t & model,
	                   const scene_t & scene,
	                   const camera_t & camera,
	                   const viewport_t & viewport) override
	{
		vertice_transform_matrix = model.orientation;
		vertice_transform_matrix.Translate(model.position.x(), model.position.y(), model.position.z());

		vertices.clear();
		vertices.reserve(model.mesh.vertices.size());
		for (const vertex_t & v : model.mesh.vertices)
		{
			vertex_t vec = Transform(v, vertice_transform_matrix);
			vertices.emplace_back(vec);
		}
 	}

	virtual vertex_t shade_one(vertex_t v) override
	{
		return Transform(v, vertice_transform_matrix);
	}
};

} // namespace
