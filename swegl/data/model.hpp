
#pragma once

#include <swegl/projection/camera.hpp>
#include <swegl/projection/vec2f.hpp>
#include <swegl/projection/points.hpp>
#include <swegl/projection/matrix44.hpp>
#include <swegl/data/texture.hpp>

namespace swegl
{

class vertex_shader_t;
class pixel_shader_t;

using vertex_idx = std::uint32_t;

struct triangle_strip
{
	std::vector<vertex_idx> indices;
};
struct triangle_fan
{
	std::vector<vertex_idx> indices;
};
struct triangle_list_t
{
	std::vector<vertex_idx> indices;
};

struct mesh_vertex_t
{
	vertex_t v;
	vec2f_t tex_coords;
	normal_t normal;
};

struct mesh_t
{
	std::vector<mesh_vertex_t> vertices;
	std::vector<mesh_vertex_t> vertices_world;    // mesh_vertices after transformations into world coordinates
	std::vector<mesh_vertex_t> vertices_viewport; // mesh_vertices after transformations into viewport coordinates (pixel x,y + z depth
	std::vector<triangle_strip> triangle_strips;
	std::vector<triangle_fan>   triangle_fans;
	triangle_list_t             triangle_list;
	std::vector<std::shared_ptr<texture_t>> textures;
};

struct model_t
{
	vector_t forward;
	vector_t up;
	matrix44_t orientation;
	vertex_t position;

	//std::shared_ptr<mesh> commons;
	//std::unique_ptr<mesh> replacements;
	//std::vector    <mesh> additions;

	mesh_t mesh;

	bool smooth;
};


struct point_source_light
{
	vertex_t position;
	float intensity;
};

struct scene_t
{
	std::vector<model_t> models;

	float ambient_light_intensity;

	normal_t sun_direction;
	float sun_intensity;

