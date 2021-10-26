cmake_minimum_required(VERSION 3.3 FATAL_ERROR)

################################################################################
# from https://iscinumpy.gitlab.io/post/cry-cosmic-ray-generator-in-cmake/
# CRY
include(ExternalProject)
ExternalProject_Add(
    CRY-1.7
    URL http://nuclear.llnl.gov/simulation/cry_v1.7.tar.gz
    URL_MD5 85f240bebe81fe0b257e92ef1d390a83
    PATCH_COMMAND patch -p1 < ${CMAKE_CURRENT_SOURCE_DIR}/cry-patch
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ${CMAKE_COMMAND} -E env CPPFLAGS='-fPIC' make
    INSTALL_COMMAND ""
    BUILD_IN_SOURCE 1
    TEST_AFTER_INSTALL 1
)

ExternalProject_Get_Property(CRY-1.7 source_dir)
set(CRY_INCLUDE_DIRS "${source_dir}/src")
set(CRY_LIBRARY_DIR "${source_dir}/lib")
set(CRY_LIBRARIES "libCRY.a")
include_directories(${CRY_INCLUDE_DIRS})
link_directories(${CRY_LIBRARY_DIR})
set(LINK_LIBS ${CRY_LIBRARIES} ${LINK_LIBS})
################################################################################

project(openmc_sources CXX)
add_library(source SHARED source_cosmic_ray_neutrons.cpp)
add_dependencies(source CRY-1.7)
find_package(OpenMC REQUIRED HINTS /usr/local/lib)
target_link_libraries(source OpenMC::libopenmc ${CRY_LIBRARIES})
