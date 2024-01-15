
#pragma once

namespace swegl
{

struct pixel_colors
{
	typedef int v4i __attribute__ ((vector_size (4)));
	union
	{
		struct { unsigned char a,b,c,d; } o;
		v4i v;
	};
	inline pixel_colors(unsigned char a, unsigned char b, unsigned char c, unsigned char d)
		: o{a,b,c,d}
	{}
	inline pixel_colors(v4i v)
		: v(v)
	{}
	inline int to_int() const { return *(int*)this; }
};
inline pixel_colors operator*(const pixel_colors & left, float right)
{
	return {(unsigned char)(left.o.a*right), (unsigned char)(left.o.b*right), (unsigned char)(left.o.c*right), (unsigned char)(left.o.d*right)};
}
inline pixel_colors operator/(const pixel_colors & left, int right)
{
	return {(unsigned char)(left.o.a/right), (unsigned char)(left.o.b/right), (unsigned char)(left.o.c/right), (unsigned char)(left.o.d/right)};
}
inline pixel_colors operator+(const pixel_colors & left, const pixel_colors & right)
{
	return pixel_colors{left.v+right.v};
}

} // namespace
