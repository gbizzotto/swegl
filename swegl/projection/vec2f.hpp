
#pragma once

#include <freon/Matrix.hpp>

namespace swegl
{

	class vec2f_t : public freon::Matrix<float,1,2>
	{
	public:
		vec2f_t();
		vec2f_t(float x, float y);
		vec2f_t(const vec2f_t & other);
		vec2f_t & operator=(const vec2f_t &) = default;

		vec2f_t operator/(const vec2f_t & other) const;
	};

	inline vec2f_t operator-(const vec2f_t & left, const vec2f_t & right)
	{
		return vec2f_t{left[0][0]-right[0][0], left[0][1]-right[0][1]};
	}
	inline vec2f_t operator+(const vec2f_t & left, const vec2f_t & right)
	{
		return vec2f_t{left[0][0]+right[0][0], left[0][1]+right[0][1]};
	}
	template<typename T>
	inline vec2f_t operator*(const vec2f_t & left, const T & right)
	{
		return vec2f_t{left[0][0]*right, left[0][1]*right};
	}
}
