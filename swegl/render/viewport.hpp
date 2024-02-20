
#pragma once

#include <memory>

#include <SDL.h>

#include <swegl/projection/matrix44.hpp>
#include <swegl/projection/camera.hpp>
#include <swegl/data/model.hpp>
#include <swegl/misc/fraction.hpp>
#include <swegl/misc/screen.hpp>

namespace swegl
{

	struct new_vertex_shader_t;
	struct      pixel_shader_t;
	struct       post_shader_t;

	class viewport_t
	{
		screen_t                                m_screen        ;

	public:
		int                                     m_x, m_y        ;
		int                                     m_w, m_h        ;
		matrix44_t                              m_viewportmatrix;
		camera_t                                m_camera        ;
		post_shader_t                          &m_post_shader   ;
		std::shared_ptr<swegl::pixel_shader_t>  m_pixel_shader  ;
		std::vector<screen_t>                   m_transparency_layers;
		bool                                    m_got_transparency   ;

		viewport_t(int x, int y, int w, int h
		          ,SDL_Surface *screen
		          ,std::shared_ptr<swegl:: pixel_shader_t> & pixel_shader
		          ,int transparency_layer_count
		          ,post_shader_t & post_shader
		          );

		screen_t & output_screen();
		screen_t & intermediate_screen();

		void flatten(const fraction_t & f);
		void flatten(const fraction_t & f, screen_t & front, screen_t & back);
		//void flatten(const fraction_t & f, screen_t & front);

		void do_post_shading(const fraction_t &);


		      camera_t & camera()       { return m_camera; }
		const camera_t & camera() const { return m_camera; }

		void clear();
		void clear(const fraction_t & f);
		vertex_t transform(const vertex_t & v) const;
		void     transform(new_mesh_vertex_t & v) const;
	};

}
