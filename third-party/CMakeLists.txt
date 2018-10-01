cmake_minimum_required(VERSION 3.5)

project(third-party)

set(ROOT_DAQDB_DIR ${PROJECT_SOURCE_DIR}/..)
set(DAQDB_BIN_DIR ${ROOT_DAQDB_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${DAQDB_BIN_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${DAQDB_BIN_DIR})

###############################################################################
# Dependencies - External projects
###############################################################################
include(libfabric.cmake)
include(pmdk.cmake)
include(spdk.cmake)
include(libconfig.cmake)
include(protobuf.cmake)

###############################################################################
# Dependencies - Libraries
###############################################################################

# hdrhistogram
set(HDRHISTOGRAM ${PROJECT_SOURCE_DIR}/HdrHistogram_c)
set(HDRHISTOGRAM_INCLUDES ${HDRHISTOGRAM}/src)
set(HDRHISTOGRAM_INCLUDES_EXPORT ${HDRHISTOGRAM_INCLUDES} PARENT_SCOPE)
include_directories(${HDRHISTOGRAM_INCLUDES})
file(GLOB_RECURSE HDRHISTOGRAM_SOURCES ${HDRHISTOGRAM}/src/*.c)
add_library(hdr_histogram SHARED ${HDRHISTOGRAM_SOURCES})

# zht
set(ZHT ${PROJECT_SOURCE_DIR}/zht)
set(ZHT_INCLUDES ${ZHT})
set(ZHT_INCLUDES_EXPORT ${ZHT_INCLUDES} PARENT_SCOPE)
include_directories(${ZHT_INCLUDES})
file(GLOB_RECURSE ZHT_SOURCES ${ZHT}/*.cpp)
add_library(zht SHARED ${ZHT_SOURCES})
target_link_libraries(zht libprotobuf)
target_compile_definitions(zht PRIVATE PF_INET=1 SML_MSG=1)

###############################################################################
# Custom Targets
###############################################################################

# Cleanup tasks
add_custom_target(clean-dep)
add_dependencies(clean-dep libpmdk_clean)
add_dependencies(clean-dep libfabric_clean)
add_dependencies(clean-dep libconfig_clean)
add_dependencies(clean-dep libspdk_clean)
add_dependencies(clean-dep protobuf_clean)
