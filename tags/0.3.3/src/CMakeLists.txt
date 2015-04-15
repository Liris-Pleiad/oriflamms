# oriflamms build script

###############################################################################
# Configuration
###############################################################################
cmake_minimum_required (VERSION 2.8)
#set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "Only Debug and Release" FORCE) # must be before project()
project(oriflamms)
set(ORIFLAMMS_PACKAGE_VERSION_MAJOR "0")
set(ORIFLAMMS_PACKAGE_VERSION_MINOR "3")
set(ORIFLAMMS_PACKAGE_VERSION_PATCH "3")
set(ORIFLAMMS_PACKAGE_VERSION "${ORIFLAMMS_PACKAGE_VERSION_MAJOR}.${ORIFLAMMS_PACKAGE_VERSION_MINOR}.${ORIFLAMMS_PACKAGE_VERSION_PATCH}")
if(UNIX)
	if(NOT CMAKE_BUILD_TYPE)
		set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Choose the type of build, options are: Debug Release." FORCE)
	endif(NOT CMAKE_BUILD_TYPE)
endif(UNIX)

###############################################################################
# Includes
###############################################################################
# Override default modules with the ones in the source dir
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR})
# Use pkg-config on Linux
include(ReadPkgConfig)
#include(FindPkgConfig)
# Use Visual Studio 10 property sheets on Windows
include(ReadVS10Props)

###############################################################################
# Options
###############################################################################
option(BUILD_SHARED_LIBRARIES "Build shared libraries if on, static libraries if off" ON)

set(ORIFLAMMS_CONFIG_PATH "etc" CACHE STRING "Path to the default configuration directory")
if(UNIX)
	set(ORIFLAMMS_CONFIG_FULL_PATH "/${ORIFLAMMS_CONFIG_PATH}" CACHE STRING "Path to the default configuration directory" FORCE)
else(UNIX)
	set(ORIFLAMMS_CONFIG_FULL_PATH "${CMAKE_INSTALL_PREFIX}/${ORIFLAMMS_CONFIG_PATH}" CACHE STRING "Path to the default configuration directory" FORCE)
endif(UNIX)
if(MSVC)
	set(ORIFLAMMS_DATA_PATH "share" CACHE STRING "Path to the data files")
else(MSVC)
	set(ORIFLAMMS_DATA_PATH "share/oriflamms" CACHE STRING "Path to the data files")
endif(MSVC)
set(ORIFLAMMS_DATA_FULL_PATH "${CMAKE_INSTALL_PREFIX}/${ORIFLAMMS_DATA_PATH}" CACHE STRING "Path to the data files" FORCE)

###############################################################################
# Propagate depedencies
###############################################################################

if(MSVC)
	set(RUNTIME_INSTALL_PATH ".")
else(MSVC)
	set(RUNTIME_INSTALL_PATH "bin")
endif(MSVC)
set(LIBRARY_INSTALL_PATH "lib")
set(ARCHIVE_INSTALL_PATH "lib")

if (MSVC)
###############################################################################
# Visual Studio specifics
###############################################################################
	set(CMAKE_DEBUG_POSTFIX "-d")
	set(CMAKE_SHARED_LIBRARY_PREFIX "lib")
	set(CMAKE_STATIC_LIBRARY_PREFIX "lib")
	set(CMAKE_IMPORT_LIBRARY_PREFIX "lib")
	set(BUILD_SHARED_LIBRARIES ON CACHE BOOL "Shared libraries enabled by Visual Studio" FORCE)

	read_vs10_props("$ENV{LIBCRN_2_0_SDK_VC_PATH}/lib/libgtkcrnmm_vc10_32_debug.props" LIBGTKCRNMMDEBUG)
	read_vs10_props("$ENV{LIBCRN_2_0_SDK_VC_PATH}/lib/libgtkcrnmm_vc10_32_release.props" LIBGTKCRNMM)

else(MSVC)
###############################################################################
# g++ specifics
###############################################################################
	
	read_pkg_config("libgtkcrnmm >= 2.0" LIBGTKCRNMM)
	#pkg_check_modules(LIBGTKCRNMM libgtkcrnmm>=2.0 REQUIRED)
	#foreach(arg ${LIBGTKCRNMM_CFLAGS_OTHER})
	#set(tmp "${tmp} ${arg}")
	#endforeach(arg ${LIBGTKCRNMM_CFLAGS_OTHER})
	#set(LIBGTKCRNMM_CFLAGS_OTHER ${tmp})

	# compiler and linker options
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wformat -Winit-self -Wreturn-type -Wconversion")

	# debug
	set(CMAKE_CXX_FLAGS_DEBUG "-DDEBUG -DPRINTOPTIM -Werror -Wnon-virtual-dtor -Woverloaded-virtual -g3 -O0 -fno-default-inline -fno-inline -fno-elide-constructors")
endif(MSVC)

###############################################################################
# Build
###############################################################################

# shared or static libs
if(BUILD_SHARED_LIBRARIES)
	set(library_link_mode SHARED)
else(BUILD_SHARED_LIBRARIES)
	set(library_link_mode STATIC)
endif(BUILD_SHARED_LIBRARIES)

# put all binaries in the same directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/out")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/out")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/out")
set(CMAKE_INSTALL_RPATH "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

# Translation
set(LOCALEDIR "share/locale")
set(ORIFLAMMS_LOCALE_FULL_PATH "${CMAKE_INSTALL_PREFIX}/${LOCALEDIR}" CACHE STRING "Path to the translation files" FORCE)
include(GettextTranslate)
set(GettextTranslate_ALL ON)
add_subdirectory(po)

# Build application
add_subdirectory(utils)
add_subdirectory(oriflamms)

###############################################################################
# Packages
###############################################################################
include(InstallRequiredSystemLibraries)
set(CPACK_PACKAGE_FILE_NAME "oriflamms-${ORIFLAMMS_PACKAGE_VERSION_MAJOR}.${ORIFLAMMS_PACKAGE_VERSION_MINOR}.${ORIFLAMMS_PACKAGE_VERSION_PATCH}${ORIFLAMMS_PACK_NAME}")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "oriflamms")
set(CPACK_RESOURCE_FILE_LICENSE  "${CMAKE_CURRENT_SOURCE_DIR}/License.txt")
set(CPACK_PACKAGE_VERSION_MAJOR ${ORIFLAMMS_PACKAGE_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${ORIFLAMMS_PACKAGE_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${ORIFLAMMS_PACKAGE_VERSION_PATCH})
set(CPACK_BINARY_TZ "OFF")
if(MSVC)
	set(CPACK_BINARY_NSIS "ON")
	set(CPACK_NSIS_DISPLAY_NAME "oriflamms ${ORIFLAMMS_PACKAGE_VERSION_MAJOR}.${ORIFLAMMS_PACKAGE_VERSION_MINOR}.${ORIFLAMMS_PACKAGE_VERSION_PATCH}")
	set(CPACK_BINARY_TBZ2 "OFF")
	set(CPACK_BINARY_ZIP "ON")
	set(CPACK_NSIS_SOURCE_DIR "${PROJECT_SOURCE_DIR}")
else(MSVC)
	set(CPACK_BINARY_TBZ2 "ON")
	set(CPACK_DEBIAN_PACKAGE_VERSION ${ORIFLAMMS_PACKAGE_VERSION})
	set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Yann LEYDIER <yann@leydier.info>")
	set(CPACK_DEBIAN_PACKAGE_DESCRIPTION "Medieval text-image alignment.")
	set(CPACK_DEBIAN_PACKAGE_SECTION "devel")
	set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON) # auto depends
endif(MSVC)
include(CPack)

