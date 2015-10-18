
#ifndef SWE_R008NoTexelArtefact
#define SWE_R008NoTexelArtefact

#include <swegl/Render/R000Virtual.h>

namespace swegl
{

	/**
	 * Texturing Renderer with Bilinear Filtering, without texture mapping artefacts
	 */
	class R008NoTexelArtefact : public R000Virtual
	{
	public:
		float *m_zbuffer;
		R008NoTexelArtefact(Scene *scene, Camera *camera, ViewPort *viewport);

		void Render();
		void FillPoly(const Vec3f & v0, const Vec3f & v1, const Vec3f & v2,
					  Vec3f t0, Vec3f t1, Vec3f t2, Texture *t, float shade);
		void FillOrderedPoly(Vec3f v0, Vec3f  v1, Vec3f v2, Vec3f t0, Vec3f  t1, Vec3f t2, Texture *t, float shade);
	};

}

#endif // SWE_R008NoTexelArtefact
