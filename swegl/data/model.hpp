
#pragma once

#include <swegl/projection/camera.hpp>
#include <swegl/projection/vec2f.hpp>
#include <swegl/projection/points.hpp>
#include <swegl/projection/matrix44.hpp>
#include <swegl/data/texture.hpp>
#include <swegl/render/colors.hpp>

namespace swegl
{

class vertex_shader_t;
class pixel_shader_t;

using vertex_idx = std::uint32_t;

struct mesh_vertex_t
{
	vertex_t v;
	vertex_t v_world;    // after transformations into world coordinates
	vertex_t v_viewport; // after transformations into viewport coordinates (pixel x,y + z depth
	vec2f_t tex_coords;
	normal_t normal;
	normal_t normal_world;
	bool yes = false;
};

struct primitive_t
{
	enum index_mode_t
	{
		POINTS         = 0,
		LINES          = 1,
		LINE_LOOP      = 2,
		LINE_STRIP     = 3,
		TRIANGLES      = 4,
		TRIANGLE_STRIP = 5,
		TRIANGLE_FAN   = 6,
	};

	std::vector<mesh_vertex_t> vertices;
	std::vector<vertex_idx> indices;
	index_mode_t mode;
	int material_id;
};

struct node_t
{
	matrix44_t orientation;
	vertex_t position;

	std::vector<primitive_t> primitives;
};


struct point_source_light
{
	vertex_t position;
	float intensity;
};

struct material_t
{
	pixel_colors color;
	float metallic;
	float roughness;
	int texture_idx; // -1 means none
};

struct scene_t
{
	std::vector<node_t> nodes;

	float ambient_light_intensity;

	normal_t sun_direction;
	float sun_intensity;

	std::vector<point_source_light> point_source_lights;

