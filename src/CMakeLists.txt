# oriflamms build script

###############################################################################
# Configuration
###############################################################################
cmake_minimum_required (VERSION 2.8)
#set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "Only Debug and Release" FORCE) # must be before project()
project(oriflamms)
set(ORIFLAMMS_PACKAGE_VERSION_MAJOR "0")
set(ORIFLAMMS_PACKAGE_VERSION_MINOR "6")
set(ORIFLAMMS_PACKAGE_VERSION_PATCH "4")
set(ORIFLAMMS_PACKAGE_VERSION "${ORIFLAMMS_PACKAGE_VERSION_MAJOR}.${ORIFLAMMS_PACKAGE_VERSION_MINOR}.${ORIFLAMMS_PACKAGE_VERSION_PATCH}")

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "Choose the type of build." FORCE)
endif(NOT CMAKE_BUILD_TYPE)

if(NOT CMAKE_BUILD_TYPE)
	if(UNIX)
		set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "Choose the type of build." FORCE)
	endif(UNIX)
	if(MSVC)
		set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build." FORCE)
	endif(MSVC)
endif(NOT CMAKE_BUILD_TYPE)

###############################################################################
# Includes
###############################################################################
# Override default modules with the ones in the source dir
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR})
if (MSVC)
	set(CMAKE_FIND_ROOT_PATH "$ENV{LIBCRN_DEPS_PATH}/gtkmm2")
	set(CMAKE_PREFIX_PATH "$ENV{LIBCRN_DEPS_PATH}/gtkmm2")
	list(APPEND CMAKE_MODULE_PATH "$ENV{LIBCRN_4_SDK_PATH}/share/cmake/")
	string(REPLACE "\\" "/" CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH}")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP /D_MBCS")

	set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS _CRT_SECURE_NO_WARNINGS)
	set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS _SCL_SECURE_NO_WARNINGS)

	#set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /OPT:NOREF /OPT:NOICF /INCREMENTAL")
else()
	list(APPEND CMAKE_MODULE_PATH "/usr/local/share/cmake/")
	list(APPEND CMAKE_MODULE_PATH "/usr/share/cmake/")
endif()

###############################################################################
# Options
###############################################################################
set(ORIFLAMMS_CONFIG_PATH "etc" CACHE STRING "Path to the default configuration directory")
set(ORIFLAMMS_DATA_PATH "share/oriflamms" CACHE STRING "Path to the data files")
set(LOCALEDIR "share/locale" CACHE STRING "Path to the translation files")
set(ORIFLAMMS_DATA_FULL_PATH "${CMAKE_INSTALL_PREFIX}/${ORIFLAMMS_DATA_PATH}" CACHE STRING "Path to the data files" FORCE)
if(MSVC)
	set(ORIFLAMMS_CONFIG_FULL_PATH "" CACHE STRING "Path to the default configuration directory" FORCE)
	set(ORIFLAMMS_DATA_FULL_PATH "${ORIFLAMMS_DATA_PATH}" CACHE STRING "Path to the data files" FORCE)
	set(ORIFLAMMS_LOCALE_FULL_PATH "${LOCALEDIR}" CACHE STRING "Path to the translation files" FORCE)
else()
	set(ORIFLAMMS_CONFIG_FULL_PATH "/${ORIFLAMMS_CONFIG_PATH}" CACHE STRING "Path to the default configuration directory" FORCE)
	set(ORIFLAMMS_DATA_FULL_PATH "${CMAKE_INSTALL_PREFIX}/${ORIFLAMMS_DATA_PATH}" CACHE STRING "Path to the data files" FORCE)
	set(ORIFLAMMS_LOCALE_FULL_PATH "${CMAKE_INSTALL_PREFIX}/${LOCALEDIR}" CACHE STRING "Path to the translation files" FORCE)
endif()

if(MSVC)
	set(RUNTIME_INSTALL_PATH ".")
else()
	set(RUNTIME_INSTALL_PATH "bin")
endif()
set(LIBRARY_INSTALL_PATH "lib")
set(ARCHIVE_INSTALL_PATH "lib")

