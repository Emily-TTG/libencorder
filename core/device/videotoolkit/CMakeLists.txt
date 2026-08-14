# TODO(Emily): Add general `add_driver_target` function to de-dup.

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/vendor)

string(APPEND ENCORDER_DRIVER_INCLUDES "#include <encorder/device/videotoolkit/driver.hpp>\n")
string(APPEND ENCORDER_DRIVER_REGISTRATIONS "\t\t\t\tdriver_registration{ .kind = ENC_NATIVE_VIDEOTOOLKIT, .name = \"videotoolkit\", .create = &videotoolkit::make_driver },\n")

set(ENCORDER_DRIVER_INCLUDES "${ENCORDER_DRIVER_INCLUDES}" PARENT_SCOPE)
set(ENCORDER_DRIVER_REGISTRATIONS "${ENCORDER_DRIVER_REGISTRATIONS}" PARENT_SCOPE)

add_library(encorder-backend-videotoolkit INTERFACE)
set_target_properties(${TARGET} PROPERTIES FOLDER encorder/backend)

target_link_libraries(encorder-backend-videotoolkit INTERFACE $<LINK_LIBRARY:FRAMEWORK,VideoToolkit>)

target_include_directories(encorder-backend-videotoolkit INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include)

target_sources(
		encorder-backend-videotoolkit INTERFACE
		${CMAKE_CURRENT_LIST_DIR}/include/encorder/device/videotoolkit/driver.hpp)

target_sources(
		encorder-backend-videotoolkit INTERFACE
		${CMAKE_CURRENT_LIST_DIR}/driver.cpp)
