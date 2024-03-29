
#pragma once

#include <memory>

#include <SDL.h>

#include <swegl/projection/matrix44.hpp>
#include <swegl/projection/camera.hpp>
#include <swegl/data/model.hpp>

namespace swegl
{

	struct vertex_shader_t;
	struct  pixel_shader_t;
	struct   post_shader_t;

	struct transparency_layer_t
	{
		std::unique_ptr<pixel_colors[]> m_colors;
		std::unique_ptr<float[]>        m_zbuffer;

		transparency_layer_t(int w, int h);
	};

	struct viewport_t
	{
		int                                     m_x, m_y        ;
		int                                     m_w, m_h        ;
		SDL_Surface                            *m_screen        ;
		std::unique_ptr<float[]>                m_zbuffer       ;
		matrix44_t                              m_viewportmatrix;
		camera_t                                m_camera        ;
		std::shared_ptr<swegl::pixel_shader_t>  m_pixel_shader  ;
		post_shader_t                          *m_post_shader   ;
		std::vector<transparency_layer_t>       m_transparency_layers;
		bool                                    m_got_transparency   ;

		viewport_t(int x, int y, int w, int h
		          ,SDL_Surface *screen
		          ,std::shared_ptr<swegl:: pixel_shader_t> & pixel_shader
		          ,int transparency_layer_count
		          );

		void flatten();
		void flatten(transparency_layer_t & front, transparency_layer_t & back);
		void flatten(transparency_layer_t & front);

		inline void set_post_shader(post_shader_t & post_shader) { m_post_shader = & post_shader; }


		      float * zbuffer()       { return m_zbuffer.get(); }
		const float * zbuffer() const { return m_zbuffer.get(); }

		      camera_t & camera()       { return m_camera; }
		const camera_t & camera() const { return m_camera; }

		void clear();
		vertex_t transform(const vertex_t & v) const;
		void     transform(mesh_vertex_t & v) const;
	};

}
