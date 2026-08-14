add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/vendor)

string(APPEND ENCORDER_DRIVER_INCLUDES "#include <encorder/device/vulkan/driver.hpp>\n")
string(APPEND ENCORDER_DRIVER_REGISTRATIONS "\t\t\t\tdriver_registration{ .kind = ENC_NATIVE_VULKAN, .name = \"vulkan\", .create = &vulkan::make_driver },\n")

set(ENCORDER_DRIVER_INCLUDES "${ENCORDER_DRIVER_INCLUDES}" PARENT_SCOPE)
set(ENCORDER_DRIVER_REGISTRATIONS "${ENCORDER_DRIVER_REGISTRATIONS}" PARENT_SCOPE)

add_library(encorder-backend-vulkan INTERFACE)
set_target_properties(${TARGET} PROPERTIES FOLDER encorder/backend)

target_link_libraries(encorder-backend-vulkan INTERFACE Vulkan::Headers volk::volk)

target_include_directories(encorder-backend-vulkan INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include)

target_sources(
		encorder-backend-vulkan INTERFACE
		${CMAKE_CURRENT_LIST_DIR}/include/encorder/device/vulkan/driver.hpp)

target_sources(
		encorder-backend-vulkan INTERFACE
		${CMAKE_CURRENT_LIST_DIR}/driver.cpp)
