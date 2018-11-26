cmake_minimum_required(VERSION 3.5)

project(daqdb_func_tests)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DBOOST_LOG_DYN_LINK")

include(FindPkgConfig)
find_package(PkgConfig)
find_package(Boost REQUIRED COMPONENTS program_options log log_setup system filesystem thread)
find_package(Threads REQUIRED)

set(ROOT_DAQDB_DIR ${PROJECT_SOURCE_DIR}/../..)
set(3RDPARTY ${ROOT_DAQDB_DIR}/third-party)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${ROOT_DAQDB_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${ROOT_DAQDB_DIR}/bin)

configure_file(functests.cfg.in
	${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/functests.cfg.sample COPYONLY)

include_directories(${PROJECT_SOURCE_DIR} tests)
file(GLOB_RECURSE FUNCTESTS_SOURCES ${PROJECT_SOURCE_DIR}/*.cpp)
add_executable(functests ${FUNCTESTS_SOURCES})

set(Dpdk_LIBRARIES -Wl,--whole-archive dpdk -Wl,--no-whole-archive)
target_link_libraries(functests ${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT}
	pmem daqdb pmemobj dl numa ${Dpdk_LIBRARIES})
