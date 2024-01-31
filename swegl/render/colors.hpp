
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
		int i;
	};
	pixel_colors() = default;
	inline pixel_colors(unsigned char b, unsigned char g, unsigned char r, unsigned char a)
		: o{b,g,r,a}
	{}
	inline pixel_colors(v4i v)
		: v(v)
	{}
	inline pixel_colors(int i)
		: i(i)
	{}
	inline pixel_colors(unsigned int i)
		: i(i)
	{}
	inline int to_int() const { return *(int*)this; }
};
pixel_colors operator*(const pixel_colors & left, float right);
pixel_colors operator/(const pixel_colors & left, int right);
pixel_colors operator+(const pixel_colors & left, const pixel_colors & right);
pixel_colors blend(const pixel_colors & back, const pixel_colors & front);

} // namespace
