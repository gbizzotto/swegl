
#ifndef SWE_CUBE
#define SWE_CUBE

#include <swegl/Data/Mesh.h>
#include <swegl/Data/Texture.h>

namespace swegl
{

	class Cube : public Mesh
	{
	public:
		Cube(float size, Texture *t);
	};

}

#endif // SWE_CUBE
