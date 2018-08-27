cmake_minimum_required(VERSION 3.5)

project(fabric_node)

include(FindPkgConfig)
find_package(PkgConfig)

set(CMAKE_CXX_STANDARD 14)

set(ROOT_FOGKV_DIR ${PROJECT_SOURCE_DIR}/../..)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${ROOT_FOGKV_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${ROOT_FOGKV_DIR}/bin)
set(Dpdk_LIBRARIES -Wl,--whole-archive dpdk -Wl,--no-whole-archive)

set(3RDPARTY ${ROOT_FOGKV_DIR}/third-party)

add_executable(fabricnode fabric_node.cpp)
target_link_libraries(fabricnode fabric pmem pmemobj fogkv pthread dl numa ${Dpdk_LIBRARIES})

