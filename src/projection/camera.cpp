
#include <math.h>
#include <swegl/projection/camera.hpp>

namespace swegl
{

	Camera::Camera()
		:m_center(0,0,0)
		,m_viewmatrix(Matrix4x4::Identity)
		,m_projectionmatrix(Matrix4x4::Identity)
	{
		//this->setXFov(90);
		//this->setYFov(60);

		// point to negative Z
		m_viewmatrix[2][2] = -1;

		float n = 1.0f;
		float f = 10.0f;
		float w = 1.0f;
		float h = 1.0f;

		this->m_projectionmatrix[0][0] = 2*n / w;
		this->m_projectionmatrix[1][1] = 2*n / h;
		this->m_projectionmatrix[2][2] = f / (f-n);
		this->m_projectionmatrix[2][3] = -f*n / (f-n);
		this->m_projectionmatrix[3][2] = 1;
	}

	Camera::Camera(float aspectratio)
		:m_center(0,0,0)
		,m_viewmatrix(Matrix4x4::Identity)
		,m_projectionmatrix(Matrix4x4::Identity)
	{
		//this->setXFov(90);
		//this->setYFov(60);

		// point to negative Z
		m_viewmatrix[2][2] = -1;

		float n = 1.0f;
		float f = 10.0f;
		float w = 1.0f;
		float h = 1.0f;

		if (aspectratio > 1)
		{
			this->m_projectionmatrix[0][0] = 2*n / w;
			this->m_projectionmatrix[1][1] = aspectratio * 2*n / h;
		}
		else
		{
			this->m_projectionmatrix[0][0] = (1.0f/aspectratio) * 2*n / w;
			this->m_projectionmatrix[1][1] = 2*n / h;
		}
		this->m_projectionmatrix[2][2] = f / (f-n);
	//	this->m_projectionmatrix[2][3] = -f*n / (f-n);
		this->m_projectionmatrix[3][2] = 1;
	}

	void Camera::RotateX(float a)
	{
		this->m_viewmatrix.RotateX(-a);
	}
	void Camera::RotateY(float a)
	{
		this->m_viewmatrix.RotateY(-a);
	}
	void Camera::RotateZ(float a)
	{
		this->m_viewmatrix.RotateZ(-a);
	}

	void Camera::Translate(float x, float y, float z)
	{
		this->m_viewmatrix.Translate(-x,-y,-z);

		auto & m = this->m_viewmatrix;
		m_center.x() += x * m[0][0] + y * m[1][0] + z * m[2][0];
		m_center.y() += x * m[0][1] + y * m[1][1] + z * m[2][1];
		m_center.z() += x * m[0][2] + y * m[1][2] + z * m[2][2];
	}

}