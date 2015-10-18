
#ifndef SWE_TORE
#define SWE_TORE

#include <swegl/Data/Mesh.h>
#include <swegl/Data/Texture.h>

namespace swegl
{

	class Tore : public Mesh
	{
	public:
		Tore(unsigned int precision, Texture *t);
	};

}

#endif // SWE_TORE
