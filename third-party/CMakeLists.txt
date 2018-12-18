cmake_minimum_required(VERSION 3.5)

project(third-party)

set(ROOT_DAQDB_DIR ${PROJECT_SOURCE_DIR}/..)
set(DAQDB_BIN_DIR ${ROOT_DAQDB_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${DAQDB_BIN_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${DAQDB_BIN_DIR})

###############################################################################
# Dependencies - External projects
###############################################################################

if(NOT THIN_LIB)
	include(pmdk.cmake)
	include(spdk.cmake)
endif()

include(erpc.cmake)
include(libconfig.cmake)

###############################################################################
# Dependencies - Libraries
###############################################################################

if(NOT THIN_LIB)
	# hdrhistogram
	set(HDRHISTOGRAM ${PROJECT_SOURCE_DIR}/HdrHistogram_c)
	set(HDRHISTOGRAM_INCLUDES ${HDRHISTOGRAM}/src)
	set(HDRHISTOGRAM_INCLUDES_EXPORT ${HDRHISTOGRAM_INCLUDES} PARENT_SCOPE)
	include_directories(${HDRHISTOGRAM_INCLUDES})
	file(GLOB_RECURSE HDRHISTOGRAM_SOURCES ${HDRHISTOGRAM}/src/*.c)
	add_library(hdr_histogram SHARED ${HDRHISTOGRAM_SOURCES})
endif()

###############################################################################
# Custom Targets
###############################################################################

# Cleanup tasks
add_custom_target(clean-dep)
if(NOT THIN_LIB)
	add_dependencies(clean-dep libpmdk_clean)
	add_dependencies(clean-dep libspdk_clean)
endif()
add_dependencies(clean-dep liberpc_clean)
add_dependencies(clean-dep libconfig_clean)
