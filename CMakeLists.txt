cmake_minimum_required(VERSION 3.5)

project(fogkv)

include(FindPkgConfig)

find_package(PkgConfig)
find_package(Boost REQUIRED COMPONENTS program_options system filesystem)
include_directories(${Boost_INCLUDE_DIRS})

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-declarations")
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

# Products will be copied to bin folder
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
if (NOT EXISTS ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
	file(MAKE_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
endif()

###############################################################################
# Main library dependencies
###############################################################################

set(3RDPARTY ${PROJECT_SOURCE_DIR}/third-party)
add_subdirectory(${3RDPARTY})
include_directories(${3RDPARTY}/libfabric/include)
include_directories(${3RDPARTY}/pmdk/src/include)
include_directories(${3RDPARTY}/spdk/include)
include_directories(${HDRHISTOGRAM_INCLUDES_EXPORT})
include_directories(BEFORE ${PROTOBUF_INCLUDES_EXPORT}) # Must be before LCG
include_directories(${ZHT_INCLUDES_EXPORT})

###############################################################################
# Main library
###############################################################################

set(Spdk_LIBRARIES -Wl,--whole-archive spdk -Wl,--no-whole-archive pthread rt uuid)
include_directories(include include/db include/dht include/fabric include/FogKV
	include/store lib/debug lib/dht lib/fabric lib/offload lib/store)
file(GLOB_RECURSE FOGKV_SOURCES lib/*.cpp)

add_library(fogkv SHARED ${FOGKV_SOURCES})
target_link_libraries(fogkv fabric ${Spdk_LIBRARIES} ${Boost_LIBRARIES}
					  zht libprotobuf )

###############################################################################
# Unit Tests
###############################################################################

add_subdirectory(tests EXCLUDE_FROM_ALL)

###############################################################################
# Examples
###############################################################################

set(EXAMPLES ${PROJECT_SOURCE_DIR}/examples)
add_subdirectory(${EXAMPLES}/cli_node)
add_subdirectory(${EXAMPLES}/basic)
add_subdirectory(${EXAMPLES}/fabric_node)

###############################################################################
# Applications
###############################################################################

set(APPS ${PROJECT_SOURCE_DIR}/apps)
add_subdirectory(${APPS}/mist)
add_subdirectory(${APPS}/minidaq)

###############################################################################
# Custom Targets
###############################################################################

add_custom_target(clean-all
	COMMAND ${CMAKE_BUILD_TOOL} clean
	COMMAND ${CMAKE_BUILD_TOOL} clean-dep
	COMMAND ${CMAKE_COMMAND} -P cmake/CleanAll.cmake
	WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)

add_custom_target(tests
	COMMAND ${CMAKE_BUILD_TOOL}
	WORKING_DIRECTORY tests
)

# Initiate githooks for clang-format
ADD_CUSTOM_TARGET(do_always ALL COMMAND git config core.hooksPath "${PROJECT_SOURCE_DIR}/scripts/githooks")
