
#include <swegl/Render/Renderer.h>
#include <swegl/Projection/MatrixStack.h>
#include <swegl/Render/Filler.h>

namespace swegl
{

	/*
	typedef struct
	{
		bool horizontal;
		bool highest; // for horizontal lines whether it should return the highest or the lowest x
		float error, deltaerr;
		int x, pmax, inc;
		bool firstline;

		float alpha, alphastep;
		Vec3f u0z0, u1z1;
		float invz0, invz1;
		Vec3f ualpha;
	
		Vec3f topalpha, topstep;
		float bottomalpha, bottomstep;

	} TexCustomLineData;
	int TexCustomLineGetCurrentXThenStep(TexCustomLineData & data);
	void TexInitCustomLine(TexCustomLineData & data, const Vec3f & v0, const Vec3f & v1,
				   const Vec3f & t0, const Vec3f & t1, bool highest);
	void TexCustomLineJumpSteps(TexCustomLineData & data, int steps);
	*/


	Renderer::Renderer(Scene *scene, Camera *camera, ViewPort *viewport)
	{
		this->m_scene = scene;
		this->m_camera = camera;
		this->m_viewport = viewport;
		m_zbuffer = new float[m_viewport->m_w*m_viewport->m_h];
	}


	void Renderer::Render()
	{
		/*
		m_viewport->Clear();
		MatrixStack mstackvertices;
		MatrixStack mstacknormals;

		// TODO: sort meshes

		mstackvertices.PushMatrix(m_camera->m_projectionmatrix);
		mstackvertices.PushMatrix(m_camera->m_viewmatrix);
		mstacknormals.PushMatrix(m_camera->m_viewmatrix);

		for (int i=m_viewport->m_w*m_viewport->m_h-1 ; i>=0 ; i--)
			m_zbuffer[i] = 9999.0f;

		for (int i=0 ; i<m_scene->m_meshCount ; i++)
		{
			const Mesh * mesh = m_scene->m_meshes[i];
			mstackvertices.PushMatrix(mesh->m_worldmatrix);
			mstacknormals.PushMatrix(mesh->m_worldmatrix);

			// Transforming vertices into screen coords
			Vec3f *vertices = new Vec3f[mesh->m_vertexcount];
			for (unsigned int v=0 ; v<mesh->m_vertexcount ; v++)
			{
				Vec3f vec = mesh->m_vertexbuffer[v];			
				vec = mstackvertices.GetTopMatrix() * vec;
				vec.x = vec.x / ( (vec.z>0) ? (vec.z) : (-vec.z) );
				vec.y = vec.y / ( (vec.z>0) ? (vec.z) : (-vec.z) );
				vertices[v] = m_viewport->m_viewportmatrix * vec;
			}
		
			// Transforming normals into screen consistent normals
			Matrix4x4 topmat = mstackvertices.GetTopMatrix();
			std::vector<Vec3f> worldviewnormals;
			worldviewnormals.reserve(mesh->m_stripnorms.size() + mesh->m_fannorms.size());
			//unsigned int triangle = 0;
			for (unsigned int n = 0 ; n < mesh->m_stripnorms.size() ; n++) {
				worldviewnormals.emplace_back(mesh->m_stripnorms[n].Mul3x3(topmat));
				worldviewnormals.back().Normalize();
			}
			for (unsigned int n = 0 ; n < mesh->m_fannorms.size() ; n++) {
				worldviewnormals.emplace_back(mesh->m_fannorms[n].Mul3x3(topmat));
				worldviewnormals.back().Normalize();
			}
		
			// Transform screen normals, to know the faces culling
			std::vector<Vec3f> normals;
			normals.reserve(mesh->m_stripnorms.size() + mesh->m_fannorms.size());
			// STRIPs' normals
			for (unsigned int s=0 ; s<mesh->m_stripnorms.size() ; s++)
			{
				const Strip & strip = mesh->m_strips[s];
				const Vec3f * v0 = &vertices[strip.m_indexbuffer[0]];
				const Vec3f * v1 = &vertices[strip.m_indexbuffer[1]];
				const Vec3f * v2;
				unsigned int tex_i=0;
				for (unsigned int i=2 ; i<strip.m_indexbuffer.size() ; i++, v0=v1, v1=v2, tex_i+=3)
				{
					v2 = & vertices[strip.m_indexbuffer[i]];
					// backface culling
					Vec3f cullingnormal;
					if ((i&0x1)==0)
						normals.emplace_back((*v1-*v0).Cross((*v2-*v0)));
					else
						normals.emplace_back((*v2-*v0).Cross((*v1-*v0)));
					normals.back().Normalize();
				}
			}
			// FANs' normals
			for (unsigned int s=0 ; s<mesh->m_fans.size() ; s++)
			{
				const Strip & fan = mesh->m_fans[s];
				const Vec3f * v0 = &vertices[fan.m_indexbuffer[0]];
				const Vec3f * v1 = &vertices[fan.m_indexbuffer[1]];
				const Vec3f * v2;
				unsigned int tex_i=0;
				for (unsigned int i=2 ; i<fan.m_indexbuffer.size() ; i++, v1=v2, tex_i+=3)
				{
					v2 = &vertices[fan.m_indexbuffer[i]];
					normals.emplace_back((*v1-*v0).Cross((*v2-*v0)));
					normals.back().Normalize();
				}
			}

			// STRIPS
			// Reading index buffer and drawing lines
			for (unsigned int s=0 ; s<mesh->m_strips.size() ; s++)
			{
				const Strip & strip = mesh->m_strips[s];
				const Vec3f * v0 = &vertices[strip.m_indexbuffer[0]];
				const Vec3f * v1 = &vertices[strip.m_indexbuffer[1]];
				const Vec3f * v2;
				unsigned int tex_i=0;
				for (unsigned int i=2 ; i<strip.m_indexbuffer.size() ; i++, v0=v1, v1=v2, tex_i+=3)
				{
					v2 = & vertices[strip.m_indexbuffer[i]];

					// frustum culling
					if (  (v0->x < m_viewport->m_x && v1->x <m_viewport->m_x && v2->x < m_viewport->m_x)
						||(v0->y < m_viewport->m_y && v1->y <m_viewport->m_y && v2->y < m_viewport->m_y)
						||(v0->x >= m_viewport->m_x+m_viewport->m_w && v1->x >= m_viewport->m_x+m_viewport->m_w && v2->x >= m_viewport->m_x+m_viewport->m_w)
						||(v0->y >= m_viewport->m_y+m_viewport->m_h && v1->y >= m_viewport->m_y+m_viewport->m_h && v2->y >= m_viewport->m_y+m_viewport->m_h)
					   )
					{
						continue;
					}
	
					// backface culling
					//Vec3f cullingnormal;
					//if ((i&0x1)==0)
					//	cullingnormal = ((v1-v0).Cross((v2-v0)));
					//else
					//	cullingnormal = ((v2-v0).Cross((v1-v0)));
					if (normals[triangle].z >= 0) // TODO dotproduct with camera ?
						continue;

					// Z-near culling
					if (v0->z < 0.001 && v1->z < 0.001 && v2->z < 0.001)
						continue;

					// Reading normal
					//Vec3f normal = normals[normal_i];

					// Reading neighbours' normals
					bool neighb0visible, neighb1visible, neighb2visible;
					if (mesh->m_stripneighbours[triangle*3] == (unsigned int) -1) {
						neighb0visible = false;
					} else {
						neighb0visible = normals[mesh->m_stripneighbours[triangle*3]].z < 0;
					}
					if (mesh->m_stripneighbours[triangle*3 +1] == (unsigned int) -1) {
						neighb1visible = false;
					} else {
						neighb1visible = normals[mesh->m_stripneighbours[triangle*3 +1]].z < 0;
					}
					if (mesh->m_stripneighbours[triangle*3 +2] == (unsigned int) -1) {
						neighb2visible = false;
					} else {
						neighb2visible = normals[mesh->m_stripneighbours[triangle*3 +2]].z < 0;
					}
					// Read texture coords
					Vec3f t0 = strip->m_texturebuffer[tex_i];
					t0.z = v0.z;
					Vec3f t1 = strip->m_texturebuffer[tex_i+1];
					t1.z = v1.z;
					Vec3f t2 = strip->m_texturebuffer[tex_i+2];
					t2.z = v2.z;

					// Fill poly
					Filler::FillPoly(v0, v1, v2,
									 t0, t1, t2,
									 neighb0visible, neighb1visible, neighb2visible, 
									 mesh->m_texture, mesh->m_bumpmap, m_viewport,
									 worldviewnormals[triangle], m_zbuffer);
					//f.FillPoly(normals[triangle], m_zbuffer);
				}
			}

			// FANS
			// Reading index buffer and drawing lines
			for (unsigned int s=0 ; s<mesh->m_fancount ; s++)
			{
				Strip *fan = mesh->m_fans[s];
				Vec3f v0,v1,v2;
				v0 = vertices[fan->m_indexbuffer[0]];
				v1 = vertices[fan->m_indexbuffer[1]];
				unsigned int tex_i=0;
				for (unsigned int i=2 ; i<fan->m_indexcount ; i++, v1=v2, tex_i+=3, triangle++)
				{
					v2 = vertices[fan->m_indexbuffer[i]];

					// frustum culling
					if (      (v0.x < m_viewport->m_x && v1.x <m_viewport->m_x && v2.x < m_viewport->m_x)
							||(v0.y < m_viewport->m_y && v1.y <m_viewport->m_y && v2.y < m_viewport->m_y)
							||(v0.x >= m_viewport->m_x+m_viewport->m_w  && v1.x >= m_viewport->m_x+m_viewport->m_w  && v2.x >= m_viewport->m_x+m_viewport->m_w)
							||(v0.y >= m_viewport->m_y+m_viewport->m_h && v1.y >= m_viewport->m_y+m_viewport->m_h && v2.y >= m_viewport->m_y+m_viewport->m_h)
					   )
					{
						continue;
					}
	
					// backface culling
					//Vec3f cullingnormal = ((v1-v0).Cross((v2-v0)));
					if (normals[triangle].z >= 0) // TODO dotproduct with camera ?
						continue;

					// Z-near culling
					if (v0.z < 0.001 && v1.z < 0.001 && v2.z < 0.001)
						continue;

					// Reading normal
					//Vec3f normal = normals[normal_i];

					// Reading neighbours' normals
					bool neighb0visible, neighb1visible, neighb2visible;
					if (mesh->m_fanneighbours[triangle*3] == (unsigned int) -1) {
						neighb0visible = false;
					} else {
						neighb0visible = normals[mesh->m_fanneighbours[triangle*3]].z < 0;
					}
					if (mesh->m_fanneighbours[triangle*3 +1] == (unsigned int) -1) {
						neighb1visible = false;
					} else {
						neighb1visible = normals[mesh->m_fanneighbours[triangle*3 +1]].z < 0;
					}
					if (mesh->m_fanneighbours[triangle*3 +2] == (unsigned int) -1) {
						neighb2visible = false;
					} else {
						neighb2visible = normals[mesh->m_fanneighbours[triangle*3 +2]].z < 0;
					}

					// Read texture coords
					Vec3f t0 = fan->m_texturebuffer[tex_i];
					t0.z = v0.z;
					Vec3f t1 = fan->m_texturebuffer[tex_i+1];
					t1.z = v1.z;
					Vec3f t2 = fan->m_texturebuffer[tex_i+2];
					t2.z = v2.z;

					// Fill poly
					Filler::FillPoly(v0, v1, v2,
									 t0, t1, t2,
									 neighb0visible, neighb1visible, neighb2visible,
									 mesh->m_texture, mesh->m_bumpmap, m_viewport,
									 worldviewnormals[triangle], m_zbuffer);
					//f.FillPoly(normals[triangle], m_zbuffer);
				}
			}
			mstackvertices.PopMatrix(); // Remove world matrix (the mesh one)
			mstacknormals.PopMatrix();
			delete [] vertices;
			delete [] normals;
			delete [] worldviewnormals;
		}
		*/
	}

}
