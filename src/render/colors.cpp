
#include <cmath>
#include <swegl/render/colors.hpp>

namespace swegl
{


pixel_colors_f operator+(const pixel_colors_f & left, const pixel_colors_f & right)
{
	return pixel_colors_f{left.b+right.b, left.g+right.g, left.r+right.r, left.a+right.a};
}


pixel_colors::pixel_colors(const pixel_colors_f & pcf)
	:o{(unsigned char)round(pcf.b)
	  ,(unsigned char)round(pcf.g)
	  ,(unsigned char)round(pcf.r)
	  ,(unsigned char)round(pcf.a)
	  }
{}

pixel_colors_f operator*(const pixel_colors & left, float right)
{
	return {left.o.b*right, left.o.g*right, left.o.r*right, left.o.a*right};
}
pixel_colors operator/(const pixel_colors & left, int right)
{
	return {(unsigned char)(left.o.b/right), (unsigned char)(left.o.g/right), (unsigned char)(left.o.r/right), (unsigned char)(left.o.a/right)};
}
pixel_colors operator+(const pixel_colors & left, const pixel_colors & right)
{
	return pixel_colors{left.v+right.v};
}
pixel_colors blend(const pixel_colors & back, const pixel_colors & front)
{
	int alpha = front.o.a;

	if (alpha == 255)
		return front;
	if (alpha == 0)
		return back;

	return pixel_colors(back.o.b * ((256-alpha)/256.0) + front.o.b * (alpha/256.0)
	                   ,back.o.g * ((256-alpha)/256.0) + front.o.g * (alpha/256.0)
	                   ,back.o.r * ((256-alpha)/256.0) + front.o.r * (alpha/256.0)
	                   ,alpha);
}

} // namespace
