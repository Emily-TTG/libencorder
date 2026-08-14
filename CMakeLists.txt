cmake_minimum_required(VERSION 3.22)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(encorder C CXX)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
cmake_policy(SET CMP0077 NEW)

include(CMakeDependentOption)

include(${CMAKE_CURRENT_LIST_DIR}/cmake/Diagnostics.cmake)

set(ENCORDER_RELEASE_CONFIGS "Release,RelWithDebInfo,MinSizeRel")
set(ENCORDER_DEBUG_CONFIGS "Debug")

option(ENCORDER_BACKEND_VULKAN "Enable the Vulkan backend" ON)
option(ENCORDER_BACKEND_NVENC "Enable the NVENC backend" OFF)
option(ENCORDER_BACKEND_AMF "Enable the AMF backend" OFF)
option(ENCORDER_BACKEND_VPL "Enable the oneVPL backend" OFF)
option(ENCORDER_BACKEND_VAAPI "Enable the VAAPI backend" OFF)
cmake_dependent_option(ENCORDER_BACKEND_VIDEOTOOLBOX "Enable the VideoToolbox backend" ON APPLE OFF)
cmake_dependent_option(ENCORDER_BACKEND_D3D12 "Enable the D3D12 backend" ON WIN32 OFF)
option(ENCORDER_BACKEND_COMPUTE "Enable the compute backend" OFF)
option(ENCORDER_SOFTWARE_FALLBACK "Enable CPU encoder fallbacks" OFF)

option(ENCORDER_VALIDATION "Enable boundary validation" ON)
option(ENCORDER_STRICT_DIAGNOSTICS "Enable strict diagnostics" OFF)

option(ENCORDER_BUILD_SHARED "Build encorder as a shared library" ON)

option(ENCORDER_BUILD_TOOLS "Build tools" OFF)
option(ENCORDER_BUILD_TESTS "Build tests" ON)

option(ENCORDER_ASAN "Enable address sanitizer" OFF)
option(ENCORDER_TSAN "Enable thread sanitizer" OFF)
option(ENCORDER_UBSAN "Enable undefined behaviour sanitizer" OFF)

if(ENCORDER_ASAN AND ENCORDER_TSAN)
	message(FATAL_ERROR "Address and thread sanitizer are mutually exclusive")
endif()

if(NOT ENCORDER_BACKEND_VULKAN
		AND NOT ENCORDER_BACKEND_D3D12
		AND NOT ENCORDER_BACKEND_VIDEOTOOLBOX)

	message(
			FATAL_ERROR
			"At least one device hub must be enabled "
			"(ENCORDER_BACKEND_VULKAN, _D3D12 or _VIDEOTOOLBOX)")
endif()

if(ENCORDER_ASAN)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address")
endif()

if(ENCORDER_TSAN)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=thread")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=thread")
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=thread")
endif()

if(ENCORDER_UBSAN)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=undefined")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=undefined")
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=undefined")
endif()

if(ENCORDER_ASAN OR ENCORDER_TSAN OR ENCORDER_UBSAN)
	set(ENCORDER_SANITIZING ON)

	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-omit-frame-pointer -g")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-omit-frame-pointer -g")
endif()

set(
		CMAKE_MSVC_RUNTIME_LIBRARY
		"MultiThreaded$<$<CONFIG:${ENCORDER_DEBUG_CONFIGS}>:Debug>")

function(encorder_default_options TARGET)
	if(NOT ENCORDER_SANITIZING)
		target_compile_options(${TARGET} PRIVATE
				"$<$<CONFIG:${ENCORDER_RELEASE_CONFIGS}>:-fomit-frame-pointer>")
	endif()

	if(${ENCORDER_STRICT_DIAGNOSTICS})
		c_strict_diagnostics(${TARGET})
	endif()

	target_compile_options(
			${TARGET} PRIVATE
			"$<$<CXX_COMPILER_FRONTEND_VARIANT:GNU>:-fmacro-prefix-map=${CMAKE_SOURCE_DIR}=.>")
endfunction()

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/core)

if(ENCORDER_BUILD_TESTS)
	add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/test)
endif()
