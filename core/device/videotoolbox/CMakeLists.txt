# TODO(Emily): Add general `add_driver_target` function to de-dup.

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/vendor)

string(APPEND ENCORDER_DRIVER_INCLUDES "#include <encorder/device/videotoolbox/driver.hpp>\n")
string(APPEND ENCORDER_DRIVER_REGISTRATIONS "\t\t\t\tdriver_registration{ .kind = ENC_NATIVE_VIDEOTOOLBOX, .name = \"videotoolbox\", .create = &videotoolbox::make_driver },\n")

set(ENCORDER_DRIVER_INCLUDES "${ENCORDER_DRIVER_INCLUDES}" PARENT_SCOPE)
set(ENCORDER_DRIVER_REGISTRATIONS "${ENCORDER_DRIVER_REGISTRATIONS}" PARENT_SCOPE)

add_library(encorder-backend-videotoolbox INTERFACE)
set_target_properties(${TARGET} PROPERTIES FOLDER encorder/backend)

target_link_libraries(encorder-backend-videotoolbox INTERFACE $<LINK_LIBRARY:FRAMEWORK,VideoToolbox>)

target_include_directories(encorder-backend-videotoolbox INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include)

target_sources(
		encorder-backend-videotoolbox INTERFACE
		${CMAKE_CURRENT_LIST_DIR}/include/encorder/device/videotoolbox/driver.hpp)

target_sources(
		encorder-backend-videotoolbox INTERFACE
		${CMAKE_CURRENT_LIST_DIR}/driver.cpp)
