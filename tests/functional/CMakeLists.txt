cmake_minimum_required(VERSION 3.5)

project(daqdb_func_test)

set(CMAKE_CXX_STANDARD 14)

include(FindPkgConfig)
find_package(PkgConfig)
find_package(Boost REQUIRED COMPONENTS program_options log log_setup system filesystem thread)
find_package(Threads REQUIRED)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DBOOST_LOG_DYN_LINK")

set(ROOT_DAQDB_DIR ${PROJECT_SOURCE_DIR}/../..)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${ROOT_DAQDB_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${ROOT_DAQDB_DIR}/bin)

set(3RDPARTY ${ROOT_DAQDB_DIR}/third-party)

include_directories(${PROJECT_SOURCE_DIR} uc)
file(GLOB_RECURSE FUNC_TEST_SOURCES ${PROJECT_SOURCE_DIR}/*.cpp)
add_executable(func_tests ${FUNC_TEST_SOURCES})

set(Dpdk_LIBRARIES -Wl,--whole-archive dpdk -Wl,--no-whole-archive)
target_link_libraries(func_tests ${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT}
	fabric pmem fogkv pmemobj dl numa ${Dpdk_LIBRARIES})
