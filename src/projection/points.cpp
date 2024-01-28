
#include <cmath>
#include <swegl/projection/points.hpp>

namespace swegl
{

	vertex_t transform(const vertex_t & v, const matrix44_t & m)
	{
		return vertex_t(m[0][0]*v.x() + m[0][1]*v.y() + m[0][2]*v.z() + m[0][3],
		                m[1][0]*v.x() + m[1][1]*v.y() + m[1][2]*v.z() + m[1][3],
		                m[2][0]*v.x() + m[2][1]*v.y() + m[2][2]*v.z() + m[2][3]);
	}
	matrix44_t scale(const matrix44_t & m, const vertex_t & v)
	{
		matrix44_t result = m;
		result[0][0] *= v.x();
		result[0][1] *= v.x();
		result[0][2] *= v.x();
		result[0][3] *= v.x();
		result[1][0] *= v.y();
		result[1][1] *= v.y();
		result[1][2] *= v.y();
		result[1][3] *= v.y();
		result[2][0] *= v.z();
		result[2][1] *= v.z();
		result[2][2] *= v.z();
		result[2][3] *= v.z();
		return result;
	}

	normal_t cross(const vector_t & left, const vector_t & right)
	{
		return normal_t(left.y()*right.z() - left.z()*right.y(),
		                left.z()*right.x() - left.x()*right.z(),
		                left.x()*right.y() - left.y()*right.x());
	}
	normal_t transform(const normal_t & v, const matrix44_t & m)
	{
		return normal_t(m[0][0]*v.x() + m[0][1]*v.y() + m[0][2]*v.z() + m[0][3],
		                m[1][0]*v.x() + m[1][1]*v.y() + m[1][2]*v.z() + m[1][3],
		                m[2][0]*v.x() + m[2][1]*v.y() + m[2][2]*v.z() + m[2][3]);
	}
	normal_t rotate(const normal_t & v, const matrix44_t & m)
	{
		return normal_t(m[0][0]*v.x() + m[0][1]*v.y() + m[0][2]*v.z(),
		                m[1][0]*v.x() + m[1][1]*v.y() + m[1][2]*v.z(),
		                m[2][0]*v.x() + m[2][1]*v.y() + m[2][2]*v.z());
	}
}
