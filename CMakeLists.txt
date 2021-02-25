cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
include(GetVersionFromGit)
get_version_from_git(GR_VERSION GR_VERSION_FULL)

project(
  GR
  VERSION ${GR_VERSION}
  LANGUAGES C CXX
)

include(GNUInstallDirs)

# Honor `C_VISIBILITY_PRESET hidden` in static libraries
cmake_policy(SET CMP0063 NEW)

if("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  set(COMPILER_OPTION_ERROR_IMPLICIT
      "-Werror=implicit"
      CACHE INTERNAL "Compiler flag for generating errors on implicit declarations"
  )
else()
  set(COMPILER_OPTION_ERROR_IMPLICIT
      ""
      CACHE INTERNAL "Compiler flag for generating errors on implicit declarations"
  )
endif()

if(UNIX)
  if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX
        "/usr/local/gr"
        CACHE PATH "GR install prefix" FORCE
    )
  endif()
endif()
set(GR_DIRECTORY
    "${CMAKE_INSTALL_PREFIX}"
    CACHE STRING "Default value for GRDIR"
)
option(GR_BUILD_DEMOS "Build demos for GR" OFF)
option(GR_INSTALL "Create installation target for GR" ON)
option(GR_USE_BUNDLED_LIBRARIES "Use thirdparty libraries bundled with GR" OFF)
option(GR_MANUAL_MOC_AND_RCC "Manually run moc and rcc instead of relying on AUTOMOC and AUTORCC" OFF)
option(GR_FIND_QT5_BY_VARIABLES "Find Qt5 based on the variables Qt5_INCLUDE_DIR and Qt5_LIBRARY_DIR" OFF)

if(GR_USE_BUNDLED_LIBRARIES)
  list(APPEND CMAKE_FIND_ROOT_PATH "${CMAKE_CURRENT_LIST_DIR}/3rdparty/build/")
  set(GR_THIRDPARTY_LIBRARY_PREFIX ${CMAKE_STATIC_LIBRARY_PREFIX})
  set(GR_THIRDPARTY_LIBRARY_SUFFIX ${CMAKE_STATIC_LIBRARY_SUFFIX})
else()
  set(GR_THIRDPARTY_LIBRARY_PREFIX ${CMAKE_SHARED_LIBRARY_PREFIX})
  set(GR_THIRDPARTY_LIBRARY_SUFFIX ${CMAKE_SHARED_LIBRARY_SUFFIX})
endif()

if(GR_FIND_QT5_BY_VARIABLES)
  list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake/optional/qt5")
endif()

if(WIN32)
  set(GR_PLUGIN_SUFFIX ".dll")
else()
  set(GR_PLUGIN_SUFFIX ".so")
endif()

if(APPLE)
  set(DEFAULT_SHARED_LIBRARY_SUFFIX ".dylib")
else()
  set(DEFAULT_SHARED_LIBRARY_SUFFIX "${GR_PLUGIN_SUFFIX}")
endif()

set(GR_SHARED_LIBRARY_SUFFIX
    "${DEFAULT_SHARED_LIBRARY_SUFFIX}"
    CACHE STRING "File suffix for shared libraries"
)

find_package(Freetype)
find_package(Jpeg REQUIRED)
find_package(Libpng REQUIRED)
find_package(Zlib REQUIRED)
find_package(Qhull REQUIRED)
find_package(Tiff)
find_package(Ffmpeg 57.48.100)
find_package(Glfw)
find_package(Zeromq 4.0 EXACT)
find_package(Cairo)
find_package(X11)
find_package(Fontconfig)
find_package(OpenGL OPTIONAL_COMPONENTS OpenGL)
find_package(Gs)
find_package(Qt5 OPTIONAL_COMPONENTS Widgets Core Network)
find_package(Qt4)

if(APPLE)
  set(INSTALL_RPATH "${GR_DIRECTORY}/lib/;@loader_path/.")
else()
  set(INSTALL_RPATH "${GR_DIRECTORY}/lib/;$ORIGIN/.")
endif()

if(Qt4_FOUND
   OR (Qt5Widgets_FOUND
       AND Qt5Core_FOUND
       AND Qt5Network_FOUND
      )
)
  if(GR_MANUAL_MOC_AND_RCC)
    if(NOT QT_MOC_EXECUTABLE)
      find_program(QT_MOC_EXECUTABLE moc)
    endif()
    if(NOT QT_MOC_EXECUTABLE)
      message(FATAL_ERROR "Could not find moc but GR_MANUAL_MOC_AND_RCC is set.")
    endif()
    if(NOT QT_RCC_EXECUTABLE)
      find_program(QT_RCC_EXECUTABLE rcc)
    endif()
    if(NOT QT_RCC_EXECUTABLE)
      message(FATAL_ERROR "Could not find rcc but GR_MANUAL_MOC_AND_RCC is set.")
    endif()
  endif()
endif()
if(Qt4_FOUND)
  set(QT4_MOC_INCLUDE_FLAGS "")
  foreach(DIR IN LISTS QT_INCLUDE_DIR)
    set(QT4_MOC_INCLUDE_FLAGS ${QT4_MOC_INCLUDE_FLAGS} -I${DIR})
  endforeach()
endif()
if(Qt5Widgets_FOUND
   AND Qt5Core_FOUND
   AND Qt5Network_FOUND
)
  set(QT5_MOC_INCLUDE_FLAGS "")
  foreach(DIR IN LISTS Qt5Core_INCLUDE_DIRS Qt5Gui_INCLUDE_DIRS Qt5Widgets_INCLUDE_DIRS)
    set(QT5_MOC_INCLUDE_FLAGS ${QT5_MOC_INCLUDE_FLAGS} -I${DIR})
  endforeach()
endif()

if(X11_FOUND)
  # older versions of FindX11.cmake set variables but do not create targets
  if(NOT TARGET X11::X11
     AND X11_INCLUDE_DIR
     AND X11_X11_LIB
     AND X11_Xft_LIB
  )
    add_library(X11::X11 UNKNOWN IMPORTED)
    set_target_properties(
      X11::X11
      PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${X11_INCLUDE_DIR}/"
                 IMPORTED_LINK_INTERFACE_LANGUAGES "C"
                 IMPORTED_LOCATION "${X11_X11_LIB}"
                 INTERFACE_LINK_LIBRARIES "${X11_Xft_LIB}"
    )
  endif()
  if(NOT TARGET X11::Xt
     AND X11_INCLUDE_DIR
     AND X11_Xt_LIB
     AND TARGET X11::X11
  )
    add_library(X11::Xt UNKNOWN IMPORTED)
    set_target_properties(
      X11::Xt
      PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${X11_INCLUDE_DIR}/"
                 IMPORTED_LINK_INTERFACE_LANGUAGES "C"
                 IMPORTED_LOCATION "${X11_Xt_LIB}"
                 INTERFACE_LINK_LIBRARIES "X11::X11"
    )
  endif()
