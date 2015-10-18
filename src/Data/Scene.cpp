
#include <swegl/Data/Scene.h>

namespace swegl
{

	Scene::Scene()
	{
		m_meshCount = 0;
	}

	void Scene::AddMesh(Mesh *m)
	{
		this->m_meshes[this->m_meshCount] = m;
		this->m_meshCount++;
	}

}
