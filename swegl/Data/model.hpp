
#pragma once

#include <swegl/Projection/Vec3f.h>
#include <swegl/Projection/Vec2f.h>
#include <swegl/Projection/Matrix4x4.h>
#include <swegl/Data/Texture.h>

namespace swegl
{

using vertex_idx = std::uint32_t;

struct triangle_strip
{
	std::vector<vertex_idx> indices;
	std::vector<Vec3f> normals;
	std::vector<Vec2f> texture_mapping;
};
struct triangle_fan
{
	std::vector<vertex_idx> indices;
	std::vector<Vec3f> normals;
	std::vector<Vec2f> texture_mapping;
};
struct triangle_solo
{
	int v0, v1, v2;
	Vec3f normal;
	Vec2f t0, t1, t2;
};

/*
// this is fixed for a model
// e.g.: index buffers, texture mapping
struct model_common
{
	bool smooth; // if true, has vertex normals. if false, has face normals

	Vec3f forward;
	Vec3f up;

	std::vector<triangle_strip> triangle_strips;
	std::vector<triangle_fan>   triangle_fans;
	std::vector<triangle_solo>  triangle_list;
};
// this is fixed for a fixed model (vertices don't move relative to each other)
struct fixed_model_common : model_common
{
	std::vector<Vec3f> vertices; // changes with 
	std::vector<Vec3f> normals; // calculated if !smooth, same size as vertices if smooth
};



// this changes from instance to instance
// e.g.: skin, damage texture
struct fixed_model
{
	std::shared_ptr<fixed_model_common> common;

	Matrix4x4 orientation;
	Vec3f position;

	std::vector<std::shared_ptr<Texture>> textures; // empty if no texture

	      auto & get_vertices()       { return common->vertices; }
	const auto & get_vertices() const { return common->vertices; }
	      auto & get_normals ()       { return common->normals ; }
	const auto & get_normals () const { return common->normals ; }
};



// this changes from instance to instance
// e.g.: character posture, skin, damage texture
struct flex_model
{
	std::shared_ptr<model_common> common;

	std::vector<Vec3f> vertices; // changes with 
	std::vector<Vec3f> normals; // calculated if !smooth, same size as vertices if smooth

	Matrix4x4 orientation;
	Vec3f position;

	std::vector<std::shared_ptr<Texture>> textures; // empty if no texture

	      auto & get_vertices()       { return vertices; }
	const auto & get_vertices() const { return vertices; }
	      auto & get_normals ()       { return normals ; }
	const auto & get_normals () const { return normals ; }
};
*/

struct mesh_t
{
	std::vector<Vec3f> vertices;
	std::vector<triangle_strip> triangle_strips;
	std::vector<triangle_fan>   triangle_fans;
	std::vector<triangle_solo>  triangle_list;
	std::vector<std::shared_ptr<Texture>> textures;
};

struct model_t
{
	Vec3f forward;
	Vec3f up;
	Matrix4x4 orientation;
	Vec3f position;

	//std::shared_ptr<mesh> commons;
	//std::unique_ptr<mesh> replacements;
	//std::vector    <mesh> additions;

	mesh_t mesh;

	bool smooth;
};


struct point_source_light
{
	Vec3f position;
	float intensity;
};

struct scene
{
	std::vector<model_t> models;

	float ambient_light_intensity;

	Vec3f sun_direction;
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
			strip.normals.push_back((((i&0x1)==0) ? Cross((vertices[i2]-vertices[i0]),(vertices[i1]-vertices[i0]))
			                                      : Cross((vertices[i1]-vertices[i0]),(vertices[i2]-vertices[i0])))
			                       );
			strip.normals.back().Normalize();
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
			fan.normals.push_back(Cross((vertices[i2]-vertices[i0]),(vertices[i1]-vertices[i0])));
			fan.normals.back().Normalize();
		}
	}
	// Preca model.normals for lose triangles
	for (auto & t : model.mesh.triangle_list)
	{
		int i0 = t.v0;
		int i1 = t.v1;
		int i2 = t.v2;
		t.normal = Cross((vertices[i2]-vertices[i0]),(vertices[i1]-vertices[i0]));
		t.normal.Normalize();
	}
}


inline model_t make_tri(float size, std::shared_ptr<Texture> & texture)
{
	model_t result;

	result.mesh.vertices = std::vector<Vec3f>
		{
			Vec3f(0.0f, 0.0f, 0.0f),
			Vec3f(size, 0.0f, 0.0f),
			Vec3f(0.0f, size, 0.0f)
		};
	result.orientation = Matrix4x4::Identity;
	result.position = Vec3f{0.0,0.0,0.0};
	result.mesh.textures.push_back(texture);

	result.smooth = false;
	result.forward = Vec3f{0.0, 0.0, 1.0};
	result.up      = Vec3f{0.0, 1.0, 0.0};
	result.mesh.triangle_list.emplace_back(triangle_solo{0,1,2, Vec3f{0,0,0}, Vec2f{0.0f,0.0f}, Vec2f{0.0f,1.0f}, Vec2f{1.0f,0.0f}});

	calculate_normals(result);

	return result;	
}

inline model_t make_cube(float size, std::shared_ptr<Texture> & texture)
{
	model_t result;

	result.mesh.vertices = std::vector<Vec3f>
		{
			Vec3f(-size / 2.0f, -size / 2.0f, -size / 2.0f),
			Vec3f( size / 2.0f, -size / 2.0f, -size / 2.0f),
			Vec3f( size / 2.0f,  size / 2.0f, -size / 2.0f),
			Vec3f(-size / 2.0f,  size / 2.0f, -size / 2.0f),
			Vec3f(-size / 2.0f, -size / 2.0f,  size / 2.0f),
			Vec3f( size / 2.0f, -size / 2.0f,  size / 2.0f),
			Vec3f( size / 2.0f,  size / 2.0f,  size / 2.0f),
			Vec3f(-size / 2.0f,  size / 2.0f,  size / 2.0f)
		};
	result.orientation = Matrix4x4::Identity;
	result.position = Vec3f{0.0,0.0,0.0};
	result.mesh.textures.push_back(texture);

	result.smooth = false;
	result.forward = Vec3f{0.0, 0.0, 1.0};
	result.up      = Vec3f{0.0, 1.0, 0.0};
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
	result.forward = swegl::Vec3f(0.0, 0.0, 1.0);
	result.up      = swegl::Vec3f(0.0, 1.0, 0.0);
	result.orientation = swegl::Matrix4x4::Identity;
	result.position = swegl::Vec3f(0.0, 0.0, 0.0);
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
			vertices.push_back(Transform(Transform(Vec3f(), small), big));
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


} // namespace
