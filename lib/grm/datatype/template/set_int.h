/* Do not use an include guard since the following macros may be deleted with `undef` between multiple includes */

/* ######################### includes ############################################################################### */

#include <stdlib.h>
#include <sys/types.h>

#if defined(_MSC_VER)
#include <BaseTsd.h>
typedef SSIZE_T ssize_t;
#endif

#include "error_int.h"
#include "logging_int.h"
#include "util_int.h"


/* ######################### interface ############################################################################## */

/* ========================= datatypes ============================================================================== */

/* ------------------------- generic set ---------------------------------------------------------------------------- */

#undef DECLARE_SET_TYPE
#define DECLARE_SET_TYPE(prefix, entry_type)           \
  typedef entry_type prefix##_set_entry_t;             \
  typedef const entry_type prefix##_set_const_entry_t; \
                                                       \
  typedef struct                                       \
  {                                                    \
    prefix##_set_entry_t *set;                         \
    unsigned char *used;                               \
    size_t capacity;                                   \
    size_t size;                                       \
  } prefix##_set_t;


/* ========================= methods ================================================================================ */

#undef DECLARE_SET_METHODS
#define DECLARE_SET_METHODS(prefix)                                                                           \
  prefix##_set_t *prefix##_set_new(size_t capacity) MAYBE_UNUSED;                                             \
  prefix##_set_t *prefix##_set_new_with_data(size_t count, prefix##_set_const_entry_t *entries) MAYBE_UNUSED; \
  prefix##_set_t *prefix##_set_copy(const prefix##_set_t *set) MAYBE_UNUSED;                                  \
  void prefix##_set_delete(prefix##_set_t *set);                                                              \
  int prefix##_set_add(prefix##_set_t *set, prefix##_set_const_entry_t entry) MAYBE_UNUSED;                   \
  int prefix##_set_find(const prefix##_set_t *set, prefix##_set_const_entry_t entry,                          \
                        prefix##_set_entry_t *saved_entry) MAYBE_UNUSED;                                      \
  int prefix##_set_contains(const prefix##_set_t *set, prefix##_set_const_entry_t entry) MAYBE_UNUSED;        \
  ssize_t prefix##_set_index(const prefix##_set_t *set, prefix##_set_const_entry_t entry) MAYBE_UNUSED;       \
                                                                                                              \
  int prefix##_set_entry_copy(prefix##_set_entry_t *copy, prefix##_set_const_entry_t entry);                  \
  void prefix##_set_entry_delete(prefix##_set_entry_t entry);                                                 \
  size_t prefix##_set_entry_hash(prefix##_set_const_entry_t entry);                                           \
  int prefix##_set_entry_equals(prefix##_set_const_entry_t entry1, prefix##_set_const_entry_t entry2);


/* ######################### implementation ######################################################################### */

/* ========================= methods ================================================================================ */

/* ------------------------- generic set ---------------------------------------------------------------------------- */

#undef DEFINE_SET_METHODS
#define DEFINE_SET_METHODS(prefix)                                                                                \
  prefix##_set_t *prefix##_set_new(size_t capacity)                                                               \
  {                                                                                                               \
    prefix##_set_t *set = NULL;                                                                                   \
    size_t power2_capacity = 1;                                                                                   \
                                                                                                                  \
    /* Use the power of 2 which is equal or greater than 2*capacity as the set capacity */                        \
    power2_capacity = next_or_equal_power2(2 * capacity);                                                         \
    set = malloc(sizeof(prefix##_set_t));                                                                         \
    if (set == NULL)                                                                                              \
      {                                                                                                           \
        debug_print_malloc_error();                                                                               \
        goto error_cleanup;                                                                                       \
      }                                                                                                           \
    set->set = NULL;                                                                                              \
    set->used = NULL;                                                                                             \
    set->set = malloc(power2_capacity * sizeof(prefix##_set_entry_t));                                            \
    if (set->set == NULL)                                                                                         \
      {                                                                                                           \
        debug_print_malloc_error();                                                                               \
        goto error_cleanup;                                                                                       \
      }                                                                                                           \
    set->used = calloc(power2_capacity, sizeof(unsigned char));                                                   \
    if (set->used == NULL)                                                                                        \
      {                                                                                                           \
        debug_print_malloc_error();                                                                               \
        goto error_cleanup;                                                                                       \
      }                                                                                                           \
    set->capacity = power2_capacity;                                                                              \
    set->size = 0;                                                                                                \
                                                                                                                  \
    logger((stderr, "Created a new set with capacity: %lu\n", set->capacity));                                    \
                                                                                                                  \
    return set;                                                                                                   \
                                                                                                                  \
  error_cleanup:                                                                                                  \
    if (set != NULL)                                                                                              \
      {                                                                                                           \
        if (set->set != NULL)                                                                                     \
          {                                                                                                       \
            free(set->set);                                                                                       \
          }                                                                                                       \
        if (set->used != NULL)                                                                                    \
          {                                                                                                       \
            free(set->used);                                                                                      \
          }                                                                                                       \
        free(set);                                                                                                \
      }                                                                                                           \
                                                                                                                  \
    return NULL;                                                                                                  \
  }                                                                                                               \
                                                                                                                  \
  prefix##_set_t *prefix##_set_new_with_data(size_t count, prefix##_set_const_entry_t *entries)                   \
  {                                                                                                               \
    prefix##_set_t *set;                                                                                          \
    size_t i;                                                                                                     \
                                                                                                                  \
    set = prefix##_set_new(count);                                                                                \
    if (set == NULL)                                                                                              \
      {                                                                                                           \
        return NULL;                                                                                              \
      }                                                                                                           \
    for (i = 0; i < count; ++i)                                                                                   \
      {                                                                                                           \
        if (!prefix##_set_add(set, entries[i]))                                                                   \
          {                                                                                                       \
            prefix##_set_delete(set);                                                                             \
            return NULL;                                                                                          \
          }                                                                                                       \
      }                                                                                                           \
                                                                                                                  \
    return set;                                                                                                   \
  }                                                                                                               \
                                                                                                                  \
  prefix##_set_t *prefix##_set_copy(const prefix##_set_t *set)                                                    \
  {                                                                                                               \
    prefix##_set_t *copy;                                                                                         \
    size_t i;                                                                                                     \
                                                                                                                  \
    copy = prefix##_set_new(set->size);                                                                           \
    if (copy == NULL)                                                                                             \
      {                                                                                                           \
        return NULL;                                                                                              \
      }                                                                                                           \
    for (i = 0; i < set->capacity; ++i)                                                                           \
      {                                                                                                           \
        if (set->used[i] && !prefix##_set_add(copy, set->set[i]))                                                 \
          {                                                                                                       \
            prefix##_set_delete(copy);                                                                            \
            return NULL;                                                                                          \
          }                                                                                                       \
      }                                                                                                           \
                                                                                                                  \
    return copy;                                                                                                  \
  }                                                                                                               \
                                                                                                                  \
  void prefix##_set_delete(prefix##_set_t *set)                                                                   \
  {                                                                                                               \
    size_t i;                                                                                                     \
    for (i = 0; i < set->capacity; ++i)                                                                           \
      {                                                                                                           \
        if (set->used[i])                                                                                         \
          {                                                                                                       \
            prefix##_set_entry_delete(set->set[i]);                                                               \
          }                                                                                                       \
      }                                                                                                           \
    free(set->set);                                                                                               \
    free(set->used);                                                                                              \
    free(set);                                                                                                    \
  }                                                                                                               \
                                                                                                                  \
  int prefix##_set_add(prefix##_set_t *set, prefix##_set_const_entry_t entry)                                     \
  {                                                                                                               \
    ssize_t index;                                                                                                \
                                                                                                                  \
    index = prefix##_set_index(set, entry);                                                                       \
    if (index < 0)                                                                                                \
      {                                                                                                           \
        return 0;                                                                                                 \
      }                                                                                                           \
    if (set->used[index])                                                                                         \
      {                                                                                                           \
        prefix##_set_entry_delete(set->set[index]);                                                               \
        --(set->size);                                                                                            \
        set->used[index] = 0;                                                                                     \
      }                                                                                                           \
    if (!prefix##_set_entry_copy(set->set + index, entry))                                                        \
      {                                                                                                           \
        return 0;                                                                                                 \
      }                                                                                                           \
    ++(set->size);                                                                                                \
    set->used[index] = 1;                                                                                         \
                                                                                                                  \
    return 1;                                                                                                     \
  }                                                                                                               \
                                                                                                                  \
  int prefix##_set_find(const prefix##_set_t *set, prefix##_set_const_entry_t entry,                              \
                        prefix##_set_entry_t *saved_entry)                                                        \
  {                                                                                                               \
    ssize_t index;                                                                                                \
                                                                                                                  \
    index = prefix##_set_index(set, entry);                                                                       \
    if (index < 0 || !set->used[index])                                                                           \
      {                                                                                                           \
        return 0;                                                                                                 \
      }                                                                                                           \
    *saved_entry = set->set[index];                                                                               \
    return 1;                                                                                                     \
  }                                                                                                               \
                                                                                                                  \
  int prefix##_set_contains(const prefix##_set_t *set, prefix##_set_const_entry_t entry)                          \
  {                                                                                                               \
    ssize_t index;                                                                                                \
                                                                                                                  \
    index = prefix##_set_index(set, entry);                                                                       \
    return index >= 0 && set->used[index];                                                                        \
  }                                                                                                               \
                                                                                                                  \
  ssize_t prefix##_set_index(const prefix##_set_t *set, prefix##_set_const_entry_t entry)                         \
  {                                                                                                               \
    size_t hash;                                                                                                  \
    size_t i;                                                                                                     \
                                                                                                                  \
    hash = prefix##_set_entry_hash(entry);                                                                        \
    for (i = 0; i < set->capacity; ++i)                                                                           \
      {                                                                                                           \
        /* Quadratic probing that will visit every slot in the hash table if the capacity is a power of 2, see: \ \
         * http://research.cs.vt.edu/AVresearch/hashing/quadratic.php */                                          \
        size_t next_index = (hash + (i * i + i) / 2) % set->capacity;                                             \
        if (!set->used[next_index] || prefix##_set_entry_equals(set->set[next_index], entry))                     \
          {                                                                                                       \
            return next_index;                                                                                    \
          }                                                                                                       \
      }                                                                                                           \
    return -1;                                                                                                    \
  }
