
#include <swegl/Data/Cube.h>
#include <swegl/Projection/Vec2f.h>
#include <swegl/Projection/Vec3f.h>
#include <swegl/Data/Texture.h>

namespace swegl
{

	Cube::Cube(float size, Texture *t)
		:Mesh(t, nullptr)
	{
		this->m_vertexbuffer.reserve(8);
		m_vertexbuffer.emplace_back(std::make_pair<>(Vec3f(-size/2.0f, -size/2.0f, -size/2.0f), Vec2f((float)0                      , (float)t->m_mipmaps[0].m_height)));
		m_vertexbuffer.emplace_back(std::make_pair<>(Vec3f( size/2.0f, -size/2.0f, -size/2.0f), Vec2f((float)t->m_mipmaps[0].m_width, (float)t->m_mipmaps[0].m_height)));
		m_vertexbuffer.emplace_back(std::make_pair<>(Vec3f(-size/2.0f,  size/2.0f, -size/2.0f), Vec2f((float)0                      , (float)0                       )));
		m_vertexbuffer.emplace_back(std::make_pair<>(Vec3f( size/2.0f,  size/2.0f, -size/2.0f), Vec2f((float)t->m_mipmaps[0].m_width, (float)0                       )));
		m_vertexbuffer.emplace_back(std::make_pair<>(Vec3f(-size/2.0f, -size/2.0f,  size/2.0f), Vec2f((float)t->m_mipmaps[0].m_width, (float)t->m_mipmaps[0].m_height)));
		m_vertexbuffer.emplace_back(std::make_pair<>(Vec3f( size/2.0f, -size/2.0f,  size/2.0f), Vec2f((float)0                      , (float)t->m_mipmaps[0].m_height)));
		m_vertexbuffer.emplace_back(std::make_pair<>(Vec3f(-size/2.0f,  size/2.0f,  size/2.0f), Vec2f((float)t->m_mipmaps[0].m_width, (float)0                       )));
		m_vertexbuffer.emplace_back(std::make_pair<>(Vec3f( size/2.0f,  size/2.0f,  size/2.0f), Vec2f((float)0                      , (float)0                       )));
	
		std::vector<unsigned int> ib1;
		ib1.reserve(8);
		ib1.push_back(0);
		ib1.push_back(1);
		ib1.push_back(3);
		ib1.push_back(2);
		ib1.push_back(6);
		ib1.push_back(4);
		ib1.push_back(5);
		ib1.push_back(1);
		AddFan(std::move<>(ib1));
	//*
		std::vector<unsigned int> ib2;
		ib2.reserve(8);
		ib2.push_back(7);
		ib2.push_back(6);
		ib2.push_back(2);
		ib2.push_back(3);
		ib2.push_back(1);
		ib2.push_back(5);
		ib2.push_back(4);
		ib2.push_back(6);
		AddFan(std::move<>(ib2));
	/**/
	}

}
