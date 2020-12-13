cmake_minimum_required(VERSION 3.16)

set(VCPKG_OVERLAY_PORTS ${CMAKE_CURRENT_LIST_DIR}/external)
if(BUILD_TESTING)
    set(VCPKG_MANIFEST_FEATURES test)
endif()
project(rlisp)

find_package(vcpkg-base CONFIG REQUIRED)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB SOURCES src/*.cpp)
add_executable(rlisp ${SOURCES})
target_include_directories(rlisp PRIVATE src)
target_link_libraries(rlisp PRIVATE vcpkgbase::vcpkgbase)

if(BUILD_TESTING)
    enable_testing()
    find_package(GTest MODULE REQUIRED)
    
    file(GLOB TEST_SOURCES test/*.cpp src/*.cpp)
    list(REMOVE_ITEM TEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp")

    add_executable(rlisp-test ${TEST_SOURCES})
    target_include_directories(rlisp-test PRIVATE src test)
    target_link_libraries(rlisp-test PRIVATE vcpkgbase::vcpkgbase GTest::gtest)
    add_test(NAME rlisp-test COMMAND rlisp-test)
endif()
