
#pragma once

#include <swegl/data/texture.hpp>

#include <swegl/projection/camera.hpp>
#include <swegl/projection/matrix44.hpp>
#include <swegl/projection/vec2f.hpp>
#include <swegl/projection/points.hpp>

#include <swegl/render/viewport.hpp>
#include <swegl/render/renderer.hpp>

#if defined(_MSC_VER)
	#if !defined(BUILDING_SWEGL_LIB)

		#define SWEGL_LIB_NAME "swegl"

		#if defined(_M_X64)
			#define SWEGL_ARCH_SUFIX "_64"
		#else
			#if defined(_M_IX86)
				#define SWEGL_ARCH_SUFIX "_32"
			#else
				#error "Invalid architecture!"
			#endif
		#endif

		#if defined(_DEBUG)
			#define SWEGL_DEBUG_SUFFIX "_d"
		#else
			#define SWEGL_DEBUG_SUFFIX "_r"
		#endif

		#if _MSC_VER >= 2000
			#error "Invalid compiler!"
		#elif _MSC_VER >= 1900
			#define SWEGL_COMPILER_VERSION "_VC140"
		#elif _MSC_VER >= 1800
			#define SWEGL_COMPILER_VERSION "_VC120"
		#elif _MSC_VER >= 1600
			#define SWEGL_COMPILER_VERSION "_VC100"
		#elif _MSC_VER >= 1500
			#define SWEGL_COMPILER_VERSION "_VC90"
		#else
			#error "Invalid compiler!"
		#endif

		#if defined(_DLL)
			#define SWEGL_RT_SUFIX "_mtdll"
		#else
			#define SWEGL_RT_SUFIX "_mt"
		#endif

		#pragma comment(lib, SWEGL_LIB_NAME SWEGL_ARCH_SUFIX SWEGL_DEBUG_SUFFIX SWEGL_COMPILER_VERSION SWEGL_RT_SUFIX ".lib") 
	#endif
#endif
