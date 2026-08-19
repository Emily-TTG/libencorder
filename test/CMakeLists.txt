add_library(encorder-test-common INTERFACE)

target_include_directories(encorder-test-common INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include)
target_sources(encorder-test-common INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include/encorder/test/common.h)

target_link_libraries(encorder-test-common INTERFACE encorder)

function(encorder_test TEST_NAME LANGUAGE)
	set(_NAME "encorder-test-${TEST_NAME}")
	add_executable(${_NAME})

	if(LANGUAGE STREQUAL "c")
		target_compile_features(${_NAME} PRIVATE c_std_99)
	elseif(LANGUAGE STREQUAL "cpp")
		target_compile_features(${_NAME} PRIVATE cxx_std_23)
	endif()

	encorder_default_options(${_NAME})
	set_target_properties(${_NAME} PROPERTIES FOLDER encorder/test)

	target_link_libraries(${_NAME} PRIVATE encorder-test-common)

	target_sources(
			${_NAME} PRIVATE
			${CMAKE_CURRENT_LIST_DIR}/${TEST_NAME}/main.${LANGUAGE})
endfunction()

encorder_test(scratch cpp)
encorder_test(abi c)

if(ENCORDER_BACKEND_VULKAN)
	encorder_test(import cpp)

	target_link_libraries(encorder-test-import PRIVATE Vulkan::Headers ${CMAKE_DL_LIBS})
endif()
