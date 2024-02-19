
#include <cassert>
#include <sstream>

#include <swegl/render/renderer.hpp>
#include <swegl/projection/vec2f.hpp>
#include <swegl/projection/camera.hpp>
#include <swegl/render/pixel_shaders.hpp>
#include <swegl/render/post_shaders.hpp>
#include <swegl/render/interpolator.hpp>
#include <swegl/misc/sync.hpp>

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

void fill_triangle(int y_min, int y_max, const new_triangle_t &, const std::vector<new_mesh_vertex_t> &, viewport_t &, pixel_shader_t &);
void fill_half_triangle(int y, int y_end,
	                    line_side & side_left, line_side & side_right,
                        viewport_t & vp,
                        pixel_shader_t & pixel_shader);


struct transformed_scene_t
{
	new_scene_t * original;
	std::vector<node_t> world_view;
	std::vector<node_t> screen_view;
};

void _render(const fraction_t & thread_number, new_scene_t & scene, viewport_t & viewport)
{
	viewport.clear(thread_number);

	std::unique_ptr<pixel_shader_t> pixel_shader = viewport.m_pixel_shader->clone();
	pixel_shader->prepare_for_scene(viewport, scene);

	new_vertex_shader_t::world_to_screen(thread_number, scene, viewport, pixel_shader->need_face_normals(), pixel_shader->need_vertex_normals());

	int y_min = viewport.m_y + viewport.m_h *  thread_number.numerator    / thread_number.denominator;
	int y_max = viewport.m_y + viewport.m_h * (thread_number.numerator+1) / thread_number.denominator;

	static std::mutex m;
	{
		std::lock_guard l(m);
		scene.extra_vertices .insert(scene.extra_vertices .end(), scene.thread_local_extra_vertices [thread_number.numerator].begin(), scene.thread_local_extra_vertices [thread_number.numerator].end());
		scene.extra_triangles.insert(scene.extra_triangles.end(), scene.thread_local_extra_triangles[thread_number.numerator].begin(), scene.thread_local_extra_triangles[thread_number.numerator].end());
	}

	// wait for all threads to finish up testing triangles
	static sync_point_t sync_point_1;
	sync_point_1.sync(thread_number.denominator);

	//std::stringstream ss;
	//ss << '[' << thread_number.numerator << '/' << thread_number.denominator << "] y_min: " << std::to_string(y_min) << ", y_max: " << y_max;
	//printf("%s\n", ss.str().c_str());

	size_t i;
	for (i=1 ; i<scene.triangles.size() ; i++)
	{
		__builtin_prefetch(scene.vertices.data() + scene.triangles[i].i0);
		__builtin_prefetch(scene.vertices.data() + scene.triangles[i].i1);
		__builtin_prefetch(scene.vertices.data() + scene.triangles[i].i2);
		if(scene.triangles[i-1].yes)
			fill_triangle(y_min, y_max, scene.triangles[i-1], scene.vertices, viewport, *pixel_shader);
	}
	if (scene.triangles.back().yes)
			fill_triangle(y_min, y_max, scene.triangles.back(), scene.vertices, viewport, *pixel_shader);
	//for (auto & triangle : scene.triangles)
	//	if (triangle.yes)
	//		fill_triangle(y_min, y_max, triangle, scene.vertices, viewport, *pixel_shader);
	for (auto & triangle : scene.extra_triangles)
		if (triangle.yes)
			fill_triangle(y_min, y_max, triangle, scene.extra_vertices, viewport, *pixel_shader);

	viewport.flatten(thread_number);
	viewport.m_post_shader->shade(thread_number, viewport);
}


