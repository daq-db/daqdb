cmake_minimum_required(VERSION 3.5)

include(ExternalProject)

set(DPDK_PATH ${PROJECT_SOURCE_DIR}/spdk/dpdk/build)

ExternalProject_Add(project_erpc
	PREFIX ${PROJECT_SOURCE_DIR}/eRPC
	SOURCE_DIR ${PROJECT_SOURCE_DIR}/eRPC
	BUILD_IN_SOURCE ${PROJECT_SOURCE_DIR}/eRPC
	CMAKE_ARGS -DPERF=ON -DTRANSPORT=${ERPC_TRANSPORT_MODE} -DLTO=OFF -DCMAKE_PREFIX_PATH=${DPDK_PATH} -DCMAKE_POSITION_INDEPENDENT_CODE=ON
	BUILD_COMMAND ${CMAKE_MAKE_PROGRAM}
	INSTALL_COMMAND ""
	DEPENDS project_spdk
)
add_library(liberpc STATIC IMPORTED GLOBAL)
set_target_properties(liberpc PROPERTIES IMPORTED_LOCATION
    ${PROJECT_SOURCE_DIR}/eRPC/build/liberpc.a)
add_dependencies(liberpc project_erpc)

add_custom_target(liberpc_clean
	COMMAND make clean
	WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/eRPC
)
