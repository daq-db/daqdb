cmake_minimum_required(VERSION 3.5)

include(ExternalProject)

set(LIBCONFIG_INCLUDES_EXPORT ${PROJECT_SOURCE_DIR}/libconfig/lib PARENT_SCOPE)
ExternalProject_Add(project_libconfig
	PREFIX ${PROJECT_SOURCE_DIR}/libconfig
	SOURCE_DIR ${PROJECT_SOURCE_DIR}/libconfig
	BUILD_IN_SOURCE ${PROJECT_SOURCE_DIR}/libconfig
	CMAKE_ARGS -DBUILD_EXAMPLES=OFF -DBUILD_TESTS=OFF
	BUILD_COMMAND make
	INSTALL_COMMAND ${CMAKE_COMMAND} -E copy_if_different
			${PROJECT_SOURCE_DIR}/libconfig/out/liblibconfig++.so
			${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/liblibconfig++.so
)
add_library(libconfig STATIC IMPORTED GLOBAL)
add_dependencies(libconfig project_libconfig)
set_target_properties(libconfig PROPERTIES IMPORTED_LOCATION
	${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/liblibconfig++.so)

add_custom_target(libconfig_clean
	COMMAND make clean
	WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/libconfig
)
	