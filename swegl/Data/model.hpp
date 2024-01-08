
#pragma once

#include <swegl/Projection/Vec2f.h>
#include <swegl/Projection/points.hpp>
#include <swegl/Projection/Matrix4x4.h>
#include <swegl/Data/Texture.h>

namespace swegl
{

class vertex_shader_t;
class pixel_shader_t;

using vertex_idx = std::uint32_t;

struct triangle_strip
{
	std::vector<vertex_idx> indices;
	std::vector<normal_t> normals;
	std::vector<Vec2f> texture_mapping;
};
struct triangle_fan
{
	std::vector<vertex_idx> indices;
	std::vector<normal_t> normals;
	std::vector<Vec2f> texture_mapping;
};
struct triangle_list_t
{
	std::vector<vertex_idx> indices;
	std::vector<normal_t> normals;
	std::vector<Vec2f> texture_mapping;
};

struct mesh_t
{
	std::vector<vertex_t> vertices;
	std::vector<triangle_strip> triangle_strips;
	std::vector<triangle_fan>   triangle_fans;
	triangle_list_t             triangle_list;
	std::vector<std::shared_ptr<Texture>> textures;
};

struct model_t
{
	vertex_shader_t * vertex_shader;
	pixel_shader_t * pixel_shader;

	vector_t forward;
	vector_t up;
	Matrix4x4 orientation;
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


inline void calculate_normals(model_t & model)
{
	// model cant have curved surfaces
	if (model.smooth)
		return;

	const auto & vertices = model.mesh.vertices;

	// Preca model.normals for strips
	for (auto & strip : model.mesh.triangle_strips)
	{
		int i0 = strip.indices[0];
		int i1 = strip.indices[1];
		int i2;
		for (unsigned int i=2 ; i<strip.indices.size() ; i++, i0=i1, i1=i2)
		{
			i2 = strip.indices[i];
			strip.normals.push_back((((i&0x1)==0) ? Cross(vertices[i2]-vertices[i0], vertices[i1]-vertices[i0])
			                                      : Cross(vertices[i1]-vertices[i0], vertices[i2]-vertices[i0]))
			                       );
			strip.normals.back().normalize();
		}
	}
	// Preca model.normals for fans
	for (auto & fan : model.mesh.triangle_fans)
	{
		int i0 = fan.indices[0];
		int i1 = fan.indices[1];
		int i2;
		for (unsigned int i=2 ; i<fan.indices.size() ; i++, i1=i2)
		{
			i2 = fan.indices[i];
			fan.normals.push_back(Cross(vertices[i2]-vertices[i0], vertices[i1]-vertices[i0]));
			fan.normals.back().normalize();
		}
	}
	// Preca model.normals for lose triangles
	for (unsigned int i=2 ; i<model.mesh.triangle_list.indices.size() ; i+=3)
	{
		int i0 = model.mesh.triangle_list.indices[i-2];
		int i1 = model.mesh.triangle_list.indices[i-1];
		int i2 = model.mesh.triangle_list.indices[i  ];
		model.mesh.triangle_list.normals.push_back(Cross(vertices[i2]-vertices[i0], vertices[i1]-vertices[i0]));
		model.mesh.triangle_list.normals.back().normalize();
	}
}


inline model_t make_tri(float size, std::shared_ptr<Texture> & texture)
{
	model_t result;

	result.mesh.vertices = std::vector<vertex_t>
		{
			vertex_t(0.0f, 0.0f, 0.0f),
			vertex_t(size, 0.0f, 0.0f),
			vertex_t(0.0f, size, 0.0f)
		};
	result.orientation = Matrix4x4::Identity;
	result.position = vertex_t(0.0,0.0,0.0);
	result.mesh.textures.push_back(texture);

	result.smooth = false;
	result.forward = vector_t(0.0, 0.0, 1.0);
	result.up      = vector_t(0.0, 1.0, 0.0);
	result.mesh.triangle_list = triangle_list_t{{0,1,2}, {}, {Vec2f{0.0f,0.0f}, Vec2f{0.0f,1.0f}, Vec2f{1.0f,0.0f}}};

	calculate_normals(result);

	return result;	
}

inline model_t make_cube(float size, std::shared_ptr<Texture> & texture)
{
	model_t result;

	result.mesh.vertices = std::vector<vertex_t>
		{
			vertex_t(-size / 2.0f, -size / 2.0f, -size / 2.0f),
			vertex_t( size / 2.0f, -size / 2.0f, -size / 2.0f),
			vertex_t( size / 2.0f,  size / 2.0f, -size / 2.0f),
			vertex_t(-size / 2.0f,  size / 2.0f, -size / 2.0f),
			vertex_t(-size / 2.0f, -size / 2.0f,  size / 2.0f),
			vertex_t( size / 2.0f, -size / 2.0f,  size / 2.0f),
			vertex_t( size / 2.0f,  size / 2.0f,  size / 2.0f),
			vertex_t(-size / 2.0f,  size / 2.0f,  size / 2.0f)
		};
	result.orientation = Matrix4x4::Identity;
	result.position = vertex_t(0.0,0.0,0.0);
	result.mesh.textures.push_back(texture);

	result.smooth = false;
	result.forward = vector_t(0.0, 0.0, 1.0);
	result.up      = vector_t(0.0, 1.0, 0.0);
	//result.common->triangle_strips.push_back(triangle_strip{{0,1,3,2,7,6,4,5}, {{0.0,1.0},{0.5,1.0},{0.0,0.666},{0.5,0.666},{0.0,0.333},{0.5,0.333},{0.0,0.0},{0.5,0.0}}});
	//result.common->triangle_strips.push_back(triangle_strip{{6,2,5,1,4,0,7,3}, {{0.5,1.0},{1.0,1.0},{0.5,0.666},{1.0,0.666},{0.5,0.333},{1.0,0.333},{0.5,0.0},{1.0,0.0}}});
	result.mesh.triangle_fans.push_back(triangle_fan{{2,1,5,6,7,3,0,1}, {}, {{0.5,0.333},{0.5,0.666},{0.0,0.666},{0.0,0.333},{0.0,0.0},{0.5,0.0},{1.0,0.0},{1.0,0.333}}});
	result.mesh.triangle_fans.push_back(triangle_fan{{4,7,6,5,1,0,3,7}, {}, {{0.5,0.666},{0.5,0.333},{1.0,0.333},{1.0,0.666},{1.0,1.0},{0.5,1.0},{0.0,1.0},{0.0,0.666}}});

	calculate_normals(result);

	return result;	
}

inline model_t make_tore(unsigned int precision, std::shared_ptr<Texture> & texture)
{
	model_t result;

	result.smooth = false;
	result.forward = vector_t(0.0, 0.0, 1.0);
	result.up      = vector_t(0.0, 1.0, 0.0);
	result.orientation = swegl::Matrix4x4::Identity;
	result.position = vertex_t(0.0,0.0,0.0);
	result.mesh.textures.push_back(texture);

	auto & vertices = result.mesh.vertices;
	//auto & normals  = result.get_normals ();

	float angle = (2 * 3.141592653589f) / precision;
	Matrix4x4 big = Matrix4x4::Identity;
	big.Translate(2.0f, 0.0f, 0.0f);

	for (unsigned int bg = 0; bg < precision; bg++)
	{
		Matrix4x4 small = Matrix4x4::Identity;
		small.Translate(0.8f, 0.0f, 0.0f);

		for (unsigned int sm = 0; sm < precision; sm++)
		{
			vertices.push_back(Transform(Transform(vertex_t(0.0f, 0.0f, 0.0f), small), big));
			//normals.push_back((vertices.back() - Transform(Vec3f(), big)).Normalize());
			//vb.emplace_back(std::make_pair<>(Transform(Transform(Vec3f(), small), big),
			//                                 Vec2f(texture->m_mipmaps[0].m_width*(float)bg / precision, texture->m_mipmaps[0].m_height*(float)sm / precision)));
			small.RotateZ(angle);
		}

		big.RotateY(angle);
	}

	for (unsigned int bg = 1; bg <= precision; bg++)
	{
		result.mesh.triangle_strips.emplace_back();
		auto & strip = result.mesh.triangle_strips.back();
		for (unsigned int sm = 0; sm <= precision; sm++)
		{
			strip.indices.push_back((bg-1          )*precision + (sm%precision));
			strip.indices.push_back((bg%precision  )*precision + (sm%precision));
			strip.texture_mapping.emplace_back((float)(bg  ) / precision, (float)(sm) / precision);
			strip.texture_mapping.emplace_back((float)(bg-1) / precision, (float)(sm) / precision);
		}
	}

	calculate_normals(result);

	return result;
}

inline model_t make_sphere(unsigned int precision, float size, std::shared_ptr<Texture> & texture)
{
	model_t result;

	result.smooth = false;
	result.forward = vector_t(0.0, 0.0, 1.0);
	result.up      = vector_t(0.0, 1.0, 0.0);
	result.orientation = swegl::Matrix4x4::Identity;
	result.position = vertex_t(0.0,0.0,0.0);
	result.mesh.textures.push_back(texture);

	auto & vertices = result.mesh.vertices;
	//auto & normals  = result.get_normals ();

	float angle = (2 * 3.141592653589f) / precision;
	Matrix4x4 big = Matrix4x4::Identity;
	//big.Translate(0.0f, size, 0.0f);

	for (unsigned int bg = 0; bg < precision; bg++)
	{
		Matrix4x4 small = Matrix4x4::Identity;

		for (unsigned int sm = 0; sm <= precision; sm++)
		{
			auto v = Transform(vertex_t(0.0f, size, 0.0f),small);
			auto v2 = Transform(v, big);
			vertices.push_back(v2);
			//normals.push_back((vertices.back() - Transform(Vec3f(), big)).Normalize());
			//vb.emplace_back(std::make_pair<>(Transform(Transform(Vec3f(), small), big),
			//                                 Vec2f(texture->m_mipmaps[0].m_width*(float)bg / precision, texture->m_mipmaps[0].m_height*(float)sm / precision)));
			small.RotateZ(angle/2);
		}

		big.RotateY(-angle);
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


} // namespace
