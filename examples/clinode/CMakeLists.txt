cmake_minimum_required(VERSION 3.5)

project(clinode)

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

include_directories(linenoise)
add_library(linenoise SHARED linenoise/linenoise.c linenoise/linenoise.h)

file(GLOB CLINODE_SOURCES ${EXAMPLES}/clinode/*.cpp ${EXAMPLES}/clinode/*.h)
add_executable(clinode ${CLINODE_SOURCES})

configure_file(clinode.cfg.in
	${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/clinode.cfg.sample COPYONLY)

set(Dpdk_LIBRARIES -Wl,--whole-archive dpdk -Wl,--no-whole-archive)
target_link_libraries(clinode ${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT}
	pmem daqdb linenoise pmemobj dl numa ${Dpdk_LIBRARIES})
