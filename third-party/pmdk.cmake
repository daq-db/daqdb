cmake_minimum_required(VERSION 3.5)

include(ExternalProject)

include_directories(pmdk/src/include)
ExternalProject_Add(project_pmdk
	PREFIX ${PROJECT_SOURCE_DIR}/pmdk
	SOURCE_DIR ${PROJECT_SOURCE_DIR}/pmdk
	BUILD_IN_SOURCE ${PROJECT_SOURCE_DIR}/pmdk
	CONFIGURE_COMMAND ""
	BUILD_COMMAND make NDCTL_ENABLE=n install prefix=${PROJECT_SOURCE_DIR}/pmdk
	INSTALL_COMMAND ${CMAKE_COMMAND} -E copy_if_different
			${PROJECT_SOURCE_DIR}/pmdk/lib/libpmem.so
			${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libpmem.so.1 &&
			${CMAKE_COMMAND} -E copy_if_different
			${PROJECT_SOURCE_DIR}/pmdk/lib/libpmemobj.so
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
	COMMAND make NDCTL_ENABLE=n clean
	WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/pmdk
)