cmake_minimum_required(VERSION 3.5)

include(ExternalProject)

set(PROTOBUF_DIR ${PROJECT_SOURCE_DIR}/protobuf)
set(PROTOBUF_INCLUDES ${PROTOBUF_DIR}/src)
include_directories(BEFORE ${PROTOBUF_INCLUDES})
set(PROTOBUF_INCLUDES_EXPORT ${PROTOBUF_INCLUDES} PARENT_SCOPE)
set(PROTOBUF_LIB ${PROTOBUF_DIR}/src/.libs/libprotobuf.so.17.0.0)
ExternalProject_Add(project_protobuf
	PREFIX ${PROTOBUF_SRC}
	SOURCE_DIR ${PROTOBUF_DIR}
	BUILD_IN_SOURCE ${PROTOBUF_DIR}
	CONFIGURE_COMMAND libtoolize -cf &&
		cp -r ${PROTOBUF_DIR}/m4 ${PROTOBUF_DIR}/third_party/googletest &&
		cp -r ${PROTOBUF_DIR}/m4 ${PROTOBUF_DIR}/third_party/googletest/googlemock &&
		cp -r ${PROTOBUF_DIR}/m4 ${PROTOBUF_DIR}/third_party/googletest/googletest &&
		autoreconf -v -I m4 -f -i -Wall,no-obsolete &&
		./configure
	BUILD_COMMAND make
	INSTALL_COMMAND ${CMAKE_COMMAND} -E copy_if_different
			${PROTOBUF_LIB}
			${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libprotobuf.so.17
)
add_library(libprotobuf STATIC IMPORTED GLOBAL)
add_dependencies(libprotobuf project_protobuf)
set_target_properties(libprotobuf PROPERTIES IMPORTED_LOCATION
	${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libprotobuf.so.17)

add_custom_target(protobuf_clean
	COMMAND make clean
	COMMAND rm -rf ${PROJECT_SOURCE_DIR}/project_protobuf-prefix
	WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/protobuf
)
	