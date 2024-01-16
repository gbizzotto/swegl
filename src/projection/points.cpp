
#include <cmath>
#include <swegl/projection/points.hpp>

namespace swegl
{

	vertex_t Transform(const vertex_t & v, const Matrix4x4 & m)
	{
		return vertex_t(m[0][0]*v.x() + m[0][1]*v.y() + m[0][2]*v.z() + m[0][3],
		                m[1][0]*v.x() + m[1][1]*v.y() + m[1][2]*v.z() + m[1][3],
		                m[2][0]*v.x() + m[2][1]*v.y() + m[2][2]*v.z() + m[2][3]);
	}

	normal_t Cross(const vector_t & left, const vector_t & right)
	{
		return normal_t(left.y()*right.z() - left.z()*right.y(),
		                left.z()*right.x() - left.x()*right.z(),
		                left.x()*right.y() - left.y()*right.x());
	}
	normal_t Transform(const normal_t & v, const Matrix4x4 & m)
	{
		return normal_t(m[0][0]*v.x() + m[0][1]*v.y() + m[0][2]*v.z() + m[0][3],
		                m[1][0]*v.x() + m[1][1]*v.y() + m[1][2]*v.z() + m[1][3],
		                m[2][0]*v.x() + m[2][1]*v.y() + m[2][2]*v.z() + m[2][3]);
	}
}
