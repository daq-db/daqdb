project(third-party)

set(CMAKE_CXX_STANDARD 14)

set(ROOT_FOGKV_DIR ${PROJECT_SOURCE_DIR}/..)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${ROOT_FOGKV_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${ROOT_FOGKV_DIR}/bin)

# cchord
set(CCHORD ${PROJECT_SOURCE_DIR}/cChord)
set(CCHORD_INCLUDES ${CCHORD}/engine/helper
	${CCHORD}/engine/helper/crypto/
	${CCHORD}/engine/helper/mongoose_http_server/callbacks
	${CCHORD}/engine/helper/mongoose_http_server
	${CCHORD}/engine/helper/thread
	${CCHORD}/engine/helper/request
	${CCHORD}/engine/p2p/node
	${CCHORD}/engine/p2p/protocols
	${CCHORD}/engine/p2p/protocols/chord
	${CCHORD}/engine/p2p/transport/http
	${CCHORD}/engine/p2p/transport)
set(CCHORD_INCLUDES_EXPORT ${CCHORD_INCLUDES} PARENT_SCOPE)
include_directories(${CCHORD_INCLUDES})
file(GLOB_RECURSE CCHORD_SOURCES ${CCHORD}/engine/*.cpp ${CCHORD}/engine/*.h)
add_library(cchord SHARED ${CCHORD_SOURCES})

# libfabric
set(LIBFABRIC_PATH ${3RDPARTY}/libfabric/src/.libs/libfabric.so)
ExternalProject_Add(project_libfabric
	PREFIX ${PROJECT_SOURCE_DIR}/libfabric
	SOURCE_DIR ${PROJECT_SOURCE_DIR}/libfabric
	BUILD_IN_SOURCE ${PROJECT_SOURCE_DIR}/libfabric
	CONFIGURE_COMMAND libtoolize && ./autogen.sh && ./configure --prefix=${PROJECT_SOURCE_DIR}/libfabric
	BUILD_COMMAND make
	INSTALL_COMMAND ${CMAKE_COMMAND} -E copy_if_different
			${LIBFABRIC_PATH}
			${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libfabric.so.1
)
add_library(fabric SHARED IMPORTED GLOBAL)
add_dependencies(fabric project_libfabric)
set_target_properties(fabric PROPERTIES IMPORTED_LOCATION ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libfabric.so.1)

# pmdk
include_directories(pmdk/src/include)
ExternalProject_Add(project_pmdk
	PREFIX ${PROJECT_SOURCE_DIR}/pmdk
	SOURCE_DIR ${PROJECT_SOURCE_DIR}/pmdk
	BUILD_IN_SOURCE ${PROJECT_SOURCE_DIR}/pmdk
	CONFIGURE_COMMAND ""
	BUILD_COMMAND make install prefix=${PROJECT_SOURCE_DIR}/pmdk
	INSTALL_COMMAND ${CMAKE_COMMAND} -E copy_if_different
			${PROJECT_SOURCE_DIR}/pmdk/lib/libpmem.so
			${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libpmem.so.1 &&
			${CMAKE_COMMAND} -E copy_if_different
			${PROJECT_SOURCE_DIR}/pmdk/lib/libpmemobj.so
			${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libpmemobj.so.1
)
add_library(pmem SHARED IMPORTED GLOBAL)
add_dependencies(pmem project_pmdk)
set_target_properties(pmem PROPERTIES IMPORTED_LOCATION ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libpmem.so.1)
add_library(pmemobj SHARED IMPORTED GLOBAL)
add_dependencies(pmemobj project_pmdk)
set_target_properties(pmemobj PROPERTIES IMPORTED_LOCATION ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libpmemobj.so.1)

# spdk
ExternalProject_Add(project_spdk
	PREFIX ${PROJECT_SOURCE_DIR}/spdk
	SOURCE_DIR ${PROJECT_SOURCE_DIR}/spdk
	BUILD_IN_SOURCE ${PROJECT_SOURCE_DIR}/spdk
	CONFIGURE_COMMAND ""
	BUILD_COMMAND make
	INSTALL_COMMAND ${ROOT_FOGKV_DIR}/scripts/prepare_spdk_libs.sh
)
add_library(spdk STATIC IMPORTED GLOBAL)
set_property(TARGET spdk PROPERTY IMPORTED_LOCATION ${PROJECT_SOURCE_DIR}/spdk/libspdk.a)
add_dependencies(spdk project_spdk)
add_library(dpdk STATIC IMPORTED GLOBAL)
set_property(TARGET dpdk PROPERTY IMPORTED_LOCATION ${PROJECT_SOURCE_DIR}/spdk/libdpdk.a)
add_dependencies(dpdk project_spdk)

# hdrhistogram
set(HDRHISTOGRAM ${PROJECT_SOURCE_DIR}/HdrHistogram_c)
set(HDRHISTOGRAM_INCLUDES ${HDRHISTOGRAM}/src)
set(HDRHISTOGRAM_INCLUDES_EXPORT ${HDRHISTOGRAM_INCLUDES} PARENT_SCOPE)
include_directories(${HDRHISTOGRAM_INCLUDES})
file(GLOB_RECURSE HDRHISTOGRAM_SOURCES ${HDRHISTOGRAM}/src/*.c)
add_library(hdr_histogram SHARED ${HDRHISTOGRAM_SOURCES})

# Cleanup tasks
add_custom_target(clean-dep)
add_custom_target(libpmdk_clean
	COMMAND make clean
	WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/pmdk
)
add_dependencies(clean-dep libpmdk_clean)
add_custom_target(libfabric_clean
	COMMAND make clean
	WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/libfabric
)
add_dependencies(clean-dep libfabric_clean)
add_custom_target(libspdk_clean
	COMMAND make clean && rm ${PROJECT_SOURCE_DIR}/spdk/libspdk.a
	WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/spdk
)
add_dependencies(clean-dep libspdk_clean)
