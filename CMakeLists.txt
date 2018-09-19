cmake_minimum_required(VERSION 3.5)

project(fogkv)

include(FindPkgConfig)
include(CTest)
include(ExternalProject)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-declarations")

set(3RDPARTY ${PROJECT_SOURCE_DIR}/third-party)
set(EXAMPLES ${PROJECT_SOURCE_DIR}/examples)
set(APPS ${PROJECT_SOURCE_DIR}/apps)

find_package(PkgConfig)
find_package(Boost REQUIRED COMPONENTS program_options system filesystem)
include_directories(${Boost_INCLUDE_DIRS})

# All products will be copied to bin folder
if (NOT EXISTS ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
	file(MAKE_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
endif()

# target to cleanup repository
add_custom_target(clean-all
   COMMAND ${CMAKE_BUILD_TOOL} clean
   COMMAND ${CMAKE_BUILD_TOOL} clean-dep
   COMMAND ${CMAKE_COMMAND} -P cmake/CleanAll.cmake
)

# dependencies from third-party folder
add_subdirectory(third-party)
include_directories(${3RDPARTY}/libfabric/include)
include_directories(${3RDPARTY}/pmdk/src/include)
include_directories(${3RDPARTY}/spdk/include)
include_directories(${HDRHISTOGRAM_INCLUDES_EXPORT})

set(Spdk_LIBRARIES -Wl,--whole-archive spdk -Wl,--no-whole-archive pthread rt uuid)
# Build and link fogkv library
include_directories(include include/db include/dht include/fabric include/FogKV include/store)
file(GLOB_RECURSE FOGKV_SOURCES
	lib/debug/*.cpp lib/debug/*.h 
	lib/dht/*.cpp lib/dht/*.h 
	lib/fabric/*.cpp lib/fabric/*.h 
	lib/store/*.cpp lib/store/*.h)
add_library(fogkv SHARED ${FOGKV_SOURCES})
target_link_libraries(fogkv ${Jsoncpp_LIBRARY} fabric ${Spdk_LIBRARIES} ${Boost_LIBRARIES})

# Build test folder
add_subdirectory(tests)

# Build example folder
add_subdirectory(${EXAMPLES}/cli_node)
add_subdirectory(${EXAMPLES}/basic)
add_subdirectory(${EXAMPLES}/fabric_node)

# Build apps folder
add_subdirectory(${APPS}/mist)
add_subdirectory(${APPS}/minidaq)

# Initiate githooks for clang-format
ADD_CUSTOM_TARGET(do_always ALL COMMAND git config core.hooksPath "${PROJECT_SOURCE_DIR}/scripts/githooks")

