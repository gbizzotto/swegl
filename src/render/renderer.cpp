
#include <cassert>

#include <swegl/render/renderer.hpp>

#include <swegl/projection/vec2f.hpp>
#include <swegl/projection/camera.hpp>
#include <swegl/render/pixel_shaders.hpp>
#include <swegl/render/post_shaders.hpp>
#include <swegl/render/interpolator.hpp>

namespace swegl
{

union 
{
	int i = 0x7F7F7F7F;
	float f;
} max_z;

struct line_side
{
	interpolator_g<1> interpolator;
	float ratio;
	float x;
};

void crude_line(viewport_t & viewport, int x1, int y1, int x2, int y2);
bool do_triangle(const scene_t & scene, const primitive_t & primitive, vertex_idx i0, vertex_idx i1, vertex_idx i2);
void fill_triangle(vertex_idx i0,
                   vertex_idx i1,
                   vertex_idx i2,
                   node_t & node,
                   primitive_t & primitive,
                   viewport_t & vp,
                   pixel_shader_t & pixel_shader);
void fill_triangle_2(vertex_idx i0,
                     vertex_idx i1,
                     vertex_idx i2,
                     primitive_t & primitive,
                     viewport_t & vp,
                     pixel_shader_t & pixel_shader, 
                     bool front_face_visible);
void fill_half_triangle(int y, int y_end,
	                    line_side & side_left, line_side & side_right,
                        viewport_t & vp,
                        pixel_shader_t & pixel_shader);


struct transformed_scene_t
{
	scene_t * original;
	std::vector<node_t> world_view;
	std::vector<node_t> screen_view;
};


bool inside_camera_frustum(const mesh_vertex_t & v0, const mesh_vertex_t & v1, const mesh_vertex_t & v2)
{
	// frustum clipping
	return  ((v0.v_viewport.x() >= -1   ) || (v1.v_viewport.x() >= -1   ) || (v2.v_viewport.x() >= -1   ))
	      &&((v0.v_viewport.y() >= -1   ) || (v1.v_viewport.y() >= -1   ) || (v2.v_viewport.y() >= -1   ))
	      &&((v0.v_viewport.x()  <  1   ) || (v1.v_viewport.x()  <  1   ) || (v2.v_viewport.x()  <  1   ))
	      &&((v0.v_viewport.y()  <  1   ) || (v1.v_viewport.y()  <  1   ) || (v2.v_viewport.y()  <  1   ))
	      &&((v0.v_viewport.z() >= 0.001) || (v1.v_viewport.z() >= 0.001) || (v2.v_viewport.z() >= 0.001))
	      // object must span at least 1 pixel in width AND in height
	      &&(v0.v_viewport.x() != v1.v_viewport.x() || v0.v_viewport.x() != v2.v_viewport.x())
	      &&(v0.v_viewport.y() != v1.v_viewport.y() || v0.v_viewport.y() != v2.v_viewport.y())
	      ;
}

bool front_face_visible(const mesh_vertex_t & v0, const mesh_vertex_t & v1, const mesh_vertex_t & v2)
{
	return cross((v1.v_viewport-v0.v_viewport),(v2.v_viewport-v0.v_viewport)).z() > 0;
}

void _render(scene_t & scene, viewport_t & viewport)
{
	vertex_shader_t::world_to_camera_or_frustum(scene, viewport);
	viewport.clear();

	pixel_shader_t & pixel_shader = *viewport.m_pixel_shader;
	for (auto & node : scene.nodes)
	{
		// determine which vertices will be part of visible triangles and need more transformation 
		for (auto & primitive : node.primitives)
		{
			auto & vertices = primitive.vertices;
			const auto & indices  = primitive.indices ;
			const bool double_sided = primitive.material_id != -1 && scene.materials[primitive.material_id].double_sided;

			if (double_sided)
			{
				// STRIPS
				if (primitive.mode == primitive_t::index_mode_t::TRIANGLE_STRIP)
					for (unsigned int i=2 ; i<indices.size() ; i++)
					{
						assert(indices[i-2] < vertices.size());
						assert(indices[i-1] < vertices.size());
						assert(indices[i-0] < vertices.size());
						if (inside_camera_frustum(vertices[indices[i-2]], vertices[indices[i-1]], vertices[indices[i]]))
						{
							vertices[indices[i-2]].yes = true;
							vertices[indices[i-1]].yes = true;
							vertices[indices[i  ]].yes = true;
						}
					}
				// FANS
				if (primitive.mode == primitive_t::index_mode_t::TRIANGLE_FAN)
					for (unsigned int i=2 ; i<indices.size() ; i++)
					{
						assert(indices[i-2] < vertices.size());
						assert(indices[i-1] < vertices.size());
						assert(indices[i-0] < vertices.size());
						if (inside_camera_frustum(vertices[indices[0]], vertices[indices[i-1]], vertices[indices[i]]))
						{
							vertices[indices[0  ]].yes = true;
							vertices[indices[i-1]].yes = true;
							vertices[indices[i  ]].yes = true;
						}
					}
				// TRIs
				if (primitive.mode == primitive_t::index_mode_t::TRIANGLES)
					for (unsigned int i=2 ; i<indices.size() ; i+= 3)
					{
						assert(indices[i-2] < vertices.size());
						assert(indices[i-1] < vertices.size());
						assert(indices[i-0] < vertices.size());
						if (inside_camera_frustum(vertices[indices[i-2]], vertices[indices[i-1]], vertices[indices[i]]))
						{
							vertices[indices[i-2]].yes = true;
							vertices[indices[i-1]].yes = true;
							vertices[indices[i  ]].yes = true;
						}
					}
			}
			else
			{
				// STRIPS
				if (primitive.mode == primitive_t::index_mode_t::TRIANGLE_STRIP)
					for (unsigned int i=2 ; i<indices.size() ; i++)
					{
						assert(indices[i-2] < vertices.size());
						assert(indices[i-1] < vertices.size());
						assert(indices[i-0] < vertices.size());
						if (   inside_camera_frustum(vertices[indices[i-2]], vertices[indices[i-1        ]], vertices[indices[i        ]])
							&& front_face_visible   (vertices[indices[i-2]], vertices[indices[i-1+(i&0x1)]], vertices[indices[i-(i&0x1)]]))
						{
							vertices[indices[i-2]].yes = true;
							vertices[indices[i-1]].yes = true;
							vertices[indices[i  ]].yes = true;
						}
					}
				// FANS
				if (primitive.mode == primitive_t::index_mode_t::TRIANGLE_FAN)
					for (unsigned int i=2 ; i<indices.size() ; i++)
					{
						assert(indices[i-2] < vertices.size());
						assert(indices[i-1] < vertices.size());
						assert(indices[i-0] < vertices.size());
						if (   inside_camera_frustum(vertices[indices[0]], vertices[indices[i-1]], vertices[indices[i]])
						    && front_face_visible   (vertices[indices[0]], vertices[indices[i-1]], vertices[indices[i]]))
						{
							vertices[indices[0  ]].yes = true;
							vertices[indices[i-1]].yes = true;
							vertices[indices[i  ]].yes = true;
						}
					}
				// TRIs
				if (primitive.mode == primitive_t::index_mode_t::TRIANGLES)
					for (unsigned int i=2 ; i<indices.size() ; i+= 3)
					{
						assert(indices[i-2] < vertices.size());
						assert(indices[i-1] < vertices.size());
						assert(indices[i-0] < vertices.size());
						if (   inside_camera_frustum(vertices[indices[i-2]], vertices[indices[i-1]], vertices[indices[i]])
						    && front_face_visible   (vertices[indices[i-2]], vertices[indices[i-1]], vertices[indices[i]]))
						{
							vertices[indices[i-2]].yes = true;
							vertices[indices[i-1]].yes = true;
							vertices[indices[i  ]].yes = true;
						}
					}
			}
		}

		// do the rest of the transformations to the vertices that are part of visible triangles
		vertex_shader_t::frustum_to_viewport(node, viewport);

		// do the painting
		for (auto & primitive : node.primitives)
		{
			auto & indices = primitive.indices;

			pixel_shader.prepare_for_primitive(primitive, scene, viewport);

			// STRIPS
			if (primitive.mode == primitive_t::index_mode_t::TRIANGLE_STRIP)
				for (unsigned int i=2 ; i<indices.size() ; i++)
					fill_triangle(indices[i-2]
						         ,indices[i-1+(i&0x1)]
						         ,indices[i  -(i&0x1)]
						         ,node
						         ,primitive
						         ,viewport
						         ,pixel_shader
						         );
			// FANSv
			if (primitive.mode == primitive_t::index_mode_t::TRIANGLE_FAN)
				for (unsigned int i=2 ; i<indices.size() ; i++)
					fill_triangle(indices[0  ]
					             ,indices[i-1]
					             ,indices[i  ]
					             ,node
					             ,primitive
					             ,viewport
					             ,pixel_shader
					             );
			// TRIs
			if (primitive.mode == primitive_t::index_mode_t::TRIANGLES)
				for (unsigned int i=2 ; i<indices.size() ; i+= 3)
					fill_triangle(indices[i-2]
					             ,indices[i-1]
					             ,indices[i  ]
					             ,node
					             ,primitive
					             ,viewport
					             ,pixel_shader
					             );
		}
	}

	viewport.flatten();
	viewport.m_post_shader->shade(viewport);
}




void fill_triangle(vertex_idx i0,
                   vertex_idx i1,
                   vertex_idx i2,
                   node_t & node,
                   primitive_t & primitive,
                   viewport_t & vp,
                   pixel_shader_t & pixel_shader)
{
	if (  !primitive.vertices[i0].yes
	    ||!primitive.vertices[i1].yes
	    ||!primitive.vertices[i2].yes )
	{
	    return;
	}
	const vertex_t * v0 = &primitive.vertices[i0].v_viewport;
	const vertex_t * v1 = &primitive.vertices[i1].v_viewport;
	const vertex_t * v2 = &primitive.vertices[i2].v_viewport;

	bool front_face_visible = cross((*v1-*v0),(*v2-*v0)).z() < 0;

	// handle cases where 1 or 2 vertices have screen z values below zero (behind the camera)

	bool inverted_order = false;

	// sort by Z DESC
	if (v1->z() > v0->z()) {
		std::swap(v0, v1);
		std::swap(i0, i1);
		inverted_order = ! inverted_order;
	}
	if (v2->z() > v1->z()) {
		std::swap(v1, v2);
		std::swap(i1, i2);
		inverted_order = ! inverted_order;
	}
	if (v1->z() > v0->z()) {
		std::swap(v0, v1);
		std::swap(i0, i1);
		inverted_order = ! inverted_order;
	}

	if (v2->z() >= 0.001)
	{
		// normal case
		fill_triangle_2(i0, i1, i2, primitive, vp, pixel_shader, front_face_visible);
	}
	else if (v1->z() < 0.001)
	{
		// only v0 in front of the camera
		float cut_1 = (v0->z()-0.001f) / (v0->z() - v1->z());
		mesh_vertex_t & new_vertex_1 = primitive.vertices.emplace_back();
		new_vertex_1.v_world    = primitive.vertices[i0].v_world      + (primitive.vertices[i1].v_world     -primitive.vertices[i0].v_world     )*cut_1;
		new_vertex_1.tex_coords = primitive.vertices[i0].tex_coords   + (primitive.vertices[i1].tex_coords  -primitive.vertices[i0].tex_coords  )*cut_1;
		if (primitive.vertices[i1].normal == primitive.vertices[i0].normal)
			new_vertex_1.normal = primitive.vertices[i0].normal;
		else
			new_vertex_1.normal = primitive.vertices[i0].normal       + (primitive.vertices[i1].normal      -primitive.vertices[i0].normal      )*cut_1;
		vertex_shader_t::world_to_viewport(new_vertex_1, node, vp);

		float cut_2 = (v0->z()-0.001f) / (v0->z() - v2->z());
		mesh_vertex_t & new_vertex_2 = primitive.vertices.emplace_back();
		new_vertex_2.v_world    = primitive.vertices[i0].v_world      + (primitive.vertices[i2].v_world     -primitive.vertices[i0].v_world     )*cut_2;
		new_vertex_2.tex_coords = primitive.vertices[i0].tex_coords   + (primitive.vertices[i2].tex_coords  -primitive.vertices[i0].tex_coords  )*cut_2;
		if (primitive.vertices[i2].normal == primitive.vertices[i0].normal)
			new_vertex_2.normal = primitive.vertices[i0].normal;
		else
			new_vertex_2.normal = primitive.vertices[i0].normal       + (primitive.vertices[i2].normal      -primitive.vertices[i0].normal      )*cut_2;
		vertex_shader_t::world_to_viewport(new_vertex_2, node, vp);

		front_face_visible = cross((new_vertex_1.v_viewport-*v0),(new_vertex_2.v_viewport-*v0)).z() < 0;
		if (inverted_order)
			front_face_visible = ! front_face_visible;

		fill_triangle_2(i0, primitive.vertices.size()-2, primitive.vertices.size()-1, primitive, vp, pixel_shader, front_face_visible);
		primitive.vertices.pop_back();
		primitive.vertices.pop_back();
		return;
	}
	else if (v2->z() < 0.001)
	{
		// only v2 is in the back of the camera
		float cut_0 = (v0->z()-0.001f) / (v0->z() - v2->z());
		mesh_vertex_t & new_vertex_1 = primitive.vertices.emplace_back();
		new_vertex_1.v_world    = primitive.vertices[i0].v_world      + (primitive.vertices[i2].v_world     -primitive.vertices[i0].v_world     )*cut_0;
		new_vertex_1.tex_coords = primitive.vertices[i0].tex_coords   + (primitive.vertices[i2].tex_coords  -primitive.vertices[i0].tex_coords  )*cut_0;
		if (primitive.vertices[i2].normal == primitive.vertices[i0].normal)
			new_vertex_1.normal = primitive.vertices[i0].normal;
		else
			new_vertex_1.normal = primitive.vertices[i0].normal       + (primitive.vertices[i2].normal      -primitive.vertices[i0].normal      )*cut_0;
		vertex_shader_t::world_to_viewport(new_vertex_1, node, vp);

		float cut_1 = (v1->z()-0.001f) / (v1->z() - v2->z());
		mesh_vertex_t & new_vertex_2 = primitive.vertices.emplace_back();
		new_vertex_2.v_world    = primitive.vertices[i1].v_world      + (primitive.vertices[i2].v_world     -primitive.vertices[i1].v_world     )*cut_1;
		new_vertex_2.tex_coords = primitive.vertices[i1].tex_coords   + (primitive.vertices[i2].tex_coords  -primitive.vertices[i1].tex_coords  )*cut_1;
		if (primitive.vertices[i2].normal == primitive.vertices[i1].normal)
			new_vertex_2.normal = primitive.vertices[i1].normal;
		else
			new_vertex_2.normal = primitive.vertices[i1].normal       + (primitive.vertices[i2].normal      -primitive.vertices[i1].normal      )*cut_1;
		vertex_shader_t::world_to_viewport(new_vertex_2, node, vp);

		front_face_visible = cross((*v1-*v0),(new_vertex_2.v_viewport-*v0)).z() < 0;
		if (inverted_order)
			front_face_visible = ! front_face_visible;

		fill_triangle_2(i0, i1, primitive.vertices.size()-1, primitive, vp, pixel_shader, front_face_visible);

		front_face_visible = cross((new_vertex_2.v_viewport-*v0),(new_vertex_1.v_viewport-*v0)).z() < 0;
		if (inverted_order)
			front_face_visible = ! front_face_visible;

		fill_triangle_2(i0, primitive.vertices.size()-1, primitive.vertices.size()-2, primitive, vp, pixel_shader, front_face_visible);

		primitive.vertices.pop_back();
		primitive.vertices.pop_back();
		return;
	}

	
}

void fill_triangle_2([[maybe_unused]] vertex_idx i0,
                     [[maybe_unused]] vertex_idx i1,
                     [[maybe_unused]] vertex_idx i2,
                     [[maybe_unused]] primitive_t & primitive,
                     [[maybe_unused]] viewport_t & vp,
                     [[maybe_unused]] pixel_shader_t & pixel_shader, 
                     bool front_face_visible)
{
	const vertex_t * v0 = &primitive.vertices[i0].v_viewport;
	const vertex_t * v1 = &primitive.vertices[i1].v_viewport;
	const vertex_t * v2 = &primitive.vertices[i2].v_viewport;

	bool inverted = !front_face_visible;

	// Sort points by screen Y ASC
	if (v1->y() < v0->y()) {
		std::swap(v0, v1);
		std::swap(i0, i1);
	}
	if (v2->y() < v1->y()) {
		std::swap(v1, v2);
		std::swap(i1, i2);
	}
	if (v1->y() < v0->y()) {
		std::swap(v0, v1);
		std::swap(i0, i1);
	}

	// get pixel limits
	int y0 = (int) ceil(v0->y());
	int y1 = (int) ceil(v1->y());
	int y2 = (int) ceil(v2->y());

	if (y0==y2) return; // All on 1 scanline, not worth drawing

	line_side side_long;
	line_side side_short;

	// Init long line
	side_long.ratio = (v2->x()-v0->x()) / (v2->y()-v0->y()); // never div#0 because y0!=y2
	side_long.interpolator.InitSelf(v2->y() - v0->y(), v0->z(), v2->z()); // *t0, *t2
	if (y0 < vp.m_y) {
		// start at first scanline of viewport
		side_long.interpolator.DisplaceStartingPoint(vp.m_y-v0->y());
		side_long.x = v0->x() + side_long.ratio*(vp.m_y-v0->y());
	} else {
		side_long.interpolator.DisplaceStartingPoint(y0-v0->y());
		side_long.x = v0->x() + side_long.ratio*(y0-v0->y());
	}

	int y, y_end; // scanlines upper and lower bound of whole triangle

	pixel_shader.prepare_for_triangle(i0, i1, i2, inverted);

	// upper half of the triangle
	if (y1 >= vp.m_y) // dont skip: at least some part is in the viewport
	{
		side_short.ratio = (v1->x()-v0->x()) / (v1->y()-v0->y()); // never div#0 because y0!=y2
		side_short.interpolator.InitSelf(v1->y() - v0->y(), v0->z(), v1->z()); // *t0, *t1
		y = std::max(y0, vp.m_y);
		y_end = std::min(y1, vp.m_y + vp.m_h);
		side_short.interpolator.DisplaceStartingPoint(y-v0->y());
		side_short.x = v0->x() + side_short.ratio*(y-v0->y());

		bool long_line_on_right = side_long.ratio > side_short.ratio;
		auto [side_left, side_right] = [&]() -> std::tuple<line_side&,line_side&> {
				if (long_line_on_right)
					return {side_short, side_long};
				else
					return {side_long, side_short};
			}();

		pixel_shader.prepare_for_upper_triangle(long_line_on_right);

		fill_half_triangle(y, y_end, side_left, side_right, vp, pixel_shader);
	}

	// lower half of the triangle
	if (y1 < vp.m_y + vp.m_h) // dont skip: at least some part is in the viewport
	{
		side_short.ratio = (v2->x()-v1->x()) / (v2->y()-v1->y()); // never div#0 because y0!=y2
		side_short.interpolator.InitSelf(v2->y() - v1->y(), v1->z(), v2->z()); // *t1, *t2
		y = std::max(y1, vp.m_y);
		y_end = std::min(y2, vp.m_y + vp.m_h);
		side_short.interpolator.DisplaceStartingPoint(y-v1->y());
		side_short.x = v1->x() + side_short.ratio*(y-v1->y());

		bool long_line_on_right = side_long.ratio < side_short.ratio;
		auto [side_left, side_right] = [&]() -> std::tuple<line_side&,line_side&> {
				if (long_line_on_right)
					return {side_short, side_long};
				else
					return {side_long, side_short};
			}();

		pixel_shader.prepare_for_lower_triangle(long_line_on_right);

		fill_half_triangle(y, y_end, side_left, side_right, vp, pixel_shader);
	}
}

void fill_half_triangle(int y, int y_end,
	                    line_side & side_left, line_side & side_right,
                        viewport_t & vp,
                        pixel_shader_t & pixel_shader)
{
	for ( ; y < y_end ; y++)
	{
		int x1 = std::max((int)ceil(side_left .x), vp.m_x);
		int x2 = std::min((int)ceil(side_right.x), vp.m_x+vp.m_w);

		if (x1 < x2)
		{
			pixel_shader.prepare_for_scanline(side_left .interpolator.progress()
			                                 ,side_right.interpolator.progress());
			interpolator_g<1> qpixel;
			qpixel.InitSelf(side_right.x - side_left.x,
			            side_left .interpolator.value(0),
			            side_right.interpolator.value(0));
			qpixel.DisplaceStartingPoint(x1 - side_left.x);

			// fill_line
			pixel_colors *video = &((pixel_colors*)vp.m_screen->pixels)[(int) ( y*vp.m_screen->pitch/vp.m_screen->format->BytesPerPixel + x1)];
			int zero_based_offset = (int) ( (y-vp.m_y)*vp.m_w + (x1-vp.m_x));
			float * zb = &vp.zbuffer()[zero_based_offset];
			for ( ; x1 < x2 ; x1++,video++,zb++,zero_based_offset++,qpixel.Step() )
			{
				float z = qpixel.value(0);
				if (z <= 0.001) // Ugly z-near clipping
					continue;
				if (z >= *zb)
					continue;
				pixel_colors new_color = pixel_shader.shade(qpixel.progress());
				if (vp.m_got_transparency == false)
				{
					*video = new_color;
					*zb = z;
					continue;
				}
				size_t layer_idx = 0;
				for (layer_idx=0 ; layer_idx<vp.m_transparency_layers.size() ; layer_idx++)
					if (vp.m_transparency_layers[layer_idx].m_zbuffer[zero_based_offset] == max_z.f
					  ||vp.m_transparency_layers[layer_idx].m_zbuffer[zero_based_offset] < z)
						break;
				if (new_color.o.a == 255)
				{
					// solid color, use the base (deepest, backest) layer
					*video = new_color;
					*zb = z;
					// eliminat transparency layers that were further away
					size_t i,k;
					for (i=0,k=layer_idx ; k<vp.m_transparency_layers.size() ; i++,k++)
					{
						vp.m_transparency_layers[i].m_zbuffer[zero_based_offset] = vp.m_transparency_layers[k].m_zbuffer[zero_based_offset];
						vp.m_transparency_layers[i].m_colors [zero_based_offset] = vp.m_transparency_layers[k].m_colors [zero_based_offset];
					}
					// zero remaining now-unused upper (fronter) transparency layers
					for ( ; i<vp.m_transparency_layers.size() ; i++)
					{
						vp.m_transparency_layers[i].m_zbuffer[zero_based_offset] = max_z.f;
						vp.m_transparency_layers[i].m_colors [zero_based_offset] = {0,0,0,0};
					}
				}
				else
				{
					// transparency color, let's not user the base layer
					// let's insert a transparency layer at layer_idx

					bool all_layers_used = vp.m_transparency_layers.back().m_zbuffer[zero_based_offset] != max_z.f;
					if (all_layers_used)
					{
						// shift layers down
						while(layer_idx-->0)
						{
							std::swap(vp.m_transparency_layers[layer_idx].m_zbuffer[zero_based_offset],         z);
							std::swap(vp.m_transparency_layers[layer_idx].m_colors [zero_based_offset], new_color);
						}
					}
					else
					{
						// shift layers up
						for ( ; layer_idx < vp.m_transparency_layers.size() ; layer_idx++)
						{
							std::swap(vp.m_transparency_layers[layer_idx].m_zbuffer[zero_based_offset],         z);
							std::swap(vp.m_transparency_layers[layer_idx].m_colors [zero_based_offset], new_color);
							if (z == max_z.f)
								break; // we've reached the last used layer
						}
					}
				}
			}
		}
		side_left .x += side_left .ratio;
		side_right.x += side_right.ratio;
		side_left .interpolator.Step();
		side_right.interpolator.Step();
	}
}

void crude_line(viewport_t & vp, int x1, int y1, int x2, int y2)
{
	if ((x2-x1) == 0)
		return;
	if (x2<x1)
		return crude_line(vp, x2, y2, x1, y1);
	for (int x=x1 ; x<=x2 ; x++)
	{
		int y = y1 + (y2-y1)*(x-x1)/(x2-x1);
		unsigned int *video = &((unsigned int*)vp.m_screen->pixels)[(int) ( y*vp.m_screen->pitch/4 + x)];
		if (x<0 || y<0 || x>vp.m_screen->pitch/4 || y>479)
			continue;
		*video = 0xFFFF0000;
	}
}

} // namespace
