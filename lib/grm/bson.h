#ifndef GRM_BSON_INT_H_INCLUDED
#define GRM_BSON_INT_H_INCLUDED

/* ######################### includes ############################################################################### */

#include <stdarg.h>

#include "args.h"
#include "error_int.h"
#include "memwriter_int.h"
#include "json_int.h"


/* ######################### internal interface ##################################################################### */

/* ========================= global varibales ======================================================================= */

/* ========================= macros ================================================================================= */

/* ========================= datatypes ============================================================================== */

/* ------------------------- bson deserializer ---------------------------------------------------------------------- */

typedef struct
{
  int length;
  int num_bytes_read_before;
  int num_elements;
} frombson_array_infos_t;

typedef struct
{
  int length;
  int num_bytes_read_before;
} frombson_object_infos_t;

typedef struct
{
  grm_args_t *args;
  const char *cur_byte;
  int num_read_bytes;
  char cur_value_format;
  void *cur_value_buf;
  const char *cur_key;
  frombson_array_infos_t *array_infos;
  frombson_object_infos_t *object_infos;
} frombson_state_t;


/* ------------------------- bson serializer ------------------------------------------------------------------------ */

typedef error_t (*tobson_post_processing_callback_t)(memwriter_t *, unsigned int, const char *);

typedef struct
{
  int apply_padding;
  size_t array_length;
  int read_length_from_string;
  const void *data_ptr;
  va_list *vl;
  int data_offset;
  int wrote_output;
  int add_data;
  tojson_serialization_result_t serial_result;
  unsigned int struct_nested_level;
} tobson_shared_state_t;

typedef struct
{
  memwriter_t *memwriter;
  char *data_type_ptr;
  char current_data_type;
  char *additional_type_info;
  int is_type_info_incomplete;
  int add_data_without_separator;
  tobson_shared_state_t *shared;
} tobson_state_t;

typedef struct
{
  tojson_serialization_result_t serial_result;
  unsigned int struct_nested_level;
} tobson_permanent_state_t;

/* ========================= small helper functions ================================================================= */

void revmemcpy(void *dest, const void *src, size_t len);

void memcpy_rev_chunks(void *dest, const void *src, size_t len, size_t chunk_size);

char byte_to_type(const char *byte);

void int_to_bytes(int i, char **bytes);

void bytes_to_int(int *i, const char *bytes);

void double_to_bytes(double d, char **bytes);

void bytes_to_double(double *d, const char *bytes);

/* ------------------------- bson deserializer ---------------------------------------------------------------------- */

error_t frombson_read(grm_args_t *args, const char *bson_bytes);
error_t frombson_read_value_format(frombson_state_t *state, char *value_format);
error_t frombson_read_key(frombson_state_t *state, const char **key);
error_t frombson_skip_key(frombson_state_t *state);
error_t frombson_read_length(frombson_state_t *state, int *length);

error_t frombson_read_double_value(frombson_state_t *state, double *d);
error_t frombson_read_int_value(frombson_state_t *state, int *i);
error_t frombson_read_string_value(frombson_state_t *state, const char **s);
error_t frombson_read_bool_value(frombson_state_t *state, int *b);
error_t frombson_read_object(frombson_state_t *state);

error_t frombson_parse_double(frombson_state_t *state);
error_t frombson_parse_int(frombson_state_t *state);
error_t frombson_parse_bool(frombson_state_t *state);
error_t frombson_parse_array(frombson_state_t *state);
error_t frombson_parse_object(frombson_state_t *state);

error_t frombson_read_int_array(frombson_state_t *state);
error_t frombson_read_double_array(frombson_state_t *state);
error_t frombson_read_string_array(frombson_state_t *state);
error_t frombson_read_bool_array(frombson_state_t *state);

void frombson_init_static_variables(void);

/* ------------------------- bson serializer ------------------------------------------------------------------------ */

error_t tobson_int_value(memwriter_t *memwriter, int value);
error_t tobson_double_value(memwriter_t *memwriter, double value);
error_t tobson_char_value(memwriter_t *memwriter, char value);
error_t tobson_string_value(memwriter_t *memwriter, char *value);
error_t tobson_bool_value(memwriter_t *memwriter, int value);
error_t tobson_args_value(memwriter_t *memwriter, grm_args_t *args);

error_t tobson_int(tobson_state_t *state);
error_t tobson_double(tobson_state_t *state);
error_t tobson_char(tobson_state_t *state);
error_t tobson_string(tobson_state_t *state);
error_t tobson_bool(tobson_state_t *state);
error_t tobson_args(tobson_state_t *state);

error_t tobson_int_array(tobson_state_t *state);
error_t tobson_double_array(tobson_state_t *state);
error_t tobson_optimized_array(tobson_state_t *state);
error_t tobson_char_array(tobson_state_t *state);
error_t tobson_string_array(tobson_state_t *state);
error_t tobson_bool_array(tobson_state_t *state);
error_t tobson_args_array(tobson_state_t *state);

error_t tobson_read_array_length(tobson_state_t *state);
error_t tobson_skip_bytes(tobson_state_t *state);
error_t tobson_object(tobson_state_t *state);
error_t tobson_close_object(tobson_state_t *state);

int tobson_get_member_count(const char *data_desc);
int tobson_is_bson_array_needed(const char *data_desc);
void tobson_read_datatype(tobson_state_t *state);
error_t tobson_unzip_membernames_and_datatypes(char *mixed_ptr, char ***member_name_ptr, char ***data_type_ptr);
error_t tobson_escape_special_chars(char **escaped_string, const char *unescaped_string, unsigned int *length);
error_t tobson_serialize(memwriter_t *memwriter, char *data_desc, const void *data, va_list *vl, int apply_padding,
                         int add_data, int add_data_without_separator, unsigned int *struct_nested_level,
                         tojson_serialization_result_t *serial_result, tobson_shared_state_t *shared_state);
void tobson_init_static_variables(void);
error_t tobson_init_variables(int *add_data, int *add_data_without_separator, char **_data_desc, const char *data_desc);
error_t tobson_write_vl(memwriter_t *memwriter, const char *data_desc, va_list *vl);
error_t tobson_write_buf(memwriter_t *memwriter, const char *data_desc, const void *buffer, int apply_padding);
error_t tobson_write_arg(memwriter_t *memwriter, const arg_t *arg);
error_t tobson_write_args(memwriter_t *memwriter, const grm_args_t *args);
int tobson_is_complete(void);
int tobson_struct_nested_level(void);


#endif /* ifndef GRM_BSON_INT_H_INCLUDED */
