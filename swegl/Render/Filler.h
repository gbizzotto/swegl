
#pragma once

#include <swegl/Projection/Vec3f.h>
#include <swegl/Data/Texture.h>
#include <swegl/Render/ViewPort.h>
#include <swegl/Data/Texture.h>

namespace swegl
{

	/**
	 * Poly Filler withour texture artefacts.
	 */
	class Filler
	{
	public:
		static void FillPoly(const Vec3f & v0, const Vec3f & v1, const Vec3f & v2,
							 const Vec2f & t0, const Vec2f & t1, const Vec2f & t2,
							 const std::shared_ptr<Texture> & t, ViewPort * vp, unsigned char shade, float * zbuffer);
	};

}
