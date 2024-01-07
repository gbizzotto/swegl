
#pragma once

#include <freon/Matrix.hpp>

namespace swegl
{

struct interpolator_t
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

	inline static void div(freon::Matrix<float,1,2> & a, const freon::Matrix<float,1,2> & b, float c)
	{
		a[0][0] = b[0][0] / c;
		a[0][1] = b[0][1] / c;
	};
};

} // namespace