	std::vector<material_t> materials;
	std::vector<texture_t> images;
};


inline void calculate_face_normals(primitive_t & primitive)
{
	auto & vertices = primitive.vertices;

	// Preca node.normals for strips
	if (primitive.mode == primitive_t::index_mode_t::TRIANGLE_STRIP)
	{
		int i0 = primitive.indices[0];
		int i1 = primitive.indices[1];
		int i2 = primitive.indices[2];
		normal_t n(cross(vertices[i1].v-vertices[i0].v, vertices[i2].v-vertices[i0].v));
		vertices[i0].normal = n;
		vertices[i1].normal = n;
		vertices[i2].normal = n;
		for (unsigned int i=3 ; i<primitive.indices.size() ; i++, i0=i1, i1=i2)
		{
			i2 = primitive.indices[i];
			vertices[i2].normal = normal_t(((i&0x1)==0) ? cross(vertices[i1].v-vertices[i0].v, vertices[i2].v-vertices[i0].v)
			                                            : cross(vertices[i2].v-vertices[i0].v, vertices[i1].v-vertices[i0].v));
		}
	}
	// Preca node.normals for fans
	if (primitive.mode == primitive_t::index_mode_t::TRIANGLE_FAN)
	{
		int i0 = primitive.indices[0];
		int i1 = primitive.indices[1];
		int i2 = primitive.indices[2];
		normal_t n(cross(vertices[i1].v-vertices[i0].v, vertices[i2].v-vertices[i0].v));
		vertices[i0].normal = n;
		vertices[i1].normal = n;
		vertices[i2].normal = n;
		for (unsigned int i=3 ; i<primitive.indices.size() ; i++, i1=i2)
		{
			i2 = primitive.indices[i];
			vertices[i2].normal = normal_t(cross(vertices[i1].v-vertices[i0].v, vertices[i2].v-vertices[i0].v));
		}
	}
	// Preca node.normals for lose triangles
	if (primitive.mode == primitive_t::index_mode_t::TRIANGLE_FAN)
		for (unsigned int i=2 ; i<primitive.indices.size() ; i+=3)
		{
			int i0 = primitive.indices[i-2];
			int i1 = primitive.indices[i-1];
			int i2 = primitive.indices[i  ];
			vector_t n(cross(vertices[i1].v-vertices[i0].v, vertices[i2].v-vertices[i0].v));
			vertices[i0].normal = n;
			vertices[i1].normal = n;
			vertices[i2].normal = n;
		}
}


inline node_t make_tri(float size, int material_idx)
{
	node_t result;

	result.primitives = std::vector<primitive_t>
		{
			primitive_t{std::vector<mesh_vertex_t>{mesh_vertex_t{vertex_t{0.0f, 0.0f, 0.0f}, {}, {}, vec2f_t{0.0f,0.0f}, normal_t(0,0,1), {}}
			                                      ,mesh_vertex_t{vertex_t{size, 0.0f, 0.0f}, {}, {}, vec2f_t{0.0f,1.0f}, normal_t(0,0,1), {}}
			                                      ,mesh_vertex_t{vertex_t{0.0f, size, 0.0f}, {}, {}, vec2f_t{1.0f,0.0f}, normal_t(0,0,1), {}}
			                                      }
			           ,{0,1,2}
			           ,primitive_t::index_mode_t::TRIANGLES
			           ,material_idx
		               },
		};
	result.orientation = matrix44_t::Identity;
	result.position = vertex_t(0.0,0.0,0.0);

	//calculate_face_normals(result);
	
	return result;	
}

inline node_t make_cube(float size, int material_idx)
{
	node_t result;

	result.primitives = std::vector<primitive_t>
		{
			// 0 fan (top face): value 1
			primitive_t{std::vector<mesh_vertex_t>{mesh_vertex_t{vertex_t(-size / 2.0f,  size / 2.0f,  size / 2.0f), {}, {}, vec2f_t{0.0,0.0  }, normal_t(0,1,0), {}, false}
			                                      ,mesh_vertex_t{vertex_t( size / 2.0f,  size / 2.0f,  size / 2.0f), {}, {}, vec2f_t{0.5,0.0  }, normal_t(0,1,0), {}, false}
			                                      ,mesh_vertex_t{vertex_t( size / 2.0f,  size / 2.0f, -size / 2.0f), {}, {}, vec2f_t{0.5,0.333}, normal_t(0,1,0), {}, false}
			                                      ,mesh_vertex_t{vertex_t(-size / 2.0f,  size / 2.0f, -size / 2.0f), {}, {}, vec2f_t{0.0,0.333}, normal_t(0,1,0), {}, false}
			                                      }
			           ,{0,1,2,3}
			           ,primitive_t::index_mode_t::TRIANGLE_FAN
			           ,material_idx
		               },
			// 1 fan (front face): value 2
			primitive_t{std::vector<mesh_vertex_t>{mesh_vertex_t{vertex_t(-size / 2.0f, -size / 2.0f,  size / 2.0f), {}, {}, vec2f_t{0.5,0.0  }, normal_t(0,0,1), {}, false}
			                                      ,mesh_vertex_t{vertex_t( size / 2.0f, -size / 2.0f,  size / 2.0f), {}, {}, vec2f_t{1.0,0.0  }, normal_t(0,0,1), {}, false}
			                                      ,mesh_vertex_t{vertex_t( size / 2.0f,  size / 2.0f,  size / 2.0f), {}, {}, vec2f_t{1.0,0.333}, normal_t(0,0,1), {}, false}
			                                      ,mesh_vertex_t{vertex_t(-size / 2.0f,  size / 2.0f,  size / 2.0f), {}, {}, vec2f_t{0.5,0.333}, normal_t(0,0,1), {}, false}
			                                      }
			           ,{0,1,2,3}
			           ,primitive_t::index_mode_t::TRIANGLE_FAN
			           ,material_idx
		               },
			// 2 fan (right face): value 4
			primitive_t{std::vector<mesh_vertex_t>{mesh_vertex_t{vertex_t( size / 2.0f, -size / 2.0f,  size / 2.0f), {}, {}, vec2f_t{0.0,0.333}, normal_t(1,0,0), {}, false}
			                                      ,mesh_vertex_t{vertex_t( size / 2.0f, -size / 2.0f, -size / 2.0f), {}, {}, vec2f_t{0.5,0.333}, normal_t(1,0,0), {}, false}
			                                      ,mesh_vertex_t{vertex_t( size / 2.0f,  size / 2.0f, -size / 2.0f), {}, {}, vec2f_t{0.5,0.667}, normal_t(1,0,0), {}, false}
			                                      ,mesh_vertex_t{vertex_t( size / 2.0f,  size / 2.0f,  size / 2.0f), {}, {}, vec2f_t{0.0,0.667}, normal_t(1,0,0), {}, false}
			                                      }
			           ,{0,1,2,3}
			           ,primitive_t::index_mode_t::TRIANGLE_FAN
			           ,material_idx
		               },
			// 3 fan (back face): value 6
			primitive_t{std::vector<mesh_vertex_t>{mesh_vertex_t{vertex_t( size / 2.0f, -size / 2.0f, -size / 2.0f), {}, {}, vec2f_t{0.5,0.667}, normal_t(0,0,-1), {}, false}
			                                      ,mesh_vertex_t{vertex_t(-size / 2.0f, -size / 2.0f, -size / 2.0f), {}, {}, vec2f_t{1.0,0.667}, normal_t(0,0,-1), {}, false}
			                                      ,mesh_vertex_t{vertex_t(-size / 2.0f,  size / 2.0f, -size / 2.0f), {}, {}, vec2f_t{1.0,1.0  }, normal_t(0,0,-1), {}, false}
			                                      ,mesh_vertex_t{vertex_t( size / 2.0f,  size / 2.0f, -size / 2.0f), {}, {}, vec2f_t{0.5,1.0  }, normal_t(0,0,-1), {}, false}
			                                      }
			           ,{0,1,2,3}
			           ,primitive_t::index_mode_t::TRIANGLE_FAN
			           ,material_idx
		               },
			// 4 fan (left face): value 3
			primitive_t{std::vector<mesh_vertex_t>{mesh_vertex_t{vertex_t(-size / 2.0f, -size / 2.0f, -size / 2.0f), {}, {}, vec2f_t{0.5,0.333}, normal_t(-1,0,0), {}, false}
			                                      ,mesh_vertex_t{vertex_t(-size / 2.0f, -size / 2.0f,  size / 2.0f), {}, {}, vec2f_t{1.0,0.333}, normal_t(-1,0,0), {}, false}
			                                      ,mesh_vertex_t{vertex_t(-size / 2.0f,  size / 2.0f,  size / 2.0f), {}, {}, vec2f_t{1.0,0.667}, normal_t(-1,0,0), {}, false}
			                                      ,mesh_vertex_t{vertex_t(-size / 2.0f,  size / 2.0f, -size / 2.0f), {}, {}, vec2f_t{0.5,0.667}, normal_t(-1,0,0), {}, false}
			                                      }
			           ,{0,1,2,3}
			           ,primitive_t::index_mode_t::TRIANGLE_FAN
			           ,material_idx
		               },
			// 6 fan (bottom face): value 5
			primitive_t{std::vector<mesh_vertex_t>{mesh_vertex_t{vertex_t(-size / 2.0f, -size / 2.0f, -size / 2.0f), {}, {}, vec2f_t{0.0,0.667}, normal_t(0,-1,0), {}, false}
			                                      ,mesh_vertex_t{vertex_t( size / 2.0f, -size / 2.0f, -size / 2.0f), {}, {}, vec2f_t{0.5,0.667}, normal_t(0,-1,0), {}, false}
			                                      ,mesh_vertex_t{vertex_t( size / 2.0f, -size / 2.0f,  size / 2.0f), {}, {}, vec2f_t{0.5,1.0  }, normal_t(0,-1,0), {}, false}
			                                      ,mesh_vertex_t{vertex_t(-size / 2.0f, -size / 2.0f,  size / 2.0f), {}, {}, vec2f_t{0.0,1.0  }, normal_t(0,-1,0), {}, false}
			                                      }
			           ,{0,1,2,3}
			           ,primitive_t::index_mode_t::TRIANGLE_FAN
			           ,material_idx
		               },
		};
	result.orientation = matrix44_t::Identity;
	result.position = vertex_t(0.0,0.0,0.0);

	//calculate_face_normals(result);

	return result;	
}

/*
inline node_t make_tore(unsigned int precision, int material_idx)
{
	node_t result;

	result.orientation = swegl::matrix44_t::Identity;
	result.position = vertex_t(0.0,0.0,0.0);
	result.mesh.material_id = material_idx;

	auto & vertices = result.mesh.vertices;

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
			                                ,{}
			                                ,{}
			                                ,vec2f_t(1.0*bg/precision, 1.0*sm/precision)
			                                ,transform(transform(normal_t(1.0f, 0.0f, 0.0f), small_normal), big_normal)
			                                ,{}
				                            }
			                  );
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
	}

	result.mesh.vertices.reserve(result.mesh.vertices.size() + 2); // allow for 2 extra vertices in case we have triangles intersecting the 0,0 camera plane

	return result;
}

inline node_t make_sphere(unsigned int precision, float radius, int material_idx)
{
	node_t result;

	result.orientation = swegl::matrix44_t::Identity;
	result.position = vertex_t(0.0,0.0,0.0);
	result.mesh.material_id = material_idx;

	auto & vertices = result.mesh.vertices;

	float angle = (2 * 3.141592653589f) / precision;
	matrix44_t big = matrix44_t::Identity;

	for (unsigned int bg = 0; bg <= precision; bg++)
	{
		matrix44_t small = matrix44_t::Identity;
		for (unsigned int sm = 0; sm <= precision; sm++)
		{
			vertex_t v = transform(transform(vertex_t(0.0f, radius, 0.0f),small), big);
			vertices.push_back(mesh_vertex_t{v
			                                ,{}
			                                ,{}
			                                ,vec2f_t(1.0*sm/precision, 1.0*bg/precision)
			                                ,normal_t(v.x(), v.y(), v.z())
			                                ,{}
			                                }
			                  );
			small.rotate_z(angle/2);
		}
		big.rotate_y(angle);
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
	}

	result.mesh.vertices.reserve(result.mesh.vertices.size() + 2); // allow for 2 extra vertices in case we have triangles intersecting the 0,0 camera plane

	return result;
}
*/

} // namespace
