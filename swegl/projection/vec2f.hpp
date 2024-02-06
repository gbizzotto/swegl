
#pragma once

#include <freon/Matrix.hpp>

namespace swegl
{

	class vec2f_t
	{
		float _x, _y;

	public:
		vec2f_t() = default;
		inline vec2f_t(float x, float y)
			: _x(x)
			, _y(y)
		{}
		vec2f_t(const vec2f_t & other) = default;
		vec2f_t & operator=(const vec2f_t &) = default;

		inline float & x()       { return _x; }
		inline float   x() const { return _x; }
		inline float & y()       { return _y; }
		inline float   y() const { return _y; }

		vec2f_t operator/(const vec2f_t & other) const;
	};

	inline vec2f_t operator-(const vec2f_t & left, const vec2f_t & right)
	{
		return vec2f_t{left.x()-right.x(), left.y()-right.y()};
	}
	inline vec2f_t operator+(const vec2f_t & left, const vec2f_t & right)
	{
		return vec2f_t{left.x()+right.x(), left.y()+right.y()};
	}
	template<typename T>
	inline vec2f_t operator*(const vec2f_t & left, const T & right)
	{
		return vec2f_t{left.x()*right, left.y()*right};
	}
	inline bool operator==(const vec2f_t & left, const vec2f_t & right)
	{
		return left.x() == right.x()
			&& left.y() == right.y()
			;
	}
	inline bool operator<(const vec2f_t & left, const vec2f_t & right)
	{
		return left.x() < right.x()
			|| (left.x() == right.x() && left.y() < right.y())
			;
	}


	struct vec4f_t
	{
		float _x, _y, _z, _w;

		vec4f_t() = default;
		inline vec4f_t(float x, float y, float z, float w)
			: _x(x)
			, _y(y)
			, _z(z)
			, _w(w)
		{}
		vec4f_t(const vec4f_t & other) = default;
		vec4f_t & operator=(const vec4f_t &) = default;

		inline float & x()       { return _x; }
		inline float   x() const { return _x; }
		inline float & y()       { return _y; }
		inline float   y() const { return _y; }
		inline float & z()       { return _z; }
		inline float   z() const { return _z; }
		inline float & w()       { return _w; }
		inline float   w() const { return _w; }

		inline void normalize()
		{
			float len = sqrt(x()*x() + y()*y() + z()*z() + w()*w());
			if (len != 0)
			{
				_x = x()/len;
				_y = y()/len;
				_z = z()/len;
				_w = w()/len;
			};
		}
	};
	template<typename T>
	vec4f_t operator*(const vec4f_t & left, const T & right)
	{
		return vec4f_t{left.x()*right, left.y()*right, left.z()*right, left.w()*right};
	}
	template<typename T>
	vec4f_t operator+(const vec4f_t & left, const T & right)
	{
		return vec4f_t{left.x()+right.x(), left.y()+right.y(), left.z()+right.z(), left.w()+right.w()};
	}
	inline bool operator!=(const vec4f_t & left, const vec4f_t & right)
	{
		return false
			|| (left.x() != right.x())
			|| (left.y() != right.y())
			|| (left.z() != right.z())
			|| (left.w() != right.w())
			;
	}
	template<typename O>
	O & operator<<(O & out, const vec4f_t & v)
	{
		return out << "{" << v.x() << "," << v.y() << "," << v.z() << "," << v.w() << "}";
	}
}
