
#pragma once

namespace swegl
{

template<typename T>
T lerp_unclipped(const T & a, const T & b, float x)
{
	return a + (b-a)*x;
}
template<typename T>
float inv_lerp_unclipped(const T & a, const T & b, const T & x)
{
	return (x-a) / (b-a);
}
template<typename T1, typename T2>
T2 remap_unclipped(const T1 & a, const T1 & b, const T2 & u, const T2 & v, const T1 & t)
{
	return lerp(u, v, inv_lerp(a, b, t));
}


template<typename T>
T lerp_clipped(const T & a, const T & b, float x)
{
	if (x<=0) return a;
	if (x>=b) return b;
	return a + (b-a)*x;
}
template<typename T>
float inv_lerp_clipped(const T & a, const T & b, const T & x)
{
	if (a==b) return 0.5;
	if (x<=a) return 0;
	if (x>=b) return 1;
	return (x-a) / (b-a);
}
template<typename T1, typename T2>
T2 remap_clipped(const T1 & a, const T1 & b, const T2 & u, const T2 & v, const T1 & t)
{
	return lerp_clipped(u, v, inv_lerp_clipped(a, b, t));
}

} // namespace
