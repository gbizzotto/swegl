
#include <filesystem>
#include <cassert>
#include <set>
#include <vector>

#include <swegl/data/gltf.hpp>
#include <json.hpp>

#define assertm(exp, msg) assert(((void)msg, exp))


namespace swegl
{

accessor_t::accessor_t(std::vector<buffer_view_t> & buffer_views, const nlohmann::json & accessor)
	: type(accessor["type"])
	, component_type(component_type_e(accessor["componentType"].template get<int>()))
	, count(accessor["count"].template get<int>())
{
	auto & bv = buffer_views[accessor["bufferView"].template get<int>()];
	stride = bv.byte_stride;
	if (stride == 0)
	{
		// tightly packed
		int size_of_component = [&]()
			{
				switch(component_type)
				{
					case component_type_e::         BYTE : return 1;
					case component_type_e::UNSIGNED_BYTE : return 1;
					case component_type_e::         SHORT: return 2;
					case component_type_e::UNSIGNED_SHORT: return 2;
					case component_type_e::UNSIGNED_INT  : return 4;
					case component_type_e::         FLOAT: return 4;
					default:                assert(false); return 0;
				}
			}();
		int number_of_components = [&]()
			{
				     if (type == "SCALAR") return  1;
				else if (type ==   "VEC2") return  2;
				else if (type ==   "VEC3") return  3;
				else if (type ==   "VEC4") return  4;
				else if (type ==   "MAT2") return  4;
				else if (type ==   "MAT3") return  9;
				else if (type ==   "MAT4") return 16;
				else        assert(false); return 0;
			}();
		stride = size_of_component * number_of_components;
	}

	buffer_view = bv.sub(accessor.value("byteOffset", 0), stride*count);
}


swegl::new_scene_t load_scene_json(const std::string & filename, char * file_beginning_ptr, nlohmann::json & j, std::vector<view_t<char>> & buffers)
{
	std::filesystem::path root_path = std::filesystem::path(filename).parent_path();

	swegl::new_scene_t result;

	std::vector<buffer_view_t> buffer_views;
	for (const auto & buffer_view : j["bufferViews"])
	{
		assert(buffer_view["buffer"].template get<size_t>() < buffers.size());
		assert(buffer_view["buffer"].template get<int>() >= 0);
		auto & buffer = buffers[buffer_view["buffer"].template get<int>()];
		buffer_views.push_back(buffer_view_t{buffer.sub(buffer_view.value("byteOffset", 0)
		                                               ,buffer_view["byteLength"].template get<int>())
		                                    ,buffer_view.value("byteStride", 0)
		                                    }
		                      );
	}
	std::vector<accessor_t> accessors;
	for (const auto & accessor : j["accessors"])
		accessors.push_back(accessor_t(buffer_views, accessor));

	if (j.contains("images"))
	{
		for (const auto & jimage : j["images"])
		{
			if (jimage.contains("uri"))
			{
				texture_t image = read_image_file(root_path / jimage["uri"].template get<std::string>());
				if (image.m_mipmaps.size() == 0)
					assertm(false, "cant load image format");
				result.images.emplace_back(std::move(image));
			}
			else if (jimage["mimeType"] == "image/png")
			{
				assert(jimage["bufferView"].template get<size_t>() < buffer_views.size());
				assert(jimage["bufferView"].template get<int>() >= 0);
				auto & buffer_view = buffer_views[jimage["bufferView"].template get<int>()];
				int file_offset = &buffer_view.data[0] - file_beginning_ptr;
				texture_t image = read_png_file(filename.c_str(), file_offset);
				result.images.emplace_back(std::move(image));
			}
			else if (jimage["mimeType"] == "image/jpg" || jimage["mimeType"] == "image/jpeg")
			{
				assert(jimage["bufferView"].template get<size_t>() < buffer_views.size());
				assert(jimage["bufferView"].template get<int>() >= 0);
				auto & buffer_view = buffer_views[jimage["bufferView"].template get<int>()];
				int file_offset = &buffer_view.data[0] - file_beginning_ptr;
				texture_t image = read_jpeg_file(filename.c_str(), file_offset);
				result.images.emplace_back(std::move(image));
			}
			else
			{
				assertm(false, "dont know how to load image");
			}
		}
	}

	if (j.contains("materials"))
	{
		result.materials.reserve(j["materials"].size());
		for (const auto & material : j["materials"])
		{
			pixel_colors color{255,255,255,255};
			float metallic = 1.0;
			float roughness = 1.0;
			int img_idx = -1;
			bool double_sided = material.value("doubleSided", false);
			if ( ! material.contains("pbrMetallicRoughness"))
			{
				result.materials.push_back(material_t{color, metallic, roughness, img_idx});
				continue;
			}
			if (material["pbrMetallicRoughness"].contains("baseColorFactor"))
			{
				color = pixel_colors{(unsigned char)(255 * material["pbrMetallicRoughness"]["baseColorFactor"][2].template get<float>()) // g // b
			                        ,(unsigned char)(255 * material["pbrMetallicRoughness"]["baseColorFactor"][1].template get<float>()) // g
			                        ,(unsigned char)(255 * material["pbrMetallicRoughness"]["baseColorFactor"][0].template get<float>()) // r
			                        ,(unsigned char)(255 * material["pbrMetallicRoughness"]["baseColorFactor"][3].template get<float>()) // a
			                        };
			}
			if (material["pbrMetallicRoughness"].contains("baseColorTexture"))
			{
				int texture_idx = material["pbrMetallicRoughness"]["baseColorTexture"]["index"].template get<int>();
				img_idx = j["textures"][texture_idx]["source"].template get<int>();
			}
			if (material["pbrMetallicRoughness"].contains("metallicFactor"))
			{
				metallic = material["pbrMetallicRoughness"]["metallicFactor"].template get<float>();
			}
			if (material["pbrMetallicRoughness"].contains("roughnessFactor"))
			{
				roughness = material["pbrMetallicRoughness"]["roughnessFactor"].template get<float>();
			}
			result.materials.push_back(material_t{color, metallic, roughness, img_idx, double_sided});
		}
	}

	struct fake_mesh_t
	{
		std::map<new_mesh_vertex_t,int> vertices;  // unique vertices of the mesh (duplicates removed) + int will contain the index in the map
		using temp_idx = std::map<new_mesh_vertex_t,int>::iterator;
		struct fake_primitive_t
		{
			std::vector<temp_idx> temp_indices; // conversion table between gltf indices and our vertices
			std::vector<std::tuple<temp_idx,temp_idx,temp_idx>> triangles; // i0, i1, i2 point into vertices
			int material_idx;
		};
		std::vector<fake_primitive_t> fake_primitives;
		int node_idx;
	};
	std::vector<fake_mesh_t> fake_meshes;
	if (j.contains("meshes"))
		fake_meshes.resize(j["meshes"].size());



	if (j.contains("nodes"))
	{
		for (const auto & jnode : j["nodes"])
		{
			node_t & node = result.nodes.emplace_back();
			if (jnode.contains("rotation"))
			{
				assert(jnode["rotation"].size() == 4);
				float q0 = jnode["rotation"][0].template get<float>();
				float q1 = jnode["rotation"][1].template get<float>();
				float q2 = jnode["rotation"][2].template get<float>();
				float q3 = jnode["rotation"][3].template get<float>();
				node.rotation = matrix44_t::from_quaternion(q0, q1, q2, q3);
			}
			if (jnode.contains("translation"))
			{
				assert(jnode["translation"].size() == 3);
				node.translation.x() = jnode["translation"][0].template get<float>();
				node.translation.y() = jnode["translation"][1].template get<float>();
				node.translation.z() = jnode["translation"][2].template get<float>();
			}
			if (jnode.contains("scale"))
			{
				assert(jnode["scale"].size() == 3);
				node.scale.x() = jnode["scale"][0].template get<float>();
				node.scale.y() = jnode["scale"][1].template get<float>();
				node.scale.z() = jnode["scale"][2].template get<float>();
			}
			if (jnode.contains("matrix"))
			{
				assert(jnode["matrix"].size() == 16);
				matrix44_t m
					{ // gltf is column-major, swegl is row-major
						jnode["matrix"][ 0].template get<float>(),
						jnode["matrix"][ 4].template get<float>(),
						jnode["matrix"][ 8].template get<float>(),
						jnode["matrix"][12].template get<float>(),

						jnode["matrix"][ 1].template get<float>(),
						jnode["matrix"][ 5].template get<float>(),
						jnode["matrix"][ 9].template get<float>(),
						jnode["matrix"][13].template get<float>(),

						jnode["matrix"][ 2].template get<float>(),
						jnode["matrix"][ 6].template get<float>(),
						jnode["matrix"][10].template get<float>(),
						jnode["matrix"][14].template get<float>(),

						jnode["matrix"][ 3].template get<float>(),
						jnode["matrix"][ 7].template get<float>(),
						jnode["matrix"][11].template get<float>(),
						jnode["matrix"][15].template get<float>(),
					};
				node.translation.x() = m[0][3];
				node.translation.y() = m[1][3];
				node.translation.z() = m[2][3];
				node.scale.x() = vector_t(m[0][0], m[0][1], m[0][2]).len();
				node.scale.y() = vector_t(m[1][0], m[1][1], m[1][2]).len();
				node.scale.z() = vector_t(m[2][0], m[2][1], m[2][2]).len();
				m[0][3] = 0;
				m[1][3] = 0;
				m[2][3] = 0;
				if (node.scale.x() != 0)
				{
					m[0][0] /= node.scale.x();
					m[1][0] /= node.scale.x();
					m[2][0] /= node.scale.x();
				}
				if (node.scale.y() != 0)
				{
					m[0][1] /= node.scale.y();
					m[1][1] /= node.scale.y();
					m[2][1] /= node.scale.y();
				}
				if (node.scale.z() != 0)
				{
					m[0][2] /= node.scale.z();
					m[1][2] /= node.scale.z();
					m[2][2] /= node.scale.z();
				}
				node.rotation = m;
			}
			if (jnode.contains("mesh"))
			{
				auto & fake_mesh = fake_meshes[jnode["mesh"].template get<int>()];
				fake_mesh.node_idx = result.nodes.size() - 1;
			}
			if (jnode.contains("children"))
				for (size_t k=0 ; k<jnode["children"].size() ; k++)
					node.children_idx.push_back(jnode["children"][k].template get<int>());
		}

		// get root nodes
		for (size_t i=0 ; i<result.nodes.size() ; i++)
			for (size_t k=0 ; k<result.nodes[i].children_idx.size() ; k++)
				result.nodes[result.nodes[i].children_idx[k]].root = false;
		for (size_t i=0 ; i<result.nodes.size() ; i++)
			if (result.nodes[i].root)
				result.root_nodes.emplace_back(i);
	}


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

	size_t total_vertice_count = 0;

	if (j.contains("meshes"))
	{
		auto & jmeshes = j["meshes"];
		for (size_t jmesh_idx = 0 ; jmesh_idx < jmeshes.size() ; jmesh_idx++)
		{
			const auto & mesh = jmeshes[jmesh_idx];
			if ( ! mesh.contains("primitives"))
				continue;

			fake_mesh_t & fake_mesh = fake_meshes[jmesh_idx];
			
			for (size_t i=0 ; i<mesh["primitives"].size() ; i++)
			{
				fake_mesh_t::fake_primitive_t & fake_primitive = fake_mesh.fake_primitives.emplace_back();
				//primitive_t & primitive = node.primitives.emplace_back();

				fake_primitive.material_idx = mesh["primitives"][i].value("material", -1);
				index_mode_t mode = (index_mode_t) mesh["primitives"][i].value("mode", 4);

				assert(mesh["primitives"][i]["attributes"]["POSITION"].template get<size_t>() < accessors.size());
				assert(mesh["primitives"][i]["attributes"]["POSITION"].template get<int>() >= 0);
				accessor_t & accessor_vertices = accessors[mesh["primitives"][i]["attributes"]["POSITION"].template get<int>()];

				for (int vi=0 ; vi<accessor_vertices.count ; vi++)
				{
					new_mesh_vertex_t vertex;
					vertex.node_idx = fake_mesh.node_idx;
					vertex.v.x() = *(float*)&accessor_vertices.buffer_view.data[0+vi*accessor_vertices.stride];
					vertex.v.y() = *(float*)&accessor_vertices.buffer_view.data[4+vi*accessor_vertices.stride];
					vertex.v.z() = *(float*)&accessor_vertices.buffer_view.data[8+vi*accessor_vertices.stride];

					if (mesh["primitives"][i]["attributes"].contains("NORMAL"))
					{
						accessor_t & accessor_normals = accessors[mesh["primitives"][i]["attributes"]["NORMAL"  ].template get<int>()];
						vertex.normal.x() = *(float*)&accessor_normals.buffer_view.data[0+vi*accessor_normals.stride];
						vertex.normal.y() = *(float*)&accessor_normals.buffer_view.data[4+vi*accessor_normals.stride];
						vertex.normal.z() = *(float*)&accessor_normals.buffer_view.data[8+vi*accessor_normals.stride];
					}
					else
					{
						vertex.normal.x() = 0;
						vertex.normal.y() = 0;
						vertex.normal.z() = 0;
					}
					if (mesh["primitives"][i]["attributes"].contains("TEXCOORD_0"))
					{
						accessor_t & accessor_texcoords = accessors[mesh["primitives"][i]["attributes"]["TEXCOORD_0"].template get<int>()];
						vertex.tex_coords.x() = *(float*)&accessor_texcoords.buffer_view.data[4+vi*accessor_texcoords.stride];
						vertex.tex_coords.y() = *(float*)&accessor_texcoords.buffer_view.data[0+vi*accessor_texcoords.stride];
					}
					else
					{
						vertex.tex_coords.x() = 0;
						vertex.tex_coords.y() = 0;
					}

					++total_vertice_count;
					auto it = fake_mesh.vertices.insert(std::make_pair(vertex,0)).first;
					fake_primitive.temp_indices.push_back(it);
				}

				if (mesh["primitives"][i].contains("indices"))
				{
					assert(mesh["primitives"][i]["indices"].template get<size_t>() < accessors.size());
					assert(mesh["primitives"][i]["indices"].template get<int>() >= 0);
					accessor_t & accessor_indices = accessors[mesh["primitives"][i]["indices"].template get<int>()];
					for (int j=2 ; j<accessor_indices.count ; j++)
					{
						fake_primitive.triangles.emplace_back();
						auto & triangle = fake_primitive.triangles.back();
						if (mode == index_mode_t::TRIANGLES)
						{
							std::get<0>(triangle) = fake_primitive.temp_indices[*(std::uint16_t*)&accessor_indices.buffer_view.data[(j-2)*accessor_indices.stride]];
							std::get<1>(triangle) = fake_primitive.temp_indices[*(std::uint16_t*)&accessor_indices.buffer_view.data[(j-1)*accessor_indices.stride]];
							std::get<2>(triangle) = fake_primitive.temp_indices[*(std::uint16_t*)&accessor_indices.buffer_view.data[(j-0)*accessor_indices.stride]];
							j+=2;
						}
						else if (mode == index_mode_t::TRIANGLE_STRIP)
						{
							std::get<0>(triangle) = fake_primitive.temp_indices[*(std::uint16_t*)&accessor_indices.buffer_view.data[(j-2        )*accessor_indices.stride]];
							std::get<1>(triangle) = fake_primitive.temp_indices[*(std::uint16_t*)&accessor_indices.buffer_view.data[(j-1+(j&0x1))*accessor_indices.stride]];
							std::get<2>(triangle) = fake_primitive.temp_indices[*(std::uint16_t*)&accessor_indices.buffer_view.data[(j  -(j&0x1))*accessor_indices.stride]];
						}
						else if (mode == index_mode_t::TRIANGLE_FAN)
						{
							std::get<0>(triangle) = fake_primitive.temp_indices[*(std::uint16_t*)&accessor_indices.buffer_view.data[(  0)*accessor_indices.stride]];
							std::get<1>(triangle) = fake_primitive.temp_indices[*(std::uint16_t*)&accessor_indices.buffer_view.data[(j-1)*accessor_indices.stride]];
							std::get<2>(triangle) = fake_primitive.temp_indices[*(std::uint16_t*)&accessor_indices.buffer_view.data[(j-0)*accessor_indices.stride]];
						}
					}
				}
			}
		}
	}

	// let's flatten fake_meshes into scene
	for (auto & fake_mesh : fake_meshes)
	{
		unsigned int base_idx = result.vertices.size();
		for (int i=0 ; auto & [it,idx] : fake_mesh.vertices)
			idx = i++;
		for (auto & fake_vertex : fake_mesh.vertices)
			result.vertices.push_back(fake_vertex.first);
		for (auto & fake_primitive : fake_mesh.fake_primitives)
			for (auto & fake_triangle : fake_primitive.triangles)
			{
				result.triangles.push_back(new_triangle_t{
						base_idx + (unsigned int)std::get<0>(fake_triangle)->second, // i0
						base_idx + (unsigned int)std::get<1>(fake_triangle)->second, // i1
						base_idx + (unsigned int)std::get<2>(fake_triangle)->second, // i2
						{0,0,0}, // normal
						{0,0,0}, // normal_workd
						fake_primitive.material_idx,
						fake_mesh.node_idx,
						true, // yes
						false // backface
					});
			}
	}

	std::cout << "Scene's vertex count: " << total_vertice_count << std::endl
	          << "Our vertex count: " << result.vertices.size() << std::endl;

	struct sampler_t
	{
		int input;
		int output;
		std::string interpolation_type;
	};

	if (j.contains("animations"))
	{
		for (auto & janim : j["animations"])
		{
			auto & animation = result.animations.emplace_back();

			std::vector<sampler_t> samplers;
			for (auto & jsampler : janim["samplers"])
				samplers.push_back(sampler_t{jsampler["input"].template get<int>()
				                            ,jsampler["output"].template get<int>()
				                            ,jsampler.value("interpolation", "LINEAR")});
			for (auto & jchannel : janim["channels"])
			{
				int node_idx = jchannel["target"]["node"].template get<int>();
				std::string path_str = jchannel["target"]["path"].template get<std::string>();
				animation_channel_t::path_t path = [&]()
					{
						     if (path_str == "scale"      ) return animation_channel_t::path_t::SCALE      ;
						else if (path_str == "rotation"   ) return animation_channel_t::path_t::ROTATION   ;
						else if (path_str == "translation") return animation_channel_t::path_t::TRANSLATION;
						else if (path_str == "weights"    ) return animation_channel_t::path_t::WEIGHTS    ;
						else                                return animation_channel_t::path_t::NONE       ;
					}();
				auto & animation_channel = animation.channels.emplace_back(animation_channel_t{node_idx, path, {}});

				assert(jchannel["sampler"].template get<size_t>() < samplers.size());
				assert(jchannel["sampler"].template get<int>() >= 0);
				sampler_t & sampler = samplers[jchannel["sampler"].template get<int>()];
				assert(sampler. input < (int)accessors.size());
				assert(sampler.output < (int)accessors.size());
				accessor_t & accessor_times  = accessors[sampler.input];
				accessor_t & accessor_values = accessors[sampler.output];

				animation_channel.steps.reserve(accessor_times.count);
				for (int i=0 ; i<accessor_times.count ; i++)
				{
					float time = *(float*)&accessor_times.buffer_view.data[i*accessor_times.stride];
					vec4f_t value(0,0,0,1);
					if (animation_channel.path == animation_channel_t::path_t::SCALE)
					{
						value.x() = *(float*)&accessor_values.buffer_view.data[ 0+i*accessor_values.stride];
						value.y() = *(float*)&accessor_values.buffer_view.data[ 4+i*accessor_values.stride];
						value.z() = *(float*)&accessor_values.buffer_view.data[ 8+i*accessor_values.stride];
					}
					else if (animation_channel.path == animation_channel_t::path_t::ROTATION)
					{
						value.x() = *(float*)&accessor_values.buffer_view.data[ 0+i*accessor_values.stride];
						value.y() = *(float*)&accessor_values.buffer_view.data[ 4+i*accessor_values.stride];
						value.z() = *(float*)&accessor_values.buffer_view.data[ 8+i*accessor_values.stride];
						value.w() = *(float*)&accessor_values.buffer_view.data[12+i*accessor_values.stride];
					}
					else if (animation_channel.path == animation_channel_t::path_t::TRANSLATION)
					{
						value.x() = *(float*)&accessor_values.buffer_view.data[ 0+i*accessor_values.stride];
						value.y() = *(float*)&accessor_values.buffer_view.data[ 4+i*accessor_values.stride];
						value.z() = *(float*)&accessor_values.buffer_view.data[ 8+i*accessor_values.stride];
					}
					else if (animation_channel.path == animation_channel_t::path_t::WEIGHTS)
					{
						//assert(false);
					}
					else
					{
						assert(false);
					}
					animation_channel.steps.emplace_back(animation_step_t{time, value});
				}
				std::sort(animation_channel.steps.begin(), animation_channel.steps.end(), [](const animation_step_t & left, const animation_step_t & right)
					{
						return left.time < right.time;
					});
				animation.end_time = std::max(animation.end_time, animation_channel.steps.back().time);
			}
		}
	}

	return result;
}

swegl::new_scene_t load_scene_glb(const std::string & filename)
{
	std::filesystem::path root_path = std::filesystem::path(filename).parent_path();

	auto [glb_data, file_size] = read_file(filename);

	glb_header * header = (glb_header*) glb_data.get();
	std::vector<glb_chunk> chunks;
	for (int i=12 ; i<header->length ; i += 8 + chunks.back().length)
		chunks.push_back(glb_chunk{*(unsigned int*)&glb_data[i], *(unsigned int*)&glb_data[i+4], (char*)&glb_data[i+8]});

	nlohmann::json j = nlohmann::json::parse(chunks[0].data, chunks[0].data + chunks[0].length);

	std::vector<view_t<char>> buffers;
	for (auto & jbuffer : j["buffers"])
	{
		if (jbuffer.contains("uri"))
		{
			auto [data_uptr,file_size] = read_file(root_path / jbuffer["uri"].template get<std::string>());
			buffers.emplace_back(view_t<char>{data_uptr.get(), data_uptr.get() + jbuffer["byteLength"].template get<int>(), std::move(data_uptr)});
		}
		else
			buffers.emplace_back(view_t<char>{chunks[1].data, chunks[1].data + jbuffer["byteLength"].template get<int>(), nullptr});
	}

	return load_scene_json(filename, glb_data.get(), j, buffers);
}
swegl::new_scene_t load_scene_gltf(const std::string & filename)
{
	std::filesystem::path root_path = std::filesystem::path(filename).parent_path();
	auto [gltf_data, file_size] = read_file(filename);
	nlohmann::json j = nlohmann::json::parse(gltf_data.get(), gltf_data.get() + file_size);

	std::vector<view_t<char>> buffers;
	for (auto & jbuffer : j["buffers"])
	{
		if (jbuffer.contains("uri"))
		{
			auto [data_uptr,file_size] = read_file(root_path / jbuffer["uri"].template get<std::string>());
			buffers.emplace_back(view_t<char>{data_uptr.get(), data_uptr.get() + jbuffer["byteLength"].template get<int>(), std::move(data_uptr)});
		}
		else
			assertm(false, "can't load buffer from self in gltf mode.");
	}

	return load_scene_json(filename, nullptr, j, buffers);
}

swegl::new_scene_t load_scene(std::string filename)
{
	std::string filename_lower = to_lower(filename);
	     if (ends_with(filename_lower, "glb" )) return load_scene_glb (filename);
	else if (ends_with(filename_lower, "gltf")) return load_scene_gltf(filename);
	else                                        return new_scene_t();
}


} // namespace
