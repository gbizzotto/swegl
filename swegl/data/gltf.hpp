
#pragma once

#include <string>
#include <vector>
#include <memory>

#include <swegl/misc/file.hpp>
#include <swegl/data/model.hpp>
#include <swegl/misc/json.hpp>
#include <swegl/misc/image.hpp>


namespace swegl
{

enum buffer_target_e
{
	ARRAY_BUFFER         = 34962,
	ELEMENT_ARRAY_BUFFER = 34963,
};

enum primitive_mode_t
{
	POINTS         = 0,
	LINES          = 1,
	LINE_LOOP      = 2,
	LINE_STRIP     = 3,
	TRIANGLES      = 4,
	TRIANGLE_STRIP = 5,
	TRIANGLE_FAN   = 6,
};

template<typename T>
struct view_t
{
	T *begin;
	T *end;
	size_t size() const { return end-begin; }
	      T & operator[](size_t idx)       { return *(begin+idx); }
	const T & operator[](size_t idx) const { return *(begin+idx); }
	view_t sub(int offset, int length) { return {begin+offset, begin+offset+length}; }
};

struct buffer_view_t
{
	view_t<char> data;
	int byte_stride;

	buffer_view_t sub(int offset, int length) { return buffer_view_t{data.sub(offset, length), byte_stride}; }
};

struct accessor_t
{
	enum type_e
	{
		SCALAR = 0,
		VEC2   = 1,
		VEC3   = 2,
		VEC4   = 3,
		MAT2   = 4,
		MAT3   = 5,
		MAT4   = 6,
	};
	enum component_type_e
	{
		BYTE           = 5120,
		UNSIGNED_BYTE  = 5121,
		SHORT          = 5122,
		UNSIGNED_SHORT = 5123,
		UNSIGNED_INT   = 5125,
		FLOAT          = 5126,
	};
	buffer_view_t    buffer_view;
	std::string      type;
	component_type_e component_type;
	int              count;

