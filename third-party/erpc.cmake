cmake_minimum_required(VERSION 3.5)

include(ExternalProject)

set(DPDK_PATH ${PROJECT_SOURCE_DIR}/dpdk/build)

ExternalProject_Add(project_erpc
	PREFIX ${PROJECT_SOURCE_DIR}/eRPC-fork
	SOURCE_DIR ${PROJECT_SOURCE_DIR}/eRPC-fork
	BUILD_IN_SOURCE ${PROJECT_SOURCE_DIR}/eRPC-fork
	CMAKE_ARGS -DPERF=ON -DTRANSPORT=${ERPC_TRANSPORT_MODE} -DLTO=OFF -DCMAKE_PREFIX_PATH=${DPDK_PATH} -DCMAKE_POSITION_INDEPENDENT_CODE=ON
	BUILD_COMMAND ${CMAKE_MAKE_PROGRAM}
	INSTALL_COMMAND ""
	DEPENDS project_dpdk
)
add_library(liberpc STATIC IMPORTED GLOBAL)
set_target_properties(liberpc PROPERTIES IMPORTED_LOCATION
    ${PROJECT_SOURCE_DIR}/eRPC-fork/build/liberpc.a)
add_dependencies(liberpc project_erpc)
