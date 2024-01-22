
#include <cmath>
#include <swegl/projection/vec2f.hpp>

namespace swegl
{

	vec2f_t vec2f_t::operator/(const vec2f_t & other) const
	{
		return vec2f_t(this->x()/other.x(), this->y()/other.y());
	}

}
