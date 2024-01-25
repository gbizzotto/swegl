
#include <swegl/render/colors.hpp>

namespace swegl
{

pixel_colors operator*(const pixel_colors & left, float right)
{
	return {(unsigned char)(left.o.b*right), (unsigned char)(left.o.g*right), (unsigned char)(left.o.r*right), (unsigned char)(left.o.a*right)};
}
pixel_colors operator/(const pixel_colors & left, int right)
{
	return {(unsigned char)(left.o.b/right), (unsigned char)(left.o.g/right), (unsigned char)(left.o.r/right), (unsigned char)(left.o.a/right)};
}
pixel_colors operator+(const pixel_colors & left, const pixel_colors & right)
{
	return pixel_colors{left.v+right.v};
}

} // namespace
