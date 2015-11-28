
#pragma once

#include <swegl/Projection/Matrix4x4.h>

namespace swegl
{

	class Vec3f : public freon::Matrix<float,1,3>
	{
	public:
		Vec3f();
		Vec3f(float x, float y, float z);

		float Len() const;
		void Normalize();
		float Dot(const Vec3f &) const;

		Vec3f operator/(const Vec3f &) const;

		const Vec3f & operator=(const freon::Matrix<float,1,3> &);

		inline Vec3f & DivXY(float z) { (*this)[0][0]/=z; (*this)[0][1]/=z; return *this; }
	};

	Vec3f Cross(const freon::Matrix<float,1,3> &, const freon::Matrix<float,1,3> &);
	Vec3f Transform(const Vec3f &, const Matrix4x4 &);
}
