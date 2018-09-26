cmake_minimum_required(VERSION 3.5)

include(ExternalProject)

include_directories(spdk/dpdk/build/include)
ExternalProject_Add(project_erpc
    PREFIX ${PROJECT_SOURCE_DIR}/eRPC
    SOURCE_DIR ${PROJECT_SOURCE_DIR}/eRPC
    BUILD_IN_SOURCE ${PROJECT_SOURCE_DIR}/eRPC
    CMAKE_ARGS -DPERF=OFF -DTRANSPORT=dpdk
	BUILD_COMMAND make
	INSTALL_COMMAND ""
    DEPENDS dpdk
)
