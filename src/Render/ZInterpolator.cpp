
#include <swegl/Render/ZInterpolator.h>
#include <swegl/Projection/Vec2f.h>
#include <swegl/Projection/Vec3f.h>

namespace swegl
{

	void ZInterpolator::Init(const MajorType & mt, const Vec3f & v0, const Vec3f & v1, const Vec2f & t0, const Vec2f & t1)
	{
		//alpha = 0.0f;
		float alphastep;
		if (mt == VERTICAL) {
			alphastep = 1.0f / (v1[0][1]-v0[0][1]); // always v1.y != v0.y
		} else {
			//if ((v1.x-v0.x) != 0)
				alphastep = 1.0f / (v1[0][0]-v0[0][0]); // always v1.x != v0.x
			//else
			//	alphastep = 0.00001f;
		}
		bottomalpha = 1.0f / v0[0][2];
		float invz1 = 1.0f / v1[0][2];
		ualpha[0][0] = t0[0][0];
		ualpha[0][1] = t0[0][1];
		ualpha[0][2] = v0[0][2];
		topalpha = ualpha;
		topalpha *= bottomalpha;
		topstep[0][0] = (t1[0][0] * invz1 - topalpha[0][0]) * alphastep;
		topstep[0][1] = (t1[0][1] * invz1 - topalpha[0][1]) * alphastep;
		topstep[0][2] = (v1[0][2] * invz1 - topalpha[0][2]) * alphastep;
		bottomstep = (invz1-bottomalpha)*alphastep;

		/*
		Vec3f u0z0(t0[0][0], t0[0][1], v0[0][2]);
		u0z0 /= v0[0][2];//((v0.z>0)?v0.z:-v0.z); // TODO: Find a solution that works for case when z<0
		Vec3f u1z1(t1[0][0], t1[0][1], v1[0][2]);
		u1z1 /= v1[0][2];//((v1.z>0)?v1.z:-v1.z); // TODO: Better frustum culling will do (cut faces down to frustum)
		float invz0 = 1.0f / v0[0][2];//((v0.z>0)?v0.z:-v0.z);
		float invz1 = 1.0f / v1[0][2];//((v1.z>0)?v1.z:-v1.z);

		topalpha = u0z0;
		bottomalpha = invz0;
		ualpha = topalpha;
		ualpha /= bottomalpha;
		topstep = u1z1;
		topstep -= u0z0;
		topstep *= alphastep;
		bottomstep = (invz1-invz0)*alphastep;
		*/
	}

}
