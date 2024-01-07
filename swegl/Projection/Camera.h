
#pragma once

#include <swegl/Projection/points.hpp>
#include <swegl/Projection/Matrix4x4.h>

namespace swegl
{

	class Camera
	{
	public:
		vertex_t m_center;
		vector_t m_direction;
		vector_t m_roll;
		float m_xfov, m_yfov;
		Matrix4x4 m_viewmatrix;
		Matrix4x4 m_projectionmatrix;

		Camera();
		Camera(float aspectratio);
		void RotateX(float a);
		void RotateY(float a);
		void RotateZ(float a);
		void Translate(float x, float y , float z);
	};

}