	accessor_t(std::vector<buffer_view_t> buffer_views, const nlohmann::json & accessor)
		: type(accessor["type"])
		, component_type(component_type_e(accessor["componentType"].template get<int>()))
		, count(accessor["count"].template get<int>())
	{
		auto & bv = buffer_views[accessor["bufferView"].template get<int>()];
		int stride = bv.byte_stride;
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
						case component_type_e::         FLOAT: return 5;
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

		buffer_view = bv.sub(accessor["byteOffset"].template get<int>(), stride*count);
		buffer_view.byte_stride = stride;
	}
};


struct glb_header
{
	int magic;
	int vesion;
	int length;
};
struct glb_chunk
{
	unsigned int length;
	unsigned int type;
	char * data;
};

swegl::scene_t load_scene(std::string filename)
{
	swegl::scene_t result;

	std::unique_ptr<char[]> glb_data = read_file(filename);

	glb_header * header = (glb_header*) glb_data.get();
	std::vector<glb_chunk> chunks;
	for (int i=12 ; i<header->length ; i += 8 + chunks.back().length)
		chunks.push_back(glb_chunk{*(unsigned int*)&glb_data[i], *(unsigned int*)&glb_data[i+4], (char*)&glb_data[i+8]});

	nlohmann::json j = nlohmann::json::parse(chunks[0].data, chunks[0].data + chunks[0].length);


	std::vector<view_t<char>> buffers;
	for (auto & buffer : j["buffers"])
		buffers.emplace_back(view_t<char>{chunks[1].data, chunks[1].data + buffer["byteLength"].template get<int>()});
	std::vector<buffer_view_t> buffer_views;
	for (auto & buffer_view : j["bufferViews"])
	{
		auto & buffer = buffers[buffer_view["buffer"].template get<int>()];
		buffer_views.push_back(buffer_view_t{buffer.sub(buffer_view["byteOffset"].template get<int>()
		                                               ,buffer_view["byteLength"].template get<int>())
		                                    ,buffer_view.value("byteStride", 0)
		                                    }
		                      );
	}
	std::vector<accessor_t> accessors;
	for (auto & accessor : j["accessors"])
		accessors.push_back(accessor_t(buffer_views, accessor));

	if (j.contains("images"))
	{
		for (auto & image : j["images"])
		{
			if (image["mimeType"] == "image/png")
			{
				auto & buffer_view = buffer_views[image["bufferView"].template get<int>()];
				int file_offset = &buffer_view.data[0] - glb_data.get();
				texture_t image = read_png_file(filename.c_str(), file_offset);
				result.images.emplace_back(std::move(image));
			}
		}
	}

	if (j.contains("materials"))
	{
		result.materials.reserve(j["materials"].size());
		for (auto & material : j["materials"])
		{
			if ( ! material.contains("pbrMetallicRoughness"))
				continue;
			pixel_colors color{255,255,255,255};
			float metallic = 1.0;
			float roughness = 1.0;
			int img_idx = -1;
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


	auto & meshes = j["meshes"];
	for (auto & mesh : meshes)
	{
		model_t & model = result.models.emplace_back();
		model.orientation = matrix44_t::Identity;
		model.position = {0,0,0};

		if ( ! mesh.contains("primitives"))
			continue;

		for (size_t i=0 ; i<mesh["primitives"].size() ; i++)
		{
			primitive_t & primitive = model.primitives.emplace_back();

			primitive.material_id = mesh["primitives"][i]["material"].template get<int>();
			primitive.mode        = (decltype(primitive.mode)) mesh["primitives"][i]["mode"    ].template get<int>();

			accessor_t & accessor_vertices = accessors[mesh["primitives"][i]["attributes"]["POSITION"].template get<int>()];

			primitive.vertices.resize(accessor_vertices.count + 2);
			primitive.vertices.reserve(accessor_vertices.count + 2);
			for (int i=0 ; i<accessor_vertices.count ; i++)
			{
				auto & vertex = primitive.vertices[i];
				vertex.v.x() = *(float*)&accessor_vertices.buffer_view.data[0+i*accessor_vertices.buffer_view.byte_stride];
				vertex.v.y() = *(float*)&accessor_vertices.buffer_view.data[4+i*accessor_vertices.buffer_view.byte_stride];
				vertex.v.z() = *(float*)&accessor_vertices.buffer_view.data[8+i*accessor_vertices.buffer_view.byte_stride];
			}
			if (mesh["primitives"][i]["attributes"].contains("NORMAL"))
			{
				accessor_t & accessor_normals  = accessors[mesh["primitives"][i]["attributes"]["NORMAL"  ].template get<int>()];
				for (int i=0 ; i<accessor_normals.count ; i++)
				{
					auto & vertex = primitive.vertices[i];
					vertex.normal.x() = *(float*)&accessor_normals.buffer_view.data[0+i*accessor_normals.buffer_view.byte_stride];
					vertex.normal.y() = *(float*)&accessor_normals.buffer_view.data[4+i*accessor_normals.buffer_view.byte_stride];
					vertex.normal.z() = *(float*)&accessor_normals.buffer_view.data[8+i*accessor_normals.buffer_view.byte_stride];
				}
			}
			if (mesh["primitives"][i]["attributes"].contains("TEXCOORD_0"))
			{
				accessor_t & accessor_texcoords = accessors[mesh["primitives"][i]["attributes"]["TEXCOORD_0"].template get<int>()];
				for (int i=0 ; i<accessor_texcoords.count ; i++)
				{
					auto & vertex = primitive.vertices[i];
					vertex.tex_coords.x() = *(float*)&accessor_texcoords.buffer_view.data[0+i*accessor_texcoords.buffer_view.byte_stride];
					vertex.tex_coords.y() = *(float*)&accessor_texcoords.buffer_view.data[4+i*accessor_texcoords.buffer_view.byte_stride];				
				}
			}

			if (mesh["primitives"][i].contains("indices"))
			{
				accessor_t & accessor_indices = accessors[mesh["primitives"][i]["indices"].template get<int>()];
				primitive.indices.reserve(accessor_indices.count);
				for (int i=0 ; i<accessor_indices.count ; i++)
					primitive.indices.push_back(*(std::uint16_t*)&accessor_indices.buffer_view.data[i*accessor_indices.buffer_view.byte_stride]);
			}
		}
	}

	return result;
}

} // mamespace
