
#pragma once

#include <cmath>

#include <swegl/projection/matrix44.hpp>

namespace swegl
{

class vertex_t
{
	float _x, _y, _z;

public:
	vertex_t() = default;
	inline vertex_t(float x, float y, float z)
		: _x(x)
		, _y(y)
		, _z(z)
	{}
	vertex_t & operator=(const vertex_t &) = default;

	      float & x()       { return _x; }
	const float & x() const { return _x; }
	      float & y()       { return _y; }
	const float & y() const { return _y; }
	      float & z()       { return _z; }
	const float & z() const { return _z; }
};

vertex_t transform(const vertex_t &, const matrix44_t &);

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
	float _x, _y, _z;	

public:
	vector_t() = default;
	inline vector_t(float x, float y, float z)
		: _x(x)
		, _y(y)
		, _z(z)
	{}
	vector_t & operator=(const vector_t &) = default;

	      float & x()       { return _x; }
	const float & x() const { return _x; }
	      float & y()       { return _y; }
	const float & y() const { return _y; }
	      float & z()       { return _z; }
	const float & z() const { return _z; }

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
		_x /= l;
		_y /= l;
		_z /= l;
		return *this;
	}
	inline float dot(const vector_t & other) const
	{
		return x()*other.x() + y()*other.y() + z()*other.z();
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
inline vector_t operator-(const vector_t & left, const vector_t & right)
{
	return vector_t(left.x()-right.x(), left.y()-right.y(), left.z()-right.z());
}
inline vertex_t operator-(const vertex_t & left, const vector_t & right)
{
	return vertex_t(left.x()-right.x(), left.y()-right.y(), left.z()-right.z());
}
inline vector_t operator+(const vector_t & left, const vector_t & right)
{
	return vector_t(left.x()+right.x(), left.y()+right.y(), left.z()+right.z());
}

class normal_t : public vector_t
{
public:
	inline normal_t() : vector_t() {}
	inline normal_t(const vector_t && v) : vector_t(v) {}
	inline normal_t(float x, float y, float z): vector_t(x, y, z)
	{
		normalize();
	}
	inline normal_t & operator=(const vector_t & v)
	{
		x() = v.x();
		y() = v.y();
		z() = v.z();
		normalize();
		return *this;
	}
};

inline vector_t operator-(const vertex_t left, const vertex_t right)
{
	return vector_t{left.x() - right.x(),
	                left.y() - right.y(),
	                left.z() - right.z()};
}
normal_t cross(const vector_t &, const vector_t &);
normal_t transform(const normal_t &, const matrix44_t &);
normal_t rotate   (const normal_t &, const matrix44_t &);

inline normal_t operator+(const normal_t & left, const normal_t & right)
{
	return normal_t(left.x()+right.x(), left.y()+right.y(), left.z()+right.z());
}
inline normal_t operator-(const normal_t & left, const normal_t & right)
{
	return normal_t(left.x()-right.x(), left.y()-right.y(), left.z()-right.z());
}
template<typename T>
normal_t operator*(const normal_t & left, const T & right)
{
	return normal_t(left.x()*right, left.y()*right, left.z()*right);
}

template<typename O>
O & operator<<(O & out, const vertex_t & v)
{
	out << '{' << v.x() << ',' << v.y() << ',' << v.z() << '}';
	return out;
}
template<typename O>
O & operator<<(O & out, const vector_t & v)
{
	return out << '{' << v.x() << ',' << v.y() << ',' << v.z() << '}';
}
template<typename O>
O & operator<<(O & out, const normal_t & v)
{
	return out << '{' << v.x() << ',' << v.y() << ',' << v.z() << '}';
}

} // namespace
