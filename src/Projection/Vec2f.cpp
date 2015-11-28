
#include <cmath>
#include <swegl/Projection/Vec2f.h>
#include <swegl/Projection/Vec3f.h>

namespace swegl
{

	Vec2f::Vec2f()
		:freon::Matrix<float,1,2>(freon::Matrix<float,1,2>::Zero)
	{}
	Vec2f::Vec2f(float x, float y)
		:freon::Matrix<float,1,2>({x,y})
	{}
	Vec2f::Vec2f(const Vec3f & other)
		:freon::Matrix<float,1,2>({other[0][0], other[0][1]})
	{}

	Vec2f Vec2f::operator/(const Vec2f & other) const
	{
		return Vec2f((*this)[0][0]/other[0][0],(*this)[0][1]/other[0][1]);
	}

}
