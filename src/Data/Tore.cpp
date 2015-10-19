
#include <swegl/Data/Tore.h>
#include <swegl/Data/Texture.h>
#include <swegl/Projection/Vec3f.h>
#include <swegl/Projection/Vec2f.h>

namespace swegl
{

	Tore::Tore(unsigned int precision, Texture *t)
		:Mesh(t, nullptr)
	{
		this->m_vertexbuffer.reserve((precision+1)*(precision+1));

		float angle = (2*3.141592653589f) / precision;
		Matrix4x4 big;
		big.SetIdentity();		
		big.Translate(2.0f, 0.0f, 0.0f);
		
		for (unsigned int bg=0 ; bg<=precision ; bg++)
		{
			Matrix4x4 small;
			small.SetIdentity();
			small.Translate(0.8f, 0.0f, 0.0f);

			for (unsigned int sm=0 ; sm<=precision ; sm++)
			{
				Vec3f v;
				v = small * v;
				v = big * v;
				m_vertexbuffer.emplace_back(std::make_pair<>(v, Vec2f(t->m_mipmaps[0].m_width*(float)bg/precision, t->m_mipmaps[0].m_height*(float)sm/precision)));
				small.RotateZ(angle);
			}

			big.RotateY(angle);
		}

		for (unsigned int bg=0 ; bg<precision ; bg++)
		{
			std::vector<unsigned int> ib;
			ib.reserve((precision)*2 + 2);

			ib.emplace_back (bg   *(precision+1));
			ib.emplace_back((bg+1)*(precision+1));
			for (unsigned int i=1 ; i<=precision ; i++)
			{
				ib.emplace_back( bg   *(precision+1) + i);
				ib.emplace_back((bg+1)*(precision+1) + i);
			}
			this->AddStrip(std::move<>(ib));
		}
	}

}
