cmake_minimum_required(VERSION 3.5)

project(mist)

include(FindPkgConfig)
find_package(PkgConfig)

set(CMAKE_CXX_STANDARD 14)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/../../bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/../../bin)

set(3RDPARTY ${PROJECT_SOURCE_DIR}/../../third-party)

find_package(Boost REQUIRED COMPONENTS program_options system filesystem)
find_package(Jsoncpp)
# TODO add pkg-config installation
#find_package(config++ REQUIRED)

include_directories(${Jsoncpp_INCLUDE_DIR})

file(GLOB MIST_SOURCES ${APPS}/mist/*.cpp ${APPS}/mist/*.hpp)
add_executable(mist ${MIST_SOURCES})
target_link_libraries(mist ${Jsoncpp_LIBRARY} ${Boost_LIBRARIES} libfabric libpmem fogkv config++ libpmemobj)

