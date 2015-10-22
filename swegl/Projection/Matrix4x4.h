
#pragma once

#include <memory.h>
#include <initializer_list>

namespace swegl
{

	class Vec3f;

	class Matrix4x4
	{
	private:
		float m_data[16];

	public:
		Matrix4x4()
			:m_data{ 0 }
		{}

		template <typename... T>
		Matrix4x4(T... ts)
			:m_data{ ts... }
		{}

		Matrix4x4 operator*(const Matrix4x4 & other) const;
		Vec3f operator*(const Vec3f & v) const;
		const Matrix4x4 & operator=(const Matrix4x4 & other);
		float   operator[](const int idx) const;
		float & operator[](const int idx);
		void RotateX(float a);
		void RotateY(float a);
		void RotateZ(float a);
		void SetRotateXY(float x, float y);
		void Translate(float x, float y, float z);

		static const Matrix4x4 Identity;
	};
	
}
