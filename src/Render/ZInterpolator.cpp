
#include <swegl/Render/ZInterpolator.h>
#include <swegl/Projection/Vec2f.h>
#include <swegl/Projection/Vec3f.h>

namespace swegl
{

	ZInterpolator::ZInterpolator() {}

	void ZInterpolator::Init(const MajorType & mt, const Vec3f & v0, const Vec3f & v1, const Vec2f & t0, const Vec2f & t1)
	{
		alpha = 0.0f;
		if (mt == VERTICAL) {
			alphastep = 1.0f / (v1.y-v0.y); // always v1.y != v0.y
		} else {
			//if ((v1.x-v0.x) != 0)
				alphastep = 1.0f / (v1.x-v0.x); // always v1.x != v0.x
			//else
			//	alphastep = 0.00001f;
		}
		Vec3f u0z0 = Vec3f(t0.x, t0.y, v0.z) / v0.z/*((v0.z>0)?v0.z:-v0.z)*/; // TODO: Find a solution that works for case when z<0
		Vec3f u1z1 = Vec3f(t1.x, t1.y, v1.z) / v1.z/*((v1.z>0)?v1.z:-v1.z)*/; // TODO: Better frustum culling will do (cut faces down to frustum)
		float invz0 = 1.0f / v0.z/*((v0.z>0)?v0.z:-v0.z)*/;
		float invz1 = 1.0f / v1.z/*((v1.z>0)?v1.z:-v1.z)*/;
		
		topalpha = u0z0;
		bottomalpha = invz0;
		ualpha = topalpha / bottomalpha;
		topstep = (u1z1-u0z0)*alphastep;
		bottomstep = (invz1-invz0)*alphastep;
	}

}
