cmake_minimum_required(VERSION 3.5)

include(ExternalProject)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
	MESSAGE(STATUS "Enabling PMDK debug mode.")
	set(PMDK_SOURCE_DIR ${PROJECT_SOURCE_DIR}/pmdk/src/debug)
else (CMAKE_BUILD_TYPE STREQUAL "Debug")
	MESSAGE(STATUS "Enabling PMDK release mode.")
	set(PMDK_SOURCE_DIR ${PROJECT_SOURCE_DIR}/pmdk/lib)
endif (CMAKE_BUILD_TYPE STREQUAL "Debug")

include_directories(pmdk/src/include)
ExternalProject_Add(project_pmdk
	PREFIX ${PROJECT_SOURCE_DIR}/pmdk
	SOURCE_DIR ${PROJECT_SOURCE_DIR}/pmdk
	BUILD_IN_SOURCE ${PROJECT_SOURCE_DIR}/pmdk
	CONFIGURE_COMMAND ""
	BUILD_COMMAND ${CMAKE_MAKE_PROGRAM} NDCTL_ENABLE=n install prefix=${PROJECT_SOURCE_DIR}/pmdk
	INSTALL_COMMAND ${CMAKE_COMMAND} -E copy_if_different
			${PMDK_SOURCE_DIR}/libpmem.so
			${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libpmem.so.1 &&
			${CMAKE_COMMAND} -E copy_if_different
			${PMDK_SOURCE_DIR}/libpmemobj.so
			${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libpmemobj.so.1
)
add_library(pmem SHARED IMPORTED GLOBAL)
add_dependencies(pmem project_pmdk)
set_target_properties(pmem PROPERTIES IMPORTED_LOCATION
	${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libpmem.so.1)
add_library(pmemobj SHARED IMPORTED GLOBAL)
add_dependencies(pmemobj project_pmdk)
set_target_properties(pmemobj PROPERTIES IMPORTED_LOCATION
	${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libpmemobj.so.1)

add_custom_target(libpmdk_clean
	COMMAND ${CMAKE_MAKE_PROGRAM} NDCTL_ENABLE=n clean
	WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/pmdk
)
