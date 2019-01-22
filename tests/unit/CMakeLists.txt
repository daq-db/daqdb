cmake_minimum_required(VERSION 3.5)

project(daqdb_unit_test)

include(CTest)
include(FindPkgConfig)
find_package(PkgConfig)

include(BoostTestHelpers)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DBOOST_TEST_DYN_LINK")

find_package(Boost REQUIRED COMPONENTS unit_test_framework program_options system filesystem)
find_package(Threads REQUIRED)

include_directories(mock)

add_boost_test(dht/DhtUtilsTest.cpp)
add_boost_test(dht/DhtClientTest.cpp)
add_boost_test(pmem/PmemPollerTest.cpp)
add_boost_test(offload/OffloadPollerTest.cpp)
add_boost_test(offload/OffloadFreeListTest.cpp)
