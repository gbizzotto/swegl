
#pragma once

#include <freon/Matrix.hpp>

namespace swegl
{

	class Vec2f : public freon::Matrix<float,1,2>
	{
	public:
		Vec2f();
		Vec2f(float x, float y);
		Vec2f(const Vec2f & other);

		Vec2f operator/(const Vec2f & other) const;
	};

	inline Vec2f operator-(const Vec2f & left, const Vec2f & right)
	{
		return Vec2f{left[0][0]-right[0][0], left[0][1]-right[0][1]};
	}
	inline Vec2f operator+(const Vec2f & left, const Vec2f & right)
	{
		return Vec2f{left[0][0]+right[0][0], left[0][1]+right[0][1]};
	}
	template<typename T>
	inline Vec2f operator*(const Vec2f & left, const T & right)
	{
		return Vec2f{left[0][0]*right, left[0][1]*right};
	}
}
