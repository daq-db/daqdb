cmake_minimum_required(VERSION 3.5)

project(basic)

include(FindPkgConfig)
find_package(PkgConfig)
find_package(Threads REQUIRED)

set(CMAKE_CXX_STANDARD 14)

set(ROOT_DAQDB_DIR ${PROJECT_SOURCE_DIR}/../..)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${ROOT_DAQDB_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${ROOT_DAQDB_DIR}/bin)

set(3RDPARTY ${ROOT_DAQDB_DIR}/third-party)
set(Dpdk_LIBRARIES -Wl,--whole-archive dpdk -Wl,--no-whole-archive)

file(GLOB BASIC_SOURCES *.cpp)
add_executable(basic ${BASIC_SOURCES})
target_link_libraries(basic ${Boost_LIBRARIES} fogkv pmemobj dl numa ${CMAKE_THREAD_LIBS_INIT} ${Dpdk_LIBRARIES})
