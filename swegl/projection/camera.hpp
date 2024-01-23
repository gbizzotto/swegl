
#pragma once

#include <swegl/projection/points.hpp>
#include <swegl/projection/matrix44.hpp>

namespace swegl
{

	class camera_t
	{
	public:
		float m_xfov, m_yfov;
		vertex_t m_center;
		matrix44_t m_viewmatrix;
		matrix44_t m_projectionmatrix;

		camera_t(float aspect_ratio);
		void rotate_x(float a);
		void rotate_y(float a);
		void rotate_z(float a);
		void translate(float x, float y , float z);
		inline vertex_t position() const { return m_center; };
	};

}