if(WIN32)
	string(REPLACE "/" "\\" ORIFLAMMS_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}")
endif(WIN32)

#------------------------------------------------------------------------------
# GNUCXX (export CXX=g++-4.8 CC=gcc-4.8)
#------------------------------------------------------------------------------
if(CMAKE_COMPILER_IS_GNUCXX)
	set(ENABLE_CXX11 "-std=c++11")

	EXECUTE_PROCESS(COMMAND "${CMAKE_CXX_COMPILER} -dumpversion" OUTPUT_VARIABLE GCC_VERSION)
	if (GCC_VERSION LESS 4.8)
		message(FATAL_ERROR "g++ >= 4.8 needed.")
	endif()

	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${ENABLE_CXX11} -Wfatal-errors -pthread")
	message("GNUCXX compiler detected (with ${ENABLE_CXX11} activated)")
endif()
#------------------------------------------------------------------------------
# Clang (export CXX=clang++ CC=clang)
#------------------------------------------------------------------------------
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
	set(CMAKE_COMPILER_IS_CLANGXX 1)
endif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")

if(CMAKE_COMPILER_IS_CLANGXX)
	set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
	set(ENABLE_CXX11 "-std=c++11")

	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${ENABLE_CXX11} -Wfatal-errors -pthread")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${ENABLE_CXX11}")
	message("Clang compiler detected (with ${ENABLE_CXX11} activated)")
