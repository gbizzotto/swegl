
#pragma once

#include <swegl/Projection/Vec3f.h>
#include <swegl/Projection/Vec2f.h>

namespace swegl
{

	struct ZInterpolator
	{
		Vec3f topalpha, topstep;
		Vec3f ualpha;
		float bottomalpha, bottomstep;

		enum MajorType
		{
			VERTICAL,
			HORIZONTAL
		};

		void Init(const MajorType & mt, const Vec3f & v0, const Vec3f & v1, const Vec2f & t0, const Vec2f & t1);
		inline void DisplaceStartingPoint(const float & move) {
			topalpha += topstep * move;
			bottomalpha += bottomstep * move;
			ualpha = topalpha;
			ualpha /= bottomalpha;
		}
		inline void Step() {
			topalpha += topstep;
			bottomalpha += bottomstep;
			ualpha = topalpha;
			ualpha /= bottomalpha;
		}
	};

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

}
