
#pragma once

#include "swegl/Data/model.hpp"
#include "swegl/Projection/points.hpp"

namespace swegl
{

class vertex_shader_t
{
public:
	virtual std::vector<vertex_t> shade(std::vector<vertex_t> & vertices,
	                                    std::vector<normal_t> & normals,
	                                    const model_t & model,
	                                    const scene_t & scene,
	                                    const Camera & camera,
	                                    const ViewPort & viewport) = 0;
};

struct vertex_shader_standard : public vertex_shader_t
{
	virtual std::vector<vertex_t> shade(std::vector<vertex_t> & vertices,
	                                    std::vector<normal_t> & normals,
	                                    const model_t & model,
	                                    const scene_t & scene,
	                                    const Camera & camera,
	                                    const ViewPort & viewport) override
	{
		auto model_matrix = model.orientation;
		model_matrix.Translate(model.position.x(), model.position.y(), model.position.z());
		auto world_matrix = camera.m_viewmatrix * model_matrix;
		auto vertice_transform_matrix = camera.m_projectionmatrix * world_matrix;

		vertices.clear();
		vertices.reserve(model.mesh.vertices.size());
		for (const vertex_t & v : model.mesh.vertices)
		{
			vertex_t vec = Transform(v, vertice_transform_matrix);
			vec.x() /= fabs(vec.z());
			vec.y() /= fabs(vec.z());
			vertices.emplace_back(Transform(vec, viewport.m_viewportmatrix));
		}

		return vertices;
	}
};

} // namespace
