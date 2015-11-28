
#include <cmath>
#include <swegl/Projection/Vec3f.h>
#include <swegl/Projection/Vec2f.h>

namespace swegl
{

	Vec3f::Vec3f()
		:freon::Matrix<float,1,3>(freon::Matrix<float,1,3>::Zero)
	{}
	Vec3f::Vec3f(float x, float y, float z)
		:freon::Matrix<float,1,3>({x,y,z})
	{}

	float Vec3f::Len() const
	{
		return (float) sqrt((*this)[0][0]*(*this)[0][0]+(*this)[0][1]*(*this)[0][1]+(*this)[0][2]*(*this)[0][2]);
	}

	void Vec3f::Normalize()
	{
		float len = Len();
		(*this)[0][0] /= len;
		(*this)[0][1] /= len;
		(*this)[0][2] /= len;
	}

	float Vec3f::Dot(const Vec3f &other) const
	{
		return (*this)[0][0]*other[0][0] + (*this)[0][1]*other[0][1] + (*this)[0][2]*other[0][2];
	}

	Vec3f Vec3f::operator/(const Vec3f & other) const
	{
		return Vec3f((*this)[0][0] / other[0][0],
		             (*this)[0][1] / other[0][1],
		             (*this)[0][2] + other[0][2]);
	}

	const Vec3f & Vec3f::operator=(const freon::Matrix<float,1,3> & other)
	{
		freon::Matrix<float,1,3>::operator=(other);
		return *this;
	}


	Vec3f Cross(const freon::Matrix<float,1,3> & left, const freon::Matrix<float,1,3> & right)
	{
		return Vec3f(left[0][1]*right[0][2] - left[0][2]*right[0][1],
						 left[0][2]*right[0][0] - left[0][0]*right[0][2],
						 left[0][0]*right[0][1] - left[0][1]*right[0][0]);
	}

	Vec3f Transform(const Vec3f & v, const Matrix4x4 & m)
	{
		return Vec3f(m[0][0]*v[0][0] + m[0][1]*v[0][1] + m[0][2]*v[0][2] + m[0][3],
		             m[1][0]*v[0][0] + m[1][1]*v[0][1] + m[1][2]*v[0][2] + m[1][3],
		             m[2][0]*v[0][0] + m[2][1]*v[0][1] + m[2][2]*v[0][2] + m[2][3]);
	}
}
