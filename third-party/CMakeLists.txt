project(third-party)

set(CMAKE_CXX_STANDARD 14)

set(ROOT_FOGKV_DIR ${PROJECT_SOURCE_DIR}/..)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${ROOT_FOGKV_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${ROOT_FOGKV_DIR}/bin)

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
set_target_properties(pmem PROPERTIES IMPORTED_LOCATION ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libpmem.so.1)
add_library(pmemobj SHARED IMPORTED GLOBAL)
add_dependencies(pmemobj project_pmdk)
set_target_properties(pmemobj PROPERTIES IMPORTED_LOCATION ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libpmemobj.so.1)

# spdk
ExternalProject_Add(project_spdk
	PREFIX ${PROJECT_SOURCE_DIR}/spdk
	SOURCE_DIR ${PROJECT_SOURCE_DIR}/spdk
	BUILD_IN_SOURCE ${PROJECT_SOURCE_DIR}/spdk
	CONFIGURE_COMMAND "./configure"
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

#libconfig
set(LIBCONFIG_INCLUDES_EXPORT ${PROJECT_SOURCE_DIR}/libconfig/lib PARENT_SCOPE)
ExternalProject_Add(project_libconfig
	PREFIX ${PROJECT_SOURCE_DIR}/libconfig
	SOURCE_DIR ${PROJECT_SOURCE_DIR}/libconfig
	BUILD_IN_SOURCE ${PROJECT_SOURCE_DIR}/libconfig
	CMAKE_ARGS -DBUILD_EXAMPLES=OFF -DBUILD_TESTS=OFF
	BUILD_COMMAND make
	INSTALL_COMMAND ${CMAKE_COMMAND} -E copy_if_different
			${PROJECT_SOURCE_DIR}/libconfig/out/liblibconfig++.so
			${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/liblibconfig++.so
)
add_library(libconfig STATIC IMPORTED GLOBAL)
add_dependencies(libconfig project_libconfig)
set_target_properties(libconfig PROPERTIES IMPORTED_LOCATION ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/liblibconfig++.so)

# protobuf
set(PROTOBUF_INCLUDES ${PROJECT_SOURCE_DIR}/protobuf/src)
include_directories(BEFORE ${PROTOBUF_INCLUDES})
set(PROTOBUF_INCLUDES_EXPORT ${PROTOBUF_INCLUDES} PARENT_SCOPE)
set(PROTOBUF_LIB ${PROJECT_SOURCE_DIR}/protobuf/src/.libs/libprotobuf.so.17.0.0)
ExternalProject_Add(project_protobuf
	PREFIX ${PROJECT_SOURCE_DIR}/protobuf
	SOURCE_DIR ${PROJECT_SOURCE_DIR}/protobuf
	BUILD_IN_SOURCE ${PROJECT_SOURCE_DIR}/protobuf
	CONFIGURE_COMMAND libtoolize && autoreconf -I m4 -f -i -Wall,no-obsolete && ./configure --prefix=${PROJECT_SOURCE_DIR}/protobuf
	BUILD_COMMAND make
	INSTALL_COMMAND ${CMAKE_COMMAND} -E copy_if_different
			${PROTOBUF_LIB}
			${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libprotobuf.so.17
)
add_library(libprotobuf STATIC IMPORTED GLOBAL)
add_dependencies(libprotobuf project_protobuf)
set_target_properties(libprotobuf PROPERTIES IMPORTED_LOCATION ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libprotobuf.so.17)

# zht
set(ZHT ${PROJECT_SOURCE_DIR}/zht)
set(ZHT_INCLUDES ${ZHT})
set(ZHT_INCLUDES_EXPORT ${ZHT_INCLUDES} PARENT_SCOPE)
include_directories(${ZHT_INCLUDES})
file(GLOB_RECURSE ZHT_SOURCES ${ZHT}/*.cpp)
add_library(zht SHARED ${ZHT_SOURCES})
target_link_libraries(zht libprotobuf)

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
