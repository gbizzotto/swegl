
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



	class vertex_t : public Vec3f
	{
	public:
		inline vertex_t() : Vec3f() {}
		inline vertex_t(float x, float y, float z) : Vec3f(x, y, z) {}

		      float & x()       { return (*this)[0][0]; }
		const float & x() const { return (*this)[0][0]; }
		      float & y()       { return (*this)[0][1]; }
		const float & y() const { return (*this)[0][1]; }
		      float & z()       { return (*this)[0][2]; }
		const float & z() const { return (*this)[0][2]; }
	};
	
	vertex_t Transform(const vertex_t &, const Matrix4x4 &);



	class vector_t : public Vec3f
	{
	public:
		inline vector_t() : Vec3f() {}
		inline vector_t(float x, float y, float z) : Vec3f(x, y, z) {}

		      float & x()       { return (*this)[0][0]; }
		const float & x() const { return (*this)[0][0]; }
		      float & y()       { return (*this)[0][1]; }
		const float & y() const { return (*this)[0][1]; }
		      float & z()       { return (*this)[0][2]; }
		const float & z() const { return (*this)[0][2]; }
	};

	class normal_t : public vector_t
	{
	public:
		inline normal_t() : vector_t() {}
		inline normal_t(float x, float y, float z): vector_t(x, y, z) {}
	};

	inline vector_t operator-(const vertex_t left, const vertex_t right)
	{
		return vector_t{left.x() - right.x(),
		                left.y() - right.y(),
		                left.z() - right.z()};
	}
	normal_t Cross(const vector_t &, const vector_t &);
	normal_t Transform(const normal_t &, const Matrix4x4 &);
}
