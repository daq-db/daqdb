cmake_minimum_required(VERSION 3.5)

include(ExternalProject)

ExternalProject_Add(project_spdk
	PREFIX ${PROJECT_SOURCE_DIR}/spdk
	SOURCE_DIR ${PROJECT_SOURCE_DIR}/spdk
	BUILD_IN_SOURCE ${PROJECT_SOURCE_DIR}/spdk
	CONFIGURE_COMMAND "./configure" --with-dpdk=${PROJECT_SOURCE_DIR}/dpdk/build --with-igb-uio-driver
	BUILD_COMMAND ${CMAKE_MAKE_PROGRAM}
	INSTALL_COMMAND ${ROOT_DAQDB_DIR}/scripts/prepare_spdk_libs.sh
)
add_dependencies(project_spdk project_dpdk)

add_library(spdk STATIC IMPORTED GLOBAL)
set_property(TARGET spdk PROPERTY IMPORTED_LOCATION ${PROJECT_SOURCE_DIR}/spdk/libspdk.a)
add_dependencies(spdk project_spdk)

add_custom_target(libspdk_clean
	COMMAND ${CMAKE_MAKE_PROGRAM} clean 
	COMMAND rm -f ${PROJECT_SOURCE_DIR}/spdk/libspdk.a
	WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/spdk
)
