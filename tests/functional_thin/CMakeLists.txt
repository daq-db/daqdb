cmake_minimum_required(VERSION 3.5)

project(daqdb_thin_func_tests)

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

configure_file(functests_thin.cfg.in
	${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/functests_thin.cfg.sample COPYONLY)
configure_file(functests_thin_clinode.cfg.in
	${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/functests_thin_clinode.cfg.sample COPYONLY)

include_directories(${PROJECT_SOURCE_DIR} tests)
file(GLOB_RECURSE FUNCTESTS_THIN_SOURCES ${PROJECT_SOURCE_DIR}/*.cpp)
add_executable(functests_thin ${FUNCTESTS_THIN_SOURCES})

set(Dpdk_LIBRARIES -Wl,--whole-archive dpdk -Wl,--no-whole-archive)
target_link_libraries(functests_thin ${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT}
	${Dpdk_LIBRARIES} dl daqdb_thin)
