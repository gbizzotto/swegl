
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

	Mesh MakeCube(float size, std::shared_ptr<Texture> & texture)
	{
		Mesh::VertexBuffer vb;
		vb.reserve(8);
		vb.emplace_back(std::make_pair<>(Vec3f(-size / 2.0f, -size / 2.0f, -size / 2.0f), Vec2f((float)0, (float)texture->m_mipmaps[0].m_height)));
		vb.emplace_back(std::make_pair<>(Vec3f(size / 2.0f, -size / 2.0f, -size / 2.0f), Vec2f((float)texture->m_mipmaps[0].m_width, (float)texture->m_mipmaps[0].m_height)));
		vb.emplace_back(std::make_pair<>(Vec3f(-size / 2.0f, size / 2.0f, -size / 2.0f), Vec2f((float)0, (float)0)));
		vb.emplace_back(std::make_pair<>(Vec3f(size / 2.0f, size / 2.0f, -size / 2.0f), Vec2f((float)texture->m_mipmaps[0].m_width, (float)0)));
		vb.emplace_back(std::make_pair<>(Vec3f(-size / 2.0f, -size / 2.0f, size / 2.0f), Vec2f((float)texture->m_mipmaps[0].m_width, (float)texture->m_mipmaps[0].m_height)));
		vb.emplace_back(std::make_pair<>(Vec3f(size / 2.0f, -size / 2.0f, size / 2.0f), Vec2f((float)0, (float)texture->m_mipmaps[0].m_height)));
		vb.emplace_back(std::make_pair<>(Vec3f(-size / 2.0f, size / 2.0f, size / 2.0f), Vec2f((float)texture->m_mipmaps[0].m_width, (float)0)));
		vb.emplace_back(std::make_pair<>(Vec3f(size / 2.0f, size / 2.0f, size / 2.0f), Vec2f((float)0, (float)0)));
		
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

		Mesh result(std::move(vb));
		result.AddFan(std::move<>(ib1));
		result.AddFan(std::move<>(ib2));

		result.SetTexture(texture);
		return result;
	}

	Mesh MakeTore(unsigned int precision, std::shared_ptr<Texture> & texture)
	{
		Mesh::VertexBuffer vb;
		vb.reserve((precision + 1)*(precision + 1));

		float angle = (2 * 3.141592653589f) / precision;
		Matrix4x4 big;
		big.SetIdentity();
		big.Translate(2.0f, 0.0f, 0.0f);

		for (unsigned int bg = 0; bg <= precision; bg++)
		{
			Matrix4x4 small;
			small.SetIdentity();
			small.Translate(0.8f, 0.0f, 0.0f);

			for (unsigned int sm = 0; sm <= precision; sm++)
			{
				Vec3f v;
				v = small * v;
				v = big * v;
				vb.emplace_back(std::make_pair<>(v, Vec2f(texture->m_mipmaps[0].m_width*(float)bg / precision, texture->m_mipmaps[0].m_height*(float)sm / precision)));
				small.RotateZ(angle);
			}

			big.RotateY(angle);
		}

		Mesh result(std::move(vb));

		for (unsigned int bg = 0; bg<precision; bg++)
		{
			std::vector<unsigned int> ib;
			ib.reserve((precision)* 2 + 2);

			ib.emplace_back(bg   *(precision + 1));
			ib.emplace_back((bg + 1)*(precision + 1));
			for (unsigned int i = 1; i <= precision; i++)
			{
				ib.emplace_back(bg   *(precision + 1) + i);
				ib.emplace_back((bg + 1)*(precision + 1) + i);
			}
			result.AddStrip(std::move<>(ib));
		}

		result.SetTexture(texture);
		return result;
	}
}

