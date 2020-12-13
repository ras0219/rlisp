cmake_minimum_required(VERSION 3.14)

project(vcpkg-base CXX)

include(cmake/utilities.cmake)

file(GLOB VCPKGLIB_BASE_SOURCES CONFIGURE_DEPENDS src/vcpkg/base/*.cpp)
file(GLOB VCPKGLIB_BASE_INCLUDES CONFIGURE_DEPENDS include/vcpkg/base/*.h include/vcpkg/base/fwd/*.h)

vcpkg_detect_compiler()
vcpkg_detect_standard_library()
vcpkg_detect_std_filesystem()

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)

add_library(vcpkgbase
    ${VCPKGLIB_BASE_SOURCES}
    ${VCPKGLIB_SOURCES}
    ${VCPKGLIB_BASE_INCLUDES}
    ${VCPKGLIB_INCLUDES})

target_include_directories(vcpkgbase PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>)
target_compile_definitions(vcpkgbase PUBLIC
    VCPKG_USE_STD_FILESYSTEM=$<BOOL:${VCPKG_USE_STD_FILESYSTEM}>)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(vcpkgbase PRIVATE Threads::Threads)

if(VCPKG_CXXFS_LIBRARY)
    target_link_libraries(vcpkgbase PRIVATE ${VCPKG_CXXFS_LIBRARY})
endif()

if(MSVC)
    get_target_property(_srcs vcpkgbase SOURCES)

    if(NOT CMAKE_GENERATOR MATCHES "Visual Studio .*")
        set_property(SOURCE src/pch.cpp APPEND PROPERTY OBJECT_OUTPUTS "${CMAKE_CURRENT_BINARY_DIR}/pch.pch")
        set_property(SOURCE ${_srcs} APPEND PROPERTY OBJECT_DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/pch.pch")
    endif()

    set_source_files_properties(src/pch.cpp PROPERTIES COMPILE_FLAGS "/Ycpch.h")
    target_sources(vcpkgbase PRIVATE src/pch.cpp)
    target_compile_options(vcpkgbase PRIVATE /Yupch.h /FIpch.h /Zm200)
else()
    target_compile_options(vcpkgbase PRIVATE -include "${CMAKE_CURRENT_SOURCE_DIR}/include/pch.h")
endif()

if(MINGW)
    target_compile_definitions(vcpkgbase
        PUBLIC
            UNICODE
            _WIN32_WINNT=0x0601
            WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY=4
            __fastfail=exit)
endif()
if(WIN32)
    target_link_libraries(vcpkgbase PUBLIC winhttp bcrypt version ole32 uuid)
endif()

file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/vcpkg-base-config.cmake [=[
include(CMakeFindDependencyMacro)
find_dependency(Threads)
include(${CMAKE_CURRENT_LIST_DIR}/vcpkg-base-targets.cmake)
]=])
install(TARGETS vcpkgbase EXPORT vcpkg-base-targets)
install(EXPORT vcpkg-base-targets DESTINATION share/vcpkg-base NAMESPACE vcpkgbase::)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/vcpkg-base-config.cmake DESTINATION share/vcpkg-base)
install(DIRECTORY include/vcpkg/base DESTINATION include/vcpkg)
install(FILES include/vcpkg/textrowcol.h DESTINATION include/vcpkg)
