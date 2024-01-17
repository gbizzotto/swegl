
#pragma once

#include <memory>

#include <SDL.h>

#include <swegl/projection/matrix44.hpp>
#include <swegl/projection/camera.hpp>

namespace swegl
{

	struct vertex_shader_t;
	struct  pixel_shader_t;
	struct   post_shader_t;

	struct viewport_t
	{
		int                      m_x, m_y;
		int                      m_w, m_h;
		SDL_Surface             *m_screen;
		std::unique_ptr<float[]> m_zbuffer;
		matrix44_t               m_viewportmatrix;
		camera_t                 m_camera;
		std::shared_ptr<swegl::vertex_shader_t> m_vertex_shader      ;
		std::shared_ptr<swegl:: pixel_shader_t> m_pixel_shader_smooth;
		std::shared_ptr<swegl:: pixel_shader_t> m_pixel_shader_sharp ;
		std::shared_ptr<swegl::  post_shader_t> m_post_shader        ;

		viewport_t(int x, int y, int w, int h
		          ,SDL_Surface *screen
		          ,std::shared_ptr<swegl::vertex_shader_t> & vertex_shader
		          ,std::shared_ptr<swegl:: pixel_shader_t> & pixel_shader_smooth
		          ,std::shared_ptr<swegl:: pixel_shader_t> & pixel_shader_sharp
		          ,std::shared_ptr<swegl::  post_shader_t> & post_shader
		          );

		      float * zbuffer()       { return m_zbuffer.get(); }
		const float * zbuffer() const { return m_zbuffer.get(); }

		      camera_t & camera()       { return m_camera; }
		const camera_t & camera() const { return m_camera; }

		void clear();
	};

}
