#ifndef GRM_DUMP_H_INCLUDED
#define GRM_DUMP_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

/* ######################### includes ############################################################################### */

#include <stdio.h>

#include "args.h"
#include "util.h"


/* ######################### public interface ####################################################################### */

/* ========================= functions ============================================================================== */

/* ------------------------- dump ----------------------------------------------------------------------------------- */

#ifndef NDEBUG
EXPORT void grm_dump(const grm_args_t *args, FILE *f);
EXPORT void grm_dump_json(const grm_args_t *args, FILE *f);
EXPORT void grm_dump_bson(const grm_args_t *args, FILE *f);
EXPORT void grm_dump_bson_and_parse(const grm_args_t *args, FILE *f);
EXPORT char *grm_dump_json_str(void);
#else
#define grm_dump
#define grm_dump_json
#define grm_dump_json_str
#endif

#ifdef __cplusplus
}
#endif
#endif /* ifndef GRM_DUMP_H_INCLUDED */
