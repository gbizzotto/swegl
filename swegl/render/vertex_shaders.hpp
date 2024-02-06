
#pragma once

#include "swegl/data/model.hpp"
#include "swegl/projection/points.hpp"
#include "swegl/misc/lerp.hpp"

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

	static inline void cut_triangle_if_needed(fraction_t thread_number, new_scene_t & scene, const viewport_t & viewport, new_triangle_t & triangle, bool face_normals, bool vertex_normals)
	{
		auto & camera_matrix = viewport.camera().m_viewmatrix;

		new_mesh_vertex_t * mv0 = &scene.vertices[triangle.i0];
		new_mesh_vertex_t * mv1 = &scene.vertices[triangle.i1];
		new_mesh_vertex_t * mv2 = &scene.vertices[triangle.i2];

		auto & new_vertices  = scene.thread_local_extra_vertices [thread_number.numerator];
		auto & new_triangles = scene.thread_local_extra_triangles[thread_number.numerator];

		bool inverted_order = false;

		// sort by Z DESC
		if (mv1->v_viewport.z() > mv0->v_viewport.z())
		{
			std::swap(mv0, mv1);
			inverted_order = ! inverted_order;
		}
		if (mv2->v_viewport.z() > mv1->v_viewport.z())
		{
			std::swap(mv1, mv2);
			inverted_order = ! inverted_order;
		}
		if (mv1->v_viewport.z() > mv0->v_viewport.z())
		{
			std::swap(mv0, mv1);
			inverted_order = ! inverted_order;
		}

		if (mv0->v_viewport.z() < 0.001)
		{
			// triangle fully back of the camera. no cutting.
			return;
		}
		else if (mv1->v_viewport.z() >= 0.001)
		{
			// only mv2 is behind the camera

			// just copy mv0
			{
				new_mesh_vertex_t & new_vertex = new_vertices.emplace_back();
				new_vertex.v_world    = mv0->v_world;
				new_vertex.v_viewport = mv0->v_viewport;
				new_vertex.tex_coords = mv0->tex_coords;
				if (vertex_normals)
				{
					new_vertex.node_idx = mv0->node_idx;
					new_vertex.normal   = mv0->normal;
				}
			}
			// just copy mv1
			{
				new_mesh_vertex_t & new_vertex = new_vertices.emplace_back();
				new_vertex.v_world    = mv1->v_world;
				new_vertex.v_viewport = mv1->v_viewport;
				new_vertex.tex_coords = mv1->tex_coords;
				if (vertex_normals)
				{
					new_vertex.node_idx = mv1->node_idx;
					new_vertex.normal   = mv1->normal;
				}
			}
			// interpoate mv0-mv2
			{
				new_mesh_vertex_t & new_vertex = new_vertices.emplace_back();
				float cut = (mv0->v_viewport.z()-0.001f) / (mv0->v_viewport.z() - mv2->v_viewport.z());
				new_vertex.v_world    = lerp_unclipped(mv0->v_world   , mv2->v_world   , cut);
				new_vertex.tex_coords = lerp_unclipped(mv0->tex_coords, mv2->tex_coords, cut);
				if (vertex_normals)
				{
					new_vertex.node_idx = mv0->node_idx;
					if (mv0->normal == mv1->normal)
						new_vertex.normal = mv0->normal;
					else
						new_vertex.normal = lerp_unclipped(mv0->normal, mv2->normal, cut);
				}
				new_vertex.v_viewport = transform(new_vertex.v_world, camera_matrix);
				new_vertex.v_viewport = transform(new_vertex.v_viewport, viewport.camera().m_projectionmatrix);
				if (new_vertex.v_viewport.z() != 0)
				{
					new_vertex.v_viewport.x() /= fabs(new_vertex.v_viewport.z());
					new_vertex.v_viewport.y() /= fabs(new_vertex.v_viewport.z());
				}
			}
			// interpoate mv1-mv2
			{
				new_mesh_vertex_t & new_vertex = new_vertices.emplace_back();
				float cut = (mv1->v_viewport.z()-0.001f) / (mv1->v_viewport.z() - mv2->v_viewport.z());
				new_vertex.v_world    = lerp_unclipped(mv1->v_world   , mv2->v_world   , cut);
				new_vertex.tex_coords = lerp_unclipped(mv1->tex_coords, mv2->tex_coords, cut);
				if (vertex_normals)
				{
					new_vertex.node_idx = mv2->node_idx;
					if (mv1->normal == mv2->normal)
						new_vertex.normal = mv1->normal;
					else
						new_vertex.normal = lerp_unclipped(mv1->normal, mv2->normal, cut);
				}
				new_vertex.v_viewport = transform(new_vertex.v_world, camera_matrix);
				new_vertex.v_viewport = transform(new_vertex.v_viewport, viewport.camera().m_projectionmatrix);
				if (new_vertex.v_viewport.z() != 0)
				{
					new_vertex.v_viewport.x() /= fabs(new_vertex.v_viewport.z());
					new_vertex.v_viewport.y() /= fabs(new_vertex.v_viewport.z());
				}
			}
			{
				auto & new_triangle = new_triangles.emplace_back();
				if (inverted_order)
				{
					new_triangle.i0 = new_vertices.size()-4;
					new_triangle.i1 = new_vertices.size()-1;
					new_triangle.i2 = new_vertices.size()-3;
				}
				else
				{
					new_triangle.i0 = new_vertices.size()-4;
					new_triangle.i1 = new_vertices.size()-3;
					new_triangle.i2 = new_vertices.size()-1;
				}
				new_triangle.material_idx = triangle.material_idx;
				if (face_normals)
				{
					new_triangle.node_idx = triangle.node_idx;
					new_triangle.normal   = triangle.normal;
				}
			}
			{
				auto & new_triangle = new_triangles.emplace_back();
				if (inverted_order)
				{
					new_triangle.i0 = new_vertices.size()-4;
					new_triangle.i1 = new_vertices.size()-2;
					new_triangle.i2 = new_vertices.size()-1;
				}
				else
				{
					new_triangle.i0 = new_vertices.size()-4;
					new_triangle.i1 = new_vertices.size()-1;
					new_triangle.i2 = new_vertices.size()-2;
				}
				new_triangle.material_idx = triangle.material_idx;
				if (face_normals)
				{
					new_triangle.node_idx = triangle.node_idx;
					new_triangle.normal   = triangle.normal;
				}
			}
		}
		else
		{
			// mv1 and mv2 are behind the camera

			// just copy mv0
			{
				new_mesh_vertex_t & new_vertex = new_vertices.emplace_back();
				new_vertex.v_world    = mv0->v_world;
				new_vertex.v_viewport = mv0->v_viewport;
				new_vertex.tex_coords = mv0->tex_coords;
				if (vertex_normals)
				{
					new_vertex.node_idx = mv0->node_idx;
					new_vertex.normal = mv0->normal;
				}
			}
			// interpoate mv0-mv1
			{
				new_mesh_vertex_t & new_vertex = new_vertices.emplace_back();
				float cut = (mv0->v_viewport.z()-0.001f) / (mv0->v_viewport.z() - mv1->v_viewport.z());
				new_vertex.v_world    = lerp_unclipped(mv0->v_world   , mv1->v_world   , cut);
				new_vertex.tex_coords = lerp_unclipped(mv0->tex_coords, mv1->tex_coords, cut);
				if (vertex_normals)
				{
					new_vertex.node_idx = mv1->node_idx;
					if (mv0->normal == mv1->normal)
						new_vertex.normal = mv0->normal;
					else
						new_vertex.normal = lerp_unclipped(mv0->normal, mv1->normal, cut);
				}
				new_vertex.v_viewport = transform(new_vertex.v_world, camera_matrix);
				new_vertex.v_viewport = transform(new_vertex.v_viewport, viewport.camera().m_projectionmatrix);
				if (new_vertex.v_viewport.z() != 0)
				{
					new_vertex.v_viewport.x() /= fabs(new_vertex.v_viewport.z());
					new_vertex.v_viewport.y() /= fabs(new_vertex.v_viewport.z());
				}
			}
			// interpoate mv0-mv2
			{
				new_mesh_vertex_t & new_vertex = new_vertices.emplace_back();
				float cut = (mv0->v_viewport.z()-0.001f) / (mv0->v_viewport.z() - mv2->v_viewport.z());
				new_vertex.v_world    = lerp_unclipped(mv0->v_world   , mv2->v_world   , cut);
				new_vertex.tex_coords = lerp_unclipped(mv0->tex_coords, mv2->tex_coords, cut);
				if (vertex_normals)
				{
					new_vertex.node_idx = mv2->node_idx;
					if (mv0->normal == mv2->normal)
						new_vertex.normal = mv0->normal;
					else
						new_vertex.normal = lerp_unclipped(mv0->normal, mv2->normal, cut);
				}
				new_vertex.v_viewport = transform(new_vertex.v_world, camera_matrix);
				new_vertex.v_viewport = transform(new_vertex.v_viewport, viewport.camera().m_projectionmatrix);
				if (new_vertex.v_viewport.z() != 0)
				{
					new_vertex.v_viewport.x() /= fabs(new_vertex.v_viewport.z());
					new_vertex.v_viewport.y() /= fabs(new_vertex.v_viewport.z());
				}
			}

			auto & new_triangle = new_triangles.emplace_back();
			if (inverted_order)
			{
				new_triangle.i0 = new_vertices.size()-3;
				new_triangle.i1 = new_vertices.size()-1;
				new_triangle.i2 = new_vertices.size()-2;
			}
			else
			{
				new_triangle.i0 = new_vertices.size()-3;
				new_triangle.i1 = new_vertices.size()-2;
				new_triangle.i2 = new_vertices.size()-1;
			}
			new_triangle.material_idx = triangle.material_idx;
			if (face_normals)
			{
				new_triangle.node_idx = triangle.node_idx;
				new_triangle.normal   = triangle.normal;
			}
		}
	}

	static inline void world_to_screen(fraction_t thread_number, new_scene_t & scene, const viewport_t & viewport, bool face_normals, bool vertex_normals)
	{
		// world to camera
		auto & camera_matrix = viewport.camera().m_viewmatrix;
		for (auto & v : scene.vertices)
			v.v_viewport = transform(v.v_world, camera_matrix);

		auto & extra_vertices  = scene.thread_local_extra_vertices [thread_number.numerator];
		auto & extra_triangles = scene.thread_local_extra_triangles[thread_number.numerator];

		extra_vertices .clear();
		extra_triangles.clear();

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

			// frustum x and y clipping
			if ( (scene.vertices[triangle.i0].v_viewport.x() <    -1 && scene.vertices[triangle.i1].v_viewport.x() < -1    && scene.vertices[triangle.i2].v_viewport.x() <    -1)
			   ||(scene.vertices[triangle.i0].v_viewport.x() >     1 && scene.vertices[triangle.i1].v_viewport.x() >  1    && scene.vertices[triangle.i2].v_viewport.x() >     1)
			   ||(scene.vertices[triangle.i0].v_viewport.y() <    -1 && scene.vertices[triangle.i1].v_viewport.y() < -1    && scene.vertices[triangle.i2].v_viewport.y() <    -1)
			   ||(scene.vertices[triangle.i0].v_viewport.y() >     1 && scene.vertices[triangle.i1].v_viewport.y() >  1    && scene.vertices[triangle.i2].v_viewport.y() >     1)
			   ||(scene.vertices[triangle.i0].v_viewport.z() < 0.001 && scene.vertices[triangle.i1].v_viewport.z() < 0.001 && scene.vertices[triangle.i2].v_viewport.z() < 0.001)
			   )
		    {
		    	triangle.yes = false;
		    	continue;
		    }

		    // frustum znear clipping / cutting
		    if (   scene.vertices[triangle.i0].v_viewport.z() < 0.001
		    	|| scene.vertices[triangle.i1].v_viewport.z() < 0.001
		    	|| scene.vertices[triangle.i2].v_viewport.z() < 0.001)
		    {
				cut_triangle_if_needed(thread_number, scene, viewport, triangle, face_normals, vertex_normals);
				triangle.yes = false;
				continue;
			}

		    // backface culling
			bool front_face_visible = (cross_2d((scene.vertices[triangle.i1].v_viewport-scene.vertices[triangle.i0].v_viewport)
			                                   ,(scene.vertices[triangle.i2].v_viewport-scene.vertices[triangle.i0].v_viewport)).z() > 0);
			triangle.yes = front_face_visible | scene.materials[triangle.material_idx].double_sided;
			if ( ! triangle.yes)
				continue;
			triangle.backface = ! front_face_visible;

			if (face_normals)
				triangle.normal_world = scene.nodes[triangle.node_idx].transform(triangle.normal);
			scene.vertices[triangle.i0].yes = true;
			scene.vertices[triangle.i1].yes = true;
			scene.vertices[triangle.i2].yes = true;
		}
		// check culling for new temp triangles
		for (auto & triangle : extra_triangles)
		{
		    // backface culling
			bool front_face_visible = (cross_2d((extra_vertices[triangle.i1].v_viewport - extra_vertices[triangle.i0].v_viewport)
			                                   ,(extra_vertices[triangle.i2].v_viewport - extra_vertices[triangle.i0].v_viewport)).z() > 0);
			triangle.backface = ! front_face_visible;
			triangle.yes = front_face_visible | scene.materials[triangle.material_idx].double_sided;
		}

		// frustum to screen
		for (auto & v : scene.vertices)
		{
			if ( ! v.yes)
				continue;
			if (vertex_normals)
				v.normal_world = scene.nodes[v.node_idx].transform(v.normal);
			viewport.transform(v);
		}
		for (auto & v : extra_vertices)
		{
			//if ( ! v.yes)
			//	continue;
			if (vertex_normals)
				v.normal_world = scene.nodes[v.node_idx].transform(v.normal);
			viewport.transform(v);
		}
	}
};

} // namespace
