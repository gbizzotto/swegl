
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
	std::unique_ptr<char[]> data;
	inline size_t size() const { return end-begin; }
	inline       T & operator[](size_t idx)       { return *(begin+idx); }
	inline const T & operator[](size_t idx) const { return *(begin+idx); }
	inline view_t sub(int offset, int length) { return {begin+offset, begin+offset+length, nullptr}; }
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
	int              stride;

	accessor_t(std::vector<buffer_view_t> & buffer_views, const nlohmann::json & accessor);
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

swegl::scene_t load_scene(std::string filename);

} // mamespace
