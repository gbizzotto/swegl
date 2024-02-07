
#include <swegl/render/vertex_shaders.hpp>
#include <swegl/misc/sync.hpp>

namespace swegl
{

	void new_vertex_shader_t::original_to_world(new_scene_t & scene, node_t & node, const matrix44_t & vertex_parent_matrix, const matrix44_t & normal_parent_matrix)
	{
		node.vertex_original_to_world_matrix = vertex_parent_matrix * node.get_local_vertex_world_matrix();
		node.normal_original_to_world_matrix = normal_parent_matrix * node.get_local_normal_world_matrix();
		for (auto child_idx : node.children_idx)
			original_to_world(scene, scene.nodes[child_idx], node.vertex_original_to_world_matrix, node.normal_original_to_world_matrix);
	}

	void new_vertex_shader_t::original_to_world(const fraction_t & thread_number, new_scene_t & scene)
	{
		for (size_t i=thread_number.numerator*scene.vertices.size()/thread_number.denominator ; i<(thread_number.numerator+1)*scene.vertices.size()/thread_number.denominator ; i++)
		{
			auto & v = scene.vertices[i];
			v.v_world = transform(v.v, scene.nodes[v.node_idx].vertex_original_to_world_matrix);
		}
	}

	void new_vertex_shader_t::cut_triangle_if_needed(const fraction_t & thread_number, new_scene_t & scene, const viewport_t & viewport, const new_triangle_t & triangle, bool face_normals, bool vertex_normals, const matrix44_t & matrix)
	{
		const new_mesh_vertex_t * mv0 = &scene.vertices[triangle.i0];
		const new_mesh_vertex_t * mv1 = &scene.vertices[triangle.i1];
		const new_mesh_vertex_t * mv2 = &scene.vertices[triangle.i2];

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
					new_vertex.node_idx     = mv0->node_idx;
					new_vertex.normal_world = mv0->normal_world;
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
					new_vertex.node_idx     = mv1->node_idx;
					new_vertex.normal_world = mv1->normal_world;
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
						new_vertex.normal_world = mv0->normal_world;
					else
						new_vertex.normal_world = lerp_unclipped(mv0->normal_world, mv2->normal_world, cut);
				}
				new_vertex.v_viewport = transform(new_vertex.v_world, matrix);
				new_vertex.v_viewport.x() /= fabs(new_vertex.v_viewport.z());
				new_vertex.v_viewport.y() /= fabs(new_vertex.v_viewport.z());
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
						new_vertex.normal_world = mv1->normal_world;
					else
						new_vertex.normal_world = lerp_unclipped(mv1->normal_world, mv2->normal_world, cut);
				}
				new_vertex.v_viewport = transform(new_vertex.v_world, matrix);
				new_vertex.v_viewport.x() /= fabs(new_vertex.v_viewport.z());
				new_vertex.v_viewport.y() /= fabs(new_vertex.v_viewport.z());
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
					new_triangle.node_idx = triangle.node_idx;
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
					new_triangle.node_idx = triangle.node_idx;
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
					new_vertex.node_idx     = mv0->node_idx;
					new_vertex.normal_world = mv0->normal_world;
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
						new_vertex.normal_world = mv0->normal_world;
					else
						new_vertex.normal_world = lerp_unclipped(mv0->normal_world, mv1->normal_world, cut);
				}
				new_vertex.v_viewport = transform(new_vertex.v_world, matrix);
				new_vertex.v_viewport.x() /= fabs(new_vertex.v_viewport.z());
				new_vertex.v_viewport.y() /= fabs(new_vertex.v_viewport.z());
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
						new_vertex.normal_world = mv0->normal_world;
					else
						new_vertex.normal_world = lerp_unclipped(mv0->normal_world, mv2->normal_world, cut);
				}
				new_vertex.v_viewport = transform(new_vertex.v_world, matrix);
				new_vertex.v_viewport.x() /= fabs(new_vertex.v_viewport.z());
				new_vertex.v_viewport.y() /= fabs(new_vertex.v_viewport.z());
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
				new_triangle.node_idx = triangle.node_idx;
		}
	}

	void new_vertex_shader_t::world_to_screen(const fraction_t & thread_number, new_scene_t & scene, const viewport_t & viewport, bool face_normals, bool vertex_normals)
	{
		auto & extra_vertices  = scene.thread_local_extra_vertices [thread_number.numerator];
		auto & extra_triangles = scene.thread_local_extra_triangles[thread_number.numerator];
		extra_vertices .clear();
		extra_triangles.clear();

		// world to camera
		auto matrix = viewport.m_viewportmatrix * viewport.camera().m_projectionmatrix * viewport.camera().m_viewmatrix;

		// transform vertices
		for (size_t i = scene.vertices.size()* thread_number.numerator   /thread_number.denominator
			;       i < scene.vertices.size()*(thread_number.numerator+1)/thread_number.denominator
			;i++)
		{
			auto & v = scene.vertices[i];
			v.v_viewport = transform(v.v_world, matrix);
			v.v_viewport.x() /= fabs(v.v_viewport.z());
			v.v_viewport.y() /= fabs(v.v_viewport.z());
			if (vertex_normals)
				v.normal_world = scene.nodes[v.node_idx].transform(v.normal);
		}

		// wait for all threads to finish up transforming vertices
		static sync_point_t sync_point_1;
		sync_point_1.sync(thread_number.denominator);

		// check triangle visibility
		for (size_t i = scene.triangles.size()* thread_number.numerator   /thread_number.denominator
			;       i < scene.triangles.size()*(thread_number.numerator+1)/thread_number.denominator
			; i++)
		{
			auto & triangle = scene.triangles[i];
			// frustum clipping
			if ( (scene.vertices[triangle.i0].v_viewport.x() < viewport.m_x              && scene.vertices[triangle.i1].v_viewport.x() < viewport.m_x              && scene.vertices[triangle.i2].v_viewport.x() < viewport.m_x              )
			   ||(scene.vertices[triangle.i0].v_viewport.x() > viewport.m_x+viewport.m_w && scene.vertices[triangle.i1].v_viewport.x() > viewport.m_x+viewport.m_w && scene.vertices[triangle.i2].v_viewport.x() > viewport.m_x+viewport.m_w )
			   ||(scene.vertices[triangle.i0].v_viewport.y() < viewport.m_y              && scene.vertices[triangle.i1].v_viewport.y() < viewport.m_y              && scene.vertices[triangle.i2].v_viewport.y() < viewport.m_y              )
			   ||(scene.vertices[triangle.i0].v_viewport.y() > viewport.m_y+viewport.m_h && scene.vertices[triangle.i1].v_viewport.y() > viewport.m_y+viewport.m_h && scene.vertices[triangle.i2].v_viewport.y() > viewport.m_y+viewport.m_h )
			   ||(scene.vertices[triangle.i0].v_viewport.z() < 0.001                     && scene.vertices[triangle.i1].v_viewport.z() < 0.001                     && scene.vertices[triangle.i2].v_viewport.z() < 0.001                     )
			   )
		    {
		    	triangle.yes = false;
		    	continue;
		    }

		    if (scene.vertices[triangle.i0].v_viewport.z() < 0.001 || scene.vertices[triangle.i1].v_viewport.z() < 0.001 || scene.vertices[triangle.i2].v_viewport.z() < 0.001)
		    {
				cut_triangle_if_needed(thread_number, scene, viewport, triangle, face_normals, vertex_normals, matrix);
				triangle.yes = false;
				continue;
			}

		    // backface culling
			bool front_face_visible = (cross_2d((scene.vertices[triangle.i1].v_viewport-scene.vertices[triangle.i0].v_viewport)
			                                   ,(scene.vertices[triangle.i2].v_viewport-scene.vertices[triangle.i0].v_viewport)).z() < 0);
			triangle.yes = front_face_visible | scene.materials[triangle.material_idx].double_sided;
			if ( ! triangle.yes)
				continue;
			triangle.backface = ! front_face_visible;
			if (face_normals)
				triangle.normal_world = scene.nodes[triangle.node_idx].transform(triangle.normal);
		}
		for (auto & triangle : extra_triangles)
		{
		    // backface culling
			bool front_face_visible = (cross_2d((extra_vertices[triangle.i1].v_viewport - extra_vertices[triangle.i0].v_viewport)
			                                   ,(extra_vertices[triangle.i2].v_viewport - extra_vertices[triangle.i0].v_viewport)).z() < 0);
			triangle.yes = front_face_visible | scene.materials[triangle.material_idx].double_sided;
			if ( ! triangle.yes)
				continue;
			triangle.backface = ! front_face_visible;
			if (face_normals)
				triangle.normal_world = scene.nodes[triangle.node_idx].transform(triangle.normal);
		}
	}

} // namespace
