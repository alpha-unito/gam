#
# Source: https://github.com/ornladios/ADIOS2
#

######################################################
# - Try to find libfabric (http://directory.fsf.org/wiki/Libfabric)
# Once done this will define
#  LIBFABRIC_FOUND - System has libfabric
#  LIBFABRIC_INCLUDE_DIRS - The libfabric include directories
#  LIBFABRIC_LIBRARIES - The libraries needed to use libfabric
#  LIBFABRIC_DEFINITIONS - The extra CFLAGS needed to use libfabric

######################################################

# This is a bit of a wierd pattern but it allows to bypass pkg-config and
# manually specify library information
if(NOT (PC_LIBFABRIC_FOUND STREQUAL "IGNORE"))
  find_package(PkgConfig)
  if(PKG_CONFIG_FOUND)
    set(_CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH})
    if(LIBFABRIC_ROOT)
      list(INSERT CMAKE_PREFIX_PATH 0 "${LIBFABRIC_ROOT}")
    elseif(NOT ENV{LIBFABRIC_ROOT} STREQUAL "")
      list(INSERT CMAKE_PREFIX_PATH 0 "$ENV{LIBFABRIC_ROOT}")
    endif()
    set(PKG_CONFIG_USE_CMAKE_PREFIX_PATH ON)

    pkg_check_modules(PC_LIBFABRIC libfabric)

    set(CMAKE_PREFIX_PATH ${_CMAKE_PREFIX_PATH})
    unset(_CMAKE_PREFIX_PATH)

    if(PC_LIBFABRIC_FOUND)
      message(STATUS "PC_LIBFABRIC_FOUND")
      set(LIBFABRIC_INCLUDE_DIRS ${PC_LIBFABRIC_INCLUDE_DIRS})
      message(STATUS "LIBFABRIC_INCLUDE_DIRS=${LIBFABRIC_INCLUDE_DIRS}")
      set(LIBFABRIC_LIBRARIES ${PC_LIBFABRIC_LIBRARIES})
      message(STATUS "LIBFABRIC_LIBRARIES=${LIBFABRIC_LIBRARIES}")
      set(LIBFABRIC_DEFINITIONS ${PC_LIBFABRIC_CFLAGS_OTHER})
      message(STATUS "LIBFABRIC_DEFINITIONS=${LIBFABRIC_DEFINITIONS}")
    endif()
  endif()
endif()

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set LIBFABRIC_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(LIBFABRIC DEFAULT_MSG PC_LIBFABRIC_LDFLAGS)

if(LIBFABRIC_FOUND)
  message(STATUS "LIBFABRIC_FOUND")
  if(NOT TARGET libfabric::libfabric)
    add_library(libfabric::libfabric INTERFACE IMPORTED)
    if(LIBFABRIC_INCLUDE_DIRS)
      set_target_properties(libfabric::libfabric PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${LIBFABRIC_INCLUDE_DIRS}"
      )
    endif()
    if(LIBFABRIC_DEFINITIONS)
      set_target_properties(libfabric::libfabric PROPERTIES
        INTERFACE_COMPILE_OPTIONS     "${LIBFABRIC_DEFINITIONS}"
      )
    endif()
    if(LIBFABRIC_LIBRARIES)
      set_target_properties(libfabric::libfabric PROPERTIES
        INTERFACE_LINK_LIBRARIES      "${PC_LIBFABRIC_LDFLAGS}"
      )
    endif()
  endif()
endif()