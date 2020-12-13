vcpkg_check_linkage(ONLY_STATIC_LIBRARY)

vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO microsoft/vcpkg
    REF 81b67c387848731aefacc19aea7f41364e5d5ba2
    SHA512 c9e8bc2b542d570c5b2c4c22418492b943bbb1ae98a0e4dd211822f76edfe9e1e4e824f8e70905c7359c5c195aa05c0f114298cc44f8a0f2f47faabe7bb4d8c3
)

file(COPY ${CMAKE_CURRENT_LIST_DIR}/CMakeLists.txt DESTINATION ${SOURCE_PATH}/toolsrc)

vcpkg_configure_cmake(
    SOURCE_PATH ${SOURCE_PATH}/toolsrc
    PREFER_NINJA
)

vcpkg_install_cmake()
vcpkg_fixup_cmake_targets()
vcpkg_copy_pdbs()

file(REMOVE_RECURSE ${CURRENT_PACKAGES_DIR}/debug/include)

file(INSTALL ${SOURCE_PATH}/LICENSE.txt DESTINATION ${CURRENT_PACKAGES_DIR}/share/${PORT} RENAME copyright)
