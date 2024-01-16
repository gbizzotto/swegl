
#include <cmath>
#include <swegl/projection/vec2f.hpp>

namespace swegl
{

	vec2f_t::vec2f_t()
		:freon::Matrix<float,1,2>(freon::Matrix<float,1,2>::Zero)
	{}
	vec2f_t::vec2f_t(float x, float y)
		:freon::Matrix<float,1,2>({x,y})
	{}
	vec2f_t::vec2f_t(const vec2f_t & other)
		:freon::Matrix<float,1,2>({other[0][0], other[0][1]})
	{}

	vec2f_t vec2f_t::operator/(const vec2f_t & other) const
	{
		return vec2f_t((*this)[0][0]/other[0][0],(*this)[0][1]/other[0][1]);
	}

}
