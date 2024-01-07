
#pragma once

#include <freon/Matrix.hpp>

namespace swegl
{

	struct ZInterpolator
	{
		freon::Matrix<float,1,3> topalpha, topstep;
		freon::Matrix<float,1,3> ualpha;
		float bottomalpha, bottomstep;

		inline void Init(const float dist, const float z1, const float z2, const Vec2f & t0, const Vec2f & t1)
		{
			float alphastep = 1.0f / dist; // dist always != 0
			bottomalpha = 1.0f / z1;
			float invz2 = 1.0f / z2;
			ualpha[0][0] = t0[0][0];
			ualpha[0][1] = t0[0][1];
			ualpha[0][2] = z1;
			topalpha = ualpha;
			topalpha *= bottomalpha;
			topstep[0][0] = (t1[0][0] * invz2 - topalpha[0][0]) * alphastep;
			topstep[0][1] = (t1[0][1] * invz2 - topalpha[0][1]) * alphastep;
			topstep[0][2] = (z2       * invz2 - topalpha[0][2]) * alphastep;
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

		inline static void div(freon::Matrix<float,1,3> & a, const freon::Matrix<float,1,3> & b, float c)
		{
			a[0][0] = b[0][0] / c;
			a[0][1] = b[0][1] / c;
			a[0][2] = b[0][2] / c;
		}
	};

	/*
	// ZInterpolator with Quake optimization
	class QInterpolator : public ZInterpolator
	{
	public:
		Vec3f ualphastep;
		unsigned int quake;

		inline void DisplaceStartingPoint(const float & move) {
			topalpha += topstep * move;
			bottomalpha += bottomstep * move;
			ualpha = topalpha;
			ualpha /= bottomalpha;

			topstep *= 16.0f;
			bottomstep = bottomstep * 16.0f;
			topalpha += topstep;
			bottomalpha += bottomstep;
			ualphastep = topalpha;
			ualphastep /= bottomalpha;
			ualphastep -= ualpha;
			ualphastep /= 16.0f;
			quake = 15;
		}
		inline void DisplaceStartingPoint(const float & move, int stepcount) {
			if (stepcount > 16)
				stepcount = 16;
			topalpha += topstep * move;
			bottomalpha += bottomstep * move;
			ualpha = topalpha;
			ualpha /= bottomalpha;
			topstep *= (float)stepcount;
			bottomstep = bottomstep * (float)stepcount;
			topalpha += topstep;
			bottomalpha += bottomstep;
			ualphastep = topalpha;
			ualphastep /= bottomalpha;
			ualphastep -= ualpha;
			ualphastep /= (float)stepcount;
			quake = stepcount - 1;
		}
		inline void Step() {
			if (quake == 0) {
				ualpha = topalpha;
				ualpha /= bottomalpha;
				topalpha += topstep;
				bottomalpha += bottomstep;
				ualphastep = topalpha;
				ualphastep /= bottomalpha;
				ualphastep -= ualpha;
				ualphastep /= 16.0f;
				quake = 15;
			} else {
				ualpha += ualphastep;
				quake--;
			}
		}
	};
	*/

}
