#.rst:
# FindZlib
# --------
#
# Find the Z compression library.
#
# Imported targets
# ^^^^^^^^^^^^^^^^
#
# This module defines the following :prop_tgt:`IMPORTED` target:
#
# ``Zlib::Zlib``
#   The Zlib library, if found.
#
# Result variables
# ^^^^^^^^^^^^^^^^
#
# This module will set the following variables in your project:
#
# ``ZLIB_INCLUDE_DIRS``
#   where to find zlib.h, etc.
# ``ZLIB_LIBRARIES``
#   the libraries to link against to use Zlib.
# ``Zlib_FOUND``
#   If false, do not try to use Zlib.

if(NOT ZLIB_INCLUDE_DIR)
  find_path(ZLIB_INCLUDE_DIR zlib.h)
endif()

if(NOT ZLIB_LIBRARY)
  find_library(ZLIB_LIBRARY NAMES z zlib)
endif()

if(ZLIB_INCLUDE_DIR)
  if(NOT ZLIB_VERSION_STRING)
    file(READ ${ZLIB_INCLUDE_DIR}/zlib.h ZLIB_H_TEXT)
    string(REGEX REPLACE ".*#define ZLIB_VERSION \"([0-9]+.[0-9]+.[0-9]+)\".*" "\\1" ZLIB_VERSION_STRING ${ZLIB_H_TEXT})
  endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
  Zlib
  VERSION_VAR
  ZLIB_VERSION_STRING
  REQUIRED_VARS
  ZLIB_LIBRARY
  ZLIB_INCLUDE_DIR
  ZLIB_VERSION_STRING
)

if(Zlib_FOUND)
  set(ZLIB_INCLUDE_DIRS ${ZLIB_INCLUDE_DIR})
  set(ZLIB_LIBRARIES ${ZLIB_LIBRARY})

  if(NOT TARGET Zlib::Zlib)
    add_library(Zlib::Zlib UNKNOWN IMPORTED)
    set_target_properties(
      Zlib::Zlib
      PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${ZLIB_INCLUDE_DIRS}"
                 IMPORTED_LINK_INTERFACE_LANGUAGES "C"
                 IMPORTED_LOCATION "${ZLIB_LIBRARY}"
    )
  endif()
elseif(${CMAKE_FIND_PACKAGE_NAME}_FIND_REQUIRED)
  message(FATAL_ERROR "${CMAKE_FIND_PACKAGE_NAME} was required but could not be found.")
endif()

mark_as_advanced(ZLIB_INCLUDE_DIR ZLIB_LIBRARY ZLIB_H_TEXT ZLIB_VERSION_STRING)
