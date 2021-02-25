#.rst:
# FindLibpng
# ----------
#
# Find the PNG library.
#
# Imported targets
# ^^^^^^^^^^^^^^^^
#
# This module defines the following :prop_tgt:`IMPORTED` target:
#
# ``Libpng::Libpng``
#   The Libpng library, if found.
#
# Result variables
# ^^^^^^^^^^^^^^^^
#
# This module will set the following variables in your project:
#
# ``LIBPNG_INCLUDE_DIRS``
#   where to find png.h, etc.
# ``LIBPNG_LIBRARIES``
#   the libraries to link against to use Libpng.
# ``Libpng_FOUND``
#   If false, do not try to use Libpng.

find_package(Zlib)

if(NOT LIBPNG_INCLUDE_DIR)
  find_path(LIBPNG_INCLUDE_DIR png.h)
endif()

if(NOT LIBPNG_LIBRARY)
  find_library(LIBPNG_LIBRARY NAMES png libpng16)
endif()

if(LIBPNG_INCLUDE_DIR)
  if(NOT LIBPNG_VERSION_STRING)
    file(READ ${LIBPNG_INCLUDE_DIR}/png.h LIBPNG_H_TEXT)
    string(REGEX REPLACE ".*#define PNG_LIBPNG_VER_STRING \"([0-9]+.[0-9]+.[0-9]+)\".*" "\\1" LIBPNG_VERSION_STRING
                         ${LIBPNG_H_TEXT}
    )
  endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
  Libpng
  VERSION_VAR
  LIBPNG_VERSION_STRING
  REQUIRED_VARS
  LIBPNG_LIBRARY
  LIBPNG_INCLUDE_DIR
  Zlib_FOUND
  LIBPNG_VERSION_STRING
)

if(Libpng_FOUND)
  set(LIBPNG_INCLUDE_DIRS ${LIBPNG_INCLUDE_DIR})
  set(LIBPNG_LIBRARIES ${LIBPNG_LIBRARY})

  if(NOT TARGET Libpng::Libpng)
    add_library(Libpng::Libpng UNKNOWN IMPORTED)
    set_target_properties(
      Libpng::Libpng
      PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${LIBPNG_INCLUDE_DIRS}"
                 IMPORTED_LINK_INTERFACE_LANGUAGES "C"
                 IMPORTED_LOCATION "${LIBPNG_LIBRARY}"
                 INTERFACE_LINK_LIBRARIES "Zlib::Zlib"
    )
  endif()
elseif(${CMAKE_FIND_PACKAGE_NAME}_FIND_REQUIRED)
  message(FATAL_ERROR "${CMAKE_FIND_PACKAGE_NAME} was required but could not be found.")
endif()

mark_as_advanced(LIBPNG_INCLUDE_DIR LIBPNG_LIBRARY)
