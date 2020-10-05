cmake_minimum_required (VERSION 3.13 FATAL_ERROR)
project ("mulTTY" VERSION 0.0 LANGUAGES C)

# Pretty reporting of packages
include (FeatureSummary)

# Look for the module itself. Set properties here, because ARPA2CM
# might not be installed to set them itself.
find_package (ARPA2CM 0.8.0 QUIET NO_MODULE)
set_package_properties (ARPA2CM PROPERTIES
	DESCRIPTION "CMake modules for ARPA2 projects"
	TYPE REQUIRED
	URL "https://gitlab.com/arpa2/mulTTY"
	PURPOSE "Required for the CMake build system for ${PROJECT}"
)

# If found, use it, otherwise report error and stop CMake.
if (ARPA2CM_FOUND)
    set (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake ${ARPA2CM_MODULE_PATH})
else()
    feature_summary (WHAT ALL)
    message (FATAL_ERROR "ARPA2CM is required.")
endif()

include (MacroEnsureOutOfSourceBuild)
include (MacroAddUninstallTarget)
include (MacroGitVersionInfo)
include (MacroCreateConfigFiles)
include (MacroLibraryPair)
include (GNUInstallDirs)
include (ExceptionHandling)


option (DEBUG
        "Switch on output and flags that aid developpers in debugging"
        OFF)

option (CUT_THROAT_TESTING
        "Include cut-through tests: under development, still made to work, known to fail"
        OFF)

if (DEBUG)
        add_compile_options (-DDEBUG -ggdb3 -O0)
endif()

if (WIN32)
        set (NO_DAEMONS_DEFAULT ON)
else ()
        set (NO_DAEMONS_DEFAULT OFF)
endif ()

macro_ensure_out_of_source_build("Do not build ${PROJECT} in the source directory.")
get_project_git_version()

enable_testing ()


#
# Configuration Settings (Runtime)
#


# set (MTY_VAR "value" CACHE STRING "this is my life")


#
# Dependencies
#
find_package (com_err            REQUIRED)
find_package (RAGEL              REQUIRED)

include_directories ($CMAKE_CURRENT_SOURCE_DIR}/include)

add_subdirectory (lib)
add_subdirectory (src)
add_subdirectory (test)


#
# Building
#


#
# Testing
#


#
# Installation
#


install (DIRECTORY include/arpa2
	DESTINATION include
	FILES_MATCHING PATTERN "*.h"
)

install (DIRECTORY src
	DESTINATION bin
)
