
#include <stdlib.h>
#include <memory.h>
#include <swegl/Data/Mesh.h>
#include <swegl/Projection/Vec3f.h>
#include <swegl/Projection/Matrix4x4.h>

namespace swegl
{

	Strip::Strip(std::vector<unsigned int> && i)
	{
		m_indexbuffer.reserve(i.size());
		for (auto it=i.begin(),end=i.end() ; it!=end ; ++it)
			m_indexbuffer.push_back(std::make_pair<>(*it,Vec3f()));
	}

	void Strip::CalculateNormals(const std::vector<std::pair<Vec3f,Vec2f>> & vertex_buffer)
	{
		// Preca normals for strips
		unsigned int vindex0, vindex1, vindex2;
		const Vec3f * v0 = &vertex_buffer[m_indexbuffer[0].first].first;
		const Vec3f * v1 = &vertex_buffer[m_indexbuffer[1].first].first;
		vindex0 = m_indexbuffer[0].first;
		vindex1 = m_indexbuffer[1].first;
		const Vec3f * v2;
		for (unsigned int i=2 ; i<m_indexbuffer.size() ; i++, v0=v1, v1=v2, vindex0=vindex1, vindex1=vindex2)
		{
			v2 = &vertex_buffer[m_indexbuffer[i].first].first;
			vindex2 = m_indexbuffer[i].first;
			
			m_indexbuffer[i].second = ((i&0x1)==0) ? ((*v2-*v0).Cross(*v1-*v0)) : ((*v1-*v0).Cross(*v2-*v0));
			m_indexbuffer[i].second.Normalize();
		}
	}

	void Mesh::AddStrip(std::vector<unsigned int> && indexbuffer)	
	{
		m_strips.emplace_back(Strip(std::move<>(indexbuffer)));
		m_strips.back().CalculateNormals(m_vertexbuffer);
	}
	void Mesh::AddFan(std::vector<unsigned int> && indexbuffer)
	{
		m_fans.emplace_back(Strip(std::move<>(indexbuffer)));
		m_fans.back().CalculateNormals(m_vertexbuffer);
	}

}

