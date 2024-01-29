
#include <filesystem>
#include <cassert>

#include <swegl/data/gltf.hpp>

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


swegl::scene_t load_scene_json(const std::string & filename, char * file_beginning_ptr, nlohmann::json & j, std::vector<view_t<char>> & buffers)
{
	std::filesystem::path root_path = std::filesystem::path(filename).parent_path();

	swegl::scene_t result;

	std::vector<buffer_view_t> buffer_views;
	for (const auto & buffer_view : j["bufferViews"])
	{
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
				auto & buffer_view = buffer_views[jimage["bufferView"].template get<int>()];
				int file_offset = &buffer_view.data[0] - file_beginning_ptr;
				texture_t image = read_png_file(filename.c_str(), file_offset);
				result.images.emplace_back(std::move(image));
			}
			else if (jimage["mimeType"] == "image/jpg" || jimage["mimeType"] == "image/jpeg")
			{
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
				metallic = material["pbrMetallicRoughness"]["metallicFactor"].template get<float>();
			if (material["pbrMetallicRoughness"].contains("roughnessFactor"))
				roughness = material["pbrMetallicRoughness"]["roughnessFactor"].template get<float>();
			result.materials.push_back(material_t{color, metallic, roughness, img_idx});
		}
	}

	std::vector<node_t> temp_meshes;
	if (j.contains("meshes"))
	{
		temp_meshes.reserve(j["meshes"].size());
		auto & meshes = j["meshes"];
		for (const auto & mesh : meshes)
		{
			if ( ! mesh.contains("primitives"))
				continue;

			node_t & node = temp_meshes.emplace_back();
			for (size_t i=0 ; i<mesh["primitives"].size() ; i++)
			{
				primitive_t & primitive = node.primitives.emplace_back();

				primitive.material_id = mesh["primitives"][i]["material"].template get<int>();
				primitive.mode = (decltype(primitive.mode)) mesh["primitives"][i].value("mode", 4);

				accessor_t & accessor_vertices = accessors[mesh["primitives"][i]["attributes"]["POSITION"].template get<int>()];

				primitive.vertices.resize(accessor_vertices.count + 2);
				primitive.vertices.reserve(accessor_vertices.count + 2);
				for (int i=0 ; i<accessor_vertices.count ; i++)
				{
					auto & vertex = primitive.vertices[i];
					vertex.v.x() = *(float*)&accessor_vertices.buffer_view.data[0+i*accessor_vertices.stride];
					vertex.v.y() = *(float*)&accessor_vertices.buffer_view.data[4+i*accessor_vertices.stride];
					vertex.v.z() = *(float*)&accessor_vertices.buffer_view.data[8+i*accessor_vertices.stride];
				}
				if (mesh["primitives"][i]["attributes"].contains("NORMAL"))
				{
					accessor_t & accessor_normals  = accessors[mesh["primitives"][i]["attributes"]["NORMAL"  ].template get<int>()];
					for (int i=0 ; i<accessor_normals.count ; i++)
					{
						auto & vertex = primitive.vertices[i];
						vertex.normal.x() = *(float*)&accessor_normals.buffer_view.data[0+i*accessor_normals.stride];
						vertex.normal.y() = *(float*)&accessor_normals.buffer_view.data[4+i*accessor_normals.stride];
						vertex.normal.z() = *(float*)&accessor_normals.buffer_view.data[8+i*accessor_normals.stride];
					}
				}
				if (mesh["primitives"][i]["attributes"].contains("TEXCOORD_0"))
				{
					accessor_t & accessor_texcoords = accessors[mesh["primitives"][i]["attributes"]["TEXCOORD_0"].template get<int>()];
					for (int i=0 ; i<accessor_texcoords.count ; i++)
					{
						auto & vertex = primitive.vertices[i];
						vertex.tex_coords.x() = *(float*)&accessor_texcoords.buffer_view.data[4+i*accessor_texcoords.stride];
						vertex.tex_coords.y() = *(float*)&accessor_texcoords.buffer_view.data[0+i*accessor_texcoords.stride];				
					}
				}

				if (mesh["primitives"][i].contains("indices"))
				{
					accessor_t & accessor_indices = accessors[mesh["primitives"][i]["indices"].template get<int>()];
					primitive.indices.reserve(accessor_indices.count);
					for (int i=0 ; i<accessor_indices.count ; i++)
					{
						assert(i*accessor_indices.buffer_view.byte_stride + 3 < (int)accessor_indices.buffer_view.data.size());
						primitive.indices.push_back(*(std::uint16_t*)&accessor_indices.buffer_view.data[i*accessor_indices.stride]);
					}
				}
			}
		}
	}

	if (j.contains("nodes"))
	{
		for (const auto & jnode : j["nodes"])
		{
			node_t & node = result.nodes.emplace_back();
			if (jnode.contains("rotation"))
			{
				float q0 = jnode["rotation"][0].template get<float>();
				float q1 = jnode["rotation"][1].template get<float>();
				float q2 = jnode["rotation"][2].template get<float>();
				float q3 = jnode["rotation"][3].template get<float>();
				node.rotation = matrix44_t::from_quaternion(q0, q1, q2, q3);
			}
			if (jnode.contains("mesh"))
				node.primitives = std::move(temp_meshes[jnode["mesh"].template get<int>()].primitives);
			if (jnode.contains("children"))
				for (size_t k=0 ; k<jnode["children"].size() ; k++)
					node.children_idx.push_back(jnode["children"][k].template get<int>());
		}

		for (size_t i=0 ; i<result.nodes.size() ; i++)
			for (size_t k=0 ; k<result.nodes[i].children_idx.size() ; i++)
				result.nodes[result.nodes[i].children_idx[k]].root = false;
		for (size_t i=0 ; i<result.nodes.size() ; i++)
			if (result.nodes[i].root)
				result.root_nodes.emplace_back(i);
	}

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

				sampler_t & sampler = samplers[jchannel["sampler"]];
				accessor_t & accessor_times  = accessors[sampler.input];
				accessor_t & accessor_values = accessors[sampler.output];

				animation_channel.steps.reserve(accessor_times.count);
				for (int i=0 ; i<accessor_times.count ; i++)
				{
					float time = *(float*)&accessor_times.buffer_view.data[0+i*accessor_times.stride];
					vec4f_t value(0,0,0,0);
					if (animation_channel.path == animation_channel_t::path_t::SCALE)
					{
						value.x() = *(float*)&accessor_values.buffer_view.data[ 0+i*accessor_values.stride];
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
					}
					animation_channel.steps.emplace_back(animation_step_t{time, value});
					animation.end_time = std::max(animation.end_time, time);
				}
				std::sort(animation_channel.steps.begin(), animation_channel.steps.end(), [](const animation_step_t & left, const animation_step_t & right)
					{
						return left.time < right.time;
					});
			}
		}
	}

	return result;
}

swegl::scene_t load_scene_glb(const std::string & filename)
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
swegl::scene_t load_scene_gltf(const std::string & filename)
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

swegl::scene_t load_scene(std::string filename)
{
	std::string filename_lower = to_lower(filename);
	     if (ends_with(filename_lower, "glb" )) return load_scene_glb (filename);
	else if (ends_with(filename_lower, "gltf")) return load_scene_gltf(filename);
	else                                        return scene_t();
}


} // namespace
