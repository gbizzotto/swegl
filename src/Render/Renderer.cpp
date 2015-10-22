
#include <thread>
#include <mutex>
#include <cmath>
#include <swegl/Render/Renderer.h>
#include <swegl/Render/Filler.h>
#include <swegl/Projection/Vec2f.h>

namespace swegl
{

	struct PolyToFill
	{
		Vec3f v0, v1, v2;
		Vec2f t0, t1, t2;
		const std::shared_ptr<Texture> texture;
		ViewPort * viewport;
		unsigned char shade;
		float * zbuffer;

		PolyToFill(Vec3f v0, Vec3f v1, Vec3f v2,
		           Vec2f t0, Vec2f t1, Vec2f t2,
		           const std::shared_ptr<swegl::Texture> & tex,
		           unsigned char sh)
			:v0(v0), v1(v1), v2(v2), t0(t0), t1(t1), t2(t2), texture(tex), shade(sh)
			{}

	};
	

	Renderer::Renderer(Scene & scene, Camera *camera, ViewPort *viewport)
		: m_scene(scene)
		, m_camera(camera)
		, m_viewport(viewport)
	{
		m_zbuffer = new float[m_viewport->m_w*m_viewport->m_h];
	}

	void Renderer::Render()
	{
		auto concurrency = std::thread::hardware_concurrency();

		m_viewport->Clear();
		std::vector<Matrix4x4> mstackvertices(1, Matrix4x4::Identity);
		std::vector<Matrix4x4> mstacknormals(1, Matrix4x4::Identity);

		// TODO: sort meshes

		mstackvertices.push_back(mstackvertices.back() * m_camera->m_projectionmatrix);
		mstackvertices.push_back(mstackvertices.back() * m_camera->m_viewmatrix);
		mstacknormals.push_back(mstacknormals.back() * m_camera->m_viewmatrix);

		for (int i=m_viewport->m_w*m_viewport->m_h-1 ; i>=0 ; i--)
			m_zbuffer[i] = std::numeric_limits<float>::max();

		for (size_t i=0 ; i<m_scene.size() ; i++)
		{
			const Mesh & mesh = m_scene[i];
			mstackvertices.push_back(mstackvertices.back() * mesh.GetWorldMatrix());
			mstacknormals.push_back(mstacknormals.back() * mesh.GetWorldMatrix());

			// Transforming vertices into screen coords
			std::vector<std::pair<Vec3f,Vec2f>> vertices = mesh.GetVertexBuffer();
			auto vertex_transformer = [&](std::vector<std::pair<Vec3f,Vec2f>>::iterator it,
			                              std::vector<std::pair<Vec3f,Vec2f>>::iterator end)
				{
					for ( ; it!=end ; ++it)
					{
						Vec3f vec = (mstackvertices.empty()?Matrix4x4::Identity:mstackvertices.back()) * it->first;
						vec.x /= fabs(vec.z);
						vec.y /= fabs(vec.z);
						it->first = m_viewport->m_viewportmatrix * vec;
					}
				};
			std::vector<std::thread> vertex_transformer_aux_threads;
			vertex_transformer_aux_threads.reserve(concurrency - 1);
			for (size_t i=0 ; i<concurrency-1 ; ++i)
			{
				vertex_transformer_aux_threads.emplace_back([concurrency, i, &vertices, &vertex_transformer, mesh]()
					{
						vertex_transformer(vertices.begin() + (vertices.size() / concurrency) * i,
							               vertices.begin() + (vertices.size() / concurrency) * (i+1));
					});
			}
			vertex_transformer(vertices.end() - vertices.size() / concurrency,
			                   vertices.end());
			for (auto it=vertex_transformer_aux_threads.begin(),end=vertex_transformer_aux_threads.end() ; it!=end ; ++it)
				it->join();


			// Storing polys info to process later
			std::vector<std::vector<PolyToFill>> polys_to_fill(concurrency);

			// STRIPS
			// Reading index buffer and drawing lines
			{
				auto strip_renderer = [&](std::vector<Strip>::const_iterator it,
				                          std::vector<Strip>::const_iterator end,
				                          std::vector<PolyToFill> & polys)
					{
						for ( ; it!=end ; ++it)
						{
							const Strip & strip = *it;
							const Vec3f * v2;
							const Vec3f * v0 = &vertices[strip.GetIndexBuffer()[0].first].first;
							const Vec3f * v1 = &vertices[strip.GetIndexBuffer()[1].first].first;
							const Vec2f * t2;
							const Vec2f * t0 = &vertices[strip.GetIndexBuffer()[0].first].second;
							const Vec2f * t1 = &vertices[strip.GetIndexBuffer()[1].first].second;
							for (unsigned int i=2 ; i<strip.GetIndexBuffer().size() ; i++, v0=v1, v1=v2, t0=t1, t1=t2)
							{
								v2 = &vertices[strip.GetIndexBuffer()[i].first].first;
								t2 = &vertices[strip.GetIndexBuffer()[i].first].second;

								// frustum culling
								if (      (v0->x < m_viewport->m_x && v1->x <m_viewport->m_x && v2->x < m_viewport->m_x)
										||(v0->y < m_viewport->m_y && v1->y <m_viewport->m_y && v2->y < m_viewport->m_y)
										||(v0->x >= m_viewport->m_x+m_viewport->m_w && v1->x >= m_viewport->m_x+m_viewport->m_w && v2->x >= m_viewport->m_x+m_viewport->m_w)
										||(v0->y >= m_viewport->m_y+m_viewport->m_h && v1->y >= m_viewport->m_y+m_viewport->m_h && v2->y >= m_viewport->m_y+m_viewport->m_h)
								   )
								{
									continue;
								}
	
								// backface culling
								Vec3f cullingnormal;
								if ((i&0x1)==0)
									cullingnormal = ((*v1-*v0).Cross((*v2-*v0)));
								else
									cullingnormal = ((*v2-*v0).Cross((*v1-*v0)));
								if (cullingnormal.z >= 0) // TODO dotproduct with camera ?
									continue;

								// Z-near culling
								if (v0->z < 0.001 && v1->z < 0.001 && v2->z < 0.001)
									continue;
							
								// Fill poly
								//F001CustomNoArtefact::FillPoly(*v0, *v1, *v2, *t0, *t1, *t2, mesh->GetTexture(), m_viewport, 255, m_zbuffer);
								polys.emplace_back(*v0, *v1, *v2, *t0, *t1, *t2, mesh.GetTexture(), 255);
							}
						}
					};
				std::vector<std::thread> aux_threads;
				aux_threads.reserve(concurrency-1);

				for (size_t i=0 ; i<concurrency-1 ; ++i)
				{
					aux_threads.emplace_back([i, concurrency, &strip_renderer, &mesh, &polys_to_fill]()
						{
							strip_renderer(mesh.GetStrips().begin() + mesh.GetStrips().size() / concurrency * i,
							               mesh.GetStrips().begin() + mesh.GetStrips().size() / concurrency * (i+1),
							               polys_to_fill[i]);
						});
				}
				strip_renderer(mesh.GetStrips().end() - mesh.GetStrips().size() / concurrency,
				               mesh.GetStrips().end(),
				               polys_to_fill.back());

				for (auto it=aux_threads.begin(),end=aux_threads.end() ; it!=end ; ++it)
					it->join();
			}

			// FANS
			// Reading index buffer and drawing lines
			{
				auto fan_renderer = [&](std::vector<Strip>::const_iterator it,
				                        std::vector<Strip>::const_iterator end,
				                        std::vector<PolyToFill> & polys)
					{
						for ( ; it!=end ; ++it)
						{
							const Strip & fan = *it;
							const Vec3f * v2;
							const Vec3f * v0 = &vertices[fan.GetIndexBuffer()[0].first].first;
							const Vec3f * v1 = &vertices[fan.GetIndexBuffer()[1].first].first;
							const Vec2f * t2;
							const Vec2f * t0 = &vertices[fan.GetIndexBuffer()[0].first].second;
							const Vec2f * t1 = &vertices[fan.GetIndexBuffer()[1].first].second;
							for (unsigned int i=2 ; i<fan.GetIndexBuffer().size() ; i++, v1=v2, t1=t2)
							{
								v2 = &vertices[fan.GetIndexBuffer()[i].first].first;
								t2 = &vertices[fan.GetIndexBuffer()[i].first].second;

								// frustum culling
								if (  (v0->x <  m_viewport->m_x                 && v1->x <  m_viewport->m_x                 && v2->x <  m_viewport->m_x)
									||(v0->y <  m_viewport->m_y                 && v1->y <  m_viewport->m_y                 && v2->y <  m_viewport->m_y)
									||(v0->x >= m_viewport->m_x+m_viewport->m_w && v1->x >= m_viewport->m_x+m_viewport->m_w && v2->x >= m_viewport->m_x+m_viewport->m_w)
									||(v0->y >= m_viewport->m_y+m_viewport->m_h && v1->y >= m_viewport->m_y+m_viewport->m_h && v2->y >= m_viewport->m_y+m_viewport->m_h)
								   )
								{
									continue;
								}
	
								// backface culling
								Vec3f cullingnormal = ((*v1-*v0).Cross((*v2-*v0)));
								if (cullingnormal.z >= 0) // TODO dotproduct with camera ?
									continue;

								// Z-near culling
								if (v0->z < 0.001 && v1->z < 0.001 && v2->z < 0.001)
									continue;

								// Fill poly
								//F001CustomNoArtefact::FillPoly(*v0, *v1, *v2, *t0, *t1, *t2, mesh->GetTexture(), m_viewport, 255, m_zbuffer);
								polys.emplace_back(*v0, *v1, *v2, *t0, *t1, *t2, mesh.GetTexture(), 255);
							}
						}
					};
				std::vector<std::thread> aux_threads;
				aux_threads.reserve(concurrency-1);

				for (size_t i=0 ; i<concurrency-1 ; ++i)
				{
					aux_threads.emplace_back([i, concurrency, &fan_renderer, &mesh, &polys_to_fill]()
						{
							fan_renderer(mesh.GetFans().begin() + mesh.GetFans().size() / concurrency * i,
							             mesh.GetFans().begin() + mesh.GetFans().size() / concurrency * (i+1),
							             polys_to_fill[i]);
						});
				}
				fan_renderer(mesh.GetFans().end() - mesh.GetFans().size() / concurrency,
				             mesh.GetFans().end(),
				             polys_to_fill.back());

				for (auto it=aux_threads.begin(),end=aux_threads.end() ; it!=end ; ++it)
					it->join();
			}

			// actually processing the polys
			for (auto it=polys_to_fill.begin(), end=polys_to_fill.end() ; it!=end ; ++it)
				for (auto it2=it->begin(), end2=it->end() ; it2!=end2 ; ++it2)
					Filler::FillPoly(it2->v0, it2->v1, it2->v2,
					                 it2->t0, it2->t1, it2->t2,
					                 it2->texture, m_viewport, it2->shade, m_zbuffer);


			mstackvertices.pop_back(); // Remove world matrix (the mesh one)
			mstacknormals.pop_back();
		}
	}

}
