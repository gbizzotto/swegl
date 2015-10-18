
#include <swegl/Data/RectangleTriangle.h>
#include <swegl/Projection/Vec2f.h>
#include <swegl/Projection/Vec3f.h>
#include <swegl/Data/Texture.h>

namespace swegl
{
/*
	RectangleTriangle::RectangleTriangle(float xlen, float ylen, Texture *t)
		:Mesh::Mesh(t, 0)
	{
		this->m_texture = t;
		this->m_vertexcount = 3;
		this->m_vertexbuffer = new Vec3f[3];
		m_vertexbuffer[0].x=0.0f; m_vertexbuffer[0].y=0.0f; m_vertexbuffer[0].z=0.0f;
		m_vertexbuffer[1].x=xlen; m_vertexbuffer[1].y=0.0f; m_vertexbuffer[1].z=0.0f;
		m_vertexbuffer[2].x=0.0f; m_vertexbuffer[2].y=ylen; m_vertexbuffer[2].z=0.0f;
		unsigned int ib1[] = {0,1,2};
	
		Vec2f t0, t1, t2, t3;
		t0.x= 0.01f;                         t0.y=-0.01f+t->m_mipmaps[0].m_height;
		t1.x=-0.01f+t->m_mipmaps[0].m_width; t1.y=-0.01f+t->m_mipmaps[0].m_height;
		t2.x= 0.01f;                         t2.y= 0.01f;
		t3.x=-0.01f+t->m_mipmaps[0].m_width; t3.y= 0.01f;

		Vec2f tb1[3];
		tb1[0]=t0; tb1[1]=t1; tb1[2]=t2;
		AddFan(ib1, tb1, sizeof(ib1)/sizeof(ib1[0]));
	}*/

}
