cmake_minimum_required(VERSION 3.5)

project(test_daqdb)

include(FindPkgConfig)
find_package(PkgConfig)

set(CMAKE_CXX_STANDARD 14)

set(ROOT_DAQDB_DIR ${PROJECT_SOURCE_DIR}/../..)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${ROOT_DAQDB_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${ROOT_DAQDB_DIR}/bin)

find_package(Boost REQUIRED COMPONENTS program_options system filesystem)
find_package(Threads REQUIRED)

include_directories(${LIBCONFIG_INCLUDES_EXPORT})

file(GLOB PUT_SOURCES ${APPS}/daqdb-top/producer.cpp ${APPS}/daqdb-top/utils.hpp)
file(GLOB GET_SOURCES ${APPS}/daqdb-top/consumer.cpp ${APPS}/daqdb-top/utils.hpp)

add_executable(producer ${PUT_SOURCES})
set(Dpdk_LIBRARIES -Wl,--whole-archive dpdk -Wl,--no-whole-archive)
target_link_libraries(producer ${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT}
        ${Dpdk_LIBRARIES} pmem daqdb linenoise pmemobj dl numa)

add_executable(consumer ${GET_SOURCES})
set(Dpdk_LIBRARIES -Wl,--whole-archive dpdk -Wl,--no-whole-archive)
target_link_libraries(consumer ${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT}
        ${Dpdk_LIBRARIES} pmem daqdb linenoise pmemobj dl numa)

