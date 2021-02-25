#ifndef GRM_ARGS_INT_H_INCLUDED
#define GRM_ARGS_INT_H_INCLUDED

/* ######################### includes ############################################################################### */

#include <stdarg.h>
#include <stddef.h>
#include <sys/types.h>

#include "args.h"
#include "error_int.h"
#include "util_int.h"

#if defined(_MSC_VER)
#include <BaseTsd.h>
typedef SSIZE_T ssize_t;
#endif

/* ######################### internal interface ##################################################################### */

/* ========================= datatypes ============================================================================== */

/* ------------------------- argument ------------------------------------------------------------------------------- */

struct _arg_private_t
{
  unsigned int reference_count;
};


/* ------------------------- argument parsing ----------------------------------------------------------------------- */

struct _argparse_state_t
{
  va_list *vl;
  const void *in_buffer;
  int apply_padding;
  ptrdiff_t data_offset;
  void *save_buffer;
  char current_format;
  int next_is_array;
  size_t default_array_length;
  ssize_t next_array_length;
  int dataslot_count;
};
typedef struct _argparse_state_t argparse_state_t;

typedef void (*read_param_t)(argparse_state_t *);
typedef void *(*copy_value_t)(void *);
typedef void (*delete_value_t)(void *);


/* ------------------------- argument container --------------------------------------------------------------------- */

struct _args_node_t
{
  arg_t *arg;
  struct _args_node_t *next;
};

struct _grm_args_t
{
  args_node_t *kwargs_head;
  args_node_t *kwargs_tail;
  unsigned int count;
};

/* ------------------------- argument iterator ---------------------------------------------------------------------- */

struct _args_iterator_private_t
{
  const args_node_t *next_node;
  const args_node_t *end;
};


/* ------------------------- value iterator ------------------------------------------------------------------------- */

struct _args_value_iterator_private_t
{
  void *value_buffer;
  const char *value_format;
};


/* ========================= functions ============================================================================== */

/* ------------------------- argument parsing ----------------------------------------------------------------------- */

void *argparse_read_params(const char *format, const void *buffer, va_list *vl, int apply_padding, char **new_format);
void argparse_read_int(argparse_state_t *state);
void argparse_read_double(argparse_state_t *state);
void argparse_read_char(argparse_state_t *state);
void argparse_read_string(argparse_state_t *state);
void argparse_read_default_array_length(argparse_state_t *state);
void argparse_read_char_array(argparse_state_t *state, int store_array_length);
void argparse_init_static_variables(void);
size_t argparse_calculate_needed_buffer_size(const char *format, int apply_padding);
size_t argparse_calculate_needed_padding(void *buffer, char current_format);
void argparse_read_next_option(argparse_state_t *state, char **format);
const char *argparse_skip_option(const char *format);
char *argparse_convert_to_array(argparse_state_t *state);


/* ------------------------- argument container --------------------------------------------------------------------- */

arg_t *args_create_args(const char *key, const char *value_format, const void *buffer, va_list *vl, int apply_padding);
int args_validate_format_string(const char *format);
const char *args_skip_option(const char *format);
void args_copy_format_string_for_arg(char *dst, const char *format);
void args_copy_format_string_for_parsing(char *dst, const char *format);
int args_check_format_compatibility(const arg_t *arg, const char *compatible_format);
void args_decrease_arg_reference_count(args_node_t *args_node);


/* ------------------------- value copy ----------------------------------------------------------------------------- */

void *copy_value(char format, void *value_ptr);


/* ========================= methods ================================================================================ */

/* ------------------------- argument ------------------------------------------------------------------------------- */

args_value_iterator_t *arg_value_iter(const arg_t *arg);

error_t arg_increase_array(arg_t *arg, size_t increment);

int arg_first_value(const arg_t *arg, const char *first_value_format, void *first_value, unsigned int *array_length);
#define arg_first_value(arg, first_value_format, first_value, array_length) \
  arg_first_value(arg, first_value_format, (void *)first_value, array_length)
int arg_values(const arg_t *arg, const char *expected_format, ...);
int arg_values_vl(const arg_t *arg, const char *expected_format, va_list *vl);

/* ------------------------- argument container --------------------------------------------------------------------- */

void args_init(grm_args_t *args);
void args_finalize(grm_args_t *args);

grm_args_t *args_flatcopy(const grm_args_t *args) UNUSED;
grm_args_t *args_copy(const grm_args_t *copy_args);
grm_args_t *args_copy_extended(const grm_args_t *copy_args, const char **keys_copy_as_array, const char **ignore_keys);

error_t args_push_common(grm_args_t *args, const char *key, const char *value_format, const void *buffer, va_list *vl,
                         int apply_padding);
error_t args_push_vl(grm_args_t *args, const char *key, const char *value_format, va_list *vl);
error_t args_push_arg(grm_args_t *args, arg_t *arg);
error_t args_update_many(grm_args_t *args, const grm_args_t *update_args) UNUSED;
error_t args_merge(grm_args_t *args, const grm_args_t *merge_args, const char *const *merge_keys);
error_t args_setdefault_common(grm_args_t *args, const char *key, const char *value_format, const void *buffer,
                               va_list *vl, int apply_padding);
error_t args_setdefault(grm_args_t *args, const char *key, const char *value_format, ...);
error_t args_setdefault_buf(grm_args_t *args, const char *key, const char *value_format, const void *buffer,
                            int apply_padding) UNUSED;
error_t args_setdefault_vl(grm_args_t *args, const char *key, const char *value_format, va_list *vl);

void args_clear(grm_args_t *args, const char **exclude_keys);

error_t args_increase_array(grm_args_t *args, const char *key, size_t increment) UNUSED;

unsigned int args_count(const grm_args_t *args) UNUSED;

arg_t *args_at(const grm_args_t *args, const char *keyword);
int args_first_value(const grm_args_t *args, const char *keyword, const char *first_value_format, void *first_value,
                     unsigned int *array_length);
#define args_first_value(args, keyword, first_value_format, first_value, array_length) \
  args_first_value(args, keyword, first_value_format, (void *)first_value, array_length)
int args_values(const grm_args_t *args, const char *keyword, const char *expected_format, ...);

args_node_t *args_find_node(const grm_args_t *args, const char *keyword);
int args_find_previous_node(const grm_args_t *args, const char *keyword, args_node_t **previous_node);

args_iterator_t *args_iter(const grm_args_t *args);


/* ------------------------- argument iterator ---------------------------------------------------------------------- */

args_iterator_t *args_iterator_new(const args_node_t *begin, const args_node_t *end);
void args_iterator_init(args_iterator_t *args_iterator, const args_node_t *begin, const args_node_t *end);
void args_iterator_delete(args_iterator_t *args_iterator);
void args_iterator_finalize(args_iterator_t *args_iterator);
arg_t *args_iterator_next(args_iterator_t *args_iterator);


/* ------------------------- value iterator ------------------------------------------------------------------------- */

args_value_iterator_t *args_value_iterator_new(const arg_t *arg);
void args_value_iterator_init(args_value_iterator_t *args_value_iterator, const arg_t *arg);
void args_value_iterator_delete(args_value_iterator_t *args_value_iterator);
void args_value_iterator_finalize(args_value_iterator_t *args_value_iterator);

void *args_value_iterator_next(args_value_iterator_t *args_value_iterator);


#endif /* ifndef GRM_ARGS_INT_H_INCLUDED */
