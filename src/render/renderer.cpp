
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
void fill_triangle(new_triangle_t &, new_scene_t &, viewport_t &);
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

void _render(fraction_t thread_number, new_scene_t & scene, viewport_t & viewport)
{
	viewport.clear(thread_number);

	new_vertex_shader_t::world_to_screen(thread_number ,scene, viewport, viewport.m_pixel_shader->need_face_normals(), viewport.m_pixel_shader->need_vertex_normals());
	viewport.m_pixel_shader->prepare_for_scene(viewport, scene);

	for (auto & triangle : scene.triangles)
		if (triangle.yes)
			fill_triangle(triangle, scene, viewport);

	viewport.flatten();
	viewport.m_post_shader->shade(viewport);
}


void fill_triangle(new_triangle_t & triangle, new_scene_t & scene, viewport_t & vp)
{
	const new_mesh_vertex_t * mv0 = &scene.vertices[triangle.i0];
	const new_mesh_vertex_t * mv1 = &scene.vertices[triangle.i1];
	const new_mesh_vertex_t * mv2 = &scene.vertices[triangle.i2];

	// Sort points by screen Y ASC
	if (mv1->v_viewport.y() < mv0->v_viewport.y())
		std::swap(mv0, mv1);
	if (mv2->v_viewport.y() < mv1->v_viewport.y())
		std::swap(mv1, mv2);
	if (mv1->v_viewport.y() < mv0->v_viewport.y())
		std::swap(mv0, mv1);

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

	vp.m_pixel_shader->prepare_for_triangle(triangle, mv0, mv1, mv2);

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

		vp.m_pixel_shader->prepare_for_upper_triangle(long_line_on_right);

		fill_half_triangle(y, y_end, side_left, side_right, vp, *vp.m_pixel_shader);
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

		vp.m_pixel_shader->prepare_for_lower_triangle(long_line_on_right);

		fill_half_triangle(y, y_end, side_left, side_right, vp, *vp.m_pixel_shader);
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
