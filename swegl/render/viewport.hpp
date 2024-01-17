
#pragma once

#include <memory>

#include <SDL.h>

#include <swegl/projection/matrix44.hpp>
#include <swegl/projection/camera.hpp>

namespace swegl
{

	struct viewport_t
	{
		int                      m_x, m_y;
		int                      m_w, m_h;
		SDL_Surface             *m_screen;
		std::unique_ptr<float[]> m_zbuffer;
		matrix44_t               m_viewportmatrix;
		camera_t                 m_camera;

		viewport_t(int x, int y, int w, int h, SDL_Surface *screen);

		      float * zbuffer()       { return m_zbuffer.get(); }
		const float * zbuffer() const { return m_zbuffer.get(); }

		      camera_t & camera()       { return m_camera; }
		const camera_t & camera() const { return m_camera; }

		void clear();
	};

}
