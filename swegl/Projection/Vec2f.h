
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

}
