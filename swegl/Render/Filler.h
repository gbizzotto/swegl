
#ifndef SWE_FILLER
#define SWE_FILLER

#include <swegl/Projection/Vec3f.h>
#include <swegl/Data/Texture.h>
#include <swegl/Render/ViewPort.h>

namespace swegl
{

	class Filler
	{
	public:
		static void FillPoly(const Vec3f & v0, const Vec3f & v1, const Vec3f & v2,
							 const Vec2f & t0, const Vec2f & t1, const Vec2f & t2,
							 bool neighb0visible, bool neighb1visible, bool neighb2visible, 
							 Texture *t, Texture *tb, ViewPort * vp,
							 const Vec3f & facenormal, float *zbuffer);
	};

}

#endif // SWE_FILLER