endif()

set(GKS_SOURCES
    lib/gks/afm.c
    lib/gks/font.c
    lib/gks/socket.c
    lib/gks/ft.c
    lib/gks/malloc.c
    lib/gks/util.c
    lib/gks/compress.c
    lib/gks/gks.c
    lib/gks/mf.c
    lib/gks/win.c
    lib/gks/gksforbnd.c
    lib/gks/pdf.c
    lib/gks/wiss.c
    lib/gks/dl.c
    lib/gks/plugin.c
    lib/gks/error.c
    lib/gks/io.c
    lib/gks/ps.c
    lib/gks/resample.c
)

add_library(gks_static STATIC ${GKS_SOURCES})
add_library(gks_shared SHARED ${GKS_SOURCES})

foreach(LIBRARY gks_static gks_shared)
  if(LIBRARY MATCHES "static")
    set(GKS_LINK_MODE PUBLIC)
  else()
    set(GKS_LINK_MODE PRIVATE)
  endif()
  if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_definitions(${LIBRARY} PRIVATE _CRT_SECURE_NO_WARNINGS)
  endif()
  if(NOT ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC"))
    target_link_libraries(${LIBRARY} ${GKS_LINK_MODE} pthread)
    target_link_libraries(${LIBRARY} ${GKS_LINK_MODE} m)
  endif()
  if(FREETYPE_FOUND)
    target_link_libraries(${LIBRARY} ${GKS_LINK_MODE} Freetype::Freetype)
  else()
    target_compile_definitions(
      ${LIBRARY}
      ${GKS_LINK_MODE}
      NO_FT
    )
  endif()
  target_link_libraries(${LIBRARY} ${GKS_LINK_MODE} Zlib::Zlib)
  if(UNIX)
    target_link_libraries(${LIBRARY} ${GKS_LINK_MODE} dl)
  elseif(WIN32)
    target_link_libraries(${LIBRARY} ${GKS_LINK_MODE} wsock32)
    target_link_libraries(${LIBRARY} ${GKS_LINK_MODE} msimg32)
    target_link_libraries(${LIBRARY} ${GKS_LINK_MODE} gdi32)
  endif()
  if(NOT ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC"))
    target_compile_options(${LIBRARY} ${GKS_LINK_MODE} -pthread)
  endif()
  target_compile_options(${LIBRARY} PRIVATE ${COMPILER_OPTION_ERROR_IMPLICIT})
  target_include_directories(
    ${LIBRARY} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib/gks/>
                      $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  )
  target_compile_definitions(${LIBRARY} PRIVATE GRDIR="${GR_DIRECTORY}")
  set_target_properties(
    ${LIBRARY}
    PROPERTIES C_STANDARD 90
               C_EXTENSIONS OFF
               C_STANDARD_REQUIRED ON
               POSITION_INDEPENDENT_CODE ON
               PREFIX "lib"
               IMPORT_PREFIX "lib"
               OUTPUT_NAME GKS
               INSTALL_RPATH "${INSTALL_RPATH}"
  )
endforeach()
unset(GKS_LINK_MODE)
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
  set_target_properties(gks_static PROPERTIES OUTPUT_NAME GKSstatic)
endif()
set_target_properties(gks_shared PROPERTIES SUFFIX "${GR_SHARED_LIBRARY_SUFFIX}" EXPORT_NAME GKS)
add_library(GR::GKS ALIAS gks_static)
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/gr_version.h
     "#ifndef GR_VERSION\n#define GR_VERSION \"${GR_VERSION_FULL}\"\n#endif\n"
)
set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/gr_version.h PROPERTIES GENERATED TRUE)

set(GR_SOURCES
    lib/gr/boundary.c
    lib/gr/contour.c
    lib/gr/contourf.c
    lib/gr/delaunay.c
    lib/gr/gr.c
    lib/gr/grforbnd.c
    lib/gr/gridit.c
    lib/gr/image.c
    lib/gr/import.c
    lib/gr/interp2.c
    lib/gr/io.c
    lib/gr/md5.c
    lib/gr/shade.c
    lib/gr/spline.c
    lib/gr/strlib.c
    lib/gr/text.c
    lib/gr/mathtex2.c
    lib/gr/mathtex2.tab.c
    ${CMAKE_CURRENT_BINARY_DIR}/gr_version.h
)

add_library(gr_static STATIC ${GR_SOURCES})
add_library(gr_shared SHARED ${GR_SOURCES})

