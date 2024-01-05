
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
		Vec3f & Normalize();
		float Dot(const Vec3f &) const;

		Vec3f operator/(const Vec3f &) const;

		const Vec3f & operator=(const freon::Matrix<float,1,3> &);

		inline Vec3f & DivXY(float z) { (*this)[0][0]/=z; (*this)[0][1]/=z; return *this; }

		inline void ok(const Vec3f & a, float b)
		{
			(*this)[0][0] = a[0][0] / b;
			(*this)[0][1] = a[0][1] / b;
			(*this)[0][2] = a[0][2] / b;
		}
	};

	Vec3f Cross(const freon::Matrix<float,1,3> &, const freon::Matrix<float,1,3> &);
	Vec3f Transform(const Vec3f &, const Matrix4x4 &);

	inline Vec3f operator-(const Vec3f & left, const Vec3f & right) { return Vec3f(left[0][0] - right[0][0], left[0][1] - right[0][1], left[0][2] - right[0][2]); }
	inline Vec3f operator+(const Vec3f & left, const Vec3f & right) { return Vec3f(left[0][0] + right[0][0], left[0][1] + right[0][1], left[0][2] + right[0][2]); }

	template<typename O>
	O & operator<<(O & out, const Vec3f & v)
	{
		return out << "{" << v[0][0] << "," << v[0][1] << "," << v[0][2] << "}";
	}
}
