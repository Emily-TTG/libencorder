add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/vendor)

set(ENCORDER_CORE_UNIX ${CMAKE_CURRENT_LIST_DIR}/platform/unix)
set(ENCORDER_CORE_WINDOWS ${CMAKE_CURRENT_LIST_DIR}/platform/windows)

set(ENCORDER_DRIVER_INCLUDES "")
set(ENCORDER_DRIVER_REGISTRATIONS "")

if(ENCORDER_BUILD_SHARED)
	add_library(encorder SHARED)
else()
	add_library(encorder STATIC)
endif()

add_library(encorder::encorder ALIAS encorder)

if(ENCORDER_BACKEND_VULKAN)
	add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/device/vulkan)
	target_link_libraries(encorder PRIVATE encorder-backend-vulkan)
endif()

if(ENCORDER_BACKEND_VIDEOTOOLBOX)
	add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/device/videotoolbox)
	target_link_libraries(encorder PRIVATE encorder-backend-videotoolbox)
endif()

if(NOT ENCORDER_DRIVER_REGISTRATIONS)
	message(FATAL_ERROR "No backends were registered")
endif()

configure_file(
		${CMAKE_CURRENT_LIST_DIR}/registry.cpp.in
		${CMAKE_CURRENT_BINARY_DIR}/registry.cpp @ONLY)

target_link_libraries(encorder PRIVATE magic_enum::magic_enum)

target_compile_features(encorder PUBLIC cxx_std_23)
encorder_default_options(encorder)
set_target_properties(encorder PROPERTIES FOLDER encorder)

set_target_properties(
		encorder PROPERTIES
		# TODO(Emily): Use configure to hard-couple build version with these.
		VERSION 0.1.0
		SOVERSION 0
		C_VISIBILITY_PRESET hidden
		CXX_VISIBILITY_PRESET hidden
		VISIBILITY_INLINES_HIDDEN ON)

if(${ENCORDER_BUILD_SHARED})
	target_compile_definitions(encorder PRIVATE ENC_BUILD_SHARED)
	target_compile_definitions(encorder INTERFACE ENC_USE_SHARED)

	set(ENCORDER_VERSION_SCRIPT ${CMAKE_CURRENT_LIST_DIR}/encorder.map)
	set(ENCORDER_SYMBOL_LIST ${CMAKE_CURRENT_LIST_DIR}/encorder.symbols)

	if(APPLE)
		target_link_options(
				encorder PRIVATE
				"LINKER:-exported_symbols_list,${ENCORDER_SYMBOL_LIST}"
				"LINKER:-undefined,error")

		set_property(
				TARGET encorder APPEND
				PROPERTY LINK_DEPENDS ${ENCORDER_SYMBOL_LIST})
	elseif(NOT WIN32)
		target_link_options(
				encorder PRIVATE
				"LINKER:--version-script=${ENCORDER_VERSION_SCRIPT}"
				"LINKER:--no-undefined")

		set_property(
				TARGET encorder APPEND
				PROPERTY LINK_DEPENDS ${ENCORDER_VERSION_SCRIPT})
	endif()
endif()

if(ENCORDER_VALIDATION)
	target_compile_definitions(encorder PRIVATE ENCORDER_VALIDATION)
endif()

target_precompile_headers(encorder PRIVATE ${CMAKE_CURRENT_LIST_DIR}/include/encorder/core/pch.hpp)

target_include_directories(encorder PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)

target_sources(
		encorder PUBLIC
		${CMAKE_CURRENT_LIST_DIR}/include/encorder/encorder.h
		${CMAKE_CURRENT_LIST_DIR}/include/encorder/encorder_vulkan.h)

target_sources(
		encorder PRIVATE
		${CMAKE_CURRENT_LIST_DIR}/include/encorder/core/device.hpp
		${CMAKE_CURRENT_LIST_DIR}/include/encorder/core/error.hpp
		${CMAKE_CURRENT_LIST_DIR}/include/encorder/core/error.inl
		${CMAKE_CURRENT_LIST_DIR}/include/encorder/core/format.hpp
		${CMAKE_CURRENT_LIST_DIR}/include/encorder/core/format_map.hpp
		${CMAKE_CURRENT_LIST_DIR}/include/encorder/core/instance.hpp
		${CMAKE_CURRENT_LIST_DIR}/include/encorder/core/library.hpp
		${CMAKE_CURRENT_LIST_DIR}/include/encorder/core/library.inl
		${CMAKE_CURRENT_LIST_DIR}/include/encorder/core/logger.hpp
		${CMAKE_CURRENT_LIST_DIR}/include/encorder/core/logger.inl
		${CMAKE_CURRENT_LIST_DIR}/include/encorder/core/pch.hpp
		${CMAKE_CURRENT_LIST_DIR}/include/encorder/core/session.hpp)

target_sources(
		encorder PRIVATE
		${CMAKE_CURRENT_LIST_DIR}/capabilities.cpp
		${CMAKE_CURRENT_LIST_DIR}/device.cpp
		${CMAKE_CURRENT_LIST_DIR}/error.cpp
		${CMAKE_CURRENT_LIST_DIR}/format.cpp
		${CMAKE_CURRENT_LIST_DIR}/instance.cpp
		${CMAKE_CURRENT_LIST_DIR}/logger.cpp
		${CMAKE_CURRENT_LIST_DIR}/names.cpp
		${CMAKE_CURRENT_LIST_DIR}/native.cpp
		${CMAKE_CURRENT_LIST_DIR}/session.cpp

		${CMAKE_CURRENT_BINARY_DIR}/registry.cpp

		$<$<NOT:$<PLATFORM_ID:Windows>>:${ENCORDER_CORE_UNIX}/library.cpp>
		$<$<PLATFORM_ID:Windows>:${ENCORDER_CORE_WINDOWS}/library.cpp>)

target_link_libraries(encorder PRIVATE $<$<NOT:$<PLATFORM_ID:Windows>>:${CMAKE_DL_LIBS}>)
