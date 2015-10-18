
#ifndef SWE_F001CUSTOMNOARTEFACT
#define SWE_F001CUSTOMNOARTEFACT

#include <swegl/Projection/Vec3f.h>
#include <swegl/Data/Texture.h>
#include <swegl/Render/ViewPort.h>

namespace swegl
{

	/**
	 * Poly Filler withour texture artefacts.
	 */
	class F001CustomNoArtefact
	{
	public:
		static void FillPoly(const Vec3f & v0, const Vec3f & v1, const Vec3f & v2,
							 const Vec2f & t0, const Vec2f & t1, const Vec2f & t2,
							 const Texture * t, ViewPort * vp, unsigned char shade, float * zbuffer);
	};

}

#endif // SWE_F001CUSTOMNOARTEFACT
