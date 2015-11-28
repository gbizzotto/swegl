
#pragma once

#include <vector>
#include <memory>

#include <swegl/Projection/Vec3f.h>
#include <swegl/Projection/Vec2f.h>
#include <swegl/Projection/Matrix4x4.h>
#include <swegl/Data/Texture.h>

namespace swegl
{

	class Mesh;

	class Strip
	{
		friend class Mesh;

	public:
		Strip(std::vector<unsigned int> && i);

		inline const std::size_t GetTriangleCount() const { return m_indexbuffer.size()>2 ? m_indexbuffer.size()-2 : 0; }
		inline const std::vector<std::pair<unsigned int,Vec3f>> GetIndexBuffer() const { return m_indexbuffer; }

	private:
		void CalculateNormals(const std::vector<std::pair<Vec3f,Vec2f>> & vertex_buffer);

	private:
		std::vector<std::pair<unsigned int,Vec3f>> m_indexbuffer;
	};


	class Mesh
	{
	public:
		typedef std::vector<std::pair<Vec3f, Vec2f>> VertexBuffer;

		Mesh(VertexBuffer && vb)
			:m_vertexbuffer(std::forward<VertexBuffer>(vb))
		{
			m_worldmatrix = Matrix4x4::Identity;
		}
		void SetVertexBuffer(VertexBuffer && v) { m_vertexbuffer = v; }
		void SetTexture(std::shared_ptr<Texture> & texture) { m_texture = texture; }
		void AddStrip(std::vector<unsigned int> && indexbuffer);
		void AddFan  (std::vector<unsigned int> && indexbuffer);

		inline const Matrix4x4                           & GetWorldMatrix()  const { return m_worldmatrix;  }
		inline       Matrix4x4                           & GetWorldMatrix()        { return m_worldmatrix;  }
		inline const std::vector<std::pair<Vec3f,Vec2f>> & GetVertexBuffer() const { return m_vertexbuffer; }
		inline const std::vector<Strip>                  & GetStrips()       const { return m_strips;       }
		inline const std::vector<Strip>                  & GetFans()         const { return m_fans;         }
		inline const std::shared_ptr<Texture>              GetTexture()      const { return m_texture;      }

	protected:
		std::vector<std::pair<Vec3f,Vec2f>> m_vertexbuffer;
		Matrix4x4 m_worldmatrix;
		std::shared_ptr<Texture> m_texture;
		std::shared_ptr<Texture> m_bumpmap;

		std::vector<Strip> m_strips;          // array of index buffers (triangle strips)
		std::vector<Strip> m_fans;            // array of index buffers (triangle strips)
	};

	Mesh MakeCube(float size, std::shared_ptr<Texture> & texture);
	Mesh MakeTore(unsigned int precision, std::shared_ptr<Texture> & texture);

	typedef std::vector<Mesh> Scene;
}