endif()
#------------------------------------------------------------------------------
# MSVC
#------------------------------------------------------------------------------
if(MSVC)
	set(CMAKE_SHARED_LIBRARY_PREFIX "lib")
	set(CMAKE_STATIC_LIBRARY_PREFIX "lib")
	set(CMAKE_IMPORT_LIBRARY_PREFIX "lib")
	set(CMAKE_DEBUG_POSTFIX "-d")

	set(CMAKE_FIND_ROOT_PATH "$ENV{LIBCRN_DEPS_PATH}/gtkmm2")
	set(CMAKE_PREFIX_PATH "$ENV{LIBCRN_DEPS_PATH}/gtkmm2")
	list(APPEND CMAKE_MODULE_PATH "$ENV{LIBCRN_4_SDK_PATH}/share/cmake/")
	string(REPLACE "\\" "/" CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH}")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP /D_MBCS")

	set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS _CRT_SECURE_NO_WARNINGS)
	set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS _SCL_SECURE_NO_WARNINGS)

	# Create filters in VS project
	macro(GroupSources curdir)
		file(GLOB children RELATIVE ${PROJECT_SOURCE_DIR}/${curdir}
			${PROJECT_SOURCE_DIR}/${curdir}/*)
		foreach(child ${children})
			if(IS_DIRECTORY ${PROJECT_SOURCE_DIR}/${curdir}/${child})
				GroupSources(${curdir}/${child})
			else()
				string(REPLACE "/" "\\" groupname ${curdir})
				string(REPLACE "src" "" groupname ${groupname})
				source_group(${groupname} FILES
					${PROJECT_SOURCE_DIR}/${curdir}/${child})
			endif()
		endforeach()
	endmacro()

	if(EXISTS "${CMAKE_SOURCE_DIR}/cmake/msvc/kit.cmake")
		include("${CMAKE_SOURCE_DIR}/cmake/msvc/kit.cmake")
	endif()
else()
	list(APPEND CMAKE_MODULE_PATH "/usr/local/share/cmake/")
	list(APPEND CMAKE_MODULE_PATH "/usr/share/cmake/")
endif()
#------------------------------------------------------------------------------
# APPLE OS X
#------------------------------------------------------------------------------
if(APPLE)
	set(CMAKE_MACOSX_RPATH ON) # see cmake POLICY CMP0042
endif(APPLE)
#------------------------------------------------------------------------------

message("looking for libgtkcrnmm2")
find_package(GTKCRNMM2 REQUIRED)

###############################################################################
# Build
###############################################################################

# put all binaries in the same directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/out")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/out")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/out")
set(CMAKE_INSTALL_RPATH "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

# Translation
set(LANGUAGES "en_GB;fr")
include(Gettext)
if(NOT Gettext_FOUND)
	message(FATAL_ERROR "Cannot find Gettext.")
endif()
TRANSLATE("${PROJECT_SOURCE_DIR}" "oriflamms")
INSTALL_TRANSLATIONS()


###############################################################################
# Build
###############################################################################
add_subdirectory(oriflamms)
add_subdirectory(utils)

###############################################################################
# Doc
###############################################################################
# add a target to generate API documentation with Doxygen
find_package(Doxygen)
if(DOXYGEN_FOUND)
	configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
	add_custom_target(doc
		${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
		COMMENT "Generating API documentation with Doxygen" VERBATIM
		)
endif(DOXYGEN_FOUND)

###############################################################################
# Packages
###############################################################################
set(CPACK_MONOLITHIC_INSTALL 1)
set(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION ".")
include(InstallRequiredSystemLibraries)
set(CPACK_PACKAGE_FILE_NAME "oriflamms-${ORIFLAMMS_PACKAGE_VERSION_MAJOR}.${ORIFLAMMS_PACKAGE_VERSION_MINOR}.${ORIFLAMMS_PACKAGE_VERSION_PATCH}${ORIFLAMMS_PACK_NAME}")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "oriflamms")
set(CPACK_RESOURCE_FILE_LICENSE  "${CMAKE_CURRENT_SOURCE_DIR}/License.txt")
set(CPACK_PACKAGE_VERSION_MAJOR ${ORIFLAMMS_PACKAGE_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${ORIFLAMMS_PACKAGE_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${ORIFLAMMS_PACKAGE_VERSION_PATCH})
set(CPACK_BINARY_TZ OFF)
if(WIN32)
	set(CPACK_BINARY_NSIS ON)
	set(CPACK_NSIS_DISPLAY_NAME "oriflamms ${ORIFLAMMS_PACKAGE_VERSION_MAJOR}.${ORIFLAMMS_PACKAGE_VERSION_MINOR}.${ORIFLAMMS_PACKAGE_VERSION_PATCH}")
	set(CPACK_NSIS_MUI_ICON "${CMAKE_SOURCE_DIR}/oriflamms/oriflamms.ico")
	set(CPACK_NSIS_CREATE_ICONS_EXTRA "CreateShortCut '$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\Oriflamms.lnk' '$INSTDIR\\\\oriflamms.exe' '' '$INSTDIR\\\\oriflamms.ico'\\nCreateShortCut '$DESKTOP\\\\Oriflamms.lnk' '$INSTDIR\\\\oriflamms.exe' '' '$INSTDIR\\\\oriflamms.ico'")
	set(CPACK_NSIS_DELETE_ICONS_EXTRA "Delete '$SMPROGRAMS\\\\$START_MENU\\\\oriflamms.lnk'\\nDelete '$DESKTOP\\\\oriflamms.lnk'")
	set(CPACK_NSIS_MENU_LINKS
    "http://github.com/Liris-Pleiad/oriflamms" "Oriflamms Web Site")
	set(CPACK_NSIS_SOURCE_DIR "${PROJECT_SOURCE_DIR}")
	set(CPACK_NSIS_COMPONENT_INSTALL OFF)
	set(CPACK_BINARY_TBZ2 OFF)
	set(CPACK_BINARY_ZIP ON)
endif(WIN32)
if(UNIX)
	set(CPACK_BINARY_TBZ2 ON)
	set(CPACK_DEBIAN_PACKAGE_VERSION ${ORIFLAMMS_PACKAGE_VERSION})
	set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Yann LEYDIER <yann@leydier.info>")
	set(CPACK_DEBIAN_PACKAGE_DESCRIPTION "The oriflamms text alignment toolkit.")
	set(CPACK_DEBIAN_PACKAGE_SECTION "science")
	set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON) # auto depends
	set(CPACK_DEB_COMPONENT_INSTALL ON)
	set(CPACK_RPM_COMPONENT_INSTALL ON)
endif(UNIX)
set(CPACK_COMPONENTS_ALL bin dev app)
include(CPack)


