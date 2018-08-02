cmake_minimum_required(VERSION 3.5)

project(mist)

include(FindPkgConfig)
find_package(PkgConfig)

set(CMAKE_CXX_STANDARD 14)

set(ROOT_FOGKV_DIR ${PROJECT_SOURCE_DIR}/../..)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${ROOT_FOGKV_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${ROOT_FOGKV_DIR}/bin)

set(3RDPARTY ${ROOT_FOGKV_DIR}/third-party)
set(Dpdk_LIBRARIES -Wl,--whole-archive dpdk -Wl,--no-whole-archive)

find_package(Boost REQUIRED COMPONENTS program_options system filesystem)
find_package(Threads REQUIRED)
find_package(Jsoncpp)

include_directories(${Jsoncpp_INCLUDE_DIR})
include_directories(${LIBCONFIG_INCLUDES_EXPORT})

file(GLOB MIST_SOURCES ${APPS}/mist/*.cpp ${APPS}/mist/*.hpp)
add_executable(mist ${MIST_SOURCES})
target_link_libraries(mist ${Jsoncpp_LIBRARY} ${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT}
	fabric pmem fogkv libconfig pmemobj dl numa ${Dpdk_LIBRARIES})
