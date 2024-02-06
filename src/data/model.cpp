
#include <swegl/data/model.hpp>
#include <swegl/projection/points.hpp>

namespace swegl
{

bool operator==(const new_mesh_vertex_t & left, const new_mesh_vertex_t & right)
{
	return true
		&& left.v          == right.v
		&& left.tex_coords == right.tex_coords
		&& left.normal     == right. normal
		&& left.node_idx   == right.node_idx
		;
}
bool operator<(const new_mesh_vertex_t & left, const new_mesh_vertex_t & right)
{
	// Sort by node
	// then by coords     doenst make much sense but gotta sort them
	// then by normal     doenst make much sense but gotta sort them
	// then by tex_coords doenst make much sense but gotta sort them
	return left.node_idx < right.node_idx
		|| (left.node_idx == right.node_idx && left.v          < right.v         )
		|| (left.v        == right.v        && left.normal     < right.normal    )
		|| (left.normal   == right.normal   && left.tex_coords < right.tex_coords)
		;
}


normal_t node_t::transform(const normal_t & n)
{
	// TODO use a flag to mark the use of non-1,1,1 scale
	// if yes, we have to normalize the result
	// TODO use a flag to mark the use of uniform scale (all same value)
	// if yes, we can skip normalization and just divide by scale.x()
	return swegl::transform(n, normal_original_to_world_matrix);
}


std::tuple<animation_step_t&,animation_step_t&> animation_channel_t::get_steps(float time)
{
	auto it = std::lower_bound(steps.begin(), steps.end(), time, [](const animation_step_t & step, float time)
		{
			return step.time < time;
		});
	if (it == steps.end())
		return {steps.back(), steps.back()};
	if (it == steps.begin())
		return {steps.front(), steps.front()};
	return {*std::prev(it), *it};
}

void new_scene_t::animate(const float elapsed_seconds)
{
	for (auto & animation : animations)
	{
		float relative_seconds = fmod(elapsed_seconds, animation.end_time);

		for (auto & channel : animation.channels)
		{
			// calculate frame with linear interpolation
			const auto & [before_step, after_step] = channel.get_steps(relative_seconds);
			vec4f_t frame = [&]()
				{
					if (after_step.time == before_step.time)
						return before_step.value;
					else
						return (before_step.value * ((after_step.time  - relative_seconds) / (after_step.time - before_step.time)))
						     + ( after_step.value * ((relative_seconds - before_step.time) / (after_step.time - before_step.time)));
				}();

			node_t & node = nodes[channel.node_idx];
			if (channel.path == animation_channel_t::path_t::ROTATION)
			{
				frame.normalize();
				node.rotation = matrix44_t::from_quaternion(frame.x(), frame.y(), frame.z(), frame.w());
			}
			else if (channel.path == animation_channel_t::path_t::TRANSLATION)
				node.translation = vertex_t(frame.x(), frame.y(), frame.z());
			else if (channel.path == animation_channel_t::path_t::SCALE)
				node.scale = vertex_t(frame.x(), frame.y(), frame.z());
		}
	}
}


/*
void calculate_face_normals(primitive_t & primitive)
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


node_t make_tri(float size, int material_idx)
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

	//calculate_face_normals(result);
	
	return result;	
}

node_t make_cube(float size, int material_idx)
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

	//calculate_face_normals(result);

	return result;	
}

node_t make_tore(unsigned int precision, int material_idx)
{
	node_t result;

	float angle = (2 * 3.141592653589f) / precision;
	matrix44_t big = matrix44_t::Identity;
	matrix44_t big_normal = matrix44_t::Identity;
	big.translate(2.0f, 0.0f, 0.0f);

	for (unsigned int bg = 0; bg <= precision; bg++)
	{
		auto & primitive = result.primitives.emplace_back(primitive_t{{}, {}, primitive_t::index_mode_t::TRIANGLE_STRIP, material_idx});

		// first row
		{
			matrix44_t small = matrix44_t::Identity;
			matrix44_t small_normal = matrix44_t::Identity;
			small.translate(0.8f, 0.0f, 0.0f);
			for (unsigned int sm = 0; sm <= precision; sm++)
			{
				primitive.vertices.push_back(mesh_vertex_t{transform(transform(vertex_t(0.0f, 0.0f, 0.0f), small), big)
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
		}
		big.rotate_y(angle);
		big_normal.rotate_y(angle);
		// second row
		{
			matrix44_t small = matrix44_t::Identity;
			matrix44_t small_normal = matrix44_t::Identity;
			small.translate(0.8f, 0.0f, 0.0f);
			for (unsigned int sm = 0; sm <= precision; sm++)
			{
				primitive.vertices.push_back(mesh_vertex_t{transform(transform(vertex_t(0.0f, 0.0f, 0.0f), small), big)
				                                          ,{}
				                                          ,{}
				                                          ,vec2f_t(1.0*(bg+1)/precision, 1.0*sm/precision)
				                                          ,transform(transform(normal_t(1.0f, 0.0f, 0.0f), small_normal), big_normal)
				                                          ,{}
					                                      }
				                            );
				small.rotate_z(angle);
				small_normal.rotate_z(angle);
			}
		}

		for (unsigned int sm = 0; sm <= precision; sm++)
		{
			primitive.indices.push_back(precision+1 + sm);
			primitive.indices.push_back(              sm);
		}

		primitive.vertices.reserve(primitive.vertices.size() + 2); // allow for 2 extra vertices in case we have triangles intersecting the 0,0 camera plane
	}

	return result;
}

node_t make_sphere(unsigned int precision, float radius, int material_idx)
{
	node_t result;

	float angle = (2 * 3.141592653589f) / precision;
	matrix44_t big = matrix44_t::Identity;

	for (unsigned int bg = 0; bg <= precision; bg++)
	{
		auto & primitive = result.primitives.emplace_back(primitive_t{{}, {}, primitive_t::index_mode_t::TRIANGLE_STRIP, material_idx});

		// first row
		{
			matrix44_t small = matrix44_t::Identity;
			for (unsigned int sm = 0; sm <= precision; sm++)
			{
				vertex_t v = transform(transform(vertex_t(0.0f, radius, 0.0f),small), big);
				primitive.vertices.push_back(mesh_vertex_t{v
				                                          ,{}
				                                          ,{}
				                                          ,vec2f_t(1.0*sm/precision, 1.0*bg/precision)
				                                          ,normal_t(v.x(), v.y(), v.z())
				                                          ,{}
				                                          }
				                            );
				small.rotate_z(angle/2);
			}
		}
		big.rotate_y(angle);
		// second row
		{
			matrix44_t small = matrix44_t::Identity;
			for (unsigned int sm = 0; sm <= precision; sm++)
			{
				vertex_t v = transform(transform(vertex_t(0.0f, radius, 0.0f),small), big);
				primitive.vertices.push_back(mesh_vertex_t{v
				                                          ,{}
				                                          ,{}
				                                          ,vec2f_t(1.0*sm/precision, 1.0*(bg+1)/precision)
				                                          ,normal_t(v.x(), v.y(), v.z())
				                                          ,{}
				                                          }
				                            );
				small.rotate_z(angle/2);
			}
		}
		for (unsigned int sm = 0; sm <= precision; sm++)
		{
			primitive.indices.push_back(precision+1 + sm);
			primitive.indices.push_back(              sm);
		}

		primitive.vertices.reserve(primitive.vertices.size() + 2); // allow for 2 extra vertices in case we have triangles intersecting the 0,0 camera plane
	}

	//for (unsigned int bg = 0; bg < precision; bg++)
	//{
	//	result.mesh.triangle_strips.emplace_back();
	//	auto & strip = result.mesh.triangle_strips.back();
	//	strip.indices.push_back((bg+1)*(precision+1));
	//	strip.indices.push_back((bg  )*(precision+1));
	//	for (unsigned int sm = 0; sm < precision; sm++)
	//	{
	//		strip.indices.push_back((bg+1)*(precision+1) + (sm+1));
	//		strip.indices.push_back((bg  )*(precision+1) + (sm+1));
	//	}
	//}
	

	return result;
}
*/
} // namespace
