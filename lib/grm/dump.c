#ifdef __unix__
#define _POSIX_C_SOURCE 200112L
#endif

/* ######################### includes ############################################################################### */

#include <limits.h>
#include <string.h>
#ifndef _WIN32
#include <sys/ioctl.h>
#endif

#include "dump_int.h"
#include "json_int.h"
#include "bson.h"
#include "memwriter_int.h"
#include "plot_int.h"


/* ######################### public implementation ################################################################## */

/* ========================= functions ============================================================================== */

/* ------------------------- dump ----------------------------------------------------------------------------------- */

#ifndef NDEBUG
void grm_dump(const grm_args_t *args, FILE *f)
{
#define BUFFER_LEN 200
#define DEFAULT_ARRAY_PRINT_ELEMENTS_COUNT 10
#define DARK_BACKGROUND_ENV_KEY "GRM_DARK_BACKGROUND"
#define ARRAY_PRINT_TRUNCATION_ENV_KEY "GRM_ARRAY_PRINT_TRUNCATION"
  args_iterator_t *it;
  args_value_iterator_t *value_it;
  arg_t *arg;
  unsigned int i;
  char buffer[BUFFER_LEN];
  int count_characters;
  static int recursion_level = -1;
  int columns, cursor_xpos = 0;
  int use_color_codes;
  int has_dark_bg;
  struct grm_dump_color_codes_t
  {
    unsigned char k, i, d, c, s;
  } color_codes;
  unsigned int array_print_elements_count = DEFAULT_ARRAY_PRINT_ELEMENTS_COUNT;
#if defined(_WIN32) || defined(__EMSCRIPTEN__)
  columns = INT_MAX;
  use_color_codes = 0;
  has_dark_bg = 0;
#else
  struct winsize w;
  use_color_codes = isatty(fileno(f));
  ioctl(0, TIOCGWINSZ, &w);
  columns = w.ws_col;
  if (getenv(DARK_BACKGROUND_ENV_KEY) != NULL &&
      str_equals_any(getenv(DARK_BACKGROUND_ENV_KEY), 5, "1", "yes", "YES", "on", "ON"))
    {
      has_dark_bg = 1;
      color_codes.k = 122;
      color_codes.i = 81;
      color_codes.d = 215;
      color_codes.c = 228;
      color_codes.s = 155;
    }
  else
    {
      has_dark_bg = 0;
      color_codes.k = 18;
      color_codes.i = 25;
      color_codes.d = 88;
      color_codes.c = 55;
      color_codes.s = 22;
    }
#endif
  if (getenv(ARRAY_PRINT_TRUNCATION_ENV_KEY) != NULL)
    {
      if (str_equals_any(getenv(ARRAY_PRINT_TRUNCATION_ENV_KEY), 8, "", "0", "inf", "INF", "unlimited", "UNLIMITED",
                         "off", "OFF"))
        {
          array_print_elements_count = UINT_MAX;
        }
      else
        {
          str_to_uint(getenv(ARRAY_PRINT_TRUNCATION_ENV_KEY), &array_print_elements_count);
        }
    }

#define INDENT 2

#define print_indent                                                                                                   \
  do                                                                                                                   \
    {                                                                                                                  \
      if (use_color_codes)                                                                                             \
        {                                                                                                              \
          int i;                                                                                                       \
          for (i = 0; i < recursion_level; ++i)                                                                        \
            {                                                                                                          \
              fprintf(f, "\033[48;5;%dm%*s\033[0m", has_dark_bg ? (235 + (5 * i) % 25) : (255 - (5 * i) % 25), INDENT, \
                      "");                                                                                             \
            }                                                                                                          \
        }                                                                                                              \
      else                                                                                                             \
        {                                                                                                              \
          fprintf(f, "%*s", INDENT *recursion_level, "");                                                              \
        }                                                                                                              \
    }                                                                                                                  \
  while (0)

#define print_key                                                          \
  do                                                                       \
    {                                                                      \
      print_indent;                                                        \
      if (use_color_codes)                                                 \
        {                                                                  \
          fprintf(f, "\033[38;5;%dm%s\033[0m: ", color_codes.k, arg->key); \
        }                                                                  \
      else                                                                 \
        {                                                                  \
          fprintf(f, "%s: ", arg->key);                                    \
        }                                                                  \
    }                                                                      \
  while (0)

#define print_value(value_type, format_string, color_code)                                                       \
  do                                                                                                             \
    {                                                                                                            \
      if (use_color_codes)                                                                                       \
        {                                                                                                        \
          fprintf(f, "\033[38;5;%dm" format_string "\033[0m", color_code, *((value_type *)value_it->value_ptr)); \
        }                                                                                                        \
      else                                                                                                       \
        {                                                                                                        \
          fprintf(f, format_string, *((value_type *)value_it->value_ptr));                                       \
        }                                                                                                        \
    }                                                                                                            \
  while (0)

#define print_values(value_type, format_string, color_code)                                                        \
  do                                                                                                               \
    {                                                                                                              \
      int print_last_element = 0;                                                                                  \
      fputc('[', f);                                                                                               \
      cursor_xpos += strlen(arg->key) + 3;                                                                         \
      for (i = 0; i < min(value_it->array_length, array_print_elements_count); i++)                                \
        {                                                                                                          \
          if (print_last_element)                                                                                  \
            {                                                                                                      \
              i = value_it->array_length - 1;                                                                      \
            }                                                                                                      \
          if (array_print_elements_count >= value_it->array_length || i != array_print_elements_count - 2)         \
            {                                                                                                      \
              if (use_color_codes)                                                                                 \
                {                                                                                                  \
                  count_characters =                                                                               \
                      snprintf(buffer, BUFFER_LEN,                                                                 \
                               "\033[38;5;%dm" format_string "\033[0m"                                             \
                               "%s",                                                                               \
                               color_code, (*((value_type **)value_it->value_ptr))[i],                             \
                               (i < min(value_it->array_length, array_print_elements_count) - 1) ? ", " : "]");    \
                  count_characters -= (color_code >= 100) ? 15 : ((color_code >= 10) ? 14 : 13);                   \
                }                                                                                                  \
              else                                                                                                 \
                {                                                                                                  \
                  count_characters =                                                                               \
                      snprintf(buffer, BUFFER_LEN, format_string "%s", (*((value_type **)value_it->value_ptr))[i], \
                               (i < min(value_it->array_length, array_print_elements_count) - 1) ? ", " : "]");    \
                }                                                                                                  \
            }                                                                                                      \
          else                                                                                                     \
            {                                                                                                      \
              count_characters = snprintf(buffer, BUFFER_LEN, "..., ");                                            \
              print_last_element = 1;                                                                              \
            }                                                                                                      \
          if (cursor_xpos + count_characters > columns)                                                            \
            {                                                                                                      \
              fputc('\n', f);                                                                                      \
              print_indent;                                                                                        \
              cursor_xpos = INDENT * recursion_level + fprintf(f, "%*s", (int)strlen(arg->key) + 3, "") - 1;       \
            }                                                                                                      \
          fputs(buffer, f);                                                                                        \
          cursor_xpos += count_characters;                                                                         \
        }                                                                                                          \
      if (value_it->array_length == 0)                                                                             \
        {                                                                                                          \
          fputc(']', f);                                                                                           \
        }                                                                                                          \
    }                                                                                                              \
  while (0)

#define print_type(value_type, format_string, color_code)      \
  do                                                           \
    {                                                          \
      if (value_it->is_array)                                  \
        {                                                      \
          print_key;                                           \
          print_values(value_type, format_string, color_code); \
        }                                                      \
      else                                                     \
        {                                                      \
          print_key;                                           \
          print_value(value_type, format_string, color_code);  \
        }                                                      \
      fputc('\n', f);                                          \
    }                                                          \
  while (0)

  ++recursion_level;

  it = args_iter(args);
  while ((arg = it->next(it)) != NULL)
    {
      if (*arg->value_format)
        {
          value_it = arg_value_iter(arg);
          while (value_it->next(value_it) != NULL)
            {
              cursor_xpos = INDENT * recursion_level;
              switch (value_it->format)
                {
                case 'i':
                  print_type(int, "% d", color_codes.i);
                  break;
                case 'd':
                  print_type(double, "% lf", color_codes.d);
                  break;
                case 'c':
                  print_type(char, "'%c'", color_codes.c);
                  break;
                case 's':
                  print_type(char *, "\"%s\"", color_codes.s);
                  break;
                case 'a':
                  if (value_it->is_array)
                    {
                      print_key;
                      fprintf(f, "[\n");
                      for (i = 0; i < value_it->array_length; i++)
                        {
                          grm_dump((*((grm_args_t ***)value_it->value_ptr))[i], f);
                          if (i < value_it->array_length - 1)
                            {
                              print_indent;
                              fprintf(f, ",\n");
                            }
                        }
                      print_indent;
                      fprintf(f, "]\n");
                    }
                  else
                    {
                      print_key;
                      fprintf(f, "\n");
                      grm_dump(*((grm_args_t **)value_it->value_ptr), f);
                      fprintf(f, "\n");
                    }
                  break;
                default:
                  break;
                }
            }
          args_value_iterator_delete(value_it);
        }
      else
        {
          print_key;
          fprintf(f, "(none)\n");
        }
    }
  args_iterator_delete(it);

  --recursion_level;

#undef BUFFER_LEN
#undef DEFAULT_ARRAY_PRINT_ELEMENTS_COUNT
#undef DARK_BACKGROUND_ENV_KEY
#undef ARRAY_PRINT_TRUNCATION_ENV_KEY
#undef INDENT
#undef print_indent
#undef print_key
#undef print_type
}

