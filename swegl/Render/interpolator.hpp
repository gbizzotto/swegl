
#pragma once

#include <tuple>
#include <freon/Matrix.hpp>

namespace swegl
{
 struct interpolator_t_old
 {
	freon::Matrix<float,1,2> topalpha, topstep;
	freon::Matrix<float,1,2> ualpha;
	float bottomalpha, bottomstep;

	inline void Init(const float dist, const float z1, const float z2)
	{
		float alphastep = 1.0f / dist; // dist always != 0
		bottomalpha = 1.0f / z1;
		float invz2 = 1.0f / z2;
		ualpha[0][0] = 0.0f;
		ualpha[0][1] = z1;
		topalpha = ualpha;
		topalpha *= bottomalpha;
		topstep[0][0] = (1.0f * invz2 - topalpha[0][0]) * alphastep;
		topstep[0][1] = (z2   * invz2 - topalpha[0][1]) * alphastep;
		bottomstep = (invz2-bottomalpha)*alphastep;
	}
	inline void DisplaceStartingPoint(const float & move) {
		topalpha += topstep * move;
		bottomalpha += bottomstep * move;
		div(ualpha, topalpha, bottomalpha);
	}
	inline void Step() {
		topalpha += topstep;
		bottomalpha += bottomstep;
		div(ualpha, topalpha, bottomalpha);
	}

	inline float progress() const { return ualpha[0][0]; }
	inline float z() const { return ualpha[0][1]; }

	inline static void div(freon::Matrix<float,1,2> & a, const freon::Matrix<float,1,2> & b, float c)
	{
		a[0][0] = b[0][0] / c;
		a[0][1] = b[0][1] / c;
	};
};
struct interpolator_progress
{
	float topalpha, ualpha;
	float topstep;
	float bottomalpha, bottomstep;
	float z_, z_dir;

	inline void Init(const float dist, const float z1, const float z2)
	{
		z_ = z1;
		z_dir = z2-z1;
		
		float alphastep = 1.0f / dist; // dist always != 0
		bottomalpha = 1.0f / z1;
		float invz2 = 1.0f / z2;
		ualpha = 0.0f;
		topalpha = ualpha;
		topalpha *= bottomalpha;
		topstep = (1.0f * invz2 - topalpha) * alphastep;
		bottomstep = (invz2-bottomalpha)*alphastep;
	}
	inline void DisplaceStartingPoint(const float & move) {
		topalpha += topstep * move;
		bottomalpha += bottomstep * move;
		ualpha = topalpha / bottomalpha;
	}
	inline void Step() {
		topalpha += topstep;
		bottomalpha += bottomstep;
		ualpha = topalpha / bottomalpha;
	}

	inline float progress() const { return ualpha; }
	inline float z() const { return z_ + z_dir*progress(); }

	inline static void div(freon::Matrix<float,1,2> & a, const freon::Matrix<float,1,2> & b, float c)
	{
		a[0][0] = b[0][0] / c;
		a[0][1] = b[0][1] / c;
	};
};


template<size_t N>
struct interpolator_g
{
	float topalpha, topstep;
	float ualpha;
	float bottomalpha, bottomstep;

	std::array<std::array<float,2>,N> v;

	inline void Init(const float dist, const float z1, const float z2, decltype(v) && values)
	{
		for (size_t i=0 ; i<N ; i++)
		{
			v[i][0] = values[i][0];
			v[i][1] = values[i][1] - values[i][0];
		}
		float alphastep = 1.0f / dist; // dist always != 0
		bottomalpha = 1.0f / z1;
		float invz2 = 1.0f / z2;
		ualpha = 0.0f;
		topalpha = ualpha;
		topalpha *= bottomalpha;
		topstep = (1.0f * invz2 - topalpha) * alphastep;
		bottomstep = (invz2-bottomalpha)*alphastep;
	}
	inline void InitSelf(const float dist, const float z1, const float z2)
	{
		v[0][0] = z1;
		v[0][1] = z2-z1;

		float alphastep = 1.0f / dist; // dist always != 0
		bottomalpha = 1.0f / z1;
		float invz2 = 1.0f / z2;
		ualpha = 0.0f;
		topalpha = ualpha;
		topalpha *= bottomalpha;
		topstep = (1.0f * invz2 - topalpha) * alphastep;
		bottomstep = (invz2-bottomalpha)*alphastep;
	}
	inline void DisplaceStartingPoint(const float & move) {
		topalpha += topstep * move;
		bottomalpha += bottomstep * move;
		ualpha = topalpha / bottomalpha;
	}
	inline void Step() {
		topalpha += topstep;
		bottomalpha += bottomstep;
		ualpha = topalpha / bottomalpha;
	}

	inline float progress() const { return ualpha; }
	float value(size_t i) const { return v[i][0] + v[i][1] * progress(); }
};

} // namespace
