# larwirecell/CMakeLists.txt
#
# David Adams
# June 2016
#
# CMake instructions for the larwirecell package.
#
# ======================================================================
# Old build instructions:
#  cd .../path/to/build/directory
#  source .../path/to/dune_extensions/ups/setup_for_development <-d|-p>
#  cmake [-DCMAKE_INSTALL_PREFIX=/install/path]
#        -DCMAKE_BUILD_TYPE=$CETPKG_TYPE
#        $CETPKG_SOURCE
#  make
#  make test
#  make install
#  make package (builds distribution tarfile)
# ======================================================================

# use cmake 3.11 or later
cmake_minimum_required (VERSION 3.11)

project(larwirecell VERSION 08.00.00 LANGUAGES CXX)

set(fcl_dir fcl)
find_package( cetmodules )
include(CetCMakeEnv)
cet_cmake_env()

set_install_root()

cet_set_compiler_flags(DIAGS CAUTIOUS
  WERROR
  NO_UNDEFINED
  EXTRA_FLAGS -pedantic -Wno-unused-local-typedefs -Wno-deprecated-declarations
)

cet_report_compiler_flags()

# these are minimum required versions, not the actual product versions
find_package(larevt REQUIRED)
find_package(canvas_root_io REQUIRED)

cet_find_library( WIRECELL_SIGPROC_LIB NAMES WireCellSigProc NO_SYSTEM_ENVIRONMENT_PATH NO_CMAKE_SYSTEM_PATH)
cet_find_library( WIRECELL_GEN_LIB NAMES WireCellGen NO_SYSTEM_ENVIRONMENT_PATH NO_CMAKE_SYSTEM_PATH)
cet_find_library( WIRECELL_IFACE_LIB NAMES WireCellIface NO_SYSTEM_ENVIRONMENT_PATH NO_CMAKE_SYSTEM_PATH)
cet_find_library( WIRECELL_UTIL_LIB NAMES WireCellUtil NO_SYSTEM_ENVIRONMENT_PATH NO_CMAKE_SYSTEM_PATH)
cet_find_library( WIRECELL_APPS_LIB NAMES WireCellApps NO_SYSTEM_ENVIRONMENT_PATH NO_CMAKE_SYSTEM_PATH)
set(WIRECELL_LIBS ${WIRECELL_APPS_LIB} ${WIRECELL_SIGPROC_LIB} ${WIRECELL_IFACE_LIB} ${WIRECELL_UTIL_LIB} ${WIRECELL_GEN_LIB})

cet_find_simple_package(jsoncpp HEADERS json/json.h)

# macros for dictionary and simple_plugin
include(ArtDictionary)
include(ArtMake)
include(BuildPlugins)

# sources
add_subdirectory(larwirecell)
add_subdirectory(cmakeConfig)
# ups - table and config files
#add_subdirectory(ups)

# packaging utility
#include(UseCPack)

