
#pragma once

#include <cmath>

#include <swegl/Projection/Matrix4x4.h>

namespace swegl
{

class vertex_t
{
	freon::Matrix<float,1,3> matrix;

public:
	inline vertex_t() : matrix() {}
	inline vertex_t(float x, float y, float z) : matrix{x, y, z} {}

	      float & x()       { return matrix[0][0]; }
	const float & x() const { return matrix[0][0]; }
	      float & y()       { return matrix[0][1]; }
	const float & y() const { return matrix[0][1]; }
	      float & z()       { return matrix[0][2]; }
	const float & z() const { return matrix[0][2]; }
};

vertex_t Transform(const vertex_t &, const Matrix4x4 &);

template<typename T>
vertex_t operator+(const vertex_t & left, const T & right)
{
	return vertex_t(left.x()+right, left.y()+right, left.z()+right);
}
inline vertex_t operator+(const vertex_t & left, const vertex_t & right)
{
	return vertex_t(left.x()+right.x(), left.y()+right.y(), left.z()+right.z());
}
template<typename T>
vertex_t operator/(const vertex_t & left, const T & right)
{
	return vertex_t(left.x()/right, left.y()/right, left.z()/right);
}


class vector_t
{
	freon::Matrix<float,1,3> matrix;

public:
	inline vector_t() : matrix() {}
	inline vector_t(float x, float y, float z) : matrix{x, y, z} {}

	      float & x()       { return matrix[0][0]; }
	const float & x() const { return matrix[0][0]; }
	      float & y()       { return matrix[0][1]; }
	const float & y() const { return matrix[0][1]; }
	      float & z()       { return matrix[0][2]; }
	const float & z() const { return matrix[0][2]; }

	float len() const
	{
		return (float) sqrt(x()*x() + y()*y() + z()*z());
	}
	float len_squared() const
	{
		return (float) (x()*x() + y()*y() + z()*z());
	}

	inline vector_t & normalize()
	{
		float l = len();
		x() /= l;
		y() /= l;
		z() /= l;
		return *this;
	}
};

template<typename T>
vector_t operator*(const vector_t & left, const T & right)
{
	return vector_t(left.x()*right, left.y()*right, left.z()*right);
}
inline vertex_t operator+(const vertex_t & left, const vector_t & right)
{
	return vertex_t(left.x()+right.x(), left.y()+right.y(), left.z()+right.z());
}

class normal_t : public vector_t
{
public:
	inline normal_t() : vector_t() {}
	inline normal_t(float x, float y, float z): vector_t(x, y, z) {}

	inline float dot(const vector_t & other) const
	{
		return x()*other.x() + y()*other.y() + z()*other.z();
	}
};

inline vector_t operator-(const vertex_t left, const vertex_t right)
{
	return vector_t{left.x() - right.x(),
	                left.y() - right.y(),
	                left.z() - right.z()};
}
normal_t Cross(const vector_t &, const vector_t &);
normal_t Transform(const normal_t &, const Matrix4x4 &);

} // namespace