	std::vector<point_source_light> point_source_lights;
};


inline void calculate_face_normals(model_t & model)
{
	auto & vertices = model.mesh.vertices;

	// Preca model.normals for strips
	for (auto & strip : model.mesh.triangle_strips)
	{
		int i0 = strip.indices[0];
		int i1 = strip.indices[1];
		int i2 = strip.indices[2];
		normal_t n(cross(vertices[i1].v-vertices[i0].v, vertices[i2].v-vertices[i0].v));
		vertices[i0].normal = n;
		vertices[i1].normal = n;
		vertices[i2].normal = n;
		for (unsigned int i=3 ; i<strip.indices.size() ; i++, i0=i1, i1=i2)
		{
			i2 = strip.indices[i];
			vertices[i2].normal = normal_t(((i&0x1)==0) ? cross(vertices[i1].v-vertices[i0].v, vertices[i2].v-vertices[i0].v)
			                                            : cross(vertices[i2].v-vertices[i0].v, vertices[i1].v-vertices[i0].v));
		}
	}
	// Preca model.normals for fans
	for (auto & fan : model.mesh.triangle_fans)
	{
		int i0 = fan.indices[0];
		int i1 = fan.indices[1];
		int i2 = fan.indices[2];
		normal_t n(cross(vertices[i1].v-vertices[i0].v, vertices[i2].v-vertices[i0].v));
		vertices[i0].normal = n;
		vertices[i1].normal = n;
		vertices[i2].normal = n;
		for (unsigned int i=3 ; i<fan.indices.size() ; i++, i1=i2)
		{
			i2 = fan.indices[i];
			vertices[i2].normal = normal_t(cross(vertices[i1].v-vertices[i0].v, vertices[i2].v-vertices[i0].v));
		}
	}
	// Preca model.normals for lose triangles
	for (unsigned int i=2 ; i<model.mesh.triangle_list.indices.size() ; i+=3)
	{
		int i0 = model.mesh.triangle_list.indices[i-2];
		int i1 = model.mesh.triangle_list.indices[i-1];
		int i2 = model.mesh.triangle_list.indices[i  ];
		vector_t n(cross(vertices[i1].v-vertices[i0].v, vertices[i2].v-vertices[i0].v));
		vertices[i0].normal = n;
		vertices[i1].normal = n;
		vertices[i2].normal = n;
	}
}


inline model_t make_tri(float size, std::shared_ptr<texture_t> & texture)
{
	model_t result;

	result.mesh.vertices = std::vector<mesh_vertex_t>
		{
			mesh_vertex_t{vertex_t{0.0f, 0.0f, 0.0f}, vec2f_t{0.0f,0.0f}, normal_t(0,0,0)},
			mesh_vertex_t{vertex_t{size, 0.0f, 0.0f}, vec2f_t{0.0f,1.0f}, normal_t(0,0,0)},
			mesh_vertex_t{vertex_t{0.0f, size, 0.0f}, vec2f_t{1.0f,0.0f}, normal_t(0,0,0)}
		};
	result.orientation = matrix44_t::Identity;
	result.position = vertex_t(0.0,0.0,0.0);
	result.mesh.textures.push_back(texture);

	result.smooth = false;
	result.forward = vector_t(0.0, 0.0, 1.0);
	result.up      = vector_t(0.0, 1.0, 0.0);
	result.mesh.triangle_list = triangle_list_t{{0,1,2}};

	calculate_face_normals(result);

	result.mesh.vertices_world = result.mesh.vertices; // make a full copy
	result.mesh.vertices_world.reserve(result.mesh.vertices.size() + 2); // allow for 2 extra vertices in case we have triangles intersecting the 0,0 camera plane
	result.mesh.vertices_viewport = result.mesh.vertices; // make a full copy
	result.mesh.vertices_viewport.reserve(result.mesh.vertices.size() + 2); // allow for 2 extra vertices in case we have triangles intersecting the 0,0 camera plane

	return result;	
}

inline model_t make_cube(float size, std::shared_ptr<texture_t> & texture)
{
	model_t result;

	result.mesh.vertices = std::vector<mesh_vertex_t>
		{
			// 0 fan (top face): value 1
			mesh_vertex_t{vertex_t(-size / 2.0f,  size / 2.0f,  size / 2.0f), vec2f_t{0.0,0.0  }, normal_t(0,0,0)},
			mesh_vertex_t{vertex_t( size / 2.0f,  size / 2.0f,  size / 2.0f), vec2f_t{0.5,0.0  }, normal_t(0,0,0)},
			mesh_vertex_t{vertex_t( size / 2.0f,  size / 2.0f, -size / 2.0f), vec2f_t{0.5,0.333}, normal_t(0,0,0)},
			mesh_vertex_t{vertex_t(-size / 2.0f,  size / 2.0f, -size / 2.0f), vec2f_t{0.0,0.333}, normal_t(0,0,0)},
			// 1 fan (front face): value 2
			mesh_vertex_t{vertex_t(-size / 2.0f, -size / 2.0f,  size / 2.0f), vec2f_t{0.5,0.0  }, normal_t(0,0,0)},
			mesh_vertex_t{vertex_t( size / 2.0f, -size / 2.0f,  size / 2.0f), vec2f_t{1.0,0.0  }, normal_t(0,0,0)},
			mesh_vertex_t{vertex_t( size / 2.0f,  size / 2.0f,  size / 2.0f), vec2f_t{1.0,0.333}, normal_t(0,0,0)},
			mesh_vertex_t{vertex_t(-size / 2.0f,  size / 2.0f,  size / 2.0f), vec2f_t{0.5,0.333}, normal_t(0,0,0)},
			// 2 fan (right face): value 4
			mesh_vertex_t{vertex_t( size / 2.0f, -size / 2.0f,  size / 2.0f), vec2f_t{0.0,0.333}, normal_t(0,0,0)},
			mesh_vertex_t{vertex_t( size / 2.0f, -size / 2.0f, -size / 2.0f), vec2f_t{0.5,0.333}, normal_t(0,0,0)},
			mesh_vertex_t{vertex_t( size / 2.0f,  size / 2.0f, -size / 2.0f), vec2f_t{0.5,0.667}, normal_t(0,0,0)},
			mesh_vertex_t{vertex_t( size / 2.0f,  size / 2.0f,  size / 2.0f), vec2f_t{0.0,0.667}, normal_t(0,0,0)},
			// 3 fan (back face): value 6
			mesh_vertex_t{vertex_t( size / 2.0f, -size / 2.0f, -size / 2.0f), vec2f_t{0.5,0.667}, normal_t(0,0,0)},
			mesh_vertex_t{vertex_t(-size / 2.0f, -size / 2.0f, -size / 2.0f), vec2f_t{1.0,0.667}, normal_t(0,0,0)},
			mesh_vertex_t{vertex_t(-size / 2.0f,  size / 2.0f, -size / 2.0f), vec2f_t{1.0,1.0  }, normal_t(0,0,0)},
			mesh_vertex_t{vertex_t( size / 2.0f,  size / 2.0f, -size / 2.0f), vec2f_t{0.5,1.0  }, normal_t(0,0,0)},
			// 4 fan (left face): value 3
			mesh_vertex_t{vertex_t(-size / 2.0f, -size / 2.0f, -size / 2.0f), vec2f_t{0.5,0.333}, normal_t(0,0,0)},
			mesh_vertex_t{vertex_t(-size / 2.0f, -size / 2.0f,  size / 2.0f), vec2f_t{1.0,0.333}, normal_t(0,0,0)},
			mesh_vertex_t{vertex_t(-size / 2.0f,  size / 2.0f,  size / 2.0f), vec2f_t{1.0,0.667}, normal_t(0,0,0)},
			mesh_vertex_t{vertex_t(-size / 2.0f,  size / 2.0f, -size / 2.0f), vec2f_t{0.5,0.667}, normal_t(0,0,0)},
			// 6 fan (bottom face): value 5
			mesh_vertex_t{vertex_t(-size / 2.0f, -size / 2.0f, -size / 2.0f), vec2f_t{0.0,0.667}, normal_t(0,0,0)},
			mesh_vertex_t{vertex_t( size / 2.0f, -size / 2.0f, -size / 2.0f), vec2f_t{0.5,0.667}, normal_t(0,0,0)},
			mesh_vertex_t{vertex_t( size / 2.0f, -size / 2.0f,  size / 2.0f), vec2f_t{0.5,1.0  }, normal_t(0,0,0)},
			mesh_vertex_t{vertex_t(-size / 2.0f, -size / 2.0f,  size / 2.0f), vec2f_t{0.0,1.0  }, normal_t(0,0,0)}
		};
	result.orientation = matrix44_t::Identity;
	result.position = vertex_t(0.0,0.0,0.0);
	result.mesh.textures.push_back(texture);

	result.smooth = false;
	result.forward = vector_t(0.0, 0.0, 1.0);
	result.up      = vector_t(0.0, 1.0, 0.0);
	//result.common->triangle_strips.push_back(triangle_strip{{0,1,3,2,7,6,4,5}, {{0.0,1.0},{0.5,1.0},{0.0,0.666},{0.5,0.666},{0.0,0.333},{0.5,0.333},{0.0,0.0},{0.5,0.0}}});
	//result.common->triangle_strips.push_back(triangle_strip{{6,2,5,1,4,0,7,3}, {{0.5,1.0},{1.0,1.0},{0.5,0.666},{1.0,0.666},{0.5,0.333},{1.0,0.333},{0.5,0.0},{1.0,0.0}}});
	result.mesh.triangle_fans.push_back(triangle_fan{{0,1,2,3}});
	result.mesh.triangle_fans.push_back(triangle_fan{{4,5,6,7}});
	result.mesh.triangle_fans.push_back(triangle_fan{{8,9,10,11}});
	result.mesh.triangle_fans.push_back(triangle_fan{{12,13,14,15}});
	result.mesh.triangle_fans.push_back(triangle_fan{{16,17,18,19}});
	result.mesh.triangle_fans.push_back(triangle_fan{{20,21,22,23}});

	calculate_face_normals(result);

	result.mesh.vertices_world = result.mesh.vertices; // make a full copy
	result.mesh.vertices_world.reserve(result.mesh.vertices.size() + 2); // allow for 2 extra vertices in case we have triangles intersecting the 0,0 camera plane
	result.mesh.vertices_viewport = result.mesh.vertices; // make a full copy
	result.mesh.vertices_viewport.reserve(result.mesh.vertices.size() + 2); // allow for 2 extra vertices in case we have triangles intersecting the 0,0 camera plane

	return result;	
}

inline model_t make_tore(unsigned int precision, std::shared_ptr<texture_t> & texture)
{
	model_t result;

	result.smooth = true;
	result.forward = vector_t(0.0, 0.0, 1.0);
	result.up      = vector_t(0.0, 1.0, 0.0);
	result.orientation = swegl::matrix44_t::Identity;
	result.position = vertex_t(0.0,0.0,0.0);
	result.mesh.textures.push_back(texture);

	auto & vertices = result.mesh.vertices;
	//auto & normals  = result.get_normals ();

	float angle = (2 * 3.141592653589f) / precision;
	matrix44_t big = matrix44_t::Identity;
	matrix44_t big_normal = matrix44_t::Identity;
	big.translate(2.0f, 0.0f, 0.0f);

	for (unsigned int bg = 0; bg <= precision; bg++)
	{
		matrix44_t small = matrix44_t::Identity;
		matrix44_t small_normal = matrix44_t::Identity;
		small.translate(0.8f, 0.0f, 0.0f);

		for (unsigned int sm = 0; sm <= precision; sm++)
		{
			vertices.push_back(mesh_vertex_t{transform(transform(vertex_t(0.0f, 0.0f, 0.0f), small), big)
			                                ,vec2f_t(1.0*bg/precision, 1.0*sm/precision)
			                                ,transform(transform(normal_t(1.0f, 0.0f, 0.0f), small_normal), big_normal)
				                            }
			                  );
			//normals.push_back((vertices.back() - transform(Vec3f(), big)).Normalize());
			//vb.emplace_back(std::make_pair<>(transform(transform(Vec3f(), small), big),
			//                                 vec2f_t(texture->m_mipmaps[0].m_width*(float)bg / precision, texture->m_mipmaps[0].m_height*(float)sm / precision)));
			small.rotate_z(angle);
			small_normal.rotate_z(angle);
		}

		big.rotate_y(angle);
		big_normal.rotate_y(angle);
	}

	for (unsigned int bg = 0; bg < precision; bg++)
	{
		result.mesh.triangle_strips.emplace_back();
		auto & strip = result.mesh.triangle_strips.back();
		strip.indices.push_back((bg+1)*(precision+1));
		strip.indices.push_back((bg  )*(precision+1));
		for (unsigned int sm = 0; sm < precision; sm++)
		{
			strip.indices.push_back((bg+1)*(precision+1) + (sm+1));
			strip.indices.push_back((bg  )*(precision+1) + (sm+1));
		}
		int a=0;
	}

	result.mesh.vertices_world = result.mesh.vertices; // make a full copy
	result.mesh.vertices_world.reserve(result.mesh.vertices.size() + 2); // allow for 2 extra vertices in case we have triangles intersecting the 0,0 camera plane
	result.mesh.vertices_viewport = result.mesh.vertices; // make a full copy
	result.mesh.vertices_viewport.reserve(result.mesh.vertices.size() + 2); // allow for 2 extra vertices in case we have triangles intersecting the 0,0 camera plane

	return result;
}
/*
inline model_t make_sphere(unsigned int precision, float size, std::shared_ptr<texture_t> & texture)
{
	model_t result;

	result.smooth = true;
	result.forward = vector_t(0.0, 0.0, 1.0);
	result.up      = vector_t(0.0, 1.0, 0.0);
	result.orientation = swegl::matrix44_t::Identity;
	result.position = vertex_t(0.0,0.0,0.0);
	result.mesh.textures.push_back(texture);

	auto & vertices = result.mesh.vertices;
	//auto & normals  = result.get_normals ();

	float angle = (2 * 3.141592653589f) / precision;
	matrix44_t big = matrix44_t::Identity;
	//big.translate(0.0f, size, 0.0f);

	for (unsigned int bg = 0; bg < precision; bg++)
	{
		matrix44_t small = matrix44_t::Identity;

		for (unsigned int sm = 0; sm <= precision; sm++)
		{
			auto v = transform(vertex_t(0.0f, size, 0.0f),small);
			auto v2 = transform(v, big);
			vertices.push_back(v2);
			//normals.push_back((vertices.back() - transform(Vec3f(), big)).Normalize());
			//vb.emplace_back(std::make_pair<>(transform(transform(Vec3f(), small), big),
			//                                 vec2f_t(texture->m_mipmaps[0].m_width*(float)bg / precision, texture->m_mipmaps[0].m_height*(float)sm / precision)));
			small.rotate_z(angle/2);
		}

		big.rotate_y(angle);
	}

	for (unsigned int bg = 1; bg <= precision; bg++)
	{
		result.mesh.triangle_strips.emplace_back();
		auto & strip = result.mesh.triangle_strips.back();
		for (unsigned int sm = 0; sm <= precision; sm++)
		{
			strip.indices.push_back((bg%precision)*(precision+1) + sm);
			strip.indices.push_back((bg-1        )*(precision+1) + sm);
			strip.texture_mapping.emplace_back((float)(bg  ) / precision, (float)(sm) / precision);
			strip.texture_mapping.emplace_back((float)(bg-1) / precision, (float)(sm) / precision);
		}
	}

	calculate_normals(result);

	return result;
}
*/

} // namespace
