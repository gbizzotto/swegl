
#ifndef SWE_R000Virtual
#define SWE_R000Virtual

#include <swegl/Projection/Camera.h>
#include <swegl/Data/Scene.h>
#include <swegl/Render/ViewPort.h>

namespace swegl
{

	/**
	 * Default Renderer
	 * Straight, no projection, only points
	 */
	class R000Virtual
	{
	public:
		Scene *m_scene;
		Camera *m_camera;
		ViewPort *m_viewport;

		R000Virtual(Scene *scene, Camera *camera, ViewPort *viewport);

		virtual void Render() = 0;
	};

}

#endif // SWE_R000Virtual
