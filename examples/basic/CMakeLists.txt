cmake_minimum_required(VERSION 3.5)

project(basic)

include(FindPkgConfig)
find_package(PkgConfig)

set(CMAKE_CXX_STANDARD 14)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/../../bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/../../bin)

set(3RDPARTY ${PROJECT_SOURCE_DIR}/../../third-party)

file(GLOB BASIC_SOURCES *.cpp)
add_executable(basic ${BASIC_SOURCES})
target_link_libraries(basic fogkv libpmemobj)

