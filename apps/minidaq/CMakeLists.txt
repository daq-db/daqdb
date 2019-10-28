cmake_minimum_required(VERSION 3.5)

project(minidaq)

include(FindPkgConfig)
find_package(PkgConfig)

option(MINIDAQ_INTEGRITY_CHECK "Enable data integrity check" OFF)
if(MINIDAQ_INTEGRITY_CHECK)
    add_definitions(-DWITH_INTEGRITY_CHECK)
    MESSAGE(STATUS "Data integrity check enabled.")
else(MINIDAQ_INTEGRITY_CHECK)
    MESSAGE(STATUS "Data integrity check disabled.")
endif(MINIDAQ_INTEGRITY_CHECK)

set(CMAKE_CXX_STANDARD 14)

set(ROOT_DAQDB_DIR ${PROJECT_SOURCE_DIR}/../..)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${ROOT_DAQDB_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${ROOT_DAQDB_DIR}/bin)

find_package(Boost REQUIRED COMPONENTS program_options)
find_package(Threads REQUIRED)
set(Dpdk_LIBRARIES -Wl,--whole-archive dpdk -Wl,--no-whole-archive)

configure_file(minidaq.cfg.in
	${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/minidaq.cfg.sample COPYONLY)

file(GLOB MINIDAQ_SOURCES *.cpp *.h)
add_executable(minidaq ${MINIDAQ_SOURCES})
set(MINIDAQ_LINK_LIBRARIES ${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} dl numa z hdr_histogram)
if(NOT THIN_LIB)
    list(APPEND MINIDAQ_LINK_LIBRARIES pmem pmemobj daqdb boost_thread ${Dpdk_LIBRARIES})
else (NOT THIN_LIB)
    list(APPEND MINIDAQ_LINK_LIBRARIES daqdb_thin boost_thread)
    if(NOT ERPC_RAW_TRANSPORT)
        list(APPEND MINIDAQ_LINK_LIBRARIES ${Dpdk_LIBRARIES})
    endif(NOT ERPC_RAW_TRANSPORT)
endif(NOT THIN_LIB)
target_link_libraries(minidaq ${MINIDAQ_LINK_LIBRARIES}) 
