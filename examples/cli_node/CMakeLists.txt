cmake_minimum_required(VERSION 3.5)

project(clinode)

include(FindPkgConfig)
find_package(PkgConfig)

set(CMAKE_CXX_STANDARD 14)

set(ROOT_FOGKV_DIR ${PROJECT_SOURCE_DIR}/../..)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${ROOT_FOGKV_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${ROOT_FOGKV_DIR}/bin)

set(3RDPARTY ${ROOT_FOGKV_DIR}/third-party)

find_package(Boost REQUIRED COMPONENTS program_options system filesystem)
find_package(Threads REQUIRED)
find_package(Jsoncpp)

include_directories(${Jsoncpp_INCLUDE_DIR})

include_directories(linenoise)
add_library(linenoise SHARED linenoise/linenoise.c linenoise/linenoise.h)

file(GLOB CLINODE_SOURCES ${EXAMPLES}/cli_node/*.cpp ${EXAMPLES}/cli_node/*.h)
add_executable(clinode ${CLINODE_SOURCES})
target_link_libraries(clinode ${Jsoncpp_LIBRARY} ${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT}
	fabric pmem pmemkv fogkv linenoise pmemobj dpdk dl numa)

