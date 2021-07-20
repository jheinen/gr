#ifndef GRM_MEMWRITER_INT_H_INCLUDED
#define GRM_MEMWRITER_INT_H_INCLUDED

/* ######################### includes ############################################################################### */

#include <stdlib.h>

#include "error_int.h"
#include "util_int.h"

/* ######################### internal interface ##################################################################### */

/* ========================= macros ================================================================================= */

/* ------------------------- memwriter ------------------------------------------------------------------------------ */

#define MEMWRITER_INITIAL_SIZE 32768
#define MEMWRITER_EXPONENTIAL_INCREASE_UNTIL 268435456
#define MEMWRITER_LINEAR_INCREMENT_SIZE 67108864

#define ETB '\027'


/* ========================= datatypes ============================================================================== */

/* ------------------------- memwriter ------------------------------------------------------------------------------ */

struct _memwriter_t
{
  char *buf;
  size_t size;
  size_t capacity;
};
typedef struct _memwriter_t memwriter_t;

/* ========================= methods ================================================================================ */

/* ------------------------- memwriter ------------------------------------------------------------------------------ */

memwriter_t *memwriter_new(void);
void memwriter_delete(memwriter_t *memwriter);
void memwriter_clear(memwriter_t *memwriter);
error_t memwriter_replace(memwriter_t *memwriter, int index, int count, const char *replacement_str);
error_t memwriter_erase(memwriter_t *memwriter, int index, int count);
error_t memwriter_insert(memwriter_t *memwriter, int index, const char *str) UNUSED;
error_t memwriter_enlarge_buf(memwriter_t *memwriter, size_t size_increment);
error_t memwriter_ensure_buf(memwriter_t *memwriter, size_t needed_additional_size);
error_t memwriter_printf(memwriter_t *memwriter, const char *format, ...);
error_t memwriter_puts(memwriter_t *memwriter, const char *s);
error_t memwriter_puts_with_len(memwriter_t *memwriter, char *s, size_t length);
error_t memwriter_memcpy(memwriter_t *memwriter, const void *source, size_t num);
error_t memwriter_memcpy_rev_chunks(memwriter_t *memwriter, const void *source, size_t num, int chunk_size);
error_t memwriter_putc(memwriter_t *memwriter, char c);
char *memwriter_buf(const memwriter_t *memwriter);
size_t memwriter_size(const memwriter_t *memwriter);


#endif /* ifndef GRM_MEMWRITER_INT_H_INCLUDED */
