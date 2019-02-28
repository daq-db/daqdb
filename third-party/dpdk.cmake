cmake_minimum_required(VERSION 3.5)

include(ExternalProject)

ExternalProject_Add(project_dpdk
	PREFIX ${PROJECT_SOURCE_DIR}/dpdk
	SOURCE_DIR ${PROJECT_SOURCE_DIR}/dpdk
	BUILD_IN_SOURCE ${PROJECT_SOURCE_DIR}/dpdk
	CONFIGURE_COMMAND ${CMAKE_MAKE_PROGRAM} config T=x86_64-native-linuxapp-gcc
	BUILD_COMMAND ${CMAKE_MAKE_PROGRAM}
	INSTALL_COMMAND ${ROOT_DAQDB_DIR}/scripts/prepare_dpdk_libs.sh
)
add_library(dpdk STATIC IMPORTED GLOBAL)
set_property(TARGET dpdk PROPERTY IMPORTED_LOCATION ${PROJECT_SOURCE_DIR}/dpdk/libdpdk.a)
add_dependencies(dpdk project_dpdk)

add_custom_target(libdpdk_clean
	COMMAND ${CMAKE_MAKE_PROGRAM} clean 
	COMMAND rm -f ${PROJECT_SOURCE_DIR}/dpdk/libdpdk.a
	WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/dpdk
)
