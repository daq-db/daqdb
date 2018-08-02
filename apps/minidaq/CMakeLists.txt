cmake_minimum_required(VERSION 3.5)

project(minidaq)

include(FindPkgConfig)
find_package(PkgConfig)

set(CMAKE_CXX_STANDARD 14)

set(ROOT_FOGKV_DIR ${PROJECT_SOURCE_DIR}/../..)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${ROOT_FOGKV_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${ROOT_FOGKV_DIR}/bin)

find_package(Boost REQUIRED COMPONENTS program_options)
find_package(Threads REQUIRED)
set(Dpdk_LIBRARIES -Wl,--whole-archive dpdk -Wl,--no-whole-archive)

file(GLOB MINIDAQ_SOURCES *.cpp *.h)
add_executable(minidaq ${MINIDAQ_SOURCES})
target_link_libraries(minidaq ${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT}
					  pmem fogkv pmemobj dl numa z hdr_histogram ${Dpdk_LIBRARIES})
