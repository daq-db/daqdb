cmake_minimum_required(VERSION 3.5)

project(mist)

include(FindPkgConfig)
find_package(PkgConfig)

set(CMAKE_CXX_STANDARD 14)

set(ROOT_DAQDB_DIR ${PROJECT_SOURCE_DIR}/../..)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${ROOT_DAQDB_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${ROOT_DAQDB_DIR}/bin)

find_package(Boost REQUIRED COMPONENTS program_options system filesystem)
find_package(Threads REQUIRED)

include_directories(${LIBCONFIG_INCLUDES_EXPORT})

file(GLOB MIST_SOURCES ${APPS}/mist/*.cpp ${APPS}/mist/*.hpp)
add_executable(mist ${MIST_SOURCES})
target_link_libraries(mist ${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT}	daqdb_thin)

configure_file(mist.cfg.in
	${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/mist.cfg.sample COPYONLY)