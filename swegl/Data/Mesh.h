
#ifndef SWE_MESH
#define SWE_MESH

#include <vector>

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
		inline Mesh(Texture * t, Texture * b)
			:m_texture(t),
			m_bumpmap(b)
		{
			this->m_worldmatrix.SetIdentity();
		}
		inline void SetVertexBuffer(std::vector<std::pair<Vec3f,Vec2f>> && v) { m_vertexbuffer = v; }
		void AddStrip(std::vector<unsigned int> && indexbuffer);
		void AddFan  (std::vector<unsigned int> && indexbuffer);

		inline const Matrix4x4                           & GetWorldMatrix()  const { return m_worldmatrix;  }
		inline       Matrix4x4                           & GetWorldMatrix()        { return m_worldmatrix;  }
		inline const std::vector<std::pair<Vec3f,Vec2f>> & GetVertexBuffer() const { return m_vertexbuffer; }
		inline const std::vector<Strip>                  & GetStrips()       const { return m_strips;       }
		inline const std::vector<Strip>                  & GetFans()         const { return m_fans;         }
		inline const Texture * const                       GetTexture()      const { return m_texture;      }

	protected:
		std::vector<std::pair<Vec3f,Vec2f>> m_vertexbuffer;
		Matrix4x4 m_worldmatrix;
		Texture *m_texture;
		Texture *m_bumpmap;

		std::vector<Strip> m_strips;          // array of index buffers (triangle strips)
		std::vector<Strip> m_fans;            // array of index buffers (triangle strips)
	};

}

#endif // SWE_MESH
