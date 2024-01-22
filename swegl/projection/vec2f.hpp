
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
}
