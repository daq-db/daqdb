cmake_minimum_required(VERSION 3.5)

include(ExternalProject)

set(LIBFABRIC_PATH ${3RDPARTY}/libfabric/src/.libs/libfabric.so)

ExternalProject_Add(project_libfabric
	PREFIX ${PROJECT_SOURCE_DIR}/libfabric
	SOURCE_DIR ${PROJECT_SOURCE_DIR}/libfabric
	BUILD_IN_SOURCE ${PROJECT_SOURCE_DIR}/libfabric
	CONFIGURE_COMMAND libtoolize && ./autogen.sh && ./configure --prefix=${PROJECT_SOURCE_DIR}/libfabric
	BUILD_COMMAND make
	INSTALL_COMMAND ${CMAKE_COMMAND} -E copy_if_different
			${LIBFABRIC_PATH}
			${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libfabric.so.1
)
add_library(fabric SHARED IMPORTED GLOBAL)
add_dependencies(fabric project_libfabric)

set_target_properties(fabric PROPERTIES IMPORTED_LOCATION
	${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libfabric.so.1)

add_custom_target(libfabric_clean
	COMMAND make clean
	WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/libfabric
)
	