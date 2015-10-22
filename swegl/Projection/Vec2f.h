
#pragma once

namespace swegl
{

	class Vec3f;

	class Vec2f
	{
	public:
		float x,y;
		
		Vec2f();
		Vec2f(float x, float y);
		Vec2f(const Vec3f & other);

		Vec2f & operator=(const Vec3f & other);
		Vec2f operator/(float f) const;
		Vec2f operator*(float f) const;
		Vec2f operator+(const Vec2f & other) const;
		Vec2f operator+=(const Vec2f & other);
		Vec2f operator-(const Vec2f & other) const;
		Vec2f operator/(const Vec2f & other) const;
	};

}
