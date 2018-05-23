cmake_minimum_required(VERSION 3.5)

project(fogkv_test)

include(FindPkgConfig)
find_package(PkgConfig)

set(CMAKE_CXX_STANDARD 14)

set(ROOT_FOGKV_DIR ${PROJECT_SOURCE_DIR}/..)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${ROOT_FOGKV_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${ROOT_FOGKV_DIR}/bin)

find_package(Boost REQUIRED COMPONENTS unit_test_framework program_options system filesystem)
find_package(Threads REQUIRED)

include_directories(${Boost_INCLUDE_DIRS})

file(GLOB_RECURSE TESTS_SOURCES *.cpp *.h)
add_executable(fogkv_test ${TESTS_SOURCES})
target_link_libraries(fogkv_test ${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} 
	libfabric pmemkv fogkv libpmemobj dpdk dl numa)

add_test(NAME fogkv_tests COMMAND ${PROJECT_SOURCE_DIR}/../bin/fogkv_test --log_level=all --catch_system_error=yes)
