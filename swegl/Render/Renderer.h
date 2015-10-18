
#ifndef SWE_RENDERER
#define SWE_RENDERER

#include <swegl/Projection/Vec3f.h>
#include <swegl/Data/Texture.h>
#include <swegl/Data/Scene.h>
#include <swegl/Projection/Camera.h>
#include <swegl/Render/ViewPort.h>

namespace swegl
{

	class Renderer
	{
	private:
		void FillPoly(const Vec3f & v0, const Vec3f & v1, const Vec3f & v2,
					  Vec3f t0, Vec3f t1, Vec3f t2, Texture *t, float shade);
		void FillOrderedPoly(Vec3f v0, Vec3f  v1, Vec3f v2, Vec3f t0, Vec3f  t1, Vec3f t2, Texture *t, float shade);

	public:
		Scene *m_scene;
		Camera *m_camera;
		ViewPort *m_viewport;
		float *m_zbuffer;

		Renderer(Scene *scene, Camera *camera, ViewPort *viewport);

		void Render();
	};

}

#endif // SWE_RENDERER
