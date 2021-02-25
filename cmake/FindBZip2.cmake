#.rst:
# FindBZip2
# ---------
#
# Find the BZip2 compression library.
#
# Imported targets
# ^^^^^^^^^^^^^^^^
#
# This module defines the following :prop_tgt:`IMPORTED` target:
#
# ``BZip2::BZip2``
#   The BZip2 library, if found.
#
# Result variables
# ^^^^^^^^^^^^^^^^
#
# This module will set the following variables in your project:
#
# ``BZip2_INCLUDE_DIRS``
#   where to find bzlib.h, etc.
# ``BZip2_LIBRARIES``
#   the libraries to link against to use BZip2.
# ``BZip2_FOUND``
#   If false, do not try to use BZip2.

if(NOT BZip2_INCLUDE_DIR)
  find_path(BZip2_INCLUDE_DIR bzlib.h)
endif()

if(NOT BZip2_LIBRARY)
  find_library(BZip2_LIBRARY NAMES ${GR_THIRDPARTY_LIBRARY_PREFIX}bz2${GR_THIRDPARTY_LIBRARY_SUFFIX} bz2 libbz2)
endif()

if(BZip2_INCLUDE_DIR)
  if(NOT BZIP2_VERSION_STRING)
    file(READ ${BZip2_INCLUDE_DIR}/bzlib.h BZIP2_H_TEXT)
    string(REGEX REPLACE ".*version ([0-9]+.[0-9]+.[0-9]).*" "\\1" BZIP2_VERSION_STRING ${BZIP2_H_TEXT})
  endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
  BZip2
  VERSION_VAR
  BZIP2_VERSION_STRING
  REQUIRED_VARS
  BZip2_LIBRARY
  BZip2_INCLUDE_DIR
  BZIP2_VERSION_STRING
)

if(BZip2_FOUND)
  set(BZip2_INCLUDE_DIRS ${BZip2_INCLUDE_DIR})
  set(BZip2_LIBRARIES ${BZip2_LIBRARY})

  if(NOT TARGET BZip2::BZip2)
    add_library(BZip2::BZip2 UNKNOWN IMPORTED)
    set_target_properties(
      BZip2::BZip2
      PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${BZip2_INCLUDE_DIRS}"
                 IMPORTED_LINK_INTERFACE_LANGUAGES "C"
                 IMPORTED_LOCATION "${BZip2_LIBRARY}"
    )
  endif()
elseif(${CMAKE_FIND_PACKAGE_NAME}_FIND_REQUIRED)
  message(FATAL_ERROR "${CMAKE_FIND_PACKAGE_NAME} was required but could not be found.")
endif()

mark_as_advanced(BZip2_INCLUDE_DIR BZip2_LIBRARY)
