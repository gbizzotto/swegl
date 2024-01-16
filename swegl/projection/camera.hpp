
#pragma once

#include <swegl/projection/points.hpp>
#include <swegl/projection/matrix44.hpp>

namespace swegl
{

	class Camera
	{
	public:
		float m_xfov, m_yfov;
		vertex_t m_center;
		Matrix4x4 m_viewmatrix;
		Matrix4x4 m_projectionmatrix;

		Camera();
		Camera(float aspectratio);
		void RotateX(float a);
		void RotateY(float a);
		void RotateZ(float a);
		void Translate(float x, float y , float z);
		inline vertex_t position() const { return m_center; };
	};

}