void grm_dump_json(const grm_args_t *args, FILE *f)
{
  static memwriter_t *memwriter = NULL;

  if (memwriter == NULL)
    {
      memwriter = memwriter_new();
    }
  tojson_write_args(memwriter, args);
  if (tojson_is_complete())
    {
      memwriter_putc(memwriter, '\0');
      fprintf(f, "%s\n", memwriter_buf(memwriter));
      memwriter_delete(memwriter);
      memwriter = NULL;
    }
}

void grm_dump_bson(const grm_args_t *args, FILE *f)
{
  static memwriter_t *memwriter = NULL;
  char *buf;
  int length;

  if (memwriter == NULL)
    {
      memwriter = memwriter_new();
    }
  tobson_write_args(memwriter, args);
  if (tobson_is_complete())
    {
      memwriter_putc(memwriter, '\0');

      buf = memwriter_buf(memwriter);

      bytes_to_int(&length, buf);

      while (length > 0)
        {
          fprintf(f, "\\x%x ", *(buf));
          length--;
          ++buf;
        }
      memwriter_delete(memwriter);
      memwriter = NULL;
    }
}

void grm_dump_bson_and_parse(const grm_args_t *args, FILE *f)
{
  static memwriter_t *memwriter = NULL;
  char *buf;
  int length;
  grm_args_t *new_args = grm_args_new();

  if (memwriter == NULL)
    {
      memwriter = memwriter_new();
    }
  tobson_write_args(memwriter, args);
  if (tobson_is_complete())
    {
      memwriter_putc(memwriter, '\0');

      buf = memwriter_buf(memwriter);

      bytes_to_int(&length, buf);

      while (length > 0)
        {
          fprintf(f, "\\x%x ", (unsigned char)*(buf));
          length--;
          ++buf;
        }
      fprintf(f, "\n");

      buf = memwriter_buf(memwriter);
      frombson_read(new_args, buf);

      grm_dump(new_args, f);
      grm_args_delete(new_args);
      memwriter_delete(memwriter);
      memwriter = NULL;
    }
}

char *grm_dump_json_str(void)
{
  static memwriter_t *memwriter = NULL;
  char *result;

  if (memwriter == NULL)
    {
      memwriter = memwriter_new();
    }
  /* tojson_write_args(memwriter, global_root_args); */
  tojson_write_args(memwriter, active_plot_args);
  if (tojson_is_complete())
    {
      memwriter_putc(memwriter, '\0');
      result = malloc(memwriter_size(memwriter) + 1);
      strcpy(result, memwriter_buf(memwriter));
      memwriter_delete(memwriter);
      memwriter = NULL;
      return result;
    }
  return "";
}

#endif