foreach(LIBRARY gr_static gr_shared)
  if(LIBRARY MATCHES "static")
    set(GR_LINK_MODE PUBLIC)
  else()
    set(GR_LINK_MODE PRIVATE)
  endif()
  target_link_libraries(${LIBRARY} ${GR_LINK_MODE} gks_static)
  if(NOT ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC"))
    target_link_libraries(${LIBRARY} ${GR_LINK_MODE} m)
  endif()
  target_link_libraries(${LIBRARY} ${GR_LINK_MODE} Qhull::Qhull)
  target_link_libraries(${LIBRARY} ${GR_LINK_MODE} Jpeg::Jpeg)
  target_link_libraries(${LIBRARY} ${GR_LINK_MODE} Libpng::Libpng)
  if(WIN32)
    target_link_libraries(${LIBRARY} ${GR_LINK_MODE} ws2_32)
  endif()
  target_include_directories(
    ${LIBRARY} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib/gr/>
                      $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  )
  target_include_directories(${LIBRARY} PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>)
  if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_definitions(${LIBRARY} PRIVATE _CRT_SECURE_NO_WARNINGS)
  endif()
  target_compile_definitions(${LIBRARY} PRIVATE GRDIR="${GR_DIRECTORY}")
  target_compile_options(${LIBRARY} PRIVATE ${COMPILER_OPTION_ERROR_IMPLICIT})
  set_target_properties(
    ${LIBRARY}
    PROPERTIES C_STANDARD 90
               C_EXTENSIONS OFF
               C_STANDARD_REQUIRED ON
               POSITION_INDEPENDENT_CODE ON
               INSTALL_RPATH "${INSTALL_RPATH}"
               PREFIX "lib"
               IMPORT_PREFIX "lib"
               OUTPUT_NAME GR
  )
endforeach()
unset(GR_LINK_MODE)
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
  set_target_properties(gr_static PROPERTIES OUTPUT_NAME GRstatic)
endif()
set_target_properties(gr_shared PROPERTIES SUFFIX "${GR_SHARED_LIBRARY_SUFFIX}" EXPORT_NAME GR)
add_library(GR::GR ALIAS gr_shared)

set(GR3_SOURCES
    lib/gr3/gr3.c
    lib/gr3/gr3_convenience.c
    lib/gr3/gr3_gr.c
    lib/gr3/gr3_html.c
    lib/gr3/gr3_jpeg.c
    lib/gr3/gr3_mc.c
    lib/gr3/gr3_png.c
    lib/gr3/gr3_povray.c
    lib/gr3/gr3_slices.c
    lib/gr3/gr3_sr.c
)

add_library(gr3_static STATIC ${GR3_SOURCES})
add_library(gr3_shared SHARED ${GR3_SOURCES})
foreach(LIBRARY gr3_static gr3_shared)
  if(LIBRARY MATCHES "static")
    set(GR3_LINK_MODE PUBLIC)
  else()
    set(GR3_LINK_MODE PRIVATE)
  endif()
  if(APPLE)
    target_sources(${LIBRARY} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/lib/gr3/gr3_cgl.c)
  elseif(UNIX AND NOT APPLE)
    target_sources(${LIBRARY} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/lib/gr3/gr3_glx.c)
  elseif(WIN32)
    target_sources(${LIBRARY} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/lib/gr3/gr3_win.c)
  endif()
  target_link_libraries(${LIBRARY} ${GR3_LINK_MODE} GR::GR)
  target_link_libraries(${LIBRARY} ${GR3_LINK_MODE} Jpeg::Jpeg)
  target_link_libraries(${LIBRARY} ${GR3_LINK_MODE} Libpng::Libpng)
  if(NOT ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC"))
    target_link_libraries(${LIBRARY} ${GR3_LINK_MODE} pthread)
    target_link_libraries(${LIBRARY} ${GR3_LINK_MODE} m)
  endif()
  if((APPLE OR WIN32) AND OpenGL_FOUND)
    target_link_libraries(${LIBRARY} ${GR3_LINK_MODE} OpenGL::GL)
  endif()
  if(APPLE)
    # Apple has deprecated OpenGL in macOS 10.14
    target_compile_definitions(${LIBRARY} PRIVATE GL_SILENCE_DEPRECATION)
  endif()
  if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_definitions(${LIBRARY} PRIVATE _CRT_SECURE_NO_WARNINGS)
  endif()
  target_compile_definitions(${LIBRARY} PRIVATE GRDIR="${GR_DIRECTORY}")
  if(NOT ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC"))
    target_compile_options(${LIBRARY} ${GR3_LINK_MODE} -pthread)
  endif()
  target_compile_options(${LIBRARY} PRIVATE ${COMPILER_OPTION_ERROR_IMPLICIT})
  target_include_directories(
    ${LIBRARY} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib/gr3/>
                      $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  )
  set_target_properties(
    ${LIBRARY}
    PROPERTIES C_STANDARD 90
               C_EXTENSIONS OFF
               C_STANDARD_REQUIRED ON
               POSITION_INDEPENDENT_CODE ON
               INSTALL_RPATH "${INSTALL_RPATH}"
               PREFIX "lib"
               IMPORT_PREFIX "lib"
               OUTPUT_NAME GR3
  )
endforeach()
unset(GR3_LINK_MODE)
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
  set_target_properties(gr3_static PROPERTIES OUTPUT_NAME GR3static)
endif()
set_target_properties(gr3_shared PROPERTIES SUFFIX "${GR_SHARED_LIBRARY_SUFFIX}" EXPORT_NAME GR3)
add_library(GR::GR3 ALIAS gr3_shared)

if(UNIX AND NOT APPLE)
  if(TARGET X11::X11
     AND TARGET OpenGL::GLX
     AND TARGET OpenGL::GL
  )
    add_library(gr3platform SHARED lib/gr3/gr3_platform_glx.c)
    target_link_libraries(gr3platform PUBLIC GR::GR)
    target_link_libraries(gr3platform PUBLIC X11::X11)
    target_link_libraries(gr3platform PUBLIC OpenGL::GLX)
    target_link_libraries(gr3platform PUBLIC OpenGL::GL)
    target_compile_options(gr3platform PRIVATE ${COMPILER_OPTION_ERROR_IMPLICIT})
    set_target_properties(
      gr3platform
      PROPERTIES C_STANDARD 90
                 C_EXTENSIONS OFF
                 C_STANDARD_REQUIRED ON
                 POSITION_INDEPENDENT_CODE ON
                 INSTALL_RPATH "${INSTALL_RPATH}"
                 OUTPUT_NAME GR3platform
    )
  else()
    target_compile_definitions(gr3_static PUBLIC -DNO_GL)
    target_compile_definitions(gr3_shared PUBLIC -DNO_GL)
    message(WARNING "libGR3platform.so requires X11 and will not be built.")
  endif()
elseif(NOT OpenGL_FOUND)
  target_compile_definitions(gr3_static PUBLIC -DNO_GL)
  target_compile_definitions(gr3_shared PUBLIC -DNO_GL)
endif()

set(GRM_SOURCES
    lib/grm/args.c
    lib/grm/base64.c
    lib/grm/dump.c
    lib/grm/dynamic_args_array.c
    lib/grm/error.c
    lib/grm/event.c
    lib/grm/interaction.c
    lib/grm/json.c
    lib/grm/logging.c
    lib/grm/memwriter.c
    lib/grm/net.c
    lib/grm/plot.c
    lib/grm/util.c
    lib/grm/datatype/double_map.c
    lib/grm/datatype/string_array_map.c
    lib/grm/datatype/string_list.c
    lib/grm/datatype/string_map.c
    lib/grm/datatype/uint_map.c
)

set(GRM_SUBDIR_HEADERS
    lib/grm/args.h
    lib/grm/dump.h
    lib/grm/event.h
    lib/grm/interaction.h
    lib/grm/net.h
    lib/grm/plot.h
    lib/grm/util.h
)

add_library(grm_static STATIC ${GRM_SOURCES})
add_library(grm_shared SHARED ${GRM_SOURCES})
add_library(grm_shared_internal SHARED ${GRM_SOURCES})

foreach(LIBRARY grm_static grm_shared grm_shared_internal)
  if(LIBRARY MATCHES "static")
    set(GRM_LINK_MODE PUBLIC)
  else()
    set(GRM_LINK_MODE PRIVATE)
  endif()
  target_link_libraries(${LIBRARY} ${GRM_LINK_MODE} GR::GKS)
  target_link_libraries(${LIBRARY} ${GRM_LINK_MODE} GR::GR)
  target_link_libraries(${LIBRARY} ${GRM_LINK_MODE} GR::GR3)
  if(NOT ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC"))
    target_link_libraries(${LIBRARY} ${GRM_LINK_MODE} m)
  endif()
  if(WIN32)
    target_link_libraries(${LIBRARY} ${GRM_LINK_MODE} ws2_32)
  endif()
  target_include_directories(
    ${LIBRARY}
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib/> $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib/grm/>
           $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  )
  target_compile_options(${LIBRARY} PRIVATE ${COMPILER_OPTION_ERROR_IMPLICIT})
  set_target_properties(
    ${LIBRARY}
    PROPERTIES C_STANDARD 90
               C_EXTENSIONS OFF
               C_STANDARD_REQUIRED ON
               POSITION_INDEPENDENT_CODE ON
               C_VISIBILITY_PRESET hidden
               INSTALL_RPATH "${INSTALL_RPATH}"
               PREFIX "lib"
               IMPORT_PREFIX "lib"
               OUTPUT_NAME GRM
  )
endforeach()
unset(GRM_LINK_MODE)
set_target_properties(grm_shared_internal PROPERTIES C_VISIBILITY_PRESET default)
set_target_properties(grm_shared_internal PROPERTIES OUTPUT_NAME GRM_int)
target_include_directories(grm_shared_internal PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/lib/gks/>)
if(WIN32)
  target_compile_definitions(grm_shared PRIVATE BUILDING_DLL)
  target_compile_definitions(grm_shared_internal PRIVATE BUILDING_DLL)
endif()
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
  set_target_properties(grm_static PROPERTIES OUTPUT_NAME GRMstatic)
endif()
set_target_properties(grm_shared PROPERTIES SUFFIX "${GR_SHARED_LIBRARY_SUFFIX}" EXPORT_NAME GRM)
set_target_properties(grm_shared_internal PROPERTIES SUFFIX "${GR_SHARED_LIBRARY_SUFFIX}")
add_library(GR::GRM ALIAS grm_shared)

set(GR_REPORT "GKS plugins:\n")

add_library(cairoplugin SHARED lib/gks/plugin/cairoplugin.c)
target_link_libraries(cairoplugin PUBLIC gks_static)
if(Cairo_FOUND)
  target_link_libraries(cairoplugin PRIVATE Cairo::Cairo)
  target_link_libraries(cairoplugin PRIVATE Jpeg::Jpeg)
  string(APPEND GR_REPORT "- cairoplugin:\n")
  string(APPEND GR_REPORT "\tPNG output: Yes\n")
  string(APPEND GR_REPORT "\tBMP output: Yes\n")
  string(APPEND GR_REPORT "\tJPEG output: Yes\n")

  if(Tiff_FOUND)
    target_link_libraries(cairoplugin PRIVATE Tiff::Tiff)
    string(APPEND GR_REPORT "\tTiff output: Yes\n")
  else()
    string(APPEND GR_REPORT "\tTiff output: No (libtiff not found)\n")
    target_compile_definitions(cairoplugin PRIVATE NO_TIFF)
  endif()
  target_compile_options(cairoplugin PRIVATE ${COMPILER_OPTION_ERROR_IMPLICIT})
  string(APPEND GR_REPORT "\tSixel output: Yes\n")
  string(APPEND GR_REPORT "\tMemory output: Yes\n")
  string(APPEND GR_REPORT "\tX11 output: No (not available in CMake build)\n")
else()
  target_compile_definitions(cairoplugin PRIVATE NO_CAIRO)
  string(APPEND GR_REPORT "- cairoplugin: No (Cairo not found)\n")
endif()
# Cairo X11 support is disabled to allow users to generate images using Cairo on systems without X11 installed
target_compile_definitions(cairoplugin PRIVATE NO_X11)
set_target_properties(
  cairoplugin
  PROPERTIES C_STANDARD 90
             C_EXTENSIONS OFF
             C_STANDARD_REQUIRED ON
             PREFIX ""
             SUFFIX "${GR_PLUGIN_SUFFIX}"
)

add_library(videoplugin SHARED lib/gks/plugin/gif.c lib/gks/plugin/vc.c lib/gks/plugin/videoplugin.c)
target_link_libraries(videoplugin PUBLIC gks_static)
if(NOT Cairo_FOUND)
  target_compile_definitions(videoplugin PRIVATE NO_CAIRO)
  target_compile_definitions(videoplugin PRIVATE NO_AV)
  string(APPEND GR_REPORT "- videoplugin: No (Cairo not found)\n")
elseif(NOT Ffmpeg_FOUND)
  target_compile_definitions(videoplugin PRIVATE NO_CAIRO)
  target_compile_definitions(videoplugin PRIVATE NO_AV)
  string(APPEND GR_REPORT "- videoplugin: No (ffmpeg / ogg / theora / vpx / openh264 not found)\n")
else()
  string(APPEND GR_REPORT "- videoplugin:\n")
  string(APPEND GR_REPORT "\tMP4 output: Yes\n")
  string(APPEND GR_REPORT "\tMOV output: Yes\n")
  string(APPEND GR_REPORT "\tWEBM output: Yes\n")
  string(APPEND GR_REPORT "\tOGG output: Yes\n")
  string(APPEND GR_REPORT "\tGIF output: Yes\n")
  target_link_libraries(videoplugin PUBLIC Ffmpeg::Ffmpeg)
endif()
target_compile_options(videoplugin PRIVATE ${COMPILER_OPTION_ERROR_IMPLICIT})
set_target_properties(
  videoplugin
  PROPERTIES C_STANDARD 99
             C_EXTENSIONS OFF
             C_STANDARD_REQUIRED ON
             LINKER_LANGUAGE CXX
             PREFIX ""
             SUFFIX "${GR_PLUGIN_SUFFIX}"
)

add_library(pgfplugin SHARED lib/gks/plugin/pgfplugin.c)
target_link_libraries(pgfplugin PUBLIC gks_static)
target_link_libraries(pgfplugin PUBLIC Libpng::Libpng)
target_compile_options(pgfplugin PRIVATE ${COMPILER_OPTION_ERROR_IMPLICIT})
set_target_properties(
  pgfplugin
  PROPERTIES C_STANDARD 90
             C_EXTENSIONS OFF
             C_STANDARD_REQUIRED ON
             PREFIX ""
             SUFFIX "${GR_PLUGIN_SUFFIX}"
)
string(APPEND GR_REPORT "- pgfplugin:\n")
string(APPEND GR_REPORT "\tTeX output: Yes\n")

add_library(wmfplugin SHARED lib/gks/plugin/wmfplugin.c)
target_link_libraries(wmfplugin PUBLIC gks_static)
target_compile_options(wmfplugin PRIVATE ${COMPILER_OPTION_ERROR_IMPLICIT})
set_target_properties(
  wmfplugin
  PROPERTIES C_STANDARD 90
             C_EXTENSIONS OFF
             C_STANDARD_REQUIRED ON
             PREFIX ""
             SUFFIX "${GR_PLUGIN_SUFFIX}"
)
string(APPEND GR_REPORT "- wmfplugin:\n")
string(APPEND GR_REPORT "\tWMF output: Yes\n")

add_library(gsplugin SHARED lib/gks/plugin/gsplugin.c)
target_link_libraries(gsplugin PUBLIC gks_static)
if(NOT TARGET X11::Xt)
  string(APPEND GR_REPORT "- gsplugin: No (X11 / Xt not found)\n")
  target_compile_definitions(gsplugin PRIVATE NO_GS)
elseif(NOT Gs_FOUND)
  string(APPEND GR_REPORT "- gsplugin: No (ghostscript not found)\n")
  target_compile_definitions(gsplugin PRIVATE NO_GS)
else()
  target_link_libraries(gsplugin PUBLIC X11::Xt)
  target_link_libraries(gsplugin PUBLIC Gs::Gs)
  string(APPEND GR_REPORT "- gsplugin:\n")
  string(APPEND GR_REPORT "\tPNG outout: Yes\n")
  string(APPEND GR_REPORT "\tBMP outout: Yes\n")
  string(APPEND GR_REPORT "\tJPEG outout: Yes\n")
  string(APPEND GR_REPORT "\tTiff outout: Yes\n")
endif()
target_compile_options(gsplugin PRIVATE ${COMPILER_OPTION_ERROR_IMPLICIT})
set_target_properties(
  gsplugin
  PROPERTIES C_STANDARD 90
             C_EXTENSIONS OFF
             C_STANDARD_REQUIRED ON
             PREFIX ""
             SUFFIX "${GR_PLUGIN_SUFFIX}"
)

add_library(svgplugin SHARED lib/gks/plugin/svgplugin.c)
target_link_libraries(svgplugin PUBLIC gks_static)
target_link_libraries(svgplugin PUBLIC Libpng::Libpng)
target_compile_options(svgplugin PRIVATE ${COMPILER_OPTION_ERROR_IMPLICIT})
set_target_properties(
  svgplugin
  PROPERTIES C_STANDARD 90
             C_EXTENSIONS OFF
             C_STANDARD_REQUIRED ON
             PREFIX ""
             SUFFIX "${GR_PLUGIN_SUFFIX}"
)
string(APPEND GR_REPORT "- svgplugin:\n")
string(APPEND GR_REPORT "\tSVG output: Yes\n")

add_library(glplugin SHARED lib/gks/plugin/glplugin.c)
target_link_libraries(glplugin PUBLIC gks_static)
if(Glfw_FOUND AND OpenGL_FOUND)
  string(APPEND GR_REPORT "- glplugin: Yes\n")
  target_link_libraries(glplugin PUBLIC Glfw::Glfw)
  target_link_libraries(glplugin PUBLIC OpenGL::GL)
else()
  string(APPEND GR_REPORT "- glplugin: No (GLFW / OpenGL not found)\n")
  target_compile_definitions(glplugin PRIVATE NO_GLFW)
endif()
if(NOT Freetype_FOUND)
  target_compile_definitions(glplugin PRIVATE NO_FT)
endif()
if(APPLE)
  # Apple has deprecated OpenGL in macOS 10.14
  target_compile_definitions(glplugin PRIVATE GL_SILENCE_DEPRECATION)
endif()
target_compile_options(glplugin PRIVATE ${COMPILER_OPTION_ERROR_IMPLICIT})
set_target_properties(
  glplugin
  PROPERTIES C_STANDARD 90
             C_EXTENSIONS OFF
             C_STANDARD_REQUIRED ON
             PREFIX ""
             SUFFIX "${GR_PLUGIN_SUFFIX}"
)

add_library(zmqplugin SHARED lib/gks/plugin/zmqplugin.c)
target_link_libraries(zmqplugin PUBLIC gks_static)
if(Zeromq_FOUND)
  string(APPEND GR_REPORT "- zmqplugin: Yes\n")
  target_link_libraries(zmqplugin PUBLIC Zeromq::Zeromq)
else()
  string(APPEND GR_REPORT "- zmqplugin: No (ZeroMQ not found)\n")
  target_compile_definitions(zmqplugin PRIVATE NO_ZMQ)
endif()
target_compile_options(zmqplugin PRIVATE ${COMPILER_OPTION_ERROR_IMPLICIT})
set_target_properties(
  zmqplugin
  PROPERTIES C_STANDARD 90
             C_EXTENSIONS OFF
             C_STANDARD_REQUIRED ON
             PREFIX ""
             SUFFIX "${GR_PLUGIN_SUFFIX}"
)

add_library(x11plugin SHARED lib/gks/plugin/x11plugin.c)
target_link_libraries(x11plugin PUBLIC gks_static)
if(NOT Freetype_FOUND)
  target_compile_definitions(x11plugin PRIVATE NO_FT)
endif()
if(TARGET X11::Xt)
  if(Xft_FOUND AND Fontconfig_FOUND)
    target_link_libraries(x11plugin PUBLIC Fontconfig::Fontconfig)
  else()
    target_compile_definitions(x11plugin PRIVATE NO_XFT)
  endif()
  target_link_libraries(x11plugin PUBLIC X11::Xt)
  string(APPEND GR_REPORT "- x11plugin: Yes\n")
else()
  target_compile_definitions(x11plugin PRIVATE NO_X11)
  target_compile_definitions(x11plugin PRIVATE NO_XFT)
  string(APPEND GR_REPORT "- x11plugin: No (X11 / Xft / Xt not found)\n")
endif()
target_compile_options(x11plugin PRIVATE ${COMPILER_OPTION_ERROR_IMPLICIT})
set_target_properties(
  x11plugin
  PROPERTIES C_STANDARD 90
             C_EXTENSIONS OFF
             C_STANDARD_REQUIRED ON
             CXX_STANDARD 11
             CXX_EXTENSIONS OFF
             CXX_STANDARD_REQUIRED ON
             PREFIX ""
             SUFFIX "${GR_PLUGIN_SUFFIX}"
)

add_library(qt5plugin SHARED lib/gks/plugin/qt5plugin.cxx)
target_link_libraries(qt5plugin PUBLIC gks_static)
if(Qt5Widgets_FOUND)
  string(APPEND GR_REPORT "- qt5plugin: Yes\n")
  target_link_libraries(qt5plugin PUBLIC Qt5::Widgets)
else()
  string(APPEND GR_REPORT "- qt5plugin: No (Qt5 not found)\n")
  target_compile_definitions(qt5plugin PRIVATE NO_QT5)
endif()
target_compile_options(qt5plugin PRIVATE ${COMPILER_OPTION_ERROR_IMPLICIT})
set_target_properties(
  qt5plugin
  PROPERTIES CXX_STANDARD 11
             CXX_EXTENSIONS OFF
             CXX_STANDARD_REQUIRED ON
             PREFIX ""
             SUFFIX "${GR_PLUGIN_SUFFIX}"
)

add_library(qtplugin SHARED lib/gks/plugin/qtplugin.cxx)
target_link_libraries(qtplugin PUBLIC gks_static)
if(Qt4_FOUND)
  string(APPEND GR_REPORT "- qtplugin: Yes\n")
  target_link_libraries(qtplugin PUBLIC Qt4::QtGui)
else()
  string(APPEND GR_REPORT "- qtplugin: No (Qt4 not found)\n")
  target_compile_definitions(qtplugin PRIVATE NO_QT)
endif()
target_compile_options(qtplugin PRIVATE ${COMPILER_OPTION_ERROR_IMPLICIT})
set_target_properties(
  qtplugin
  PROPERTIES CXX_STANDARD 11
             CXX_EXTENSIONS OFF
             CXX_STANDARD_REQUIRED ON
             PREFIX ""
             SUFFIX "${GR_PLUGIN_SUFFIX}"
)

add_library(gtkplugin SHARED lib/gks/plugin/gtkplugin.c)
target_link_libraries(gtkplugin PUBLIC gks_static)
set_target_properties(
  gtkplugin
  PROPERTIES C_STANDARD 90
             C_EXTENSIONS OFF
             C_STANDARD_REQUIRED ON
             PREFIX ""
             SUFFIX "${GR_PLUGIN_SUFFIX}"
)
target_compile_options(gtkplugin PRIVATE ${COMPILER_OPTION_ERROR_IMPLICIT})
string(APPEND GR_REPORT "- gtkplugin: No (not implemented yet)\n")

if(APPLE)
  add_library(quartzplugin SHARED lib/gks/plugin/quartzplugin.m)
  target_link_libraries(quartzplugin PUBLIC gks_static)
  if(Zeromq_FOUND)
    target_link_libraries(quartzplugin PUBLIC Zeromq::Zeromq)
    target_link_libraries(quartzplugin PUBLIC "-framework Foundation -framework ApplicationServices -framework AppKit")
    target_compile_definitions(quartzplugin PRIVATE GRDIR="${GR_DIRECTORY}")
  else()
    target_compile_definitions(quartzplugin PRIVATE NO_GKSTERM)
  endif()
  target_compile_options(quartzplugin PRIVATE ${COMPILER_OPTION_ERROR_IMPLICIT})
  set_target_properties(
    quartzplugin
    PROPERTIES C_STANDARD 90
               C_EXTENSIONS OFF
               C_STANDARD_REQUIRED ON
               PREFIX ""
               SUFFIX "${GR_PLUGIN_SUFFIX}"
  )
endif()

if(Qt4_FOUND)
  add_library(qt4gr SHARED lib/gr/qtgr/grwidget.cxx)
  target_link_libraries(qt4gr PUBLIC GR::GR)
  target_link_libraries(qt4gr PUBLIC Qt4::QtCore Qt4::QtGui)
  if(MINGW)
    target_compile_options(qt4gr PRIVATE -fno-exceptions ${COMPILER_OPTION_ERROR_IMPLICIT})
  endif()
  target_compile_options(qt4gr PRIVATE ${COMPILER_OPTION_ERROR_IMPLICIT})
  set_target_properties(qt4gr PROPERTIES SUFFIX "${GR_SHARED_LIBRARY_SUFFIX}" INSTALL_RPATH "${INSTALL_RPATH}")
  if(GR_MANUAL_MOC_AND_RCC)
    set_target_properties(qt4gr PROPERTIES AUTOMOC OFF AUTORCC OFF)
    add_custom_command(
      OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/moc_qt4_grwidget.cxx
      COMMAND ${QT_MOC_EXECUTABLE} -DGRDIR=\"$(GR_DIRECTORY)\" ${QT4_MOC_INCLUDE_FLAGS}
              ${CMAKE_CURRENT_SOURCE_DIR}/lib/gr/qtgr/grwidget.h -o ${CMAKE_CURRENT_BINARY_DIR}/moc_qt4_grwidget.cxx
      DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/lib/gr/qtgr/grwidget.cxx
    )
    target_sources(qt4gr PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/moc_qt4_grwidget.cxx)
  else()
    set_target_properties(qt4gr PROPERTIES AUTOMOC ON AUTORCC ON)
    target_sources(qt4gr PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/lib/gr/qtgr/grwidget.h)
  endif()
  add_library(GR::qt4gr ALIAS qt4gr)
endif()

if(Qt5Widgets_FOUND AND Qt5Core_FOUND)
  add_library(qt5gr SHARED lib/gr/qtgr/grwidget.cxx)
  target_link_libraries(qt5gr PUBLIC GR::GR)
  target_link_libraries(qt5gr PUBLIC Qt5::Core Qt5::Widgets)
  if(MINGW)
    target_compile_options(qt5gr PRIVATE -fno-exceptions ${COMPILER_OPTION_ERROR_IMPLICIT})
  endif()
  target_compile_options(qt5gr PRIVATE ${COMPILER_OPTION_ERROR_IMPLICIT})
  set_target_properties(qt5gr PROPERTIES SUFFIX "${GR_SHARED_LIBRARY_SUFFIX}" INSTALL_RPATH "${INSTALL_RPATH}")
  if(GR_MANUAL_MOC_AND_RCC)
    set_target_properties(qt5gr PROPERTIES AUTOMOC OFF AUTORCC OFF)
    add_custom_command(
      OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/moc_qt5_grwidget.cxx
      COMMAND ${QT_MOC_EXECUTABLE} -DGRDIR=\"$(GR_DIRECTORY)\" ${QT5_MOC_INCLUDE_FLAGS}
              ${CMAKE_CURRENT_SOURCE_DIR}/lib/gr/qtgr/grwidget.h -o ${CMAKE_CURRENT_BINARY_DIR}/moc_qt5_grwidget.cxx
      DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/lib/gr/qtgr/grwidget.cxx
    )
    target_sources(qt5gr PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/moc_qt5_grwidget.cxx)
  else()
    set_target_properties(qt5gr PROPERTIES AUTOMOC ON AUTORCC ON)
    target_sources(qt5gr PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/lib/gr/qtgr/grwidget.h)
  endif()
  add_library(GR::qt5gr ALIAS qt5gr)
endif()

string(APPEND GR_REPORT "\nGKS applications:\n")

if(APPLE)
  if(Zeromq_FOUND)
    add_executable(GKSTerm MACOSX_BUNDLE lib/gks/quartz/GKSTerm.m lib/gks/quartz/GKSView.m lib/gks/quartz/main.m)
    target_link_libraries(GKSTerm PUBLIC Zeromq::Zeromq)
    target_link_libraries(
      GKSTerm PUBLIC gks_static
                     "-framework CoreGraphics -framework CoreFoundation -framework CoreVideo -framework Cocoa"
    )

    add_custom_command(
      TARGET GKSTerm
      POST_BUILD
      COMMAND
        ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/lib/gks/quartz/English.lproj
        ${CMAKE_CURRENT_BINARY_DIR}/GKSTerm.app/Contents/Resources/English.lproj DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/gks/quartz/English.lproj
      BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/GKSTerm.app/Contents/Resources/English.lproj
    )
    target_sources(GKSTerm PRIVATE lib/gks/quartz/GKSTerm.icns)
    set_source_files_properties(lib/gks/quartz/GKSTerm.icns PROPERTIES MACOSX_PACKAGE_LOCATION Resources/)
    target_compile_options(GKSTerm PRIVATE ${COMPILER_OPTION_ERROR_IMPLICIT})
    set_target_properties(
      GKSTerm
      PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME "GKSTerm"
                 MACOSX_BUNDLE_GUI_IDENTIFIER "de.fz-juelich.GKSTerm"
                 MACOSX_BUNDLE_ICON_FILE "GKSTerm"
                 MACOSX_BUNDLE_BUNDLE_VERSION "${GR_VERSION}"
                 MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/lib/gks/quartz/MacOSXBundleInfo.plist.in"
    )
    set(GKSTerm_MACOSX_BUNDLE_SIGNATURE "GKST")
    set(GKSTerm_MACOSX_MAIN_NIB_FILE "MainMenu")
    string(APPEND GR_REPORT "- GKSTerm: Yes\n")
  else()
    string(APPEND GR_REPORT "- GKSTerm: No (ZeroMQ not found)\n")
  endif()
endif()

if(Qt4_FOUND
   OR (Qt5Widgets_FOUND
       AND Qt5Core_FOUND
       AND Qt5Network_FOUND
      )
)
  add_executable(gksqt WIN32 MACOSX_BUNDLE lib/gks/qt/gksqt.cxx lib/gks/qt/gksserver.cxx lib/gks/qt/gkswidget.cxx)
  set_target_properties(gksqt PROPERTIES CXX_STANDARD 11 CXX_EXTENSIONS OFF CXX_STANDARD_REQUIRED ON)
  target_link_libraries(gksqt PUBLIC gks_static)
  if(Qt5Widgets_FOUND
     AND Qt5Core_FOUND
     AND Qt5Network_FOUND
  )
    target_link_libraries(gksqt PUBLIC Qt5::Widgets Qt5::Core Qt5::Network)
  elseif(Qt4_FOUND)
    target_link_libraries(gksqt PUBLIC Qt4::QtCore Qt4::QtGui Qt4::QtNetwork)
  endif()
  if(MINGW)
    target_compile_options(gksqt PRIVATE -fno-exceptions ${COMPILER_OPTION_ERROR_IMPLICIT})
  endif()
  target_compile_options(gksqt PRIVATE ${COMPILER_OPTION_ERROR_IMPLICIT})
  if(GR_MANUAL_MOC_AND_RCC)
    set_target_properties(gksqt PROPERTIES AUTOMOC OFF AUTORCC OFF)
    if(Qt5Widgets_FOUND
       AND Qt5Core_FOUND
       AND Qt5Network_FOUND
    )
      set(MOC_INCLUDE_FLAGS ${QT5_MOC_INCLUDE_FLAGS})
    else()
      set(MOC_INCLUDE_FLAGS ${QT4_MOC_INCLUDE_FLAGS})
    endif()
    add_custom_command(
      OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/moc_gkswidget.cxx
      COMMAND ${QT_MOC_EXECUTABLE} -DGRDIR=\"$(GR_DIRECTORY)\" ${MOC_INCLUDE_FLAGS}
              ${CMAKE_CURRENT_SOURCE_DIR}/lib/gks/qt/gkswidget.h -o ${CMAKE_CURRENT_BINARY_DIR}/moc_gkswidget.cxx
      DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/lib/gks/qt/gkswidget.h
    )
    add_custom_command(
      OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/moc_gksserver.cxx
      COMMAND ${QT_MOC_EXECUTABLE} -DGRDIR=\"$(GR_DIRECTORY)\" ${MOC_INCLUDE_FLAGS}
              ${CMAKE_CURRENT_SOURCE_DIR}/lib/gks/qt/gksserver.h -o ${CMAKE_CURRENT_BINARY_DIR}/moc_gksserver.cxx
      DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/lib/gks/qt/gksserver.h
    )
    add_custom_command(
      OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/qrc_gksqt.cxx
      COMMAND ${QT_RCC_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/lib/gks/qt/gksqt.qrc -o
              ${CMAKE_CURRENT_BINARY_DIR}/qrc_gksqt.cxx
      DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/lib/gks/qt/gksqt.qrc
    )
    target_sources(
      gksqt PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/moc_gksserver.cxx ${CMAKE_CURRENT_BINARY_DIR}/moc_gkswidget.cxx
                    ${CMAKE_CURRENT_BINARY_DIR}/qrc_gksqt.cxx
    )
  else()
    set_target_properties(gksqt PROPERTIES AUTOMOC ON AUTORCC ON)
    target_sources(
      gksqt PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/lib/gks/qt/gksserver.h
                    ${CMAKE_CURRENT_SOURCE_DIR}/lib/gks/qt/gkswidget.h ${CMAKE_CURRENT_SOURCE_DIR}/lib/gks/qt/gksqt.qrc
    )
  endif()
  if(APPLE)
    target_sources(gksqt PRIVATE lib/gks/qt/gksqt.icns)
    set_source_files_properties(lib/gks/qt/gksqt.icns PROPERTIES MACOSX_PACKAGE_LOCATION Resources/)
    set_target_properties(
      gksqt
      PROPERTIES MACOSX_BUNDLE_BUNDLE_NAME "gksqt"
                 MACOSX_BUNDLE_GUI_IDENTIFIER "de.fz-juelich.gksqt"
                 MACOSX_BUNDLE_ICON_FILE "gksqt"
                 MACOSX_BUNDLE_BUNDLE_VERSION "${GR_VERSION}"
    )
    set(gksqt_MACOSX_BUNDLE_SIGNATURE "gksqt")
    set(gksqt_MACOSX_MAIN_NIB_FILE "")
  endif()
  string(APPEND GR_REPORT "- gksqt: Yes\n")
else()
  string(APPEND GR_REPORT "- gksqt: No (Qt4 / Qt5 not found)\n")
endif()

if(GR_BUILD_DEMOS)
  add_executable(gksdemo lib/gks/demo.c)
  target_link_libraries(gksdemo PUBLIC gks_static)
  target_compile_options(gksdemo PRIVATE ${COMPILER_OPTION_ERROR_IMPLICIT})
  set_target_properties(gksdemo PROPERTIES C_STANDARD 90 C_EXTENSIONS OFF C_STANDARD_REQUIRED ON)

  add_executable(grdemo lib/gr/demo.c)
  target_link_libraries(grdemo PUBLIC GR::GR)
  target_link_libraries(grdemo PUBLIC GR::GKS)
  target_compile_options(grdemo PRIVATE ${COMPILER_OPTION_ERROR_IMPLICIT})
  set_target_properties(grdemo PROPERTIES C_STANDARD 90 C_EXTENSIONS OFF C_STANDARD_REQUIRED ON)

  add_subdirectory(lib/grm/test/public_api grm_test_public_api)
  add_subdirectory(lib/grm/test/internal_api grm_test_internal_api)
endif()

if(GR_INSTALL)
  install(FILES LICENSE.md DESTINATION ${CMAKE_INSTALL_DOCDIR}/)
  include(CMakePackageConfigHelpers)
  configure_package_config_file(
    "cmake/Config.cmake.in" "GRConfig.cmake" INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/GR"
  )
  write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/GRConfigVersion.cmake
    VERSION ${GR_VERSION}
    COMPATIBILITY SameMajorVersion
  )
  install(FILES "${CMAKE_CURRENT_BINARY_DIR}/GRConfig.cmake" "${CMAKE_CURRENT_BINARY_DIR}/GRConfigVersion.cmake"
          DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/GR"
  )
  install(
    TARGETS gks_shared gr_shared gr3_shared grm_shared
    EXPORT GRTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  )
  if(TARGET qt4gr)
    install(
      TARGETS qt4gr
      EXPORT GRTargets
      ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
      LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
      RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
  endif()
  if(TARGET qt5gr)
    install(
      TARGETS qt5gr
      EXPORT GRTargets
      ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
      LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
      RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
  endif()
  install(
    EXPORT GRTargets
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/GR"
    NAMESPACE GR::
  )
  foreach(PACKAGE gr gr3 gks grm)
    configure_file("${PACKAGE}.pc.in" "${CMAKE_CURRENT_BINARY_DIR}/${PACKAGE}.pc" @ONLY)
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${PACKAGE}.pc" DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig/")
  endforeach()
  install(
    TARGETS gks_static
            gr_static
            gr3_static
            grm_static
            cairoplugin
            glplugin
            gsplugin
            gtkplugin
            pgfplugin
            svgplugin
            videoplugin
            wmfplugin
            x11plugin
            zmqplugin
            qtplugin
            qt5plugin
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  )
  if(TARGET gr3platform)
    install(
      TARGETS gr3platform
      LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
      RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
  endif()
  if(TARGET qt4gr)
    install(
      TARGETS qt4gr
      ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
      LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
      RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
  endif()
  if(TARGET qt5gr)
    install(
      TARGETS qt5gr
      ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
      LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
      RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
  endif()
  if(TARGET quartzplugin)
    install(
      TARGETS quartzplugin
      ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
      LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
      RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
  endif()
  if(TARGET GKSTerm)
    install(TARGETS GKSTerm BUNDLE DESTINATION Applications)
  endif()
  if(TARGET gksqt)
    install(
      TARGETS gksqt
      RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
      BUNDLE DESTINATION Applications
    )
  endif()
  install(FILES lib/gr/gr.h lib/gks/gks.h lib/gr3/gr3.h lib/grm/grm.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/)
  install(FILES ${GRM_SUBDIR_HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/grm/)
  if(TARGET qt4gr OR TARGET qt5gr)
    install(FILES lib/gr/qtgr/grwidget.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/)
  endif()
  install(
    DIRECTORY lib/gks/fonts
    DESTINATION ${CMAKE_INSTALL_PREFIX}/
    USE_SOURCE_PERMISSIONS
  )
endif()

message(${GR_REPORT})
