
#pragma once

#include "swegl/data/model.hpp"
#include "swegl/projection/points.hpp"

#ifndef _GLIBCXX_PARALLEL
#define __gnu_parallel std
#endif

namespace swegl
{

struct new_vertex_shader_t
{
	static inline void original_to_world(new_scene_t & scene, node_t & node, const matrix44_t & vertex_parent_matrix, const matrix44_t & normal_parent_matrix)
	{
		node.vertex_original_to_world_matrix = vertex_parent_matrix * node.get_local_vertex_world_matrix();
		node.normal_original_to_world_matrix = normal_parent_matrix * node.get_local_normal_world_matrix();
		for (auto child_idx : node.children_idx)
			original_to_world(scene, scene.nodes[child_idx], node.vertex_original_to_world_matrix, node.normal_original_to_world_matrix);
	}

	static inline void original_to_world(fraction_t thread_number, new_scene_t & scene)
	{
		for (int node_idx : scene.root_nodes)
			original_to_world(scene, scene.nodes[node_idx], matrix44_t::Identity, matrix44_t::Identity);

		for (auto & v : scene.vertices)
		{
			// TODO use a flag to mark a node's change is world matrix
			// if none, we can skip this transformation

			v.v_world = transform(v.v, scene.nodes[v.node_idx].vertex_original_to_world_matrix);
			v.yes = false;
		}
	}

	static inline void world_to_screen(fraction_t thread_number, new_scene_t & scene, const viewport_t & viewport, bool face_normals, bool vertex_normals)
	{
		// world to camera
		auto & camera_matrix = viewport.camera().m_viewmatrix;
		for (auto & v : scene.vertices)
			v.v_viewport = transform(v.v_world, camera_matrix);

		scene.thread_local_extra_vertices [thread_number.numerator].clear();
		scene.thread_local_extra_triangles[thread_number.numerator].clear();

		for (auto & triangle : scene.triangles)
		{
			// is the triangle fully behind the camera?
			triangle.yes = scene.vertices[triangle.i0].v_viewport.z() > 0.001
			            || scene.vertices[triangle.i1].v_viewport.z() > 0.001
			            || scene.vertices[triangle.i2].v_viewport.z() > 0.001;
			if (triangle.yes)
			{
				scene.vertices[triangle.i0].yes = true;
				scene.vertices[triangle.i1].yes = true;
				scene.vertices[triangle.i2].yes = true;

				// TODO cut up triangles that have 1 or 2 vertices behind the camera
			}
		}

		// camera to frustum
		for (auto & v : scene.vertices)
		{
			if ( ! v.yes)
				continue;
			v.yes = false;

			v.v_viewport = transform(v.v_viewport, viewport.camera().m_projectionmatrix);
			if (v.v_viewport.z() != 0)
			{
				v.v_viewport.x() /= fabs(v.v_viewport.z());
				v.v_viewport.y() /= fabs(v.v_viewport.z());
			}
		}

		// flag YES vertices that have triangles in the frustum
		for (auto & triangle : scene.triangles)
		{
			if ( ! triangle.yes)
				continue;

			// frustum clipping
			if ( (scene.vertices[triangle.i0].v_viewport.x() < -1 && scene.vertices[triangle.i1].v_viewport.x() < -1 && scene.vertices[triangle.i2].v_viewport.x() < -1)
			   ||(scene.vertices[triangle.i0].v_viewport.x() >  1 && scene.vertices[triangle.i1].v_viewport.x() >  1 && scene.vertices[triangle.i2].v_viewport.x() >  1)
			   ||(scene.vertices[triangle.i0].v_viewport.y() < -1 && scene.vertices[triangle.i1].v_viewport.y() < -1 && scene.vertices[triangle.i2].v_viewport.y() < -1)
			   ||(scene.vertices[triangle.i0].v_viewport.y() >  1 && scene.vertices[triangle.i1].v_viewport.y() >  1 && scene.vertices[triangle.i2].v_viewport.y() >  1)
			   )
		    {
		    	triangle.yes = false;
		    	continue;
		    }

		    // backface culling
			bool front_face_visible = (cross((scene.vertices[triangle.i1].v_viewport-scene.vertices[triangle.i0].v_viewport)
			                                ,(scene.vertices[triangle.i2].v_viewport-scene.vertices[triangle.i0].v_viewport)).z() > 0);
			if (front_face_visible)
			{
				triangle.backface = false;
			}
			else
			{
				if (scene.materials[triangle.material_idx].double_sided)
				{
					triangle.backface = true;
				}
				else
				{
			    	triangle.yes = false;
			    	continue;
			    }
			}
			if (face_normals)
				triangle.normal_world = scene.nodes[triangle.node_idx].transform(triangle.normal);
			scene.vertices[triangle.i0].yes = true;
			scene.vertices[triangle.i1].yes = true;
			scene.vertices[triangle.i2].yes = true;
		}
		// TODO extra triangles, too

		// frustum to screen
		for (auto & v : scene.vertices)
		{
			if (v.yes == false)
				continue;
			if (vertex_normals)
				v.normal_world = scene.nodes[v.node_idx].transform(v.normal);
			viewport.transform(v);
		}
		// TODO extra vertices, too
	}
};

} // namespace
