#ifndef GRM_UTIL_H_INCLUDED
#define GRM_UTIL_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

/* ######################### includes ############################################################################### */

#if !defined(NDEBUG) && defined(EMSCRIPTEN)
#include <stdio.h>
#endif


/* ######################### public interface ####################################################################### */

/* ========================= macros ================================================================================= */

/* ------------------------- util ----------------------------------------------------------------------------------- */

#ifdef GR_STATIC_LIB
#define EXPORT
#else
#if defined _WIN32 || defined __CYGWIN__ || defined __MINGW32__
#ifdef BUILDING_DLL
#ifdef __GNUC__
#define EXPORT __attribute__((dllexport))
#else
#define EXPORT __declspec(dllexport)
#endif
#else
#ifdef __GNUC__
#define EXPORT __attribute__((dllimport))
#else
#define EXPORT __declspec(dllimport)
#endif
#endif
#else
#if __GNUC__ >= 4
#define EXPORT __attribute__((visibility("default")))
#else
#define EXPORT
#endif
#endif
#endif


/* ========================= functions ============================================================================== */

/* ------------------------- util ----------------------------------------------------------------------------------- */

#if !defined(NDEBUG) && defined(EMSCRIPTEN)
EXPORT FILE *grm_get_stdout(void);
#endif


#ifdef __cplusplus
}
#endif
#endif /* ifndef GRM_UTIL_H_INCLUDED */
