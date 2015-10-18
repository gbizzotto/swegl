
#include <swegl/Render/R000Virtual.h>

namespace swegl
{

	R000Virtual::R000Virtual(Scene *scene, Camera *camera, ViewPort *viewport)
	{
		this->m_scene = scene;
		this->m_camera = camera;
		this->m_viewport = viewport;
	}

}
