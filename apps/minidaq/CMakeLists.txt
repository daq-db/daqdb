cmake_minimum_required(VERSION 3.5)

project(minidaq)

include(FindPkgConfig)
find_package(PkgConfig)

set(CMAKE_CXX_STANDARD 14)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/../../bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/../../bin)

find_package(Boost REQUIRED COMPONENTS program_options)
find_package(Threads REQUIRED)

file(GLOB MINIDAQ_SOURCES *.cpp *.h)
add_executable(minidaq ${MINIDAQ_SOURCES})
target_link_libraries(minidaq ${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} fogkv libpmemobj)
