cmake_minimum_required(VERSION 3.5)

include(ExternalProject)

ExternalProject_Add(project_spdk
	PREFIX ${PROJECT_SOURCE_DIR}/spdk
	SOURCE_DIR ${PROJECT_SOURCE_DIR}/spdk
	PATCH_COMMAND ${ROOT_DAQDB_DIR}/scripts/patch_spdk_isal.sh
	BUILD_IN_SOURCE ${PROJECT_SOURCE_DIR}/spdk
	CONFIGURE_COMMAND "./configure" "--with-isal"
	BUILD_COMMAND ${CMAKE_MAKE_PROGRAM}
	INSTALL_COMMAND ${ROOT_DAQDB_DIR}/scripts/prepare_spdk_libs.sh
)
add_library(spdk STATIC IMPORTED GLOBAL)
set_property(TARGET spdk PROPERTY IMPORTED_LOCATION ${PROJECT_SOURCE_DIR}/spdk/libspdk.a)
add_dependencies(spdk project_spdk)
add_library(dpdk STATIC IMPORTED GLOBAL)
set_property(TARGET dpdk PROPERTY IMPORTED_LOCATION ${PROJECT_SOURCE_DIR}/spdk/libdpdk.a)
add_dependencies(dpdk project_spdk)

add_custom_target(libspdk_clean
	COMMAND ${CMAKE_MAKE_PROGRAM} clean 
	COMMAND rm -f ${PROJECT_SOURCE_DIR}/spdk/libspdk.a
	COMMAND rm -f ${PROJECT_SOURCE_DIR}/spdk/libdpdk.a
	WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/spdk
)