void fill_triangle(int y_min, int y_max, const new_triangle_t & triangle, const std::vector<new_mesh_vertex_t> & vertices, viewport_t & vp, pixel_shader_t & pixel_shader)
{
	const new_mesh_vertex_t * mv0 = &vertices[triangle.i0];
	const new_mesh_vertex_t * mv1 = &vertices[triangle.i1];
	const new_mesh_vertex_t * mv2 = &vertices[triangle.i2];

	// Sort points by screen Y ASC
	if (mv1->v_viewport.y() < mv0->v_viewport.y())
		std::swap(mv0, mv1);
	if (mv2->v_viewport.y() < mv1->v_viewport.y())
		std::swap(mv1, mv2);
	if (mv1->v_viewport.y() < mv0->v_viewport.y())
		std::swap(mv0, mv1);

	if (mv2->v_viewport.y() < y_min)
		return;
	if (mv0->v_viewport.y() >= y_max)
		return;

	const vertex_t * v0 = &mv0->v_viewport;
	const vertex_t * v1 = &mv1->v_viewport;
	const vertex_t * v2 = &mv2->v_viewport;

	// get pixel limits
	int y0 = (int) ceil(v0->y());
	int y1 = (int) ceil(v1->y());
	int y2 = (int) ceil(v2->y());

	if (y0==y2) return; // All on 1 scanline, not worth drawing

	line_side side_long;
	line_side side_short;

	// Init long line
	side_long.ratio = (v2->x()-v0->x()) / (v2->y()-v0->y()); // never div#0 because y0!=y2
	side_long.interpolator.InitSelf(v2->y() - v0->y(), v0->z(), v2->z());
	if (y0 < y_min) {
		// start at first scanline of viewport
		side_long.interpolator.DisplaceStartingPoint(y_min-v0->y());
		side_long.x = v0->x() + side_long.ratio*(y_min-v0->y());
	} else {
		side_long.interpolator.DisplaceStartingPoint(y0-v0->y());
		side_long.x = v0->x() + side_long.ratio*(y0-v0->y());
	}

	int y, y_end; // scanlines upper and lower bound of whole triangle

	pixel_shader.prepare_for_triangle(triangle, mv0, mv1, mv2);

	// upper half of the triangle
	if (y1 >= y_min && y0 != y1) // dont skip: at least some part is in the viewport
	{
		side_short.ratio = (v1->x()-v0->x()) / (v1->y()-v0->y());
		side_short.interpolator.InitSelf(v1->y() - v0->y(), v0->z(), v1->z());
		y = std::max(y0, y_min);
		y_end = std::min(y1, y_max);
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
	if (y1 < y_max && y1 != y2) // dont skip: at least some part is in the viewport
	{
		side_short.ratio = (v2->x()-v1->x()) / (v2->y()-v1->y());
		side_short.interpolator.InitSelf(v2->y() - v1->y(), v1->z(), v2->z());
		y = std::max(y1, y_min);
		y_end = std::min(y2, y_max);
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
	auto it_screen = vp.m_screen.iterator_at_line(y);
	for ( ; y < y_end ; y++,it_screen.next_line())
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
			pixel_colors *video = &*it_screen + x1;
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
					if (vp.m_transparency_layers[layer_idx].zbuffer()[zero_based_offset] == max_z.f
					  ||vp.m_transparency_layers[layer_idx].zbuffer()[zero_based_offset] < z)
						break;
				if (new_color.o.a == 255)
				{
					// solid color, use the base (deepest, backest) layer
					*video = new_color;
					*zb = z;
					if (layer_idx != 0)
					{
						// eliminat transparency layers that were further away
						size_t i,k;
						for (i=0,k=layer_idx ; k<vp.m_transparency_layers.size() ; i++,k++)
						{
							vp.m_transparency_layers[i].zbuffer()[zero_based_offset] = vp.m_transparency_layers[k].zbuffer()[zero_based_offset];
							vp.m_transparency_layers[i].pixels ()[zero_based_offset] = vp.m_transparency_layers[k].pixels ()[zero_based_offset];
						}
						// zero remaining now-unused upper (fronter) transparency layers
						for ( ; i<vp.m_transparency_layers.size() ; i++)
						{
							vp.m_transparency_layers[i].zbuffer()[zero_based_offset] = max_z.f;
							vp.m_transparency_layers[i].pixels ()[zero_based_offset] = {0,0,0,0};
						}
					}
				}
				else
				{
					// transparency color, let's not user the base layer
					// let's insert a transparency layer at layer_idx

					bool all_layers_used = vp.m_transparency_layers.back().zbuffer()[zero_based_offset] != max_z.f;
					if (all_layers_used)
					{
						// shift layers down
						while(layer_idx-->0)
						{
							std::swap(vp.m_transparency_layers[layer_idx].zbuffer()[zero_based_offset],         z);
							std::swap(vp.m_transparency_layers[layer_idx].pixels ()[zero_based_offset], new_color);
						}
					}
					else
					{
						// shift layers up
						for ( ; layer_idx < vp.m_transparency_layers.size() ; layer_idx++)
						{
							std::swap(vp.m_transparency_layers[layer_idx].zbuffer()[zero_based_offset],         z);
							std::swap(vp.m_transparency_layers[layer_idx].pixels ()[zero_based_offset], new_color);
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

} // namespace
