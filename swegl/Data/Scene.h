
#ifndef SWE_SCENE
#define SWE_SCENE

#include <swegl/Data/Mesh.h>
#include <swegl/Projection/Matrix4x4.h>

namespace swegl
{

	class Scene
	{
	public:
		Mesh *m_meshes[10000];
		int m_meshCount;

		Scene();
		void AddMesh(Mesh *m);
	};

}

#endif // SWE_SCENE
