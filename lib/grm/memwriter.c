#ifdef __unix__
#define _POSIX_C_SOURCE 200112L
#endif

/* ######################### includes ############################################################################### */

#include <stdarg.h>
#include <string.h>

#include "error_int.h"
#include "memwriter_int.h"


/* ######################### internal implementation ################################################################ */

/* ========================= methods ================================================================================ */

/* ------------------------- memwriter ------------------------------------------------------------------------------ */

memwriter_t *memwriter_new()
{
  memwriter_t *memwriter;

  memwriter = malloc(sizeof(memwriter_t));
  if (memwriter == NULL)
    {
      debug_print_malloc_error();
      return NULL;
    }
  memwriter->buf = malloc(MEMWRITER_INITIAL_SIZE);
  if (memwriter->buf == NULL)
    {
      free(memwriter);
      debug_print_malloc_error();
      return NULL;
    }
  memwriter->size = 0;
  memwriter->capacity = MEMWRITER_INITIAL_SIZE;

  return memwriter;
}

void memwriter_delete(memwriter_t *memwriter)
{
  if (memwriter != NULL)
    {
      free(memwriter->buf);
      free(memwriter);
    }
}

void memwriter_clear(memwriter_t *memwriter)
{
  memwriter->size = 0;
  *memwriter->buf = '\0';
}

error_t memwriter_replace(memwriter_t *memwriter, int index, int count, const char *replacement_str)
{
  int replacement_str_len = (replacement_str != NULL) ? strlen(replacement_str) : 0;
  error_t error = NO_ERROR;

  if ((replacement_str_len > count) &&
      (error = memwriter_ensure_buf(memwriter, replacement_str_len - count)) != NO_ERROR)
    {
      return error;
    }
  if (count != replacement_str_len)
    {
      memmove(memwriter->buf + index + replacement_str_len, memwriter->buf + index + count,
              memwriter->size - (index + count));
    }
  if (replacement_str != NULL)
    {
      memcpy(memwriter->buf + index, replacement_str, replacement_str_len);
    }
  memwriter->size += replacement_str_len - count;

  return error;
}

error_t memwriter_erase(memwriter_t *memwriter, int index, int count)
{
  return memwriter_replace(memwriter, index, count, NULL);
}

error_t memwriter_insert(memwriter_t *memwriter, int index, const char *str)
{
  return memwriter_replace(memwriter, index, 0, str);
}

error_t memwriter_enlarge_buf(memwriter_t *memwriter, size_t size_increment)
{
  void *new_buf;

  if (size_increment == 0)
    {
      if (memwriter->capacity >= MEMWRITER_EXPONENTIAL_INCREASE_UNTIL)
        {
          size_increment = MEMWRITER_LINEAR_INCREMENT_SIZE;
        }
      else
        {
          size_increment = memwriter->capacity;
        }
    }
  else
    {
      /* round up to the next `MEMWRITER_LINEAR_INCREMENT_SIZE` step */
      if (memwriter->capacity >= MEMWRITER_EXPONENTIAL_INCREASE_UNTIL)
        {
          size_increment =
              ((size_increment - 1) / MEMWRITER_LINEAR_INCREMENT_SIZE + 1) * MEMWRITER_LINEAR_INCREMENT_SIZE;
        }
      else
        {
          size_increment = next_or_equal_power2(memwriter->capacity + size_increment) - memwriter->capacity;
        }
    }
  new_buf = realloc(memwriter->buf, memwriter->capacity + size_increment);
  if (new_buf == NULL)
    {
      debug_print_malloc_error();
      return ERROR_MALLOC;
    }
  memwriter->buf = new_buf;
  memwriter->capacity += size_increment;

  return NO_ERROR;
}

error_t memwriter_ensure_buf(memwriter_t *memwriter, size_t needed_additional_size)
{
  if (memwriter->size + needed_additional_size > memwriter->capacity)
    {
      return memwriter_enlarge_buf(memwriter, memwriter->size + needed_additional_size - memwriter->capacity);
    }

  return NO_ERROR;
}

error_t memwriter_printf(memwriter_t *memwriter, const char *format, ...)
{
  va_list vl;
  error_t error = NO_ERROR;

  while (1)
    {
      int chars_needed;
      va_start(vl, format);
      chars_needed = vsnprintf(&memwriter->buf[memwriter->size], memwriter->capacity - memwriter->size, format, vl);
      va_end(vl);
      if (chars_needed < 0)
        {
          return ERROR_INTERNAL;
        }
      /* we need one more char because `vsnprintf` does exclude the trailing '\0' character in its calculations */
      if ((size_t)chars_needed < (memwriter->capacity - memwriter->size))
        {
          memwriter->size += chars_needed;
          break;
        }
      if ((error = memwriter_ensure_buf(memwriter, chars_needed + 1)) != NO_ERROR)
        {
          break;
        }
    }

  return error;
}

error_t memwriter_puts(memwriter_t *memwriter, const char *s)
{
  return memwriter_printf(memwriter, "%s", s);
}

error_t memwriter_puts_with_len(memwriter_t *memwriter, char *s, size_t length)
{
  error_t error = NO_ERROR;

  while (length > 0)
    {
      if ((error = memwriter_putc(memwriter, *(s++))) != NO_ERROR)
        {
          return error;
        }
      --length;
    }

  return error;
}

error_t memwriter_putc(memwriter_t *memwriter, char c)
{
  return memwriter_printf(memwriter, "%c", c);
}

error_t memwriter_memcpy(memwriter_t *memwriter, const void *source, size_t num)
{
  error_t error = NO_ERROR;

  memwriter_ensure_buf(memwriter, num);

  memcpy(&memwriter->buf[memwriter->size], source, num);

  memwriter->size += num;

  return error;
}

error_t memwriter_memcpy_rev_chunks(memwriter_t *memwriter, const void *source, size_t num, int chunk_size)
{
  error_t error = NO_ERROR;

  memwriter_ensure_buf(memwriter, num);

  char *d = &memwriter->buf[memwriter->size];
  const char *s = source;
  int i;
  int j;

  for (i = 0; i < num; i += chunk_size)
    {
      for (j = 0; j < chunk_size; j++)
        {
          d[i + chunk_size - j - 1] = s[i + j];
        }
    }

  memwriter->size += num;

  return error;
}

char *memwriter_buf(const memwriter_t *memwriter)
{
  return memwriter->buf;
}

size_t memwriter_size(const memwriter_t *memwriter)
{
  return memwriter->size;
}
