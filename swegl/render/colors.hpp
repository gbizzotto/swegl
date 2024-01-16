
#pragma once

namespace swegl
{

struct pixel_colors
{
	typedef int v4i __attribute__ ((vector_size (4)));
	union
	{
		struct { unsigned char b,g,r,a; } o;
		v4i v;
	};
	inline pixel_colors(unsigned char b, unsigned char g, unsigned char r, unsigned char a)
		: o{b,g,r,a}
	{}
	inline pixel_colors(v4i v)
		: v(v)
	{}
	inline int to_int() const { return *(int*)this; }
};
inline pixel_colors operator*(const pixel_colors & left, float right)
{
	return {(unsigned char)(left.o.b*right), (unsigned char)(left.o.g*right), (unsigned char)(left.o.r*right), 0};
}
inline pixel_colors operator/(const pixel_colors & left, int right)
{
	return {(unsigned char)(left.o.b/right), (unsigned char)(left.o.g/right), (unsigned char)(left.o.r/right), 0};
}
inline pixel_colors operator+(const pixel_colors & left, const pixel_colors & right)
{
	return pixel_colors{left.v+right.v};
}

} // namespace
