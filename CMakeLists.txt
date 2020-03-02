cmake_minimum_required(VERSION 3.5)

project(daqdb)

include(FindPkgConfig)
include(CTest)

# Fix https://github.com/boostorg/boost_install/issues/12#issuecomment-508683006
set(Boost_NO_BOOST_CMAKE ON)

find_package(PkgConfig)
find_package(Boost REQUIRED COMPONENTS program_options system filesystem)
include_directories(${Boost_INCLUDE_DIRS})

###############################################################################
# User options
###############################################################################
option(DEBUG "Enables debug mode" OFF)

if(DEBUG)
	add_definitions(-DDEBUG)
	MESSAGE(STATUS "Debug mode enabled")
else(DEBUG)
	MESSAGE(STATUS "Debug mode disabled")
endif(DEBUG)

if(SANITIZE)
	MESSAGE(STATUS "Sanitize mode enabled")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -fsanitize=address")
	set(CMAKE_LINK_FLAGS "${CMAKE_LINK_FLAGS} -fsanitize=address")
else(SANITIZE)
	MESSAGE(STATUS "Sanitize mode disabled")
endif(SANITIZE)

option(THIN_LIB "Build thin version of the library" OFF)
if(THIN_LIB)
	MESSAGE(STATUS "Build thin version of the library")
else(THIN_LIB)
	MESSAGE(STATUS "Build full version of the library")
endif(THIN_LIB)

option(ERPC_RAW_TRANSPORT "Build eRPC in RAW transport mode" ON)
if(ERPC_RAW_TRANSPORT)
	MESSAGE(STATUS "Build eRPC in RAW transport mode")
	set(ERPC_TRANSPORT_MODE raw)
	set(ERPC_LIBS numa ibverbs liberpc)
	add_definitions(-DERPC_RAW)
else(ERPC_RAW_TRANSPORT)
	MESSAGE(STATUS "Build eRPC in DPDK transport mode")
	set(ERPC_TRANSPORT_MODE dpdk)
	set(ERPC_LIBS numa liberpc)
	add_definitions(-DERPC_DPDK)
endif(ERPC_RAW_TRANSPORT)

if(LOOPBACK_DHT_SERVER)
	add_definitions(-DLOOPBACK_DHT_SERVER)
	MESSAGE(STATUS "Loopback DHT server mode enabled")
else(LOOPBACK_DHT_SERVER)
	MESSAGE(STATUS "Loopback DHT server mode disabled")
endif(LOOPBACK_DHT_SERVER)

###############################################################################
# Internal options
###############################################################################
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-declarations")
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

# Products will be copied to bin folder
if (NOT EXISTS ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
	file(MAKE_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
endif()
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)

###############################################################################
# DaqDB libs and dependencies
###############################################################################

include_directories(include include/daqdb include/config)
set(3RDPARTY ${PROJECT_SOURCE_DIR}/third-party)
add_subdirectory(${3RDPARTY})

include_directories(${LIBCONFIG_INCLUDES_EXPORT})
include_directories(${3RDPARTY}/eRPC/src)
include_directories(${3RDPARTY}/spdk/dpdk/build/include)

file(GLOB_RECURSE DAQDB_THIN_SOURCES lib/thin/*.cpp lib/common/*.cpp
	lib/dht/*.cpp lib/KVStoreBase.cpp)
list(FILTER DAQDB_THIN_SOURCES EXCLUDE REGEX "lib/dht/.*Server.*\.cpp$")
include_directories(lib/common lib/dht lib/thin)
include_directories(${HDRHISTOGRAM_INCLUDES_EXPORT})
add_library(daqdb_thin SHARED ${DAQDB_THIN_SOURCES})
target_compile_definitions(daqdb_thin PRIVATE THIN_LIB=1)
target_link_libraries(daqdb_thin ${Boost_LIBRARIES} libconfig ${ERPC_LIBS})

if(NOT THIN_LIB)
	include_directories(${3RDPARTY}/pmdk/src/include)
	include_directories(${3RDPARTY}/libpmemobj-cpp/include)
	include_directories(${3RDPARTY}/spdk/include)
	
	set(Spdk_LIBRARIES -Wl,--whole-archive spdk -Wl,--no-whole-archive pthread
		rt uuid)
	file(GLOB_RECURSE DAQDB_SOURCES lib/*.cpp)
	include_directories(lib/offload lib/pmem lib/core lib/spdk lib/primary)
	add_library(daqdb SHARED ${DAQDB_SOURCES})
	target_link_libraries(daqdb ${Spdk_LIBRARIES} ${Boost_LIBRARIES} libconfig
		${ERPC_LIBS})
endif()

###############################################################################
# Unit and functional tests
###############################################################################

if(NOT THIN_LIB)
	add_subdirectory(tests/unit EXCLUDE_FROM_ALL)
	add_subdirectory(tests/functional EXCLUDE_FROM_ALL)
	add_subdirectory(tests/functional_thin EXCLUDE_FROM_ALL)
endif()

###############################################################################
# Examples
###############################################################################

if(NOT THIN_LIB)
	set(EXAMPLES ${PROJECT_SOURCE_DIR}/examples)
	add_subdirectory(${EXAMPLES}/clinode)
	add_subdirectory(${EXAMPLES}/basic)
endif()

###############################################################################
# Applications
###############################################################################

set(APPS ${PROJECT_SOURCE_DIR}/apps)
add_subdirectory(${APPS}/mist)
add_subdirectory(${APPS}/minidaq)


###############################################################################
# Custom Targets
###############################################################################

if(NOT THIN_LIB)
	add_custom_target(clean-all
		COMMAND ${CMAKE_BUILD_TOOL} clean
		COMMAND ${CMAKE_BUILD_TOOL} clean-dep
		COMMAND ${CMAKE_COMMAND} -P cmake/CleanAll.cmake
		WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
	)
	
	add_custom_target(tests_unit
		COMMAND ${CMAKE_MAKE_PROGRAM}
		WORKING_DIRECTORY tests/unit
	)
	add_custom_target(tests_func
		COMMAND ${CMAKE_MAKE_PROGRAM}
		WORKING_DIRECTORY tests/functional
	)
	add_dependencies(tests_func daqdb)

	add_custom_target(tests_func_thin
		COMMAND ${CMAKE_MAKE_PROGRAM}
		WORKING_DIRECTORY tests/functional_thin
	)
	add_dependencies(tests_func_thin clinode daqdb_thin)

	add_custom_target(tests)
	add_dependencies(tests tests_unit tests_func tests_func_thin)
endif()

# Initiate githooks for clang-format
add_custom_target(do_always ALL COMMAND git config core.hooksPath "${PROJECT_SOURCE_DIR}/scripts/githooks")
