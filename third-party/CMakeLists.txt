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

# @TODO jradtke: change to use ExternalProject_Add
# libfabric
include_directories(libfabric/include)
if (NOT EXISTS ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/libfabric.so OR NOT EXISTS ${3RDPARTY}/libfabric/src/.libs/libfabric.so)
	add_custom_target(libfabric_target 
		COMMAND libtoolize 
		COMMAND ./autogen.sh 
		COMMAND ./configure --prefix=${PROJECT_SOURCE_DIR}/libfabric
		COMMAND make
		WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/libfabric
	)
	add_custom_command(TARGET libfabric_target POST_BUILD
	    COMMAND ${CMAKE_COMMAND} -E copy_if_different
	        ${3RDPARTY}/libfabric/src/.libs/libfabric.so
	        ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libfabric.so
	)  
endif()
add_library(libfabric SHARED IMPORTED GLOBAL)
add_dependencies(libfabric libfabric_target)
set_target_properties(libfabric
    PROPERTIES
    IMPORTED_LOCATION ${3RDPARTY}/libfabric/src/.libs/libfabric.so
    INTERFACE_INCLUDE_DIRECTORIES ${3RDPARTY}/libfabric/include)

# @TODO jradtke: change to use ExternalProject_Add
# pmdk
include_directories(pmdk/src/include)
if ((NOT EXISTS ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/libpmem.so OR NOT EXISTS ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/libpmemobj.so) OR
	(NOT EXISTS ${PROJECT_SOURCE_DIR}/pmdk/lib/libpmem.so OR NOT EXISTS ${PROJECT_SOURCE_DIR}/pmdk/lib/libpmemobj.so))
	add_custom_target(libpmdk_target 
		COMMAND make install prefix=${PROJECT_SOURCE_DIR}/pmdk
		WORKING_DIRECTORY ${3RDPARTY}/pmdk
		BYPRODUCTS ${3RDPARTY}/pmdk/lib/libpmem.so ${PROJECT_SOURCE_DIR}/pmdk/lib/libpmemobj.so
	)
	add_custom_command(TARGET libpmdk_target POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    	${PROJECT_SOURCE_DIR}/pmdk/lib/libpmem.so
        ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libpmem.so
	)
	add_custom_command(TARGET libpmdk_target POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    	${PROJECT_SOURCE_DIR}/pmdk/lib/libpmemobj.so
        ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/libpmemobj.so
	) 
endif()
add_library(libpmem SHARED IMPORTED GLOBAL)
add_dependencies(libpmem libpmdk_target)
set_target_properties(libpmem
    PROPERTIES
    IMPORTED_LOCATION ${PROJECT_SOURCE_DIR}/pmdk/lib/libpmem.so
    INTERFACE_INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}/pmdk/src/include)
add_library(libpmemobj SHARED IMPORTED GLOBAL)
add_dependencies(libpmemobj libpmdk_target)
set_target_properties(libpmemobj
    PROPERTIES
    IMPORTED_LOCATION ${PROJECT_SOURCE_DIR}/pmdk/lib/libpmemobj.so
    INTERFACE_INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}/pmdk/src/include)

# pmemkv
set(PMEMKV ${PROJECT_SOURCE_DIR}/pmemkv)
set(PMEMKV_INCLUDES ${PMEMKV}/src ${PMEMKV}/src/engines)
set(PMEMKV_INCLUDES_EXPORT ${PMEMKV_INCLUDES} PARENT_SCOPE)
include_directories(${PMEMKV_INCLUDES})
file(GLOB_RECURSE PMEMKV_SOURCES ${PMEMKV}/src/pmemkv.cc ${PMEMKV}/src/engines/*.cc)
add_library(pmemkv SHARED ${PMEMKV_SOURCES})

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
