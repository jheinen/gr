#ifdef __unix__
#define _POSIX_C_SOURCE 200112L
#endif

/* ######################### includes ############################################################################### */

#include <ctype.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <cm.h>

#include "base64_int.h"
#include "dump.h"
#include "event_int.h"
#include "gks.h"
#include "gr.h"
#include "gr3.h"
#include "logging_int.h"
#include "plot_int.h"
#include "util_int.h"

#include "datatype/double_map_int.h"
#include "datatype/string_map_int.h"
#include "datatype/string_array_map_int.h"
#include "datatype/template/map_int.h"
#include "datatype/template/set_int.h"


/* ######################### private interface ###################################################################### */

/* ========================= datatypes ============================================================================== */

/* ------------------------- args set ------------------------------------------------------------------------------- */

DECLARE_SET_TYPE(args, grm_args_t *)


/* ------------------------- string-to-plot_func map ---------------------------------------------------------------- */

DECLARE_MAP_TYPE(plot_func, plot_func_t)


/* ------------------------- string-to-args_set map ----------------------------------------------------------------- */

DECLARE_MAP_TYPE(args_set, args_set_t *)


#undef DECLARE_SET_TYPE
#undef DECLARE_MAP_TYPE


/* ========================= macros ================================================================================= */

/* ------------------------- math ----------------------------------------------------------------------------------- */

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef isfinite
#define isfinite(x) ((x) - (x) == (x) - (x))
#endif

/* ------------------------- args get ------------------------------------------------------------------------------- */

#define ARGS_VALUE_ITERATOR_GET(value_it, length, array) \
  if (value_it->next(value_it) == NULL)                  \
    {                                                    \
      args_value_iterator_delete(value_it);              \
      return ERROR_INTERNAL;                             \
    }                                                    \
  length = value_it->array_length;                       \
  array = *(double **)value_it->value_ptr;

/* ------------------------- re-implementation of x_lin/x_log ------------------------------------------------------- */

#define X_FLIP_IF(x, scale_options, xmin, xmax) \
  (GR_OPTION_FLIP_X & scale_options ? xmin + xmax : 0) + (GR_OPTION_FLIP_X & scale_options ? -1 : 1) * x

#define X_LIN(x, scale_options, xmin, xmax, a, b) \
  X_FLIP_IF((GR_OPTION_X_LOG & scale_options ? (x > 0 ? a * log10(x) + b : -FLT_MAX) : x), scale_options, xmin, xmax)

#define X_LOG(x, scale_options, xmin, xmax, a, b)                                                             \
  (GR_OPTION_X_LOG & scale_options ? (pow(10.0, (double)((X_FLIP_IF(x, scale_options, xmin, xmax) - b) / a))) \
                                   : X_FLIP_IF(x, scale_options, xmin, xmax))

/* ========================= methods ================================================================================ */

/* ------------------------- args set ------------------------------------------------------------------------------- */

DECLARE_SET_METHODS(args)


/* ------------------------- string-to-plot_func map ---------------------------------------------------------------- */

DECLARE_MAP_METHODS(plot_func)


/* ------------------------- string-to-args_set map ----------------------------------------------------------------- */

DECLARE_MAP_METHODS(args_set)


#undef DECLARE_SET_METHODS
#undef DECLARE_MAP_METHODS


/* ######################### internal implementation ################################################################ */

/* ========================= static variables ======================================================================= */

/* ------------------------- plot ----------------------------------------------------------------------------------- */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~ general ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

static int plot_static_variables_initialized = 0;
const char *plot_hierarchy_names[] = {"root", "plots", "subplots", "series", NULL};
static int plot_scatter_markertypes[] = {
    GKS_K_MARKERTYPE_SOLID_CIRCLE,   GKS_K_MARKERTYPE_SOLID_TRI_UP, GKS_K_MARKERTYPE_SOLID_TRI_DOWN,
    GKS_K_MARKERTYPE_SOLID_SQUARE,   GKS_K_MARKERTYPE_SOLID_BOWTIE, GKS_K_MARKERTYPE_SOLID_HGLASS,
    GKS_K_MARKERTYPE_SOLID_DIAMOND,  GKS_K_MARKERTYPE_SOLID_STAR,   GKS_K_MARKERTYPE_SOLID_TRI_RIGHT,
    GKS_K_MARKERTYPE_SOLID_TRI_LEFT, GKS_K_MARKERTYPE_SOLID_PLUS,   GKS_K_MARKERTYPE_PENTAGON,
    GKS_K_MARKERTYPE_HEXAGON,        GKS_K_MARKERTYPE_HEPTAGON,     GKS_K_MARKERTYPE_OCTAGON,
    GKS_K_MARKERTYPE_STAR_4,         GKS_K_MARKERTYPE_STAR_5,       GKS_K_MARKERTYPE_STAR_6,
    GKS_K_MARKERTYPE_STAR_7,         GKS_K_MARKERTYPE_STAR_8,       GKS_K_MARKERTYPE_VLINE,
    GKS_K_MARKERTYPE_HLINE,          GKS_K_MARKERTYPE_OMARK,        INT_MAX};


/* ~~~~~~~~~~~~~~~~~~~~~~~~~ args ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

static grm_args_t *global_root_args = NULL;
grm_args_t *active_plot_args = NULL;
static unsigned int active_plot_index = 0;

/* ~~~~~~~~~~~~~~~~~~~~~~~~~ event handling ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

event_queue_t *event_queue = NULL;


/* ~~~~~~~~~~~~~~~~~~~~~~~~~ kind to fmt ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

static string_map_entry_t kind_to_fmt[] = {{"line", "xys"},         {"hexbin", "xys"},
                                           {"polar", "xys"},        {"shade", "xys"},
                                           {"stem", "xys"},         {"step", "xys"},
                                           {"contour", "xyzc"},     {"contourf", "xyzc"},
                                           {"tricont", "xyzc"},     {"trisurf", "xyzc"},
                                           {"surface", "xyzc"},     {"wireframe", "xyzc"},
                                           {"plot3", "xyzc"},       {"scatter", "xyzc"},
                                           {"scatter3", "xyzc"},    {"quiver", "xyuv"},
                                           {"heatmap", "xyzc"},     {"hist", "x"},
                                           {"barplot", "y"},        {"isosurface", "c"},
                                           {"imshow", "c"},         {"nonuniformheatmap", "xyzc"},
                                           {"polar_histogram", "x"}};


/* ~~~~~~~~~~~~~~~~~~~~~~~~~ kind to func ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

static plot_func_map_entry_t kind_to_func[] = {{"line", plot_line},
                                               {"step", plot_step},
                                               {"scatter", plot_scatter},
                                               {"quiver", plot_quiver},
                                               {"stem", plot_stem},
                                               {"hist", plot_hist},
                                               {"barplot", plot_barplot},
                                               {"contour", plot_contour},
                                               {"contourf", plot_contourf},
                                               {"hexbin", plot_hexbin},
                                               {"heatmap", plot_heatmap},
                                               {"wireframe", plot_wireframe},
                                               {"surface", plot_surface},
                                               {"plot3", plot_plot3},
                                               {"scatter3", plot_scatter3},
                                               {"imshow", plot_imshow},
                                               {"isosurface", plot_isosurface},
                                               {"polar", plot_polar},
                                               {"trisurf", plot_trisurf},
                                               {"tricont", plot_tricont},
                                               {"shade", plot_shade},
                                               {"nonuniformheatmap", plot_heatmap},
                                               {"polar_histogram", plot_polar_histogram}};


/* ~~~~~~~~~~~~~~~~~~~~~~~~~ maps ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

static double_map_t *meters_per_unit_map = NULL;
static string_map_t *fmt_map = NULL;
static plot_func_map_t *plot_func_map = NULL;
static string_map_t *plot_valid_keys_map = NULL;
static string_array_map_t *type_map = NULL;


/* ~~~~~~~~~~~~~~~~~~~~~~~~~ plot clear ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

const char *plot_clear_exclude_keys[] = {"array_index", "in_use", NULL};


/* ~~~~~~~~~~~~~~~~~~~~~~~~~ plot merge ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

const char *plot_merge_ignore_keys[] = {"id", "series_id", "subplot_id", "plot_id", "array_index", "in_use", NULL};
const char *plot_merge_clear_keys[] = {"series", NULL};


/* ~~~~~~~~~~~~~~~~~~~~~~~~~ kind to func ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

static const double_map_entry_t symbol_to_meters_per_unit[] = {
    {"m", 1.0},     {"dm", 0.1},    {"cm", 0.01},  {"mm", 0.001},        {"in", 0.0254},
    {"\"", 0.0254}, {"ft", 0.3048}, {"'", 0.0254}, {"pc", 0.0254 / 6.0}, {"pt", 0.0254 / 72.0},
};


/* ~~~~~~~~~~~~~~~~~~~~~~~~~ text encoding ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

static int pre_plot_text_encoding = -1;


/* ~~~~~~~~~~~~~~~~~~~~~~~~~ valid keys ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* IMPORTANT: Every key should only be part of ONE array -> keys can be assigned to the right object, if a user sends a
 * flat object that mixes keys of different hierarchies */

const char *valid_root_keys[] = {"plots", "append_plots", "hold_plots", NULL};
const char *valid_plot_keys[] = {"clear", "figsize", "raw", "size", "subplots", "update", NULL};

const char *valid_subplot_keys[] = {"adjust_xlim",
                                    "adjust_ylim",
                                    "adjust_zlim",
                                    "alpha",
                                    "angle_ticks",
                                    "backgroundcolor",
                                    "bar_color",
                                    "bar_width",
                                    "colormap",
                                    "font",
                                    "font_precision",
                                    "ind_bar_color",
                                    "ind_edge_color",
                                    "ind_edge_width",
                                    "keep_aspect_ratio",
                                    "kind",
                                    "labels",
                                    "levels",
                                    "location",
                                    "normalization",
                                    "panzoom",
                                    "phiflip",
                                    "reset_ranges",
                                    "rings",
                                    "rotation",
                                    "series",
                                    "style",
                                    "subplot",
                                    "tilt",
                                    "title",
                                    "xbins",
                                    "xflip",
                                    "xform",
                                    "xlabel",
                                    "xlim",
                                    "xlog",
                                    "xnotations",
                                    "ybins",
                                    "yflip",
                                    "ylabel",
                                    "ylim",
                                    "ylog",
                                    "zflip",
                                    "zlim",
                                    "zlog",
                                    NULL};
const char *valid_series_keys[] = {"a",
                                   "bin_width",
                                   "bin_edges",
                                   "bin_counts",
                                   "c",
                                   "c_dims",
                                   "crange",
                                   "draw_edges",
                                   "edge_color",
                                   "edge_width",
                                   "error",
                                   "face_color",
                                   "foreground_color",
                                   "indices",
                                   "inner_series",
                                   "isovalue",
                                   "markertype",
                                   "nbins",
                                   "philim",
                                   "rgb",
                                   "rlim",
                                   "s",
                                   "spec",
                                   "step_where",
                                   "stairs",
                                   "u",
                                   "v",
                                   "weights",
                                   "x",
                                   "xcolormap",
                                   "xrange",
                                   "y",
                                   "ycolormap",
                                   "yrange",
                                   "z",
                                   "z_dims",
                                   "zrange",
                                   NULL};

/* ~~~~~~~~~~~~~~~~~~~~~~~~~ valid types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* If multiple formats are supported use `|` as separator
 * Example: "i|s" for supporting both integer and strings */
/* TODO: type for format "s"? */
static string_map_entry_t key_to_formats[] = {{"a", "A"},
                                              {"adjust_xlim", "i"},
                                              {"adjust_ylim", "i"},
                                              {"adjust_zlim", "i"},
                                              {"alpha", "d"},
                                              {"append_plots", "i"},
                                              {"backgroundcolor", "i"},
                                              {"bar_color", "D|i"},
                                              {"c", "D|I"},
                                              {"c_dims", "I"},
                                              {"crange", "D"},
                                              {"colormap", "i"},
                                              {"edge_color", "D|i"},
                                              {"edge_width", "d"},
                                              {"error", "a"},
                                              {"fig_size", "D"},
                                              {"font", "i"},
                                              {"font_precision", "i"},
                                              {"foreground_color", "D"},
                                              {"hold_plots", "i"},
                                              {"ind_bar_color", "A"},
                                              {"ind_edge_color", "A"},
                                              {"ind_edge_width", "A"},
                                              {"isovalue", "d"},
                                              {"keep_aspect_ratio", "i"},
                                              {"kind", "s"},
                                              {"labels", "S"},
                                              {"levels", "i"},
                                              {"location", "i"},
                                              {"markertype", "i"},
                                              {"nbins", "i"},
                                              {"panzoom", "D"},
                                              {"raw", "s"},
                                              {"reset_ranges", "i"},
                                              {"rotation", "i"},
                                              {"size", "D|A"},
                                              {"spec", "s"},
                                              {"step_where", "s"},
                                              {"style", "s"},
                                              {"subplot", "D"},
                                              {"tilt", "i"},
                                              {"title", "s"},
                                              {"u", "D"},
                                              {"update", "i"},
                                              {"v", "D"},
                                              {"x", "D|I"},
                                              {"xbins", "i"},
                                              {"xcolormap", "i"},
                                              {"xflip", "i"},
                                              {"xform", "i"},
                                              {"xlabel", "s"},
                                              {"xlim", "D"},
                                              {"xrange", "D"},
                                              {"xlog", "i"},
                                              {"y", "D"},
                                              {"ybins", "i"},
                                              {"ycolormap", "i"},
                                              {"yflip", "i"},
                                              {"yform", "i"},
                                              {"ylabel", "s"},
                                              {"ylim", "D"},
                                              {"yrange", "D"},
                                              {"ylog", "i"},
                                              {"z", "D"},
                                              {"z_dims", "I"},
                                              {"zflip", "i"},
                                              {"zlim", "D"},
                                              {"zrange", "D"},
                                              {"zlog", "i"}};

/* ========================= functions ============================================================================== */

/* ------------------------- plot ----------------------------------------------------------------------------------- */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~ general ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

error_t plot_init_static_variables(void)
{
  error_t error = NO_ERROR;

  if (!plot_static_variables_initialized)
    {
      logger((stderr, "Initializing static plot variables\n"));
      event_queue = event_queue_new();
      global_root_args = grm_args_new();
      error_cleanup_and_set_error_if(global_root_args == NULL, ERROR_MALLOC);
      error = plot_init_args_structure(global_root_args, plot_hierarchy_names, 1);
      error_cleanup_if_error;
      plot_set_flag_defaults();
      error_cleanup_and_set_error_if(!args_values(global_root_args, "plots", "a", &active_plot_args), ERROR_INTERNAL);
      active_plot_index = 1;
      meters_per_unit_map = double_map_new_with_data(array_size(symbol_to_meters_per_unit), symbol_to_meters_per_unit);
      error_cleanup_and_set_error_if(meters_per_unit_map == NULL, ERROR_MALLOC);
      fmt_map = string_map_new_with_data(array_size(kind_to_fmt), kind_to_fmt);
      error_cleanup_and_set_error_if(fmt_map == NULL, ERROR_MALLOC);
      plot_func_map = plot_func_map_new_with_data(array_size(kind_to_func), kind_to_func);
      error_cleanup_and_set_error_if(plot_func_map == NULL, ERROR_MALLOC);
      {
        const char **hierarchy_keys[] = {valid_root_keys, valid_plot_keys, valid_subplot_keys, valid_series_keys, NULL};
        const char **hierarchy_names_ptr, ***hierarchy_keys_ptr, **current_key_ptr;
        plot_valid_keys_map = string_map_new(array_size(valid_root_keys) + array_size(valid_plot_keys) +
                                             array_size(valid_subplot_keys) + array_size(valid_series_keys));
        error_cleanup_and_set_error_if(plot_valid_keys_map == NULL, ERROR_MALLOC);
        hierarchy_keys_ptr = hierarchy_keys;
        hierarchy_names_ptr = plot_hierarchy_names;
        while (*hierarchy_names_ptr != NULL && *hierarchy_keys_ptr != NULL)
          {
            current_key_ptr = *hierarchy_keys_ptr;
            while (*current_key_ptr != NULL)
              {
                string_map_insert(plot_valid_keys_map, *current_key_ptr, *hierarchy_names_ptr);
                ++current_key_ptr;
              }
            ++hierarchy_names_ptr;
            ++hierarchy_keys_ptr;
          }
      }
      type_map = string_array_map_new_from_string_split(array_size(key_to_formats), key_to_formats, '|');
      error_cleanup_and_set_error_if(type_map == NULL, ERROR_MALLOC);
      plot_static_variables_initialized = 1;
    }
  return NO_ERROR;

error_cleanup:
  if (global_root_args != NULL)
    {
      grm_args_delete(global_root_args);
      global_root_args = NULL;
    }
  if (meters_per_unit_map != NULL)
    {
      double_map_delete(meters_per_unit_map);
      meters_per_unit_map = NULL;
    }
  if (fmt_map != NULL)
    {
      string_map_delete(fmt_map);
      fmt_map = NULL;
    }
  if (plot_func_map != NULL)
    {
      plot_func_map_delete(plot_func_map);
      plot_func_map = NULL;
    }
  if (plot_valid_keys_map != NULL)
    {
      string_map_delete(plot_valid_keys_map);
      plot_valid_keys_map = NULL;
    }
  if (type_map != NULL)
    {
      string_array_map_delete(type_map);
      type_map = NULL;
    }
  return error;
}


/* ~~~~~~~~~~~~~~~~~~~~~~~~~ plot arguments ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

error_t plot_merge_args(grm_args_t *args, const grm_args_t *merge_args, const char **hierarchy_name_ptr,
                        uint_map_t *hierarchy_to_id, int hold_always)
{
  static args_set_map_t *key_to_cleared_args = NULL;
  static int recursion_level = -1;
  int plot_id, subplot_id, series_id;
  int append_plots;
  args_iterator_t *merge_it = NULL;
  arg_t *arg, *merge_arg;
  args_value_iterator_t *value_it = NULL, *merge_value_it = NULL;
  const char **current_hierarchy_name_ptr;
  grm_args_t **args_array, **merge_args_array, *current_args;
  unsigned int i;
  error_t error = NO_ERROR;

  ++recursion_level;
  if (hierarchy_name_ptr == NULL)
    {
      hierarchy_name_ptr = plot_hierarchy_names;
    }
  if (hierarchy_to_id == NULL)
    {
      hierarchy_to_id = uint_map_new(array_size(plot_hierarchy_names));
      cleanup_and_set_error_if(hierarchy_to_id == NULL, ERROR_MALLOC);
    }
  else
    {
      hierarchy_to_id = uint_map_copy(hierarchy_to_id);
      cleanup_and_set_error_if(hierarchy_to_id == NULL, ERROR_MALLOC);
    }
  if (key_to_cleared_args == NULL)
    {
      key_to_cleared_args = args_set_map_new(array_size(plot_merge_clear_keys));
      cleanup_and_set_error_if(hierarchy_to_id == NULL, ERROR_MALLOC);
    }
  args_values(global_root_args, "append_plots", "i", &append_plots);
  get_id_from_args(merge_args, &plot_id, &subplot_id, &series_id);
  if (plot_id > 0)
    {
      uint_map_insert(hierarchy_to_id, "plots", plot_id);
    }
  else
    {
      uint_map_insert_default(hierarchy_to_id, "plots", append_plots ? 0 : active_plot_index);
      uint_map_at(hierarchy_to_id, "plots", (unsigned int *)&plot_id);
      logger((stderr, "Using plot_id \"%u\"\n", plot_id));
    }
  if (subplot_id > 0)
    {
      uint_map_insert(hierarchy_to_id, "subplots", subplot_id);
    }
  else
    {
      uint_map_insert_default(hierarchy_to_id, "subplots", 1);
      uint_map_at(hierarchy_to_id, "subplots", (unsigned int *)&subplot_id);
    }
  if (series_id > 0)
    {
      uint_map_insert(hierarchy_to_id, "series", series_id);
    }
  else
    {
      uint_map_insert_default(hierarchy_to_id, "series", 1);
      uint_map_at(hierarchy_to_id, "series", (unsigned int *)&series_id);
    }
  /* special case: clear the plot container before usage if
   * - it is the first call of `plot_merge_args` AND `hold_always` is `0` AND
   *   - `plot_id` is `1` and `hold_plots` is not set OR
   *   - `hold_plots` is true and no plot will be appended (`plot_id` > 0)
   */
  if (strcmp(*hierarchy_name_ptr, "root") == 0 && plot_id > 0 && !hold_always)
    {
      int hold_plots_key_available, hold_plots;
      hold_plots_key_available = args_values(args, "hold_plots", "i", &hold_plots);
      if (hold_plots_key_available)
        {
          logger((stderr, "Do%s hold plots\n", hold_plots ? "" : " not"));
        }
      if ((hold_plots_key_available && !hold_plots) || (!hold_plots_key_available && plot_id == 1))
        {
          cleanup_and_set_error_if(!args_values(args, "plots", "A", &args_array), ERROR_INTERNAL);
          current_args = args_array[plot_id - 1];
          grm_args_clear(current_args);
          error = plot_init_args_structure(current_args, hierarchy_name_ptr + 1, 1);
          cleanup_if_error;
          logger((stderr, "Cleared current args\n"));
        }
      else
        {
          logger((stderr, "Held current args\n"));
        }
    }
#ifndef NDEBUG
  if (strcmp(*hierarchy_name_ptr, "root") == 0 && hold_always)
    {
      logger((stderr, "\"hold_always\" is set\n"));
    }
#endif /* ifndef  */
  merge_it = args_iter(merge_args);
  cleanup_and_set_error_if(merge_it == NULL, ERROR_MALLOC);
  while ((merge_arg = merge_it->next(merge_it)) != NULL)
    {
      if (str_equals_any_in_array(merge_arg->key, plot_merge_ignore_keys))
        {
          continue;
        }
      /* First, find the correct hierarchy level where the current merge value belongs to. */
      error = plot_get_args_in_hierarchy(args, hierarchy_name_ptr, merge_arg->key, hierarchy_to_id,
                                         (const grm_args_t **)&current_args, &current_hierarchy_name_ptr);
      if (error == ERROR_PLOT_UNKNOWN_KEY)
        {
          logger((stderr, "WARNING: The key \"%s\" is not assigned to any hierarchy level.\n", merge_arg->key));
        }
      cleanup_if_error;
      if (str_equals_any_in_array(*current_hierarchy_name_ptr, plot_merge_clear_keys))
        {
          int clear_args = 1;
          args_set_t *cleared_args = NULL;
          if (args_set_map_at(key_to_cleared_args, *current_hierarchy_name_ptr, &cleared_args))
            {
              clear_args = !args_set_contains(cleared_args, current_args);
            }
          if (clear_args)
            {
              logger((stderr, "Perform a clear on the current args container\n"));
              grm_args_clear(current_args);
              if (cleared_args == NULL)
                {
                  cleared_args = args_set_new(10); /* FIXME: do not use a magic number, use a growbable set instead! */
                  cleanup_and_set_error_if(cleared_args == NULL, ERROR_MALLOC);
                  cleanup_and_set_error_if(
                      !args_set_map_insert(key_to_cleared_args, *current_hierarchy_name_ptr, cleared_args),
                      ERROR_INTERNAL);
                }
              logger((stderr, "Add args container \"%p\" to cleared args with key \"%s\"\n", (void *)current_args,
                      *current_hierarchy_name_ptr));
              cleanup_and_set_error_if(!args_set_add(cleared_args, current_args), ERROR_INTERNAL);
            }
        }
      /* If the current key is a hierarchy key, perform a merge. Otherwise (else branch) put the value in without a
       * merge.
       */
      if (current_hierarchy_name_ptr[1] != NULL && strcmp(merge_arg->key, current_hierarchy_name_ptr[1]) == 0)
        {
          /* `args_at` cannot fail in this case because the `args` object was initialized with an empty structure
           * before. If `arg` is NULL, an internal error occurred. */
          arg = args_at(current_args, merge_arg->key);
          cleanup_and_set_error_if(arg == NULL, ERROR_INTERNAL);
          value_it = arg_value_iter(arg);
          merge_value_it = arg_value_iter(merge_arg);
          cleanup_and_set_error_if(value_it == NULL, ERROR_MALLOC);
          cleanup_and_set_error_if(merge_value_it == NULL, ERROR_MALLOC);
          /* Do not support two dimensional argument arrays like `nAnA`) -> a loop would be needed with more memory
           * management */
          cleanup_and_set_error_if(value_it->next(value_it) == NULL, ERROR_MALLOC);
          cleanup_and_set_error_if(merge_value_it->next(merge_value_it) == NULL, ERROR_MALLOC);
          /* Increase the array size of the internal args array if necessary */
          if (merge_value_it->array_length > value_it->array_length)
            {
              error = plot_init_arg_structure(arg, current_hierarchy_name_ptr, merge_value_it->array_length);
              cleanup_if_error;
              args_value_iterator_delete(value_it);
              value_it = arg_value_iter(arg);
              cleanup_and_set_error_if(value_it == NULL, ERROR_MALLOC);
              cleanup_and_set_error_if(value_it->next(value_it) == NULL, ERROR_MALLOC);
              args_array = *(grm_args_t ***)value_it->value_ptr;
            }
          else
            {
              /* The internal args container stores always an array for hierarchy levels */
              args_array = *(grm_args_t ***)value_it->value_ptr;
            }
          if (merge_value_it->is_array)
            {
              merge_args_array = *(grm_args_t ***)merge_value_it->value_ptr;
            }
          else
            {
              merge_args_array = (grm_args_t **)merge_value_it->value_ptr;
            }
          /* Merge array entries pairwise */
          for (i = 0; i < merge_value_it->array_length; ++i)
            {
              logger((stderr, "Perform a recursive merge on key \"%s\", array index \"%d\"\n", merge_arg->key, i));
              error = plot_merge_args(args_array[i], merge_args_array[i], current_hierarchy_name_ptr + 1,
                                      hierarchy_to_id, hold_always);
              cleanup_if_error;
            }
        }
      else
        {
          const char *compatible_format;
          /* Only accept the new value, if it has a compatible type. Convert if possible */
          if ((compatible_format = get_compatible_format(merge_arg->key, merge_arg->value_format)) != NULL)
            {
              logger((stderr, "Perform a replace on key \"%s\"\n", merge_arg->key));
              if (strlen(merge_arg->value_format) == 1 &&
                  tolower(*merge_arg->value_format) == *merge_arg->value_format &&
                  toupper(*compatible_format) == *compatible_format)
                {
                  /* A single value is given but an array is needed
                   * -> Push the given value as array instead
                   * **IMPORTANT**: In this case the existing `merge_arg` node cannot be shared with `current_args`
                   * (since the stored format does not fit), so the value must be copied (it is owned by the `merge_arg`
                   * node and would be freed twice otherwise). */
                  char array_format[3] = "n";
                  void *copy_buffer;
                  array_format[1] = *compatible_format;
                  logger((stderr, "Convert the given format \"%s\" to an array format \"%s\" \n",
                          merge_arg->value_format, array_format));
                  copy_buffer = copy_value(tolower(*compatible_format), merge_arg->value_ptr);
                  cleanup_and_set_error_if(copy_buffer == NULL, ERROR_MALLOC);
                  grm_args_push(current_args, merge_arg->key, array_format, 1, copy_buffer);
                  /* x -> copy_buffer -> value, value is now stored and the buffer is not needed any more */
                  free(copy_buffer);
                }
              else
                {
                  /* Otherwise, push without conversion (needed conversion is done later when extracting values with
                   * functions like `args_values`) */
                  error = args_push_arg(current_args, merge_arg);
                  cleanup_if_error;
                }
            }
          else
            {
              logger((stderr, "The type \"%s\" of key \"%s\" was rejected and will not be merged.\n",
                      merge_arg->value_format, merge_arg->key));
            }
        }
    }

cleanup:
  if (recursion_level == 0)
    {
      if (key_to_cleared_args != NULL)
        {
          args_set_t *cleared_args = NULL;
          const char **current_key_ptr = plot_merge_clear_keys;
          while (*current_key_ptr != NULL)
            {
              if (args_set_map_at(key_to_cleared_args, *current_key_ptr, &cleared_args))
                {
                  args_set_delete(cleared_args);
                }
              ++current_key_ptr;
            }
          args_set_map_delete(key_to_cleared_args);
          key_to_cleared_args = NULL;
        }
    }
  if (hierarchy_to_id != NULL)
    {
      uint_map_delete(hierarchy_to_id);
      hierarchy_to_id = NULL;
    }
  if (merge_it != NULL)
    {
      args_iterator_delete(merge_it);
    }
  if (value_it != NULL)
    {
      args_value_iterator_delete(value_it);
    }
  if (merge_value_it != NULL)
    {
      args_value_iterator_delete(merge_value_it);
    }

  --recursion_level;

  return error;
}

error_t plot_init_arg_structure(arg_t *arg, const char **hierarchy_name_ptr, unsigned int next_hierarchy_level_max_id)
{
  grm_args_t **args_array = NULL;
  unsigned int args_old_array_length;
  unsigned int i;
  error_t error = NO_ERROR;

  logger((stderr, "Init plot args structure for hierarchy: \"%s\"\n", *hierarchy_name_ptr));

  ++hierarchy_name_ptr;
  if (*hierarchy_name_ptr == NULL)
    {
      return NO_ERROR;
    }
  arg_first_value(arg, "A", NULL, &args_old_array_length);
  if (next_hierarchy_level_max_id <= args_old_array_length)
    {
      return NO_ERROR;
    }
  logger((stderr, "Increase array for key \"%s\" from %d to %d\n", *hierarchy_name_ptr, args_old_array_length,
          next_hierarchy_level_max_id));
  error = arg_increase_array(arg, next_hierarchy_level_max_id - args_old_array_length);
  return_if_error;
  arg_values(arg, "A", &args_array);
  for (i = args_old_array_length; i < next_hierarchy_level_max_id; ++i)
    {
      args_array[i] = grm_args_new();
      grm_args_push(args_array[i], "array_index", "i", i);
      return_error_if(args_array[i] == NULL, ERROR_MALLOC);
      error = plot_init_args_structure(args_array[i], hierarchy_name_ptr, 1);
      return_if_error;
      if (strcmp(*hierarchy_name_ptr, "plots") == 0)
        {
          grm_args_push(args_array[i], "in_use", "i", 0);
        }
    }

  return NO_ERROR;
}

error_t plot_init_args_structure(grm_args_t *args, const char **hierarchy_name_ptr,
                                 unsigned int next_hierarchy_level_max_id)
{
  arg_t *arg = NULL;
  grm_args_t **args_array = NULL;
  unsigned int i;
  error_t error = NO_ERROR;

  logger((stderr, "Init plot args structure for hierarchy: \"%s\"\n", *hierarchy_name_ptr));

  ++hierarchy_name_ptr;
  if (*hierarchy_name_ptr == NULL)
    {
      return NO_ERROR;
    }
  arg = args_at(args, *hierarchy_name_ptr);
  if (arg == NULL)
    {
      args_array = calloc(next_hierarchy_level_max_id, sizeof(grm_args_t *));
      error_cleanup_and_set_error_if(args_array == NULL, ERROR_MALLOC);
      for (i = 0; i < next_hierarchy_level_max_id; ++i)
        {
          args_array[i] = grm_args_new();
          grm_args_push(args_array[i], "array_index", "i", i);
          error_cleanup_and_set_error_if(args_array[i] == NULL, ERROR_MALLOC);
          error = plot_init_args_structure(args_array[i], hierarchy_name_ptr, 1);
          error_cleanup_if_error;
          if (strcmp(*hierarchy_name_ptr, "plots") == 0)
            {
              grm_args_push(args_array[i], "in_use", "i", 0);
            }
        }
      error_cleanup_if(!grm_args_push(args, *hierarchy_name_ptr, "nA", next_hierarchy_level_max_id, args_array));
      free(args_array);
      args_array = NULL;
    }
  else
    {
      error = plot_init_arg_structure(arg, hierarchy_name_ptr - 1, next_hierarchy_level_max_id);
      error_cleanup_if_error;
    }

  return NO_ERROR;

error_cleanup:
  if (args_array != NULL)
    {
      for (i = 0; i < next_hierarchy_level_max_id; ++i)
        {
          if (args_array[i] != NULL)
            {
              grm_args_delete(args_array[i]);
            }
        }
      free(args_array);
    }

  return error;
}

void plot_set_flag_defaults(void)
{
  /* Use a standalone function for initializing flags instead of `plot_set_attribute_defaults` to guarantee the flags
   * are already set before `grm_plot` is called (important for `grm_merge`) */

  logger((stderr, "Set global flag defaults\n"));

  args_setdefault(global_root_args, "append_plots", "i", ROOT_DEFAULT_APPEND_PLOTS);
}

void plot_set_attribute_defaults(grm_args_t *plot_args)
{
  const char *kind;
  grm_args_t **current_subplot, **current_series;
  double garbage0, garbage1;

  logger((stderr, "Set plot attribute defaults\n"));

  args_setdefault(plot_args, "clear", "i", PLOT_DEFAULT_CLEAR);
  args_setdefault(plot_args, "update", "i", PLOT_DEFAULT_UPDATE);
  if (!grm_args_contains(plot_args, "figsize"))
    {
      args_setdefault(plot_args, "size", "dd", PLOT_DEFAULT_WIDTH, PLOT_DEFAULT_HEIGHT);
    }

  args_values(plot_args, "subplots", "A", &current_subplot);
  while (*current_subplot != NULL)
    {
      args_setdefault(*current_subplot, "kind", "s", PLOT_DEFAULT_KIND);
      args_values(*current_subplot, "kind", "s", &kind);
      if (grm_args_contains(*current_subplot, "labels"))
        {
          args_setdefault(*current_subplot, "location", "i", PLOT_DEFAULT_LOCATION);
        }
      args_setdefault(*current_subplot, "subplot", "dddd", PLOT_DEFAULT_SUBPLOT_MIN_X, PLOT_DEFAULT_SUBPLOT_MAX_X,
                      PLOT_DEFAULT_SUBPLOT_MIN_Y, PLOT_DEFAULT_SUBPLOT_MAX_Y);
      args_setdefault(*current_subplot, "xlog", "i", PLOT_DEFAULT_XLOG);
      args_setdefault(*current_subplot, "ylog", "i", PLOT_DEFAULT_YLOG);
      args_setdefault(*current_subplot, "zlog", "i", PLOT_DEFAULT_ZLOG);
      args_setdefault(*current_subplot, "xflip", "i", PLOT_DEFAULT_XFLIP);
      args_setdefault(*current_subplot, "yflip", "i", PLOT_DEFAULT_YFLIP);
      args_setdefault(*current_subplot, "zflip", "i", PLOT_DEFAULT_ZFLIP);
      if (strcmp(kind, "heatmap") == 0)
        {
          args_setdefault(*current_subplot, "adjust_xlim", "i", 0);
          args_setdefault(*current_subplot, "adjust_ylim", "i", 0);
        }
      else
        {
          args_setdefault(
              *current_subplot, "adjust_xlim", "i",
              (args_values(*current_subplot, "xlim", "dd", &garbage0, &garbage1) ? 0 : PLOT_DEFAULT_ADJUST_XLIM));
          args_setdefault(
              *current_subplot, "adjust_ylim", "i",
              (args_values(*current_subplot, "ylim", "dd", &garbage0, &garbage1) ? 0 : PLOT_DEFAULT_ADJUST_YLIM));
          args_setdefault(
              *current_subplot, "adjust_zlim", "i",
              (args_values(*current_subplot, "zlim", "dd", &garbage0, &garbage1) ? 0 : PLOT_DEFAULT_ADJUST_ZLIM));
        }
      args_setdefault(*current_subplot, "colormap", "i", PLOT_DEFAULT_COLORMAP);
      args_setdefault(*current_subplot, "font", "i", PLOT_DEFAULT_FONT);
      args_setdefault(*current_subplot, "font_precision", "i", PLOT_DEFAULT_FONT_PRECISION);
      args_setdefault(*current_subplot, "rotation", "i", PLOT_DEFAULT_ROTATION);
      args_setdefault(*current_subplot, "tilt", "i", PLOT_DEFAULT_TILT);
      args_setdefault(*current_subplot, "keep_aspect_ratio", "i", PLOT_DEFAULT_KEEP_ASPECT_RATIO);

      if (str_equals_any(kind, 2, "contour", "contourf"))
        {
          args_setdefault(*current_subplot, "levels", "i", PLOT_DEFAULT_CONTOUR_LEVELS);
        }
      else if (strcmp(kind, "tricont") == 0)
        {
          args_setdefault(*current_subplot, "levels", "i", PLOT_DEFAULT_TRICONT_LEVELS);
        }

      args_values(*current_subplot, "series", "A", &current_series);
      while (*current_series != NULL)
        {
          args_setdefault(*current_series, "spec", "s", SERIES_DEFAULT_SPEC);
          if (strcmp(kind, "step") == 0)
            {
              args_setdefault(*current_series, "step_where", "s", PLOT_DEFAULT_STEP_WHERE);
            }
          else if (strcmp(kind, "hexbin") == 0)
            {
              args_setdefault(*current_series, "nbins", "i", PLOT_DEFAULT_HEXBIN_NBINS);
            }
          ++current_series;
        }
      ++current_subplot;
    }
}

void plot_pre_plot(grm_args_t *plot_args)
{
  int clear;

  logger((stderr, "Pre plot processing\n"));

  plot_set_text_encoding();
  args_values(plot_args, "clear", "i", &clear);
  logger((stderr, "Got keyword \"clear\" with value %d\n", clear));
  if (clear)
    {
      gr_clearws();
    }
  plot_process_wswindow_wsviewport(plot_args);
}

void plot_set_text_encoding(void)
{
  gr_inqtextencoding(&pre_plot_text_encoding);
  gr_settextencoding(ENCODING_UTF8);
}

void plot_process_wswindow_wsviewport(grm_args_t *plot_args)
{
  int pixel_width, pixel_height;
  int previous_pixel_width, previous_pixel_height;
  double metric_width, metric_height;
  double aspect_ratio_ws;
  double wsviewport[4] = {0.0, 0.0, 0.0, 0.0};
  double wswindow[4] = {0.0, 0.0, 0.0, 0.0};

  get_figure_size(plot_args, &pixel_width, &pixel_height, &metric_width, &metric_height);

  if (!args_values(plot_args, "previous_pixel_size", "ii", &previous_pixel_width, &previous_pixel_height) ||
      (previous_pixel_width != pixel_width || previous_pixel_height != pixel_height))
    {
      /* TODO: handle error return value? */
      event_queue_enqueue_size_event(event_queue, active_plot_index - 1, pixel_width, pixel_height);
    }

  aspect_ratio_ws = metric_width / metric_height;
  if (aspect_ratio_ws > 1)
    {
      wsviewport[1] = metric_width;
      wsviewport[3] = metric_width / aspect_ratio_ws;
      wswindow[1] = 1.0;
      wswindow[3] = 1.0 / aspect_ratio_ws;
    }
  else
    {
      wsviewport[1] = metric_height * aspect_ratio_ws;
      wsviewport[3] = metric_height;
      wswindow[1] = aspect_ratio_ws;
      wswindow[3] = 1.0;
    }

  gr_setwsviewport(wsviewport[0], wsviewport[1], wsviewport[2], wsviewport[3]);
  gr_setwswindow(wswindow[0], wswindow[1], wswindow[2], wswindow[3]);

  grm_args_push(plot_args, "wswindow", "dddd", wswindow[0], wswindow[1], wswindow[2], wswindow[3]);
  grm_args_push(plot_args, "wsviewport", "dddd", wsviewport[0], wsviewport[1], wsviewport[2], wsviewport[3]);
  grm_args_push(plot_args, "previous_pixel_size", "ii", pixel_width, pixel_height);

  logger((stderr, "Stored wswindow (%lf, %lf, %lf, %lf)\n", wswindow[0], wswindow[1], wswindow[2], wswindow[3]));
  logger(
      (stderr, "Stored wsviewport (%lf, %lf, %lf, %lf)\n", wsviewport[0], wsviewport[1], wsviewport[2], wsviewport[3]));
}

error_t plot_pre_subplot(grm_args_t *subplot_args)
{
  const char *kind;
  double alpha;
  error_t error = NO_ERROR;

  logger((stderr, "Pre subplot processing\n"));

  args_values(subplot_args, "kind", "s", &kind);
  logger((stderr, "Got keyword \"kind\" with value \"%s\"\n", kind));
  plot_process_viewport(subplot_args);
  error = plot_store_coordinate_ranges(subplot_args);
  return_if_error;
  plot_process_window(subplot_args);

  plot_process_colormap(subplot_args);
  plot_process_font(subplot_args);

  if (str_equals_any(kind, 2, "polar", "polar_histogram"))
    {
      plot_draw_polar_axes(subplot_args);
    }
  else
    {
      plot_draw_axes(subplot_args, 1);
    }

  gr_uselinespec(" ");

  gr_savestate();
  if (args_values(subplot_args, "alpha", "d", &alpha))
    {
      gr_settransparency(alpha);
    }

  return NO_ERROR;
}

void plot_process_colormap(grm_args_t *subplot_args)
{
  int colormap;

  if (args_values(subplot_args, "colormap", "i", &colormap))
    {
      gr_setcolormap(colormap);
    }
  /* TODO: Implement other datatypes for `colormap` */
}

void plot_process_font(grm_args_t *subplot_args)
{
  int font, font_precision;

  /* `font` and `font_precision` are always set */
  if (args_values(subplot_args, "font", "i", &font) &&
      args_values(subplot_args, "font_precision", "i", &font_precision))
    {
      logger((stderr, "Using font: %d with precision %d\n", font, font_precision));
      gr_settextfontprec(font, font_precision);
    }
  /* TODO: Implement other datatypes for `font` and `font_precision` */
}

void plot_process_viewport(grm_args_t *subplot_args)
{
  const char *kind;
  const double *subplot;
  int keep_aspect_ratio;
  double metric_width, metric_height;
  double aspect_ratio_ws;
  double vp[4];
  double viewport[4] = {0.0, 0.0, 0.0, 0.0};
  int background_color_index;

  args_values(subplot_args, "kind", "s", &kind);
  args_values(subplot_args, "subplot", "D", &subplot);
  args_values(subplot_args, "keep_aspect_ratio", "i", &keep_aspect_ratio);
  logger((stderr, "Using subplot: %lf, %lf, %lf, %lf\n", subplot[0], subplot[1], subplot[2], subplot[3]));

  get_figure_size(NULL, NULL, NULL, &metric_width, &metric_height);

  aspect_ratio_ws = metric_width / metric_height;
  memcpy(vp, subplot, sizeof(vp));
  if (aspect_ratio_ws > 1)
    {
      vp[2] /= aspect_ratio_ws;
      vp[3] /= aspect_ratio_ws;
      if (keep_aspect_ratio)
        {
          double border = 0.5 * (vp[1] - vp[0]) * (1.0 - 1.0 / aspect_ratio_ws);
          vp[0] += border;
          vp[1] -= border;
        }
    }
  else
    {
      vp[0] *= aspect_ratio_ws;
      vp[1] *= aspect_ratio_ws;
      if (keep_aspect_ratio)
        {
          double border = 0.5 * (vp[3] - vp[2]) * (1.0 - aspect_ratio_ws);
          vp[2] += border;
          vp[3] -= border;
        }
    }

  if (str_equals_any(kind, 5, "wireframe", "surface", "plot3", "scatter3", "trisurf"))
    {
      double tmp_vp[4];
      double extent;

      if (str_equals_any(kind, 2, "surface", "trisurf"))
        {
          extent = min(vp[1] - vp[0] - 0.1, vp[3] - vp[2]);
        }
      else
        {
          extent = min(vp[1] - vp[0], vp[3] - vp[2]);
        }
      tmp_vp[0] = 0.5 * (vp[0] + vp[1] - extent);
      tmp_vp[1] = 0.5 * (vp[0] + vp[1] + extent);
      tmp_vp[2] = 0.5 * (vp[2] + vp[3] - extent);
      tmp_vp[3] = 0.5 * (vp[2] + vp[3] + extent);
      memcpy(vp, tmp_vp, 4 * sizeof(double));
    }

  viewport[0] = vp[0] + 0.125 * (vp[1] - vp[0]);
  viewport[1] = vp[0] + 0.925 * (vp[1] - vp[0]);
  viewport[2] = vp[2] + 0.125 * (vp[3] - vp[2]);
  viewport[3] = vp[2] + 0.925 * (vp[3] - vp[2]);

  if (aspect_ratio_ws > 1)
    {
      viewport[2] += (1 - (subplot[3] - subplot[2]) * (subplot[3] - subplot[2])) * 0.02;
    }
  if (str_equals_any(kind, 6, "imshow", "contour", "contourf", "heatmap", "hexbin", "quiver"))
    {
      viewport[1] -= 0.1;
    }

  if (args_values(subplot_args, "backgroundcolor", "i", &background_color_index))
    {
      gr_savestate();
      gr_selntran(0);
      gr_setfillintstyle(GKS_K_INTSTYLE_SOLID);
      gr_setfillcolorind(background_color_index);
      if (aspect_ratio_ws > 1)
        {
          gr_fillrect(subplot[0], subplot[1], subplot[2] / aspect_ratio_ws, subplot[3] / aspect_ratio_ws);
        }
      else
        {
          gr_fillrect(subplot[0] * aspect_ratio_ws, subplot[1] * aspect_ratio_ws, subplot[2], subplot[3]);
        }
      gr_selntran(1);
      gr_restorestate();
    }

  if (str_equals_any(kind, 2, "polar", "polar_histogram"))
    {
      double x_center, y_center, r;

      x_center = 0.5 * (viewport[0] + viewport[1]);
      y_center = 0.5 * (viewport[2] + viewport[3]);
      r = 0.5 * min(viewport[1] - viewport[0], viewport[3] - viewport[2]);
      if (grm_args_contains(subplot_args, "title"))
        {
          y_center = (y_center + r) / 2.0;
        }
      viewport[0] = x_center - r;
      viewport[1] = x_center + r;
      viewport[2] = y_center - r;
      viewport[3] = y_center + r;
    }

  gr_setviewport(viewport[0], viewport[1], viewport[2], viewport[3]);

  grm_args_push(subplot_args, "vp", "dddd", vp[0], vp[1], vp[2], vp[3]);
  grm_args_push(subplot_args, "viewport", "dddd", viewport[0], viewport[1], viewport[2], viewport[3]);

  logger((stderr, "Stored vp (%lf, %lf, %lf, %lf)\n", vp[0], vp[1], vp[2], vp[3]));
  logger((stderr, "Stored viewport (%lf, %lf, %lf, %lf)\n", viewport[0], viewport[1], viewport[2], viewport[3]));
}

void plot_process_window(grm_args_t *subplot_args)
{
  int scale = 0;
  const char *kind;
  int xlog, ylog, zlog;
  int xflip, yflip, zflip;
  int major_count, x_major_count, y_major_count;
  const double *stored_window;
  double x_min, x_max, y_min, y_max, z_min, z_max;
  double x, y, xzoom, yzoom;
  int adjust_xlim, adjust_ylim, adjust_zlim;
  double x_tick, y_tick;
  double x_org_low, x_org_high, y_org_low, y_org_high;
  int reset_ranges = 0;

  args_values(subplot_args, "kind", "s", &kind);
  args_values(subplot_args, "xlog", "i", &xlog);
  args_values(subplot_args, "ylog", "i", &ylog);
  args_values(subplot_args, "zlog", "i", &zlog);
  args_values(subplot_args, "xflip", "i", &xflip);
  args_values(subplot_args, "yflip", "i", &yflip);
  args_values(subplot_args, "zflip", "i", &zflip);

  if (!str_equals_any(kind, 2, "polar", "polar_histogram"))
    {
      scale |= xlog ? GR_OPTION_X_LOG : 0;
      scale |= ylog ? GR_OPTION_Y_LOG : 0;
      scale |= zlog ? GR_OPTION_Z_LOG : 0;
      scale |= xflip ? GR_OPTION_FLIP_X : 0;
      scale |= yflip ? GR_OPTION_FLIP_Y : 0;
      scale |= zflip ? GR_OPTION_FLIP_Z : 0;
    }

  if (str_equals_any(kind, 2, "polar", "polar_histogram"))
    {
      y_min = x_min = -1.1;
      y_max = x_max = 1.1;
    }
  else
    {
      args_values(subplot_args, "_xlim", "dd", &x_min, &x_max);
      args_values(subplot_args, "_ylim", "dd", &y_min, &y_max);
      if (args_values(subplot_args, "reset_ranges", "i", &reset_ranges) && reset_ranges)
        {
          if (args_values(subplot_args, "_original_xlim", "dd", &x_min, &x_max) &&
              args_values(subplot_args, "_original_ylim", "dd", &y_min, &y_max) &&
              args_values(subplot_args, "_original_adjust_xlim", "i", &adjust_xlim) &&
              args_values(subplot_args, "_original_adjust_ylim", "i", &adjust_ylim))
            {
              grm_args_push(subplot_args, "_xlim", "dd", x_min, x_max);
              grm_args_push(subplot_args, "_ylim", "dd", y_min, y_max);
              grm_args_push(subplot_args, "adjust_xlim", "i", adjust_xlim);
              grm_args_push(subplot_args, "adjust_ylim", "i", adjust_ylim);
              grm_args_remove(subplot_args, "_original_xlim");
              grm_args_remove(subplot_args, "_original_ylim");
              grm_args_remove(subplot_args, "_original_adjust_xlim");
              grm_args_remove(subplot_args, "_original_adjust_ylim");
            }
          grm_args_remove(subplot_args, "reset_ranges");
        }
      if (grm_args_contains(subplot_args, "panzoom"))
        {
          if (!grm_args_contains(subplot_args, "_original_xlim"))
            {
              grm_args_push(subplot_args, "_original_xlim", "dd", x_min, x_max);
              args_values(subplot_args, "adjust_xlim", "i", &adjust_xlim);
              grm_args_push(subplot_args, "_original_adjust_xlim", "i", adjust_xlim);
              grm_args_push(subplot_args, "adjust_xlim", "i", 0);
            }
          if (!grm_args_contains(subplot_args, "_original_ylim"))
            {
              grm_args_push(subplot_args, "_original_ylim", "dd", y_min, y_max);
              args_values(subplot_args, "adjust_ylim", "i", &adjust_ylim);
              grm_args_push(subplot_args, "_original_adjust_ylim", "i", adjust_ylim);
              grm_args_push(subplot_args, "adjust_ylim", "i", 0);
            }
          if (!args_values(subplot_args, "panzoom", "dddd", &x, &y, &xzoom, &yzoom))
            {
              if (args_values(subplot_args, "panzoom", "ddd", &x, &y, &xzoom))
                {
                  yzoom = xzoom;
                }
              else
                {
                  /* TODO: Add error handling for type mismatch (-> next statement would fail) */
                  args_values(subplot_args, "panzoom", "dd", &x, &y);
                  yzoom = xzoom = 0.0;
                }
            }
          /* Ensure the correct window is set in GR */
          if (args_values(subplot_args, "window", "D", &stored_window))
            {
              gr_setwindow(stored_window[0], stored_window[1], stored_window[2], stored_window[3]);
              logger((stderr, "Window before `gr_panzoom` (%lf, %lf, %lf, %lf)\n", stored_window[0], stored_window[1],
                      stored_window[2], stored_window[3]));
            }
          gr_panzoom(x, y, xzoom, yzoom, &x_min, &x_max, &y_min, &y_max);
          logger((stderr, "Window after `gr_panzoom` (%lf, %lf, %lf, %lf)\n", x_min, x_max, y_min, y_max));
          grm_args_push(subplot_args, "_xlim", "dd", x_min, x_max);
          grm_args_push(subplot_args, "_ylim", "dd", y_min, y_max);
          grm_args_remove(subplot_args, "panzoom");
        }

      if (str_equals_any(kind, 6, "wireframe", "surface", "plot3", "scatter3", "polar", "trisurf"))
        {
          major_count = 2;
        }
      else
        {
          major_count = 5;
        }

      if (!(scale & GR_OPTION_X_LOG))
        {
          args_values(subplot_args, "adjust_xlim", "i", &adjust_xlim);
          if (adjust_xlim)
            {
              logger((stderr, "_xlim before \"gr_adjustlimits\": (%lf, %lf)\n", x_min, x_max));
              gr_adjustlimits(&x_min, &x_max);
              logger((stderr, "_xlim after \"gr_adjustlimits\": (%lf, %lf)\n", x_min, x_max));
            }
          if (strcmp(kind, "barplot") == 0)
            {
              const char *xnotations[5];
              unsigned int xnotations_length;
              x_tick = 1;
              if (args_first_value(subplot_args, "xnotations", "S", &xnotations, &xnotations_length))
                {
                  x_major_count = 0;
                }
              else
                {
                  x_major_count = 1;
                }
            }
          else
            {
              x_major_count = major_count;
              x_tick = gr_tick(x_min, x_max) / x_major_count;
            }
        }
      else
        {
          x_tick = x_major_count = 1;
        }
      if (!(scale & GR_OPTION_FLIP_X))
        {
          x_org_low = x_min;
          x_org_high = x_max;
        }
      else
        {
          x_org_low = x_max;
          x_org_high = x_min;
        }
      grm_args_push(subplot_args, "xtick", "d", x_tick);
      grm_args_push(subplot_args, "xorg", "dd", x_org_low, x_org_high);
      grm_args_push(subplot_args, "xmajor", "i", x_major_count);

      if (!(scale & GR_OPTION_Y_LOG))
        {
          args_values(subplot_args, "adjust_ylim", "i", &adjust_ylim);
          if (adjust_ylim)
            {
              logger((stderr, "_ylim before \"gr_adjustlimits\": (%lf, %lf)\n", y_min, y_max));
              gr_adjustlimits(&y_min, &y_max);
              logger((stderr, "_ylim after \"gr_adjustlimits\": (%lf, %lf)\n", y_min, y_max));
            }
          y_major_count = major_count;
          y_tick = gr_tick(y_min, y_max) / y_major_count;
        }
      else
        {
          y_tick = y_major_count = 1;
        }
      if (!(scale & GR_OPTION_FLIP_Y))
        {
          y_org_low = y_min;
          y_org_high = y_max;
        }
      else
        {
          y_org_low = y_max;
          y_org_high = y_min;
        }
      grm_args_push(subplot_args, "ytick", "d", y_tick);
      grm_args_push(subplot_args, "yorg", "dd", y_org_low, y_org_high);
      grm_args_push(subplot_args, "ymajor", "i", y_major_count);
    }

  logger((stderr, "Storing window (%lf, %lf, %lf, %lf)\n", x_min, x_max, y_min, y_max));
  grm_args_push(subplot_args, "window", "dddd", x_min, x_max, y_min, y_max);
  gr_setwindow(x_min, x_max, y_min, y_max);

  if (str_equals_any(kind, 5, "wireframe", "surface", "plot3", "scatter3", "trisurf"))
    {
      double z_major_count;
      double z_tick;
      double z_org_low, z_org_high;
      int rotation, tilt;

      args_values(subplot_args, "_zlim", "dd", &z_min, &z_max);
      if (!(scale & GR_OPTION_Z_LOG))
        {
          args_values(subplot_args, "adjust_zlim", "i", &adjust_zlim);
          if (adjust_zlim)
            {
              logger((stderr, "_zlim before \"gr_adjustlimits\": (%lf, %lf)\n", z_min, z_max));
              gr_adjustlimits(&z_min, &z_max);
              logger((stderr, "_zlim after \"gr_adjustlimits\": (%lf, %lf)\n", z_min, z_max));
            }
          z_major_count = major_count;
          z_tick = gr_tick(z_min, z_max) / z_major_count;
        }
      else
        {
          z_tick = z_major_count = 1;
        }
      if (!(scale & GR_OPTION_FLIP_Z))
        {
          z_org_low = z_min;
          z_org_high = z_max;
        }
      else
        {
          z_org_low = z_max;
          z_org_high = z_min;
        }
      grm_args_push(subplot_args, "ztick", "d", z_tick);
      grm_args_push(subplot_args, "zorg", "dd", z_org_low, z_org_high);
      grm_args_push(subplot_args, "zmajor", "i", z_major_count);

      args_values(subplot_args, "rotation", "i", &rotation);
      args_values(subplot_args, "tilt", "i", &tilt);
      gr_setspace(z_min, z_max, rotation, tilt);
    }

  grm_args_push(subplot_args, "scale", "i", scale);
  gr_setscale(scale);
}

error_t plot_store_coordinate_ranges(grm_args_t *subplot_args)
{
  const char *kind;
  const char *style = "";
  const char *fmt;
  grm_args_t **current_series;
  unsigned int series_count;
  grm_args_t **inner_series;
  unsigned int inner_series_count;
  const char *data_component_names[] = {"x", "y", "z", "c", NULL};
  const char **current_component_name;
  double *current_component = NULL;
  unsigned int current_point_count = 0;
  struct
  {
    const char *subplot;
    const char *series;
  } * current_range_keys,
      range_keys[] = {{"xlim", "xrange"}, {"ylim", "yrange"}, {"zlim", "zrange"}, {"clim", "crange"}};
  double *bins = NULL;
  unsigned int i;
  error_t error = NO_ERROR;

  logger((stderr, "Storing coordinate ranges\n"));

  /* If a pan and/or zoom was performed before, do not overwrite limits
   * -> the user fully controls limits by interaction */
  if (grm_args_contains(subplot_args, "_original_xlim"))
    {
      logger((stderr, "Panzoom active, do not modify limits...\n"));
      return NO_ERROR;
    }

  args_values(subplot_args, "kind", "s", &kind);
  args_values(subplot_args, "style", "s", &style);
  string_map_at(fmt_map, kind, (char **)&fmt); /* TODO: check if the map access was successful */
  if (strcmp(kind, "polar_histogram") != 0)
    {
      current_component_name = data_component_names;
      current_range_keys = range_keys;
      while (*current_component_name != NULL)
        {
          double min_component = DBL_MAX;
          double max_component = -DBL_MAX;
          double step = -DBL_MAX;
          if (strchr(fmt, **current_component_name) == NULL)
            {
              ++current_range_keys;
              ++current_component_name;
              continue;
            }
          /* Heatmaps need calculated range keys, so run the calculation even if limits are given */
          if (!grm_args_contains(subplot_args, current_range_keys->subplot) || strcmp(kind, "heatmap") == 0)
            {
              args_first_value(subplot_args, "series", "A", &current_series, &series_count);
              while (*current_series != NULL)
                {
                  double current_min_component = DBL_MAX, current_max_component = -DBL_MAX;
                  if (!args_values(*current_series, current_range_keys->series, "dd", &current_min_component,
                                   &current_max_component))
                    {
                      if (args_first_value(*current_series, *current_component_name, "D", &current_component,
                                           &current_point_count))
                        {
                          if (strcmp(style, "stacked") == 0)
                            {
                              current_max_component = 0.0;
                              for (i = 0; i < current_point_count; i++)
                                {
                                  current_max_component += current_component[i];
                                }
                              current_min_component = current_max_component;
                            }
                          else
                            {
                              for (i = 0; i < current_point_count; i++)
                                {
                                  current_min_component = min(current_component[i], current_min_component);
                                  current_max_component = max(current_component[i], current_max_component);
                                }
                            }
                        }
                      else if (strcmp(kind, "heatmap") == 0 && str_equals_any(*current_component_name, 2, "x", "y"))
                        {
                          /* in this case `x` or `y` (or both) are missing
                           * -> set the current min/max_component to the dimensions of `z`
                           *    (shifted by half a unit to center color blocks) */
                          const char *other_component_name = (strcmp(*current_component_name, "x") == 0) ? "y" : "x";
                          double *other_component;
                          unsigned int other_point_count;
                          if (args_first_value(*current_series, other_component_name, "D", &other_component,
                                               &other_point_count))
                            {
                              /* The other component is given -> the missing dimension can be calculated */
                              double *z;
                              unsigned int z_length;
                              cleanup_and_set_error_if(!args_first_value(*current_series, "z", "D", &z, &z_length),
                                                       ERROR_PLOT_MISSING_DATA);
                              current_point_count = z_length / other_point_count;
                            }
                          else
                            {
                              /* A heatmap without `x` and `y` values
                               * -> dimensions can only be read from `z_dims` */
                              int rows, cols;
                              cleanup_and_set_error_if(!args_values(*current_series, "z_dims", "ii", &rows, &cols),
                                                       ERROR_PLOT_MISSING_DIMENSIONS);
                              current_point_count = (strcmp(*current_component_name, "x") == 0) ? cols : rows;
                            }
                          current_min_component = -0.5;
                          current_max_component = current_point_count - 0.5;
                        }
                    }
                  else if (args_first_value(*current_series, "inner_series", "nA", &inner_series, &inner_series_count))
                    {
                      while (*inner_series != NULL)
                        {
                          if (args_first_value(*inner_series, *current_component_name, "D", &current_component,
                                               &current_point_count))
                            {
                              double current_max_component = 0;
                              for (i = 0; i < current_point_count; i++)
                                {
                                  current_max_component += current_component[i];
                                }
                              current_min_component = current_max_component;
                              max_component = max(current_max_component, max_component);
                              min_component = min(current_min_component, min_component);
                            }
                          inner_series++;
                        }
                    }
                  if (current_min_component != DBL_MAX && current_max_component != -DBL_MAX)
                    {
                      grm_args_push(*current_series, current_range_keys->series, "dd", current_min_component,
                                    current_max_component);
                    }
                  min_component = min(current_min_component, min_component);
                  max_component = max(current_max_component, max_component);
                  ++current_series;
                }
            }
          if (args_values(subplot_args, current_range_keys->subplot, "dd", &min_component, &max_component))
            {
              grm_args_push(subplot_args, private_name(current_range_keys->subplot), "dd", min_component,
                            max_component);
            }
          else if (min_component != DBL_MAX && max_component != -DBL_MAX)
            {
              if ((strcmp(kind, "barplot")) == 0 && strcmp("y", *current_component_name) == 0)
                {
                  min_component = 0;
                }
              else if (strcmp(kind, "quiver") == 0)
                {
                  step = max(find_max_step(current_point_count, current_component), step);
                  if (step > 0.0)
                    {
                      min_component -= step;
                      max_component += step;
                    }
                }
              grm_args_push(subplot_args, private_name(current_range_keys->subplot), "dd", min_component,
                            max_component);
            }
          ++current_range_keys;
          ++current_component_name;
        }
    }
  /* For quiver plots use u^2 + v^2 as z value */
  if (strcmp(kind, "quiver") == 0)
    {
      double min_component = DBL_MAX;
      double max_component = -DBL_MAX;
      if (!args_values(subplot_args, "zlim", "dd", &min_component, &max_component))
        {
          args_values(subplot_args, "series", "A", &current_series);
          while (*current_series != NULL)
            {
              double current_min_component = DBL_MAX;
              double current_max_component = -DBL_MAX;
              if (!args_values(*current_series, "zrange", "dd", &current_min_component, &current_max_component))
                {
                  double *u, *v;
                  unsigned int u_length, v_length;
                  cleanup_and_set_error_if(!args_first_value(*current_series, "u", "D", &u, &u_length),
                                           ERROR_PLOT_MISSING_DATA);
                  cleanup_and_set_error_if(!args_first_value(*current_series, "v", "D", &v, &v_length),
                                           ERROR_PLOT_MISSING_DATA);
                  cleanup_and_set_error_if(u_length != v_length, ERROR_PLOT_COMPONENT_LENGTH_MISMATCH);
                  for (i = 0; i < u_length; i++)
                    {
                      double z = u[i] * u[i] + v[i] * v[i];
                      current_min_component = min(z, current_min_component);
                      current_max_component = max(z, current_max_component);
                    }
                  current_min_component = sqrt(current_min_component);
                  current_max_component = sqrt(current_max_component);
                }
              min_component = min(current_min_component, min_component);
              max_component = max(current_max_component, max_component);
              ++current_series;
            }
        }
      grm_args_push(subplot_args, "_zlim", "dd", min_component, max_component);
    }
  else if (str_equals_any(kind, 2, "imshow", "isosurface"))
    {
      /* Iterate over `x` and `y` range keys */
      current_range_keys = range_keys;
      for (i = 0; i < 2; i++)
        {
          double min_component = 0.0;
          double max_component = 1.0;
          args_values(subplot_args, current_range_keys->subplot, "dd", &min_component, &max_component);
          grm_args_push(subplot_args, private_name(current_range_keys->subplot), "dd", min_component, max_component);
          ++current_range_keys;
        }
    }
  else if (strcmp(kind, "barplot") == 0)
    {
      double x_min = 0.0, x_max = -DBL_MAX;
      if (!args_values(subplot_args, "xlim", "dd", &x_min, &x_max))
        {
          if (str_equals_any(style, 2, "lined", "stacked"))
            {
              x_max = series_count + 1;
            }
          else
            {
              args_values(subplot_args, "series", "A", &current_series);
              while (*current_series != NULL)
                {
                  double *y;
                  args_first_value(*current_series, "y", "D", &y, &current_point_count);
                  x_max = max(current_point_count + 1, x_max);
                  ++current_series;
                }
            }
        }
      grm_args_push(subplot_args, "_xlim", "dd", x_min, x_max);
    }
  else if (strcmp(kind, "hist") == 0)
    {
      double y_min = 0.0, y_max = 0.0;
      if (!args_values(subplot_args, "ylim", "dd", &y_min, &y_max))
        {
          args_values(subplot_args, "series", "A", &current_series);
          while (*current_series != NULL)
            {
              double current_y_min = DBL_MAX, current_y_max = -DBL_MAX;
              if (!args_values(*current_series, "yrange", "dd", &current_y_min, &current_y_max))
                {
                  double *x = NULL, *weights = NULL;
                  unsigned int num_bins = 0, num_weights;
                  cleanup_and_set_error_if(!args_first_value(*current_series, "x", "D", &x, &current_point_count),
                                           ERROR_PLOT_MISSING_DATA);
                  args_values(*current_series, "nbins", "i", &num_bins);
                  args_first_value(*current_series, "weights", "D", &weights, &num_weights);
                  if (weights != NULL)
                    {
                      cleanup_and_set_error_if(current_point_count != num_weights,
                                               ERROR_PLOT_COMPONENT_LENGTH_MISMATCH);
                    }
                  if (num_bins <= 1)
                    {
                      num_bins = (int)(3.3 * log10(current_point_count) + 0.5) + 1;
                    }
                  bins = malloc(num_bins * sizeof(double));
                  cleanup_and_set_error_if(bins == NULL, ERROR_MALLOC);
                  bin_data(current_point_count, x, num_bins, bins, weights);
                  for (i = 0; i < num_bins; i++)
                    {
                      current_y_min = min(current_y_min, bins[i]);
                      current_y_max = max(current_y_max, bins[i]);
                    }
                  grm_args_push(*current_series, "bins", "nD", num_bins, bins);
                  free(bins);
                  bins = NULL;
                }
              y_min = min(current_y_min, y_min);
              y_max = max(current_y_max, y_max);
              current_series++;
            }
          grm_args_push(subplot_args, "_ylim", "dd", y_min, y_max);
        }
    }
  else if (strcmp(kind, "polar_histogram") == 0)
    {
      double r_max;
      error = classes_polar_histogram(subplot_args, &r_max);
      cleanup_if_error;
      grm_args_push(subplot_args, "r_max", "d", r_max);
    }

cleanup:
  free(bins);

  return error;
}

void plot_post_plot(grm_args_t *plot_args)
{
  int update;

  logger((stderr, "Post plot processing\n"));

  args_values(plot_args, "update", "i", &update);
  logger((stderr, "Got keyword \"update\" with value %d\n", update));
  if (update)
    {
      gr_updatews();
    }
  plot_restore_text_encoding();
}

void plot_restore_text_encoding(void)
{
  gr_inqtextencoding(&pre_plot_text_encoding);
  if (pre_plot_text_encoding >= 0)
    {
      gr_settextencoding(pre_plot_text_encoding);
      pre_plot_text_encoding = -1;
    }
}

void plot_post_subplot(grm_args_t *subplot_args)
{
  const char *kind;

  logger((stderr, "Post subplot processing\n"));

  gr_restorestate();
  args_values(subplot_args, "kind", "s", &kind);
  logger((stderr, "Got keyword \"kind\" with value \"%s\"\n", kind));
  if (str_equals_any(kind, 4, "line", "step", "scatter", "stem") && grm_args_contains(subplot_args, "labels"))
    {
      plot_draw_legend(subplot_args);
    }
}

error_t plot_get_args_in_hierarchy(grm_args_t *args, const char **hierarchy_name_start_ptr, const char *key,
                                   uint_map_t *hierarchy_to_id, const grm_args_t **found_args,
                                   const char ***found_hierarchy_name_ptr)
{
  const char *key_hierarchy_name, **current_hierarchy_name_ptr;
  grm_args_t *current_args, **args_array;
  arg_t *current_arg;
  unsigned int args_array_length, current_id;

  logger((stderr, "Check hierarchy level for key \"%s\"...\n", key));
  return_error_if(!string_map_at(plot_valid_keys_map, key, (char **)&key_hierarchy_name), ERROR_PLOT_UNKNOWN_KEY);
  logger((stderr, "... got hierarchy \"%s\"\n", key_hierarchy_name));
  current_hierarchy_name_ptr = hierarchy_name_start_ptr;
  current_args = args;
  if (strcmp(*hierarchy_name_start_ptr, key_hierarchy_name) != 0)
    {
      while (*++current_hierarchy_name_ptr != NULL)
        {
          current_arg = args_at(current_args, *current_hierarchy_name_ptr);
          return_error_if(current_arg == NULL, ERROR_INTERNAL);
          arg_first_value(current_arg, "A", &args_array, &args_array_length);
          uint_map_at(hierarchy_to_id, *current_hierarchy_name_ptr, &current_id);
          /* Check for the invalid id 0 because id 0 is set for append mode */
          if (current_id == 0)
            {
              current_id = args_array_length + 1;
              if (strcmp(*current_hierarchy_name_ptr, "plots") == 0)
                {
                  int last_plot_in_use = 0;
                  if (args_values(args_array[args_array_length - 1], "in_use", "i", &last_plot_in_use) &&
                      !last_plot_in_use)
                    {
                      /* Use the last already existing plot if it is still empty */
                      --current_id;
                    }
                }
              logger((stderr, "Append mode, set id to \"%u\"\n", current_id));
              uint_map_insert(hierarchy_to_id, *current_hierarchy_name_ptr, current_id);
            }
          if (current_id > args_array_length)
            {
              plot_init_args_structure(current_args, current_hierarchy_name_ptr - 1, current_id);
              arg_first_value(current_arg, "A", &args_array, &args_array_length);
            }
          current_args = args_array[current_id - 1];
          if (strcmp(*current_hierarchy_name_ptr, "plots") == 0)
            {
              int in_use;
              error_t error = NO_ERROR;
              args_values(current_args, "in_use", "i", &in_use);
              if (in_use)
                {
                  error = event_queue_enqueue_update_plot_event(event_queue, current_id - 1);
                }
              else
                {
                  error = event_queue_enqueue_new_plot_event(event_queue, current_id - 1);
                }
              return_if_error;
              grm_args_push(current_args, "in_use", "i", 1);
            }
          if (strcmp(*current_hierarchy_name_ptr, key_hierarchy_name) == 0)
            {
              break;
            }
        }
      return_error_if(*current_hierarchy_name_ptr == NULL, ERROR_INTERNAL);
    }
  if (found_args != NULL)
    {
      *found_args = current_args;
    }
  if (found_hierarchy_name_ptr != NULL)
    {
      *found_hierarchy_name_ptr = current_hierarchy_name_ptr;
    }

  return NO_ERROR;
}


/* ~~~~~~~~~~~~~~~~~~~~~~~~~ plotting ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

error_t plot_line(grm_args_t *subplot_args)
{
  grm_args_t **current_series;
  error_t error;
  char *kind;

  kind = "line";
  args_values(subplot_args, "series", "A", &current_series);
  args_values(subplot_args, "kind", "s", &kind);
  while (*current_series != NULL)
    {
      double *x, *y;
      unsigned int x_length, y_length;
      char *spec;
      int mask;
      return_error_if(!args_first_value(*current_series, "x", "D", &x, &x_length), ERROR_PLOT_MISSING_DATA);
      return_error_if(!args_first_value(*current_series, "y", "D", &y, &y_length), ERROR_PLOT_MISSING_DATA);
      return_error_if(x_length != y_length, ERROR_PLOT_COMPONENT_LENGTH_MISMATCH);
      args_values(*current_series, "spec", "s", &spec); /* `spec` is always set */
      mask = gr_uselinespec(spec);
      if (int_equals_any(mask, 5, 0, 1, 3, 4, 5))
        {
          gr_polyline(x_length, x, y);
        }
      if (mask & 2)
        {
          gr_polymarker(x_length, x, y);
        }
      error = plot_draw_errorbars(*current_series, x, x_length, y, kind);
      return_if_error;
      ++current_series;
    }

  return NO_ERROR;
}

error_t plot_step(grm_args_t *subplot_args)
{
  /*
   * Parameters:
   * `x` as double array
   * `y` as double array
   * optional step position `step_where` as string, modes: `pre`, `mid`, `post`, Default: `mid`
   * optional `spec`
   */
  grm_args_t **current_series;

  args_values(subplot_args, "series", "A", &current_series);
  while (*current_series != NULL)
    {
      double *x, *y, *x_step_boundaries = NULL, *y_step_values = NULL;
      unsigned int x_length, y_length, mask, i;
      char *spec;
      return_error_if(!args_first_value(*current_series, "x", "D", &x, &x_length) && x_length < 1,
                      ERROR_PLOT_MISSING_DATA);
      return_error_if(!args_first_value(*current_series, "y", "D", &y, &y_length), ERROR_PLOT_MISSING_DATA);
      return_error_if(x_length != y_length, ERROR_PLOT_COMPONENT_LENGTH_MISMATCH);
      args_values(*current_series, "spec", "s", &spec); /* `spec` is always set */
      mask = gr_uselinespec(spec);
      if (int_equals_any(mask, 5, 0, 1, 3, 4, 5))
        {
          const char *where;
          args_values(*current_series, "step_where", "s", &where); /* `spec` is always set */
          if (strcmp(where, "pre") == 0)
            {
              x_step_boundaries = calloc(2 * x_length - 1, sizeof(double));
              y_step_values = calloc(2 * x_length - 1, sizeof(double));
              x_step_boundaries[0] = x[0];
              for (i = 1; i < 2 * x_length - 2; i += 2)
                {
                  x_step_boundaries[i] = x[i / 2];
                  x_step_boundaries[i + 1] = x[i / 2 + 1];
                }
              y_step_values[0] = y[0];
              for (i = 1; i < 2 * x_length - 1; i += 2)
                {
                  y_step_values[i] = y_step_values[i + 1] = y[i / 2 + 1];
                }
              gr_polyline(2 * x_length - 1, x_step_boundaries, y_step_values);
            }
          else if (strcmp(where, "post") == 0)
            {
              x_step_boundaries = calloc(2 * x_length - 1, sizeof(double));
              y_step_values = calloc(2 * x_length - 1, sizeof(double));
              for (i = 0; i < 2 * x_length - 2; i += 2)
                {
                  x_step_boundaries[i] = x[i / 2];
                  x_step_boundaries[i + 1] = x[i / 2 + 1];
                }
              x_step_boundaries[2 * x_length - 2] = x[x_length - 1];
              for (i = 0; i < 2 * x_length - 2; i += 2)
                {
                  y_step_values[i] = y_step_values[i + 1] = y[i / 2];
                }
              y_step_values[2 * x_length - 2] = y[x_length - 1];
              gr_polyline(2 * x_length - 1, x_step_boundaries, y_step_values);
            }
          else if (strcmp(where, "mid") == 0)
            {
              x_step_boundaries = calloc(2 * x_length, sizeof(double));
              y_step_values = calloc(2 * x_length, sizeof(double));
              x_step_boundaries[0] = x[0];
              for (i = 1; i < 2 * x_length - 2; i += 2)
                {
                  x_step_boundaries[i] = x_step_boundaries[i + 1] = (x[i / 2] + x[i / 2 + 1]) / 2.0;
                }
              x_step_boundaries[2 * x_length - 1] = x[x_length - 1];
              for (i = 0; i < 2 * x_length - 1; i += 2)
                {
                  y_step_values[i] = y_step_values[i + 1] = y[i / 2];
                }
              gr_polyline(2 * x_length, x_step_boundaries, y_step_values);
            }
          free(x_step_boundaries);
          free(y_step_values);
        }
      if (mask & 2)
        {
          gr_polymarker(x_length, x, y);
        }
      ++current_series;
    }

  return NO_ERROR;
}

error_t plot_scatter(grm_args_t *subplot_args)
{
  /*
   * Parameters:
   * `x` as double array
   * `y` as double array
   * optional marker size `z` as double array
   * optional marker color `c` as double array for each marker or as single integer for all markers
   * optional `markertype` as integer (see: [Marker types](https://gr-framework.org/markertypes.html?highlight=marker))
   */
  grm_args_t **current_series;
  error_t error;
  char *kind;
  int *previous_marker_type = plot_scatter_markertypes;
  args_values(subplot_args, "series", "A", &current_series);
  args_values(subplot_args, "kind", "s", &kind);
  while (*current_series != NULL)
    {
      double *x = NULL, *y = NULL, *z = NULL, *c = NULL, c_min, c_max;
      unsigned int x_length, y_length, z_length, c_length;
      int i, c_index = -1, markertype;
      return_error_if(!args_first_value(*current_series, "x", "D", &x, &x_length), ERROR_PLOT_MISSING_DATA);
      return_error_if(!args_first_value(*current_series, "y", "D", &y, &y_length), ERROR_PLOT_MISSING_DATA);
      return_error_if(x_length != y_length, ERROR_PLOT_COMPONENT_LENGTH_MISMATCH);
      if (args_first_value(*current_series, "z", "D", &z, &z_length))
        {
          return_error_if(x_length != z_length, ERROR_PLOT_COMPONENT_LENGTH_MISMATCH);
        }
      if (args_values(*current_series, "markertype", "i", &markertype))
        {
          gr_setmarkertype(markertype);
        }
      else
        {
          gr_setmarkertype(*previous_marker_type++);
          if (*previous_marker_type == INT_MAX)
            {
              previous_marker_type = plot_scatter_markertypes;
            }
        }
      if (!args_first_value(*current_series, "c", "D", &c, &c_length) &&
          args_values(*current_series, "c", "i", &c_index))
        {
          if (c_index < 0)
            {
              logger((stderr, "Invalid scatter color %d, using 0 instead\n", c_index));
              c_index = 0;
            }
          else if (c_index > 255)
            {
              logger((stderr, "Invalid scatter color %d, using 255 instead\n", c_index));
              c_index = 255;
            }
        }
      if (z != NULL || c != NULL)
        {
          args_values(subplot_args, "_clim", "dd", &c_min, &c_max);
          for (i = 0; i < x_length; i++)
            {
              if (z != NULL)
                {
                  if (i < z_length)
                    {
                      gr_setmarkersize(z[i] / 100.0);
                    }
                  else
                    {
                      gr_setmarkersize(2.0);
                    }
                }
              if (c != NULL)
                {
                  if (i < c_length)
                    {
                      c_index = 1000 + (int)(255 * (c[i] - c_min) / (c_max - c_min));
                      if (c_index < 1000 || c_index > 1255)
                        {
                          continue;
                        }
                    }
                  else
                    {
                      c_index = 989;
                    }
                  gr_setmarkercolorind(c_index);
                }
              else if (c_index != -1)
                {
                  gr_setmarkercolorind(1000 + c_index);
                }
              gr_polymarker(1, &x[i], &y[i]);
            }
        }
      else
        {
          gr_polymarker(x_length, x, y);
        }
      error = plot_draw_errorbars(*current_series, x, x_length, y, kind);
      return_if_error;
      ++current_series;
    }

  return NO_ERROR;
}

error_t plot_quiver(grm_args_t *subplot_args)
{
  grm_args_t **current_series;
  error_t error = NO_ERROR;

  args_values(subplot_args, "series", "A", &current_series);
  while (*current_series != NULL)
    {
      double *x = NULL, *y = NULL, *u = NULL, *v = NULL;
      unsigned int x_length, y_length, u_length, v_length;
      return_error_if(!args_first_value(*current_series, "x", "D", &x, &x_length), ERROR_PLOT_MISSING_DATA);
      return_error_if(!args_first_value(*current_series, "y", "D", &y, &y_length), ERROR_PLOT_MISSING_DATA);
      return_error_if(!args_first_value(*current_series, "u", "D", &u, &u_length), ERROR_PLOT_MISSING_DATA);
      return_error_if(!args_first_value(*current_series, "v", "D", &v, &v_length), ERROR_PLOT_MISSING_DATA);
      return_error_if(x_length * y_length != u_length, ERROR_PLOT_COMPONENT_LENGTH_MISMATCH);
      return_error_if(x_length * y_length != v_length, ERROR_PLOT_COMPONENT_LENGTH_MISMATCH);

      gr_quiver(x_length, y_length, x, y, u, v, 1);

      ++current_series;
    }
  error = plot_draw_colorbar(subplot_args, 0.05, 256);

  return error;
}

error_t plot_stem(grm_args_t *subplot_args)
{
  const double *window;
  double base_line_y[2] = {0.0, 0.0};
  double stem_x[2], stem_y[2] = {0.0};
  grm_args_t **current_series;

  args_values(subplot_args, "window", "D", &window);
  args_values(subplot_args, "series", "A", &current_series);
  while (*current_series != NULL)
    {
      double *x, *y;
      unsigned int x_length, y_length;
      char *spec;
      unsigned int i;
      return_error_if(!args_first_value(*current_series, "x", "D", &x, &x_length), ERROR_PLOT_MISSING_DATA);
      return_error_if(!args_first_value(*current_series, "y", "D", &y, &y_length), ERROR_PLOT_MISSING_DATA);
      return_error_if(x_length != y_length, ERROR_PLOT_COMPONENT_LENGTH_MISMATCH);
      gr_polyline(2, (double *)window, base_line_y);
      gr_setmarkertype(GKS_K_MARKERTYPE_SOLID_CIRCLE);
      args_values(*current_series, "spec", "s", &spec);
      gr_uselinespec(spec);
      for (i = 0; i < x_length; ++i)
        {
          stem_x[0] = stem_x[1] = x[i];
          stem_y[1] = y[i];
          gr_polyline(2, stem_x, stem_y);
        }
      gr_polymarker(x_length, x, y);
      ++current_series;
    }

  return NO_ERROR;
}

error_t plot_hist(grm_args_t *subplot_args)
{
  char *kind;
  grm_args_t **current_series;
  double *bar_centers = NULL;
  int bar_color_index = 989, i;
  double bar_color_rgb[3] = {-1};
  error_t error = NO_ERROR;

  args_values(subplot_args, "kind", "s", &kind);
  args_values(subplot_args, "series", "A", &current_series);
  args_values(subplot_args, "bar_color", "ddd", &bar_color_rgb[0], &bar_color_rgb[1], &bar_color_rgb[2]);
  args_values(subplot_args, "bar_color", "i", &bar_color_index);
  if (bar_color_rgb[0] != -1)
    {
      for (i = 0; i < 3; i++)
        {
          cleanup_and_set_error_if((bar_color_rgb[i] > 1 || bar_color_rgb[i] < 0), ERROR_PLOT_OUT_OF_RANGE);
        }
      bar_color_index = 1000;
      gr_setcolorrep(bar_color_index, bar_color_rgb[0], bar_color_rgb[1], bar_color_rgb[2]);
    }

  while (*current_series != NULL)
    {
      double edge_color_index = 1;
      double edge_color_rgb[3] = {-1};
      double x_min, x_max, bar_width;
      double *bins;
      unsigned int num_bins;

      args_values(*current_series, "edge_color", "ddd", &edge_color_rgb[0], &edge_color_rgb[1], &edge_color_rgb[2]);
      args_values(*current_series, "edge_color", "i", &edge_color_index);
      if (edge_color_rgb[0] != -1)
        {
          for (i = 0; i < 3; i++)
            {
              cleanup_and_set_error_if((edge_color_rgb[i] > 1 || edge_color_rgb[i] < 0), ERROR_PLOT_OUT_OF_RANGE);
            }
          edge_color_index = 1001;
          gr_setcolorrep(edge_color_index, edge_color_rgb[0], edge_color_rgb[1], edge_color_rgb[2]);
        }

      args_first_value(*current_series, "bins", "D", &bins, &num_bins);
      args_values(*current_series, "xrange", "dd", &x_min, &x_max);

      bar_width = (x_max - x_min) / num_bins;
      for (i = 1; i < num_bins + 1; ++i)
        {
          double x = x_min + (i - 1) * bar_width;
          gr_setfillcolorind(bar_color_index);
          gr_setfillintstyle(GKS_K_INTSTYLE_SOLID);
          gr_fillrect(x, x + bar_width, 0., bins[i - 1]);
          gr_setfillcolorind(edge_color_index);
          gr_setfillintstyle(GKS_K_INTSTYLE_HOLLOW);
          gr_fillrect(x, x + bar_width, 0., bins[i - 1]);
        }
      if (grm_args_contains(*current_series, "error"))
        {
          bar_centers = malloc(num_bins * sizeof(double));
          cleanup_and_set_error_if(bar_centers == NULL, ERROR_MALLOC);
          linspace(x_min + 0.5 * bar_width, x_max - 0.5 * bar_width, num_bins, bar_centers);
          error = plot_draw_errorbars(*current_series, bar_centers, num_bins, bins, kind);
          cleanup_if_error;
          free(bar_centers);
          bar_centers = NULL;
        }
      ++current_series;
    }

cleanup:
  free(bar_centers);

  return error;
}

error_t plot_barplot(grm_args_t *subplot_args)
{
  grm_args_t **current_series;
  grm_args_t **inner_series;
  unsigned int inner_series_length;

  /* Default */
  int bar_color = 989, edge_color = 1;
  double bar_color_rgb[3] = {-1};
  double edge_color_rgb[3] = {-1};
  double bar_width = 1, edge_width = 1;
  const char *style = "default";
  int *c = NULL;
  unsigned int c_length;
  double *c_rgb = NULL;
  unsigned int c_rgb_length;
  double *y;
  unsigned int y_length;
  unsigned int fixed_y_length;
  grm_args_t **ind_bar_color = NULL;
  double(*pos_ind_bar_color)[3] = NULL;
  grm_args_t **ind_edge_color = NULL;
  double(*pos_ind_edge_color)[3] = NULL;
  grm_args_t **ind_edge_width = NULL;
  double *pos_ind_edge_width = NULL;
  int series_index = 0;
  double wfac;
  int len_std_colors = 20;
  int std_colors[20] = {989, 982, 980, 981, 996, 983, 995, 988, 986, 990,
                        991, 984, 992, 993, 994, 987, 985, 997, 998, 999};
  int color_save_spot = 1000;
  int change_bar_color = 0;
  int change_edge_color = 0;
  int change_edge_width = 0;
  unsigned int i;
  error_t error = NO_ERROR;

  args_values(subplot_args, "series", "A", &current_series);
  args_values(subplot_args, "bar_color", "ddd", &bar_color_rgb[0], &bar_color_rgb[1], &bar_color_rgb[2]);
  args_values(subplot_args, "bar_color", "i", &bar_color);
  args_values(subplot_args, "bar_width", "d", &bar_width);
  args_values(subplot_args, "style", "s", &style);
  args_first_value(*current_series, "c", "I", &c, &c_length);
  args_first_value(*current_series, "c", "D", &c_rgb, &c_rgb_length);
  if (bar_color_rgb[0] != -1)
    {
      for (i = 0; i < 3; i++)
        {
          cleanup_and_set_error_if((bar_color_rgb[i] > 1 || bar_color_rgb[i] < 0), ERROR_PLOT_OUT_OF_RANGE);
        }
    }

  /* ind_parameter */
  /* determine the length of y */
  if (!args_first_value(*current_series, "y", "D", &y, &fixed_y_length))
    {
      unsigned int temp_y_length = 0;
      cleanup_and_set_error_if(
          !args_first_value(*current_series, "inner_series", "A", &inner_series, &inner_series_length),
          ERROR_PLOT_MISSING_DATA);
      while (inner_series != NULL)
        {
          args_first_value(*inner_series, "y", "D", &y, &y_length);
          temp_y_length += fixed_y_length;
        }
      fixed_y_length = temp_y_length;
    }
  /* ind_bar_color */
  if (args_values(subplot_args, "ind_bar_color", "A", &ind_bar_color))
    {
      pos_ind_bar_color = malloc(3 * fixed_y_length * sizeof(double));
      cleanup_and_set_error_if(pos_ind_bar_color == NULL, ERROR_MALLOC);
      change_bar_color = 1;
      for (i = 0; i < fixed_y_length; ++i)
        {
          pos_ind_bar_color[i][0] = -1;
        }
      while (*ind_bar_color != NULL)
        {
          int *indices = NULL;
          unsigned int indices_length;
          int index;
          double rgb[3];
          unsigned int j;
          cleanup_and_set_error_if(!(args_first_value(*ind_bar_color, "indices", "I", &indices, &indices_length) ||
                                     args_values(*ind_bar_color, "indices", "i", &index)),
                                   ERROR_PLOT_MISSING_DATA);
          cleanup_and_set_error_if(!args_values(*ind_bar_color, "rgb", "ddd", &rgb[0], &rgb[1], &rgb[2]),
                                   ERROR_PLOT_MISSING_DATA);
          for (j = 0; j < 3; j++)
            {
              cleanup_and_set_error_if((rgb[j] > 1 || rgb[j] < 0), ERROR_PLOT_OUT_OF_RANGE);
            }

          if (indices != NULL)
            {
              for (j = 0; j < indices_length; ++j)
                {
                  cleanup_and_set_error_if(indices[j] - 1 >= (int)fixed_y_length, ERROR_PLOT_COMPONENT_LENGTH_MISMATCH);
                  pos_ind_bar_color[indices[j] - 1][0] = rgb[0];
                  pos_ind_bar_color[indices[j] - 1][1] = rgb[1];
                  pos_ind_bar_color[indices[j] - 1][2] = rgb[2];
                }
            }
          else
            {
              cleanup_and_set_error_if(index - 1 >= (int)fixed_y_length, ERROR_PLOT_COMPONENT_LENGTH_MISMATCH);
              pos_ind_bar_color[index - 1][0] = rgb[0];
              pos_ind_bar_color[index - 1][1] = rgb[1];
              pos_ind_bar_color[index - 1][2] = rgb[2];
            }
          ind_bar_color++;
        }
    }

  /* ind_edge_color */
  if (args_values(subplot_args, "ind_edge_color", "A", &ind_edge_color))
    {
      pos_ind_edge_color = malloc(3 * fixed_y_length * sizeof(double));
      cleanup_and_set_error_if(pos_ind_edge_color == NULL, ERROR_MALLOC);
      change_edge_color = 1;
      for (i = 0; i < fixed_y_length; ++i)
        {
          pos_ind_edge_color[i][0] = -1;
        }
      while (*ind_edge_color != NULL)
        {
          int *indices = NULL;
          unsigned int indices_length;
          int index;
          double rgb[3];
          unsigned int j;
          cleanup_and_set_error_if(!(args_first_value(*ind_edge_color, "indices", "I", &indices, &indices_length) ||
                                     args_values(*ind_edge_color, "indices", "i", &index)),
                                   ERROR_PLOT_MISSING_DATA);
          cleanup_and_set_error_if(!args_values(*ind_edge_color, "rgb", "ddd", &rgb[0], &rgb[1], &rgb[2]),
                                   ERROR_PLOT_MISSING_DATA);
          for (j = 0; j < 3; j++)
            {
              cleanup_and_set_error_if((rgb[j] > 1 || rgb[j] < 0), ERROR_PLOT_OUT_OF_RANGE);
            }

          if (indices != NULL)
            {
              for (j = 0; j < indices_length; ++j)
                {
                  cleanup_and_set_error_if(indices[j] - 1 >= (int)fixed_y_length, ERROR_PLOT_COMPONENT_LENGTH_MISMATCH);
                  pos_ind_edge_color[indices[j] - 1][0] = rgb[0];
                  pos_ind_edge_color[indices[j] - 1][1] = rgb[1];
                  pos_ind_edge_color[indices[j] - 1][2] = rgb[2];
                }
            }
          else
            {
              cleanup_and_set_error_if(index - 1 >= (int)fixed_y_length, ERROR_PLOT_COMPONENT_LENGTH_MISMATCH);
              pos_ind_edge_color[index - 1][0] = rgb[0];
              pos_ind_edge_color[index - 1][1] = rgb[1];
              pos_ind_edge_color[index - 1][2] = rgb[2];
            }
          ind_edge_color++;
        }
    }

  /* ind_edge_width */
  if (args_values(subplot_args, "ind_edge_width", "A", &ind_edge_width))
    {
      pos_ind_edge_width = malloc(sizeof(double) * fixed_y_length);
      cleanup_and_set_error_if(pos_ind_edge_width == NULL, ERROR_MALLOC);
      for (i = 0; i < fixed_y_length; ++i)
        {
          pos_ind_edge_width[i] = -1;
        }
      change_edge_width = 1;
      while (*ind_edge_width != NULL)
        {
          int *indices = NULL;
          unsigned int indices_length;
          int index;
          double width;
          unsigned int j;
          cleanup_and_set_error_if(!(args_first_value(*ind_edge_width, "indices", "I", &indices, &indices_length) ||
                                     args_values(*ind_edge_width, "indices", "i", &index)),
                                   ERROR_PLOT_MISSING_DATA);
          cleanup_and_set_error_if(!args_values(*ind_edge_width, "width", "d", &width), ERROR_PLOT_MISSING_DATA);
          cleanup_and_set_error_if(width < 0, ERROR_PLOT_OUT_OF_RANGE);

          if (indices != NULL)
            {
              for (j = 0; j < indices_length; ++j)
                {
                  cleanup_and_set_error_if(indices[j] - 1 >= (int)fixed_y_length, ERROR_PLOT_COMPONENT_LENGTH_MISMATCH);
                  pos_ind_edge_width[indices[j] - 1] = width;
                }
            }
          else
            {
              cleanup_and_set_error_if(index - 1 >= (int)fixed_y_length, ERROR_PLOT_COMPONENT_LENGTH_MISMATCH);
              pos_ind_edge_width[index - 1] = width;
            }
          ind_edge_width++;
        }
    }

  wfac = 0.9 * bar_width;
  while (*current_series != NULL)
    {
      /* Init */
      int inner_series_index;
      double *y;
      unsigned int y_length = 0;
      grm_args_t **inner_series = NULL;
      unsigned int inner_series_length = 0;
      /* Style Varianz */
      double vertical_change = 0;
      double x1, x2, y1, y2;

      args_values(*current_series, "edge_color", "ddd", &edge_color_rgb[0], &edge_color_rgb[1], &edge_color_rgb[2]);
      args_values(*current_series, "edge_color", "i", &edge_color);
      if (edge_color_rgb[0] != -1)
        {
          for (i = 0; i < 3; i++)
            {
              cleanup_and_set_error_if((edge_color_rgb[i] > 1 || edge_color_rgb[i] < 0), ERROR_PLOT_OUT_OF_RANGE);
            }
        }
      args_values(*current_series, "edge_width", "d", &edge_width);

      cleanup_and_set_error_if(
          !(args_first_value(*current_series, "y", "D", &y, &y_length) ||
            (args_first_value(*current_series, "inner_series", "A", &inner_series, &inner_series_length))),
          ERROR_PLOT_MISSING_DATA);
      cleanup_and_set_error_if(strcmp(style, "lined") && inner_series != NULL, ERROR_UNSUPPORTED_OPERATION);
      if (c != NULL)
        {
          cleanup_and_set_error_if((c_length < y_length) && (c_length < inner_series_length),
                                   ERROR_PLOT_COMPONENT_LENGTH_MISMATCH);
        }
      if (c_rgb != NULL)
        {
          cleanup_and_set_error_if((c_rgb_length < y_length * 3) && (c_rgb_length < inner_series_length * 3),
                                   ERROR_PLOT_COMPONENT_LENGTH_MISMATCH);
          for (i = 0; i < y_length * 3; i++)
            {
              cleanup_and_set_error_if((c_rgb[i] > 1 || c_rgb[i] < 0), ERROR_PLOT_OUT_OF_RANGE);
            }
        }

      gr_setfillintstyle(1);
      gr_setfillcolorind(bar_color);
      /* Draw Bar */
      for (i = 0; i < y_length; i++)
        {
          if (strcmp(style, "default") != 0)
            {
              int color_index = i % len_std_colors;
              gr_setfillcolorind(std_colors[color_index]);
            }
          if (bar_color_rgb[0] != -1)
            {
              gr_setcolorrep(color_save_spot, bar_color_rgb[0], bar_color_rgb[1], bar_color_rgb[2]);
              bar_color = color_save_spot;
              gr_setfillcolorind(bar_color);
            }
          if (c != NULL)
            {
              gr_setfillcolorind(c[i]);
            }
          else if (c_rgb != NULL)
            {
              gr_setcolorrep(color_save_spot, c_rgb[i * 3], c_rgb[i * 3 + 1], c_rgb[i * 3 + 2]);
              gr_setfillcolorind(color_save_spot);
            }
          else if (change_bar_color)
            {
              if (*pos_ind_bar_color[i] != -1)
                {
                  gr_setcolorrep(color_save_spot, pos_ind_bar_color[i][0], pos_ind_bar_color[i][1],
                                 pos_ind_bar_color[i][2]);
                  gr_setfillcolorind(color_save_spot);
                }
            }
          if (strcmp(style, "default") == 0)
            {
              x1 = i + 1 - 0.5 * bar_width;
              x2 = i + 1 + 0.5 * bar_width;
              y1 = 0;
              y2 = y[i];
            }
          if (strcmp(style, "stacked") == 0)
            {
              x1 = series_index + 1 - 0.5 * bar_width;
              x2 = series_index + 1 + 0.5 * bar_width;
              y1 = 0 + vertical_change;
              y2 = y[i] + vertical_change;
              vertical_change += y[i];
            }
          if (strcmp(style, "lined") == 0)
            {
              bar_width = wfac / y_length;
              x1 = series_index + 1 - 0.5 * wfac + bar_width * i;
              x2 = series_index + 1 - 0.5 * wfac + bar_width + bar_width * i;
              y1 = 0;
              y2 = y[i];
            }
          gr_fillrect(x1, x2, y1, y2);
        }

      /* Draw Bars from inner_series */
      for (inner_series_index = 0; inner_series_index < inner_series_length; inner_series_index++)
        {
          int *inner_c = NULL;
          unsigned int inner_c_length;
          double *inner_c_rgb = NULL;
          unsigned int inner_c_rgb_length;
          gr_setfillcolorind(std_colors[inner_series_index % len_std_colors]);
          args_first_value(inner_series[inner_series_index], "y", "D", &y, &y_length);
          bar_width = wfac / fixed_y_length;
          if (c != NULL)
            {
              gr_setfillcolorind(c[inner_series_index]);
            }
          else if (c_rgb != NULL)
            {
              gr_setcolorrep(color_save_spot, c_rgb[inner_series_index * 3], c_rgb[inner_series_index * 3 + 1],
                             c_rgb[inner_series_index * 3 + 2]);
              gr_setfillcolorind(color_save_spot);
            }
          else if (change_bar_color)
            {
              if (*pos_ind_bar_color[inner_series_index] != -1)
                {
                  gr_setcolorrep(color_save_spot, pos_ind_bar_color[inner_series_index][0],
                                 pos_ind_bar_color[inner_series_index][1], pos_ind_bar_color[inner_series_index][2]);
                  gr_setfillcolorind(color_save_spot);
                }
            }
          if (args_first_value(inner_series[inner_series_index], "c", "I", &inner_c, &inner_c_length))
            {
              cleanup_and_set_error_if(inner_c_length != y_length, ERROR_PLOT_COMPONENT_LENGTH_MISMATCH);
            }
          if (args_first_value(inner_series[inner_series_index], "c", "D", &inner_c_rgb, &inner_c_rgb_length))
            {
              cleanup_and_set_error_if(inner_c_rgb_length != 3 * y_length, ERROR_PLOT_COMPONENT_LENGTH_MISMATCH);
              for (i = 0; i < y_length * 3; i++)
                {
                  cleanup_and_set_error_if((inner_c_rgb[i] > 1 || inner_c_rgb[i] < 0), ERROR_PLOT_OUT_OF_RANGE);
                }
            }
          for (i = 0; i < y_length; i++)
            {
              if (inner_c != NULL)
                {
                  gr_setfillcolorind(inner_c[i]);
                }
              if (inner_c_rgb != NULL)
                {
                  gr_setcolorrep(color_save_spot, inner_c_rgb[i * 3], inner_c_rgb[i * 3 + 1], inner_c_rgb[i * 3 + 2]);
                  gr_setfillcolorind(color_save_spot);
                }
              x1 = series_index + 1 - 0.5 * wfac + bar_width * inner_series_index;
              x2 = series_index + 1 - 0.5 * wfac + bar_width + bar_width * inner_series_index;
              y1 = 0 + vertical_change;
              y2 = y[i] + vertical_change;
              vertical_change += y[i];
              gr_fillrect(x1, x2, y1, y2);
            }
          y_length = 0;
          vertical_change = 0;
        }

      vertical_change = 0;
      /* Draw Edge */
      for (i = 0; i < y_length; i++)
        {
          gr_setlinewidth(edge_width);
          if (change_edge_width)
            {
              if (pos_ind_edge_width[i] != -1)
                {
                  double width = pos_ind_edge_width[i];
                  gr_setlinewidth(width);
                }
            }
          if (edge_color_rgb[0] != -1)
            {
              gr_setcolorrep(color_save_spot, edge_color_rgb[0], edge_color_rgb[1], edge_color_rgb[2]);
              edge_color = color_save_spot;
            }
          gr_setlinecolorind(edge_color);
          if (change_edge_color)
            {
              if (*pos_ind_edge_color[i] != -1)
                {
                  gr_setcolorrep(color_save_spot, pos_ind_edge_color[i][0], pos_ind_edge_color[i][1],
                                 pos_ind_edge_color[i][2]);
                  gr_setlinecolorind(color_save_spot);
                }
            }
          if (strcmp(style, "default") == 0)
            {
              x1 = i + 1 - 0.5 * bar_width;
              x2 = i + 1 + 0.5 * bar_width;
              y1 = 0;
              y2 = y[i];
            }
          if (strcmp(style, "stacked") == 0)
            {
              x1 = series_index + 1 - 0.5 * bar_width;
              x2 = series_index + 1 + 0.5 * bar_width;
              y1 = 0 + vertical_change;
              y2 = y[i] + vertical_change;
              vertical_change += y[i];
            }
          if (strcmp(style, "lined") == 0)
            {
              bar_width = wfac / y_length;
              x1 = series_index + 1 - 0.5 * wfac + bar_width * i;
              x2 = series_index + 1 - 0.5 * wfac + bar_width + bar_width * i;
              y1 = 0;
              y2 = y[i];
            }
          gr_drawrect(x1, x2, y1, y2);
        }

      /* Draw Edges from inner_series */
      for (inner_series_index = 0; inner_series_index < inner_series_length; inner_series_index++)
        {
          gr_setlinewidth(edge_width);
          if (change_edge_width)
            {
              if (pos_ind_edge_width[inner_series_index] != -1)
                {
                  double width = pos_ind_edge_width[inner_series_index];
                  gr_setlinewidth(width);
                }
            }
          if (edge_color_rgb[0] != -1)
            {
              gr_setcolorrep(color_save_spot, edge_color_rgb[0], edge_color_rgb[1], edge_color_rgb[2]);
              edge_color = color_save_spot;
            }
          gr_setlinecolorind(edge_color);
          if (change_edge_color)
            {
              if (*pos_ind_edge_color[inner_series_index] != -1)
                {
                  gr_setcolorrep(color_save_spot, pos_ind_edge_color[inner_series_index][0],
                                 pos_ind_edge_color[inner_series_index][1], pos_ind_edge_color[inner_series_index][2]);
                  gr_setlinecolorind(color_save_spot);
                }
            }

          args_first_value(inner_series[inner_series_index], "y", "D", &y, &y_length);
          bar_width = wfac / fixed_y_length;
          for (i = 0; i < y_length; i++)
            {
              gr_setfillcolorind(std_colors[inner_series_index % len_std_colors]);
              x1 = series_index + 1 - 0.5 * wfac + bar_width * inner_series_index;
              x2 = series_index + 1 - 0.5 * wfac + bar_width + bar_width * inner_series_index;
              y1 = 0 + vertical_change;
              y2 = y[i] + vertical_change;
              vertical_change += y[i];
              gr_drawrect(x1, x2, y1, y2);
            }
          y_length = 0;
          vertical_change = 0;
        }

      series_index++;
      ++current_series;
    }

cleanup:
  if (pos_ind_bar_color != NULL)
    {
      free(pos_ind_bar_color);
    }
  if (pos_ind_edge_color != NULL)
    {
      free(pos_ind_edge_color);
    }
  if (pos_ind_edge_width != NULL)
    {
      free(pos_ind_edge_width);
    }

  return error;
}

error_t plot_contour(grm_args_t *subplot_args)
{
  double z_min, z_max;
  int num_levels;
  double *h;
  double *gridit_x = NULL, *gridit_y = NULL, *gridit_z = NULL;
  grm_args_t **current_series;
  int i;
  error_t error = NO_ERROR;

  args_values(subplot_args, "_zlim", "dd", &z_min, &z_max);
  gr_setspace(z_min, z_max, 0, 90);
  args_values(subplot_args, "levels", "i", &num_levels);
  h = malloc(num_levels * sizeof(double));
  if (h == NULL)
    {
      debug_print_malloc_error();
      error = ERROR_MALLOC;
      goto cleanup;
    }
  args_values(subplot_args, "series", "A", &current_series);
  while (*current_series != NULL)
    {
      double *x, *y, *z;
      unsigned int x_length, y_length, z_length;
      args_first_value(*current_series, "x", "D", &x, &x_length);
      args_first_value(*current_series, "y", "D", &y, &y_length);
      args_first_value(*current_series, "z", "D", &z, &z_length);
      if (x_length == y_length && x_length == z_length)
        {
          if (gridit_x == NULL)
            {
              gridit_x = malloc(PLOT_CONTOUR_GRIDIT_N * sizeof(double));
              gridit_y = malloc(PLOT_CONTOUR_GRIDIT_N * sizeof(double));
              gridit_z = malloc(PLOT_CONTOUR_GRIDIT_N * PLOT_CONTOUR_GRIDIT_N * sizeof(double));
              if (gridit_x == NULL || gridit_y == NULL || gridit_z == NULL)
                {
                  debug_print_malloc_error();
                  error = ERROR_MALLOC;
                  goto cleanup;
                }
            }
          gr_gridit(x_length, x, y, z, PLOT_CONTOUR_GRIDIT_N, PLOT_CONTOUR_GRIDIT_N, gridit_x, gridit_y, gridit_z);
          for (i = 0; i < PLOT_CONTOUR_GRIDIT_N * PLOT_CONTOUR_GRIDIT_N; i++)
            {
              z_min = min(gridit_z[i], z_min);
              z_max = max(gridit_z[i], z_max);
            }
          for (i = 0; i < num_levels; ++i)
            {
              h[i] = z_min + (1.0 * i) / num_levels * (z_max - z_min);
            }
          gr_contour(PLOT_CONTOUR_GRIDIT_N, PLOT_CONTOUR_GRIDIT_N, num_levels, gridit_x, gridit_y, h, gridit_z, 1000);
        }
      else
        {
          if (x_length * y_length != z_length)
            {
              error = ERROR_PLOT_COMPONENT_LENGTH_MISMATCH;
              goto cleanup;
            }
          for (i = 0; i < num_levels; ++i)
            {
              h[i] = z_min + (1.0 * i) / num_levels * (z_max - z_min);
            }
          gr_contour(x_length, y_length, num_levels, x, y, h, z, 1000);
        }
      ++current_series;
    }
  if ((error = plot_draw_colorbar(subplot_args, 0.0, num_levels)) != NO_ERROR)
    {
      goto cleanup;
    }

cleanup:
  free(h);
  free(gridit_x);
  free(gridit_y);
  free(gridit_z);

  return error;
}

error_t plot_contourf(grm_args_t *subplot_args)
{
  double z_min, z_max;
  int num_levels, scale;
  double *h;
  double *gridit_x = NULL, *gridit_y = NULL, *gridit_z = NULL;
  grm_args_t **current_series;
  int i;
  error_t error = NO_ERROR;

  args_values(subplot_args, "_zlim", "dd", &z_min, &z_max);
  gr_setspace(z_min, z_max, 0, 90);
  args_values(subplot_args, "levels", "i", &num_levels);
  h = malloc(num_levels * sizeof(double));
  if (h == NULL)
    {
      debug_print_malloc_error();
      error = ERROR_MALLOC;
      goto cleanup;
    }
  args_values(subplot_args, "scale", "i", &scale);
  gr_setscale(scale);
  args_values(subplot_args, "series", "A", &current_series);
  while (*current_series != NULL)
    {
      double *x, *y, *z;
      unsigned int x_length, y_length, z_length;
      args_first_value(*current_series, "x", "D", &x, &x_length);
      args_first_value(*current_series, "y", "D", &y, &y_length);
      args_first_value(*current_series, "z", "D", &z, &z_length);
      if ((error = plot_draw_colorbar(subplot_args, 0.0, num_levels)) != NO_ERROR)
        {
          goto cleanup;
        }
      gr_setlinecolorind(1);
      if (x_length == y_length && x_length == z_length)
        {
          if (gridit_x == NULL)
            {
              gridit_x = malloc(PLOT_CONTOUR_GRIDIT_N * sizeof(double));
              gridit_y = malloc(PLOT_CONTOUR_GRIDIT_N * sizeof(double));
              gridit_z = malloc(PLOT_CONTOUR_GRIDIT_N * PLOT_CONTOUR_GRIDIT_N * sizeof(double));
              if (gridit_x == NULL || gridit_y == NULL || gridit_z == NULL)
                {
                  debug_print_malloc_error();
                  error = ERROR_MALLOC;
                  goto cleanup;
                }
            }
          gr_gridit(x_length, x, y, z, PLOT_CONTOUR_GRIDIT_N, PLOT_CONTOUR_GRIDIT_N, gridit_x, gridit_y, gridit_z);
          for (i = 0; i < PLOT_CONTOUR_GRIDIT_N * PLOT_CONTOUR_GRIDIT_N; i++)
            {
              z_min = min(gridit_z[i], z_min);
              z_max = max(gridit_z[i], z_max);
            }
          for (i = 0; i < num_levels; ++i)
            {
              h[i] = z_min + (1.0 * i) / num_levels * (z_max - z_min);
            }
          gr_contourf(PLOT_CONTOUR_GRIDIT_N, PLOT_CONTOUR_GRIDIT_N, num_levels, gridit_x, gridit_y, h, gridit_z, 1000);
        }
      else
        {
          if (x_length * y_length != z_length)
            {
              error = ERROR_PLOT_COMPONENT_LENGTH_MISMATCH;
              goto cleanup;
            }
          for (i = 0; i < num_levels; ++i)
            {
              h[i] = z_min + (1.0 * i) / num_levels * (z_max - z_min);
            }
          gr_contourf(x_length, y_length, num_levels, x, y, h, z, 1000);
        }
      ++current_series;
    }

cleanup:
  free(h);
  free(gridit_x);
  free(gridit_y);
  free(gridit_z);

  return error;
}

error_t plot_hexbin(grm_args_t *subplot_args)
{
  grm_args_t **current_series;

  args_values(subplot_args, "series", "A", &current_series);
  while (*current_series != NULL)
    {
      double *x, *y;
      unsigned int x_length, y_length;
      int cntmax, nbins;
      return_error_if(!args_first_value(*current_series, "x", "D", &x, &x_length), ERROR_PLOT_MISSING_DATA);
      return_error_if(!args_first_value(*current_series, "y", "D", &y, &y_length), ERROR_PLOT_MISSING_DATA);
      return_error_if(x_length != y_length, ERROR_PLOT_COMPONENT_LENGTH_MISMATCH);
      args_values(*current_series, "nbins", "i", &nbins);
      cntmax = gr_hexbin(x_length, x, y, nbins);
      /* TODO: return an error in the else case? */
      if (cntmax > 0)
        {
          grm_args_push(subplot_args, "_zlim", "dd", 0.0, 1.0 * cntmax);
          plot_draw_colorbar(subplot_args, 0.0, 256);
        }
      ++current_series;
    }

  return NO_ERROR;
}

error_t plot_heatmap(grm_args_t *subplot_args)
{
  const char *kind = NULL;
  grm_args_t **current_series;
  int icmap[256], *rgba = NULL, *data = NULL, zlog = 0;
  unsigned int i, j, cols, rows, z_length;
  double *x = NULL, *y = NULL, *z, x_min, x_max, y_min, y_max, z_min, z_max, c_min, c_max, zv;
  error_t error = NO_ERROR;

  args_values(subplot_args, "series", "A", &current_series);
  args_values(subplot_args, "kind", "s", &kind);
  args_values(subplot_args, "zlog", "i", &zlog);
  while (*current_series != NULL)
    {
      int is_uniform_heatmap;
      args_first_value(*current_series, "x", "D", &x, &cols);
      args_first_value(*current_series, "y", "D", &y, &rows);
      is_uniform_heatmap = is_equidistant_array(cols, x) && is_equidistant_array(rows, y);
      cleanup_and_set_error_if(!is_uniform_heatmap && (x == NULL || y == NULL), ERROR_PLOT_MISSING_DATA);
      cleanup_and_set_error_if(!args_first_value(*current_series, "z", "D", &z, &z_length), ERROR_PLOT_MISSING_DATA);
      if (x == NULL && y == NULL)
        {
          /* If neither `x` nor `y` are given, we need more information about the shape of `z` */
          cleanup_and_set_error_if(!args_values(*current_series, "z_dims", "ii", &rows, &cols),
                                   ERROR_PLOT_MISSING_DIMENSIONS);
        }
      else if (x == NULL)
        {
          cols = z_length / rows;
        }
      else if (y == NULL)
        {
          rows = z_length / cols;
        }
      if (x == NULL)
        {
          args_values(*current_series, "xrange", "dd", &x_min, &x_max);
        }
      else
        {
          x_min = x[0];
          x_max = x[cols - 1];
        }
      if (x == NULL)
        {
          args_values(*current_series, "yrange", "dd", &y_min, &y_max);
        }
      else
        {
          y_min = y[0];
          y_max = y[rows - 1];
        }
      args_values(*current_series, "zrange", "dd", &z_min, &z_max);
      if (!args_values(*current_series, "crange", "dd", &c_min, &c_max))
        {
          c_min = z_min;
          c_max = z_max;
        }

      if (zlog)
        {
          z_min = log(z_min);
          z_max = log(z_max);
          c_min = log(c_min);
          c_max = log(c_max);
        }

      if (!is_uniform_heatmap)
        {
          --cols;
          --rows;
        }
      for (i = 0; i < 256; i++)
        {
          gr_inqcolor(1000 + i, icmap + i);
        }

      data = malloc(rows * cols * sizeof(int));
      cleanup_and_set_error_if(data == NULL, ERROR_MALLOC);
      if (z_max > z_min)
        {
          for (i = 0; i < cols * rows; i++)
            {
              if (zlog)
                {
                  zv = log(z[i]);
                }
              else
                {
                  zv = z[i];
                }

              if (zv > z_max || zv < z_min)
                {
                  data[i] = -1;
                }
              else
                {
                  data[i] = (int)((zv - c_min) / (c_max - c_min) * 255);
                  if (data[i] >= 255)
                    {
                      data[i] = 255;
                    }
                  else if (data[i] < 0)
                    {
                      data[i] = 0;
                    }
                }
            }
        }
      else
        {
          for (i = 0; i < cols * rows; i++)
            {
              data[i] = 0;
            }
        }
      rgba = malloc(rows * cols * sizeof(int));
      cleanup_and_set_error_if(rgba == NULL, ERROR_MALLOC);
      if (is_uniform_heatmap)
        {
          for (i = 0; i < rows * cols; i++)
            {
              if (data[i] == -1)
                {
                  rgba[i] = 0;
                }
              else
                {
                  rgba[i] = (255 << 24) + icmap[data[i]];
                }
            }
          gr_drawimage(x_min, x_max, y_min, y_max, cols, rows, rgba, 0);
        }
      else
        {
          for (i = 0; i < rows * cols; i++)
            {
              if (data[i] == -1)
                {
                  rgba[i] = 1256 + 1; /* Invalid color index -> gr_nonuniformcellarray draws a transparent rectangle */
                }
              else
                {
                  rgba[i] = data[i] + 1000;
                }
            }
          gr_nonuniformcellarray(x, y, cols, rows, 1, 1, cols, rows, rgba);
        }

      free(rgba);
      free(data);
      rgba = NULL;
      data = NULL;

      ++current_series;
    }

  plot_draw_colorbar(subplot_args, 0.0, 256);

cleanup:
  free(rgba);
  free(data);

  return error;
}

error_t plot_wireframe(grm_args_t *subplot_args)
{
  double *gridit_x = NULL, *gridit_y = NULL, *gridit_z = NULL;
  grm_args_t **current_series;
  error_t error = NO_ERROR;

  args_values(subplot_args, "series", "A", &current_series);
  while (*current_series != NULL)
    {
      double *x, *y, *z;
      unsigned int x_length, y_length, z_length;
      args_first_value(*current_series, "x", "D", &x, &x_length);
      args_first_value(*current_series, "y", "D", &y, &y_length);
      args_first_value(*current_series, "z", "D", &z, &z_length);
      gr_setfillcolorind(0);
      if (x_length == y_length && x_length == z_length)
        {
          if (gridit_x == NULL)
            {
              gridit_x = malloc(PLOT_WIREFRAME_GRIDIT_N * sizeof(double));
              gridit_y = malloc(PLOT_WIREFRAME_GRIDIT_N * sizeof(double));
              gridit_z = malloc(PLOT_WIREFRAME_GRIDIT_N * PLOT_WIREFRAME_GRIDIT_N * sizeof(double));
              if (gridit_x == NULL || gridit_y == NULL || gridit_z == NULL)
                {
                  debug_print_malloc_error();
                  error = ERROR_MALLOC;
                  goto cleanup;
                }
            }
          gr_gridit(x_length, x, y, z, PLOT_WIREFRAME_GRIDIT_N, PLOT_WIREFRAME_GRIDIT_N, gridit_x, gridit_y, gridit_z);
          gr_surface(PLOT_WIREFRAME_GRIDIT_N, PLOT_WIREFRAME_GRIDIT_N, gridit_x, gridit_y, gridit_z,
                     GR_OPTION_FILLED_MESH);
        }
      else
        {
          if (x_length * y_length != z_length)
            {
              error = ERROR_PLOT_COMPONENT_LENGTH_MISMATCH;
              goto cleanup;
            }
          gr_surface(x_length, y_length, x, y, z, GR_OPTION_FILLED_MESH);
        }
      ++current_series;
    }
  plot_draw_axes(subplot_args, 2);

cleanup:
  free(gridit_x);
  free(gridit_y);
  free(gridit_z);

  return error;
}

error_t plot_surface(grm_args_t *subplot_args)
{
  double *gridit_x = NULL, *gridit_y = NULL, *gridit_z = NULL;
  grm_args_t **current_series;
  error_t error = NO_ERROR;

  args_values(subplot_args, "series", "A", &current_series);
  while (*current_series != NULL)
    {
      double *x, *y, *z;
      unsigned int x_length, y_length, z_length;
      args_first_value(*current_series, "x", "D", &x, &x_length);
      args_first_value(*current_series, "y", "D", &y, &y_length);
      args_first_value(*current_series, "z", "D", &z, &z_length);
      /* TODO: add support for GR3 */
      if (x_length == y_length && x_length == z_length)
        {
          if (gridit_x == NULL)
            {
              gridit_x = malloc(PLOT_SURFACE_GRIDIT_N * sizeof(double));
              gridit_y = malloc(PLOT_SURFACE_GRIDIT_N * sizeof(double));
              gridit_z = malloc(PLOT_SURFACE_GRIDIT_N * PLOT_SURFACE_GRIDIT_N * sizeof(double));
              if (gridit_x == NULL || gridit_y == NULL || gridit_z == NULL)
                {
                  debug_print_malloc_error();
                  error = ERROR_MALLOC;
                  goto cleanup;
                }
            }
          gr_gridit(x_length, x, y, z, PLOT_SURFACE_GRIDIT_N, PLOT_SURFACE_GRIDIT_N, gridit_x, gridit_y, gridit_z);
          gr_surface(PLOT_SURFACE_GRIDIT_N, PLOT_SURFACE_GRIDIT_N, gridit_x, gridit_y, gridit_z,
                     GR_OPTION_COLORED_MESH);
        }
      else
        {
          if (x_length * y_length != z_length)
            {
              error = ERROR_PLOT_COMPONENT_LENGTH_MISMATCH;
              goto cleanup;
            }
          gr_surface(x_length, y_length, x, y, z, GR_OPTION_COLORED_MESH);
        }
      ++current_series;
    }
  plot_draw_axes(subplot_args, 2);
  plot_draw_colorbar(subplot_args, 0.05, 256);

cleanup:
  free(gridit_x);
  free(gridit_y);
  free(gridit_z);

  return error;
}

error_t plot_plot3(grm_args_t *subplot_args)
{
  grm_args_t **current_series;

  args_values(subplot_args, "series", "A", &current_series);
  while (*current_series != NULL)
    {
      double *x, *y, *z;
      unsigned int x_length, y_length, z_length;
      return_error_if(!args_first_value(*current_series, "x", "D", &x, &x_length), ERROR_PLOT_MISSING_DATA);
      return_error_if(!args_first_value(*current_series, "y", "D", &y, &y_length), ERROR_PLOT_MISSING_DATA);
      return_error_if(!args_first_value(*current_series, "z", "D", &z, &z_length), ERROR_PLOT_MISSING_DATA);
      return_error_if(x_length != y_length || x_length != z_length, ERROR_PLOT_COMPONENT_LENGTH_MISMATCH);
      gr_polyline3d(x_length, x, y, z);
      ++current_series;
    }
  plot_draw_axes(subplot_args, 2);

  return NO_ERROR;
}

error_t plot_scatter3(grm_args_t *subplot_args)
{
  grm_args_t **current_series;
  double c_min, c_max;
  unsigned int x_length, y_length, z_length, c_length, i, c_index;
  double *x, *y, *z, *c;
  args_values(subplot_args, "series", "A", &current_series);
  while (*current_series != NULL)
    {
      return_error_if(!args_first_value(*current_series, "x", "D", &x, &x_length), ERROR_PLOT_MISSING_DATA);
      return_error_if(!args_first_value(*current_series, "y", "D", &y, &y_length), ERROR_PLOT_MISSING_DATA);
      return_error_if(!args_first_value(*current_series, "z", "D", &z, &z_length), ERROR_PLOT_MISSING_DATA);
      return_error_if(x_length != y_length || x_length != z_length, ERROR_PLOT_COMPONENT_LENGTH_MISMATCH);
      gr_setmarkertype(GKS_K_MARKERTYPE_SOLID_CIRCLE);
      if (args_first_value(*current_series, "c", "D", &c, &c_length))
        {
          args_values(subplot_args, "_clim", "dd", &c_min, &c_max);
          for (i = 0; i < x_length; i++)
            {
              if (i < c_length)
                {
                  c_index = 1000 + (int)(255 * (c[i] - c_min) / c_max);
                }
              else
                {
                  c_index = 989;
                }
              gr_setmarkercolorind(c_index);
              gr_polymarker3d(1, x + i, y + i, z + i);
            }
        }
      else
        {
          if (args_values(*current_series, "c", "i", &c_index))
            {
              gr_setmarkercolorind(c_index);
            }
          gr_polymarker3d(x_length, x, y, z);
        }
      ++current_series;
    }
  plot_draw_axes(subplot_args, 2);

  return NO_ERROR;
}

error_t plot_imshow(grm_args_t *subplot_args)
{
  grm_args_t **current_series;
  double *c_data;
  double c_min, c_max;
  double *viewport, *vp;
  unsigned int c_data_length, i, rows, cols;
  int *real_data;
  unsigned int *shape;

  double x_min, x_max, y_min, y_max, w, h;
  error_t error;

  args_values(subplot_args, "series", "A", &current_series);
  return_error_if(!args_values(subplot_args, "_clim", "dd", &c_min, &c_max), ERROR_PLOT_MISSING_DATA);
  return_error_if(!args_values(subplot_args, "viewport", "D", &viewport), ERROR_PLOT_MISSING_DATA);
  return_error_if(!args_values(subplot_args, "vp", "D", &vp), ERROR_PLOT_MISSING_DATA);
  while (*current_series != NULL)
    {
      return_error_if(!args_first_value(*current_series, "c", "D", &c_data, &c_data_length), ERROR_PLOT_MISSING_DATA);
      return_error_if(!args_first_value(*current_series, "c_dims", "I", &shape, &i), ERROR_PLOT_MISSING_DATA);
      return_error_if(i != 2, ERROR_PLOT_COMPONENT_LENGTH_MISMATCH);
      return_error_if(shape[0] * shape[1] != c_data_length, ERROR_PLOT_COMPONENT_LENGTH_MISMATCH);
      rows = shape[0];
      cols = shape[1];

      real_data = malloc(sizeof(int) * c_data_length);
      if (real_data == NULL)
        {
          debug_print_malloc_error();
          free(real_data);
          return ERROR_MALLOC;
        }

      logger((stderr, "Got min, max %lf %lf\n", c_min, c_max));
      for (i = 0; i < c_data_length; ++i)
        {
          real_data[i] = 1000 + (int)round((1.0 * c_data[i] - c_min) / (c_max - c_min) * 255);
        }

      if (cols * (viewport[3] - viewport[2]) < rows * (viewport[1] - viewport[0]))
        {
          w = (double)cols / (double)rows * (viewport[3] - viewport[2]);
          x_min = max(0.5 * (viewport[0] + viewport[1] - w), viewport[0]);
          x_max = min(0.5 * (viewport[0] + viewport[1] + w), viewport[1]);
          y_min = viewport[2];
          y_max = viewport[3];
        }
      else
        {
          h = (double)rows / (double)cols * (viewport[1] - viewport[0]);
          x_min = viewport[0];
          x_max = viewport[1];
          y_min = max(0.5 * (viewport[3] + viewport[2] - h), viewport[2]);
          y_max = min(0.5 * (viewport[3] + viewport[2] + h), viewport[3]);
        }

      gr_selntran(0);
      gr_cellarray(x_min, x_max, y_min, y_max, cols, rows, 1, 1, cols, rows, real_data);

      gr_selntran(1);

      free(real_data);

      ++current_series;
    }

  if ((error = plot_draw_colorbar(subplot_args, 0.00, 256)) != NO_ERROR)
    {
      return error;
    }

  return NO_ERROR;
}

/*
 * Checks if the GR3_MC_DTYPE macro is set to unsigned short, as we only support this dtype in plot_isosurface.
 */
void gr3_mc_dtype_test_(void)
{
  GR3_MC_DTYPE v;
  switch (0)
    {
    case ((sizeof(GR3_MC_DTYPE) == 2) && (((GR3_MC_DTYPE)-1) > 0)):
    case 0:
      break;
    }
}

error_t plot_isosurface(grm_args_t *subplot_args)
{
  /*
   * Possible arguments to pass:
   * double[] `c` with the isodata to be drawn
   * int[3] `c_dims` shape of the c-array
   * optional double `isovalue` of the surface. All values higher or equal to isovalue are seen as inside the object,
   * all values below the isovalue are seen as outside the object.
   * optional double `rotation` in degrees
   * optional double `tilt` of the camera in degrees
   * optional double[3] `foreground_color`: color of the surface
   */
  grm_args_t **current_series;
  double *orig_data, *viewport, *temp_colors;
  unsigned int i, data_length, dims;
  unsigned int *shape;
  double c_min, c_max, isovalue, x_min, x_max, y_min, y_max, rotation, tilt;
  float foreground_colors[3], positions[3], directions[3], ups[3], scales[3];
  float r;
  int fig_width, fig_height;
  int subplot_width, subplot_height;

  unsigned short isovalue_int, *conv_data;

  int mesh; /* The mesh id of the drawn surface */
  int error;

  args_values(subplot_args, "series", "A", &current_series);
  return_error_if(!args_values(subplot_args, "viewport", "D", &viewport), ERROR_PLOT_MISSING_DATA);

  /* subpplot params */
  rotation = 40;
  tilt = 70;

  args_values(subplot_args, "rotation", "d", &rotation);
  args_values(subplot_args, "tilt", "d", &tilt);

  /* Convert to radians */
  tilt = fmod(tilt, 360.0) / 180.0 * M_PI;
  rotation = fmod(rotation, 360.0) / 180.0 * M_PI;
  logger((stderr, "tilt %lf rotation %lf\n", tilt, rotation));

  /* Calculate subplot pixel size */
  x_min = viewport[0];
  x_max = viewport[1];
  y_min = viewport[2];
  y_max = viewport[3];

  get_figure_size(NULL, &fig_width, &fig_height, NULL, NULL);
  subplot_width = (int)(max(fig_width, fig_height) * (x_max - x_min));
  subplot_height = (int)(max(fig_width, fig_height) * (y_max - y_min));

  logger((stderr, "viewport: (%lf, %lf, %lf, %lf)\n", x_min, x_max, y_min, y_max));
  logger((stderr, "viewport ratio: %lf\n", (x_min - x_max) / (y_min - y_max)));
  logger((stderr, "subplot size: (%d, %d)\n", subplot_width, subplot_height));
  logger((stderr, "subplot ratio: %lf\n", ((double)subplot_width / (double)subplot_height)));
  while (*current_series != NULL)
    {
      return_error_if(!args_first_value(*current_series, "c", "D", &orig_data, &data_length), ERROR_PLOT_MISSING_DATA);
      return_error_if(!args_first_value(*current_series, "c_dims", "I", &shape, &dims), ERROR_PLOT_MISSING_DATA);
      return_error_if(dims != 3, ERROR_PLOT_COMPONENT_LENGTH_MISMATCH);
      return_error_if(shape[0] * shape[1] * shape[2] != data_length, ERROR_PLOT_COMPONENT_LENGTH_MISMATCH);
      return_error_if(data_length <= 0, ERROR_PLOT_MISSING_DATA);

      /* Capture isovalue, rotation and tilt, also default values */
      isovalue = 0.5;
      foreground_colors[0] = 0.0;
      foreground_colors[1] = 0.5;
      foreground_colors[2] = 0.8;

      args_values(*current_series, "isovalue", "d", &isovalue);
      /* We need to convert the double values to floats, as the gr3_drawmesh expects floats, but an argument can only
       * contain doubles. */
      if (args_first_value(*current_series, "foreground_color", "D", &temp_colors, &i))
        {
          return_error_if(i != 3, ERROR_PLOT_COMPONENT_LENGTH_MISMATCH);
          while (i-- > 0)
            {
              foreground_colors[i] = (float)temp_colors[i];
            }
        }
      logger((stderr, "Colors; %f %f %f\n", foreground_colors[0], foreground_colors[1], foreground_colors[2]));

      /* Check if any value is finite in array, also calculation of real min and max */
      c_min = c_max = *orig_data;
      for (i = 0; i < data_length; ++i)
        {
          if (isfinite(orig_data[i]))
            {
              if (isnan(c_min) || c_min > orig_data[i])
                {
                  c_min = orig_data[i];
                }
              if (isnan(c_max) || c_max < orig_data[i])
                {
                  c_max = orig_data[i];
                }
            }
        }
      return_error_if(c_min == c_max || !isfinite(c_min) || !isfinite(c_max), ERROR_PLOT_MISSING_DATA);

      /* Calculate isovalue in ushort, and translate the original data into ushorts */
      isovalue_int = (unsigned short)((isovalue - c_min) / (c_max - c_min) * USHRT_MAX);

      logger((stderr, "c_min %lf c_max %lf isovalue_int %hu\n ", c_min, c_max, isovalue_int));
      conv_data = malloc(sizeof(unsigned short) * data_length);
      if (conv_data == NULL)
        {
          debug_print_malloc_error();
          free(conv_data);
          return ERROR_MALLOC;
        }

      for (i = 0; i < data_length; ++i)
        {
          if (isnan(orig_data[i]) || orig_data[i] < c_min)
            {
              conv_data[i] = 0;
            }
          else if (orig_data[i] > c_max)
            {
              conv_data[i] = USHRT_MAX;
            }
          else
            {
              conv_data[i] = (unsigned short)((orig_data[i] - c_min) / (c_max - c_min) * USHRT_MAX);
            }
        }

      gr_selntran(0);
      gr3_clear();
      error = gr3_createisosurfacemesh(&mesh, conv_data, isovalue_int,

                                       /* dim_x */ shape[0],
                                       /* dim_y */ shape[1],
                                       /* dim_z */ shape[2],

                                       /* stride_x */ shape[1] * shape[2],
                                       /* stride_y */ shape[2],
                                       /* stride_z */ 1,

                                       /* step_x */ 2.f / (shape[0] - 1),
                                       /* step_y */ 2.f / (shape[1] - 1),
                                       /* step_z */ 2.f / (shape[2] - 1),

                                       /* offset_x */ -1.f,
                                       /* offset_y */ -1.f,
                                       /* offset_z */ -1.f);
      return_error_if(error == GR3_ERROR_OUT_OF_MEM, ERROR_MALLOC);
      return_error_if(error != GR3_ERROR_NONE, ERROR_INTERNAL);

      gr3_setbackgroundcolor(1.0f, 1.0f, 1.0f, 0.0f);

      positions[0] = 0.0f;
      positions[1] = 0.0f;
      positions[2] = 0.0f;
      directions[0] = 0.0f;
      directions[1] = 0.0f;
      directions[2] = 1.0f;
      ups[0] = 0.0f;
      ups[1] = 1.0f;
      ups[2] = 0.0f;
      scales[0] = 1.0f;
      scales[1] = 1.0f;
      scales[2] = 1.0f;
      gr3_drawmesh(mesh, 1, positions, directions, ups, foreground_colors, scales);

      r = 2.5;
      ups[0] = 0.0f;
      ups[1] = (tilt == 0 ? 0.0f : 1.0f);
      ups[2] = (tilt == 0 ? 1.0f : 0.0f);

      gr3_cameralookat((float)(r * sin(tilt) * sin(rotation)), (float)(r * cos(tilt)),
                       (float)(r * sin(tilt) * cos(rotation)), 0.0f, 0.0f, 0.0f, ups[0], ups[1], ups[2]);

      logger((stderr, "gr3_drawimage returned %i\n",
              gr3_drawimage(x_min, x_max, y_min, y_max, subplot_width, subplot_height, GR3_DRAWABLE_GKS)));
      gr3_deletemesh(mesh);
      gr_selntran(1);

      free(conv_data);
      ++current_series;
    }

  return NO_ERROR;
}

error_t plot_polar(grm_args_t *subplot_args)
{
  const double *window;
  double r_min, r_max, tick;
  int n;
  grm_args_t **current_series;

  args_values(subplot_args, "window", "D", &window);
  r_min = window[2];
  r_max = window[3];
  tick = 0.5 * gr_tick(r_min, r_max);
  n = (int)ceil((r_max - r_min) / tick);
  r_max = r_min + n * tick;
  args_values(subplot_args, "series", "A", &current_series);
  while (*current_series != NULL)
    {
      double *rho, *theta, *x, *y;
      unsigned int rho_length, theta_length;
      char *spec;
      unsigned int i;
      return_error_if(!args_first_value(*current_series, "x", "D", &theta, &theta_length), ERROR_PLOT_MISSING_DATA);
      return_error_if(!args_first_value(*current_series, "y", "D", &rho, &rho_length), ERROR_PLOT_MISSING_DATA);
      return_error_if(rho_length != theta_length, ERROR_PLOT_COMPONENT_LENGTH_MISMATCH);
      x = malloc(rho_length * sizeof(double));
      y = malloc(rho_length * sizeof(double));
      if (x == NULL || y == NULL)
        {
          debug_print_malloc_error();
          free(x);
          free(y);
          return ERROR_MALLOC;
        }
      for (i = 0; i < rho_length; ++i)
        {
          double current_rho = rho[i] / r_max;
          x[i] = current_rho * cos(theta[i]);
          y[i] = current_rho * sin(theta[i]);
        }
      args_values(*current_series, "spec", "s", &spec); /* `spec` is always set */
      gr_uselinespec(spec);
      gr_polyline(rho_length, x, y);
      free(x);
      free(y);
      ++current_series;
    }

  return NO_ERROR;
}


/*!
 * Plot a polar histogram.
 *
 * \param[in] x (series)
 *            Either a list containing doubles representing angles or ints for bin_counts.
 * \param[in] normalization a string specifying the normalization (subplot)
 *            The default value is "count".
 *          + count: the default normalization.  The height of each bin is the number of observations in each bin.
 *          + probability: The height of each bin is the relative number of observations,
 *            (number of observations in bin/total number of observations).
 *            The sum of the bin heights is 1.
 *          + countdensity: The height of each bin is the number of observations in bin / (width of bin).
 *          + pdf: Probability density function estimate. The height of each bin is
 *            (number of observations in the bin)/(total number of observations * width of bin).
 *            The area of each bin is the relative number of observations. The sum of the bin areas is 1
 *          + cumcount: The height of each bin is the cumulative number of observations in each bin and all
 *            previous bins. The height of the last bin is numel (theta).
 *          + cdf: Cumulative density function estimate. The height of each bin is equal to the cumulative relative
 *            number of observations in the bin and all previous bins. The height of the last bin is 1.
 * \param[in] bin_counts an int which behaves like a boolean (series)
 *            0: the series contains angles.
 *            else: the given series contains the direct values for each bin.
 *            The default value is 0.
 * \param[in] bin_edges a list containing angles that represent the edges of the bins (series)
 *            It is not compatible with nbins or bin_width.
 *            When used with bin_counts: length of bin_edges must equal the length of bin_counts + 1.
 * \param[in] stairs an int which behaves like a boolean (series)
 *            0: everything will be drawn.
 *            else: only the plot outlines will be drawn.
 *            The default value is 0 and stairs with a value unequal 0 is not compatible with colormap.
 * \param[in] colormap a list with two ints which represent a gr colormap (series)
 *            The first int sets the angle colormap and the second one the radius colormap.
 *            Must be between (inclusive) -1 and 47.
 *            -1: no colormap
 * \param[in] draw_edges an int that acts like a boolean (series)
 *            0: bin edges will not be drawn.
 *            else: bin edges will be drawn.
 *            It is only compatible with colormap.
 *            The default value is 0.
 * \param[in] philim a list containing two double values representing start and end angle (series)
 *            The plot will only use the values in the given range and and it will plot bins between those two values.
 * \param[in] phiflip an integer that represents a boolean (subplot)
 *            0: the angles will be displayed anti clockwise.
 *            else: the angles will be displayed clockwise.
 *            If philim[0] is greater than philim[1], phiflip will be negated.
 *            The default value is 0.
 * \param[in] rlim a list containing two double values between (inclusive) 0.0 and 1.0 (series)
 *            0.0 is the center and 1.0 is the outer edge of the plot.
 *            rlim will limit the bins' start and end height.
 * \param[in] bin_width a double value between (exclusive) 0.0 and 2 * pi setting the bins' width (series)
 *            It is not compatible nbins or bin_edges.
 * \param[in] nbins an int setting the number of bins (series)
 *            It is not compatible with bin_edges, nbins or bin_counts.
 * \param[in] face_alpha double value between 0.0 and 1.0 inclusive (series)
 *            Sets the opacity of bins.
 *            A value of 1.0 means fully opaque and 0.0 means completely transparent (invisible).
 *            The default value is 0.75.
 * \param[in] face_color sets the fill color of bins (series)
 *            face_color expects an integer between 0 and 1008.
 *            The default value is 989.
 * \param[in] edge_color sets the color of the bin edges (series)
 *            edge_color expects an integer between 0 and 1008.
 *            The default value is 1.
 *
 * */
error_t plot_polar_histogram(grm_args_t *subplot_args)
{
  unsigned int num_bins;
  double *classes = NULL;
  unsigned int length;
  double max;
  double *inner = NULL, *outer = NULL;
  double r;
  double rect;
  double *liste = NULL;
  double liste0;
  double liste1;
  double *liste2 = NULL;
  double *mlist = NULL;
  double *rectlist = NULL;
  char *norm = NULL;
  double bin_width = -1.0;
  double *bin_edges = NULL;
  unsigned int num_bin_edges;
  double *bin_widths = NULL;
  double *philim = NULL;
  double *rlim = NULL;
  unsigned int dummy;
  double *r_min_list = NULL;
  double *r_min_list2 = NULL;
  int stairs;
  double r_min = 0.0;
  double r_max = 1.0;
  double *phi_array = NULL;
  double *arc_2_x = NULL;
  double *arc_2_y = NULL;
  int xcolormap;
  int ycolormap;
  int *colormap = NULL;
  double *angles = NULL;
  int draw_edges = 0;
  int phiflip = 0;
  int x;
  const double convert = 180 / M_PI;
  int edge_color = 1;
  int face_color = 989;
  double face_alpha = 0.75;
  grm_args_t **series;
  unsigned int resample = 0;
  int *lineardata = NULL;
  int *bin_counts = NULL;
  double *f1 = NULL;
  double *f2 = NULL;
  int freeable_bin_widths = 0;
  int freeable_bin_edges = 0;
  int freeable_angles = 0;
  error_t error = NO_ERROR;

  gr_inqresamplemethod(&resample);
  gr_setresamplemethod(0x2020202);

  args_values(subplot_args, "series", "A", &series);

  args_first_value(*series, "classes", "D", &classes, &length);

  /* edge_color */
  if (args_values(*series, "edge_color", "i", &edge_color) == 0)
    {
      edge_color = 1;
    }

  /* face_color */
  if (args_values(*series, "face_color", "i", &face_color) == 0)
    {
      face_color = 989;
    }

  /* face_alpha */
  if (args_values(*series, "face_alpha", "d", &face_alpha) == 0)
    {
      face_alpha = 0.75;
    }

  gr_settransparency(face_alpha);

  args_values(*series, "nbins", "i", &num_bins);

  args_values(subplot_args, "r_max", "d", &max);

  if (args_values(subplot_args, "phiflip", "i", &phiflip) == 0)
    {
      phiflip = 0;
    }

  if (args_values(subplot_args, "normalization", "s", &norm) == 0)
    {
      norm = "count";
    }

  if (args_values(*series, "draw_edges", "i", &draw_edges) == 0)
    {
      draw_edges = 0;
    }

  if (args_first_value(*series, "bin_edges", "D", &bin_edges, &num_bin_edges) == 0)
    {
      bin_edges = NULL;
      num_bin_edges = 0;
      args_values(*series, "bin_width", "d", &bin_width);
    }
  else
    {
      args_first_value(*series, "bin_widths", "D", &bin_widths, &num_bins);
    }


  if (args_values(*series, "stairs", "i", &stairs) == 0)
    {
      stairs = 0;
    }
  else
    {
      if (draw_edges != 0)
        {
          logger((stderr, "stairs is not compatible with draw_edges / colormap\n"));
          cleanup_and_set_error(ERROR_PLOT_INCOMPATIBLE_ARGUMENTS);
        }
      /* no bin_edges */
      else if (num_bin_edges == 0)
        {
          mlist = (double *)malloc(num_bins * 4 * sizeof(double));
          cleanup_and_set_error_if(mlist == NULL, ERROR_MALLOC);
          if (stairs != 0)
            {
              stairs = 1;
            }
        }
      else
        {
          rectlist = (double *)malloc(num_bins * sizeof(double));
          cleanup_and_set_error_if(rectlist == NULL, ERROR_MALLOC);
        }
    }

  if (args_first_value(*series, "rlim", "D", &rlim, &dummy) == 0)
    {
      rlim = NULL;
    }
  else
    {
      /* TODO: Potential memory leak, s. `malloc` in line 3788 */
      mlist = (double *)malloc((num_bins + 1) * 4 * sizeof(double));
      cleanup_and_set_error_if(mlist == NULL, ERROR_MALLOC);
      if (rlim[0] > rlim[1])
        {
          r_min = rlim[1];
          r_max = rlim[0];
          rlim[0] = r_min;
          rlim[1] = r_max;
        }
      else
        {
          r_min = rlim[0];
          r_max = rlim[1];
        }

      if (r_max > 1.0)
        {
          r_max = 1.0;
          logger((stderr, "the max value of rlim can not exceed 1.0\n"));
          cleanup_and_set_error(ERROR_PLOT_OUT_OF_RANGE);
        }
      if (r_min < 0.0) r_min = 0.0;
    }

  length /= num_bins;
  outer = classes;
  if (phiflip != 0)
    {
      outer += (num_bins - 1) * length;
    }

  if (!args_values(*series, "xcolormap", "i", &xcolormap) || !args_values(*series, "ycolormap", "i", &ycolormap))
    {
      colormap = NULL;
      if (draw_edges != 0)
        {
          logger((stderr, "draw_edges can only be used with colormap\n"));
          cleanup_and_set_error(ERROR_PLOT_INCOMPATIBLE_ARGUMENTS);
        }
    }
  else
    {
      if (-1 > xcolormap || xcolormap > 47 || ycolormap < -1 || ycolormap > 47)
        {
          logger((stderr, "the value for keyword \"colormap\" must contain two integer between -1 and 47\n"));
          cleanup_and_set_error(ERROR_PLOT_OUT_OF_RANGE);
        }
      else
        {
          int colormap_size = 500;
          int image_size = 2000;
          int y, center, center_x, center_y;
          double radius, angle;
          int temp1, temp2;
          int temp = 0;
          int total = 0;
          double norm_factor = 1;
          int count = 0;
          double max_radius;


          lineardata = (int *)calloc(image_size * image_size, sizeof(int));
          cleanup_and_set_error_if(lineardata == NULL, ERROR_MALLOC);

          bin_counts = (int *)malloc(num_bins * sizeof(int));
          cleanup_and_set_error_if(bin_counts == NULL, ERROR_MALLOC);

          colormap = create_colormap(xcolormap, ycolormap, colormap_size);
          cleanup_and_set_error_if(colormap == NULL, ERROR_PLOT_COLORMAP);

          if (num_bin_edges == 0)
            {
              angles = (double *)malloc((num_bins + 1) * sizeof(double));
              cleanup_and_set_error_if(angles == NULL, ERROR_MALLOC);
              freeable_angles = 1;
              linspace(0, M_PI * 2, num_bins + 1, angles);
            }
          else
            {
              angles = bin_edges;
            }

          outer = classes;

          center_x = image_size / 2;
          center_y = image_size / 2;
          center = image_size / 2;

          max_radius = center;

          for (temp1 = 0; temp1 < num_bins; temp1++)
            {
              temp = 0;
              for (temp2 = 0; temp2 < length; temp2++)
                {
                  if (classes[temp1 * length + temp2] == -1) break;
                  ++temp;
                }
              bin_counts[temp1] = temp;
            }

          if (str_equals_any(norm, 2, "probability", "pdf"))
            {
              for (temp1 = 0; temp1 < num_bins; ++temp1)
                {
                  total += bin_counts[temp1];
                }
            }
          else if (strcmp(norm, "cdf") == 0)
            total = bin_counts[num_bins - 1];


          if (str_equals_any(norm, 2, "probability", "cdf"))
            norm_factor = total;
          else if (num_bin_edges == 0 && strcmp(norm, "pdf") == 0)
            norm_factor = total * bin_width;
          else if (num_bin_edges == 0 && strcmp(norm, "countdensity") == 0)
            norm_factor = bin_width;

          if (rlim != NULL)
            {
              r_min *= max_radius;
              r_max *= max_radius;
            }
          else
            {
              r_min = 0.0;
              r_max = max_radius;
            }

          for (y = 0; y < image_size; y++)
            {
              for (x = 0; x < image_size; x++)
                {
                  int q;
                  radius = sqrt(pow(x - center_x, 2) + pow(y - center_y, 2));
                  angle = atan2(y - center_y, x - center_x);

                  if (angle < 0) angle += M_PI * 2;
                  if (phiflip == 0) angle = 2 * M_PI - angle;

                  for (q = 0; q < num_bins; ++q)
                    {
                      if (angle > angles[q] && angle <= angles[q + 1])
                        {
                          count = bin_counts[q];
                          if (strcmp(norm, "pdf") == 0 && num_bin_edges > 0)
                            {
                              norm_factor = total * bin_widths[q];
                            }
                          else if (strcmp(norm, "countdensity") == 0 && num_bin_edges > 0)
                            {
                              norm_factor = bin_widths[q];
                            }

                          if ((round(radius * 100) / 100) <=
                                  (round((count * 1.0 / norm_factor / max * center) * 100) / 100) &&
                              radius <= r_max && radius > r_min)
                            {
                              lineardata[y * image_size + x] =
                                  colormap[(int)(radius / (center * pow(2, 0.5)) * (colormap_size - 1)) *
                                               colormap_size +
                                           max(min((int)(angle / (2 * M_PI) * colormap_size), colormap_size - 1), 0)];
                            }

                        } /* end angle check */
                    }     /* end for q loop: bin check*/
                }         /* end x loop*/
            }             /* end y loop */
          if (rlim != NULL)
            {
              r_min = rlim[0];
              r_max = rlim[1];
            }
          gr_drawimage(-1.0, 1.0, -1.0, 1.0, image_size, image_size, lineardata, 0);
          free(lineardata);
          free(bin_counts);
          lineardata = NULL;
          bin_counts = NULL;
        } /* end colormap calculation*/

    } /* end colormap condition */

  outer = classes;
  if (phiflip != 0) outer += (num_bins - 1) * length;

  if (phiflip != 0 && num_bin_edges > 0)
    {
      double *temp = NULL;
      double *temp2 = NULL;
      temp = malloc(num_bin_edges * sizeof(double));
      cleanup_and_set_error_if(temp == NULL, ERROR_MALLOC);
      temp2 = malloc(num_bins * sizeof(double));
      cleanup_and_set_error_if(temp2 == NULL, ERROR_MALLOC);
      int u;
      for (u = 0; u < num_bin_edges; u++)
        {
          temp[u] = 2 * M_PI - bin_edges[num_bin_edges - 1 - u];
        }
      for (u = num_bins - 1; u >= 0; --u)
        {
          temp2[u] = bin_widths[num_bins - 1 - u];
        }
      bin_widths = temp2;
      freeable_bin_widths = 1;
      bin_edges = temp;
      temp = temp2 = NULL;
      freeable_bin_edges = 1;
    }

  /* no colormap or colormap combined with draw_edges */
  if (colormap == NULL || draw_edges == 1)
    {
      for (x = 0; x < num_bins; ++x)
        {
          double count;
          int y;

          /*
           * free memory from the previous iteration
           * (end of loop is not possible because of `continue` statements)
           * last iteration memory is freed in the cleanup block
           */
          free(liste);
          liste = NULL;
          free(liste2);
          liste2 = NULL;
          free(r_min_list);
          r_min_list = NULL;
          free(r_min_list2);
          r_min_list2 = NULL;

          count = 0.0;
          inner = outer;

          if (*inner == -1)
            {
              /* stairs bin_edges / philim  */
              if (rectlist != NULL && philim != NULL)
                rectlist[x] = r_min;
              else if (rectlist != NULL)
                rectlist[x] = 0.0;
            }


          for (y = 0; y < length; ++y)
            {
              if (*inner != -1)
                {
                  count++;
                  inner++;
                }
            }

          if (str_equals_any(norm, 2, "probability", "cdf"))
            {
              count /= length;
            }
          else if (strcmp(norm, "pdf") == 0)
            {
              if (num_bin_edges == 0)
                {
                  count /= length * bin_width;
                }
              else
                {
                  count /= (length * *(bin_widths + x));
                }
            }
          else if (strcmp(norm, "countdensity") == 0)
            {
              if (num_bin_edges == 0)
                {
                  count /= bin_width;
                }
              else
                {
                  count /= *(bin_widths + x);
                }
            }

          /* no stairs*/
          if (stairs == 0)
            {
              r = pow((count / max), num_bins * 2);
              liste = moivre(r, 2 * x, num_bins * 2);
              cleanup_and_set_error_if(liste == NULL, ERROR_MALLOC);
              liste0 = liste[0];
              liste1 = liste[1];
              rect = sqrt(liste0 * liste0 + liste1 * liste1);

              if (rlim != NULL)
                {
                  double temporary;
                  int i;
                  liste2 = moivre(r, (2 * x + 2), (num_bins * 2));
                  cleanup_and_set_error_if(liste2 == NULL, ERROR_MALLOC);

                  *(mlist + x * 4) = liste0;
                  *(mlist + x * 4 + 1) = liste1;
                  *(mlist + x * 4 + 2) = *(liste2);
                  *(mlist + x * 4 + 3) = *(liste2 + 1);

                  r_min_list = moivre(pow((r_min), (num_bins * 2)), (x * 2), num_bins * 2);
                  cleanup_and_set_error_if(r_min_list == NULL, ERROR_MALLOC);
                  r_min_list2 = moivre(pow((r_min), (num_bins * 2)), (x * 2 + 2), num_bins * 2);
                  cleanup_and_set_error_if(r_min_list2 == NULL, ERROR_MALLOC);

                  for (i = 0; i < 2; ++i)
                    {
                      temporary = fabs(sqrt(pow(mlist[x * 4 + 2 - i * 2], 2) + pow(mlist[x * 4 + 3 - i * 2], 2)));
                      if (temporary > r_max)
                        {
                          double factor = fabs(r_max / temporary);
                          mlist[x * 4 + 2 - i * 2] *= factor;
                          mlist[x * 4 + 3 - i * 2] *= factor;
                        }
                    }
                  r = count / max;
                  if (r > r_max)
                    {
                      r = r_max;
                    }
                  free(liste2);
                  liste2 = NULL;
                }

              /*  no binedges */
              if (num_bin_edges == 0)
                {
                  if (rlim != NULL)
                    {
                      double start_angle;
                      double end_angle;

                      double diff_angle;
                      int num_angle;

                      int i;

                      if (r <= r_min)
                        {
                          if (phiflip == 1)
                            outer -= length;
                          else
                            outer += length;
                          continue;
                        }

                      start_angle = x * (360.0 / num_bins) / convert;
                      end_angle = (x + 1) * (360.0 / num_bins) / convert;

                      diff_angle = end_angle - start_angle;
                      num_angle = (int)(diff_angle / (0.2 / convert));

                      phi_array = (double *)malloc(num_angle * sizeof(double));
                      cleanup_and_set_error_if(phi_array == NULL, ERROR_MALLOC);
                      linspace(start_angle, end_angle, num_angle, phi_array);

                      f1 = (double *)malloc((4 + 2 * num_angle) * sizeof(double));
                      cleanup_and_set_error_if(f1 == NULL, ERROR_MALLOC);
                      /* line_1_x[0] and [1]*/
                      f1[0] = r_min_list[0];
                      f1[1] = mlist[4 * x];
                      /* arc_1_x */
                      listcomprehension(r, cos, phi_array, num_angle, 2, f1);
                      /* reversed line_2_x [0] and [1] */
                      f1[2 + num_angle + 1] = r_min_list2[0];
                      f1[2 + num_angle] = mlist[4 * x + 2];
                      /* reversed arc_2_x */
                      arc_2_x = listcomprehension(r_min, cos, phi_array, num_angle, 0, NULL);
                      cleanup_and_set_error_if(arc_2_x == NULL, ERROR_MALLOC);
                      for (i = 0; i < num_angle; ++i)
                        {
                          f1[2 + num_angle + 2 + i] = arc_2_x[num_angle - 1 - i];
                        }
                      free(arc_2_x);
                      arc_2_x = NULL;

                      f2 = (double *)malloc((4 + 2 * num_angle) * sizeof(double));
                      cleanup_and_set_error_if(f2 == NULL, ERROR_MALLOC);
                      /* line_1_y[0] and [1] */
                      f2[0] = r_min_list[1];
                      f2[1] = mlist[4 * x + 1];
                      /*arc_1_y */
                      listcomprehension(r, sin, phi_array, num_angle, 2, f2);
                      /* reversed line_2_y [0] and [1] */
                      f2[2 + num_angle + 1] = r_min_list2[1];
                      f2[2 + num_angle] = mlist[4 * x + 3];
                      /* reversed arc_2_y */
                      arc_2_y = listcomprehension(r_min, sin, phi_array, num_angle, 0, NULL);
                      cleanup_and_set_error_if(arc_2_y == NULL, ERROR_MALLOC);
                      for (i = 0; i < num_angle; ++i)
                        {
                          f2[2 + num_angle + 2 + i] = arc_2_y[num_angle - 1 - i];
                        }
                      free(arc_2_y);
                      arc_2_y = NULL;

                      if (draw_edges == 0)
                        {
                          gr_setfillcolorind(face_color);
                          gr_setfillintstyle(1);
                          gr_fillarea(4 + 2 * num_angle, f1, f2);
                        }
                      gr_setfillintstyle(0);
                      gr_setfillcolorind(edge_color);

                      gr_fillarea(4 + 2 * num_angle, f1, f2);

                      free(f1);
                      f1 = NULL;
                      free(f2);
                      f2 = NULL;
                      free(phi_array);
                      phi_array = NULL;
                    } /* end rlim condition */
                  /* no rlim */
                  else
                    {
                      if (draw_edges == 0)
                        {
                          gr_setfillintstyle(1);
                          gr_setfillcolorind(face_color);
                          gr_fillarc(-rect, rect, -rect, rect, x * (360.0 / num_bins), (x + 1) * (360.0 / num_bins));
                        }

                      gr_setfillintstyle(0);
                      gr_setfillcolorind(edge_color);

                      gr_fillarc(-rect, rect, -rect, rect, x * (360.0 / num_bins), (x + 1) * (360.0 / num_bins));
                    }
                }
              /* bin_egdes */
              else
                {
                  if (rlim != NULL)
                    {
                      double start_angle;
                      double end_angle;

                      double diff_angle;
                      int num_angle;

                      int i;

                      if (r <= r_min)
                        {
                          if (phiflip != 0)
                            outer -= length;
                          else
                            outer += length;
                          continue;
                        }

                      start_angle = bin_edges[x];
                      end_angle = bin_edges[x + 1];

                      diff_angle = end_angle - start_angle;
                      num_angle = (int)(diff_angle / (0.2 / convert));
                      phi_array = (double *)malloc(num_angle * sizeof(double));
                      cleanup_and_set_error_if(phi_array == NULL, ERROR_MALLOC);
                      linspace(start_angle, end_angle, num_angle, phi_array);

                      f1 = (double *)malloc((4 + 2 * num_angle) * sizeof(double));
                      cleanup_and_set_error_if(f1 == NULL, ERROR_MALLOC);
                      /* line_1_x[0] and [1]*/
                      f1[0] = cos(bin_edges[x]) * r_min;
                      f1[1] = min(rect, r_max) * cos(bin_edges[x]);
                      /*arc_1_x */
                      listcomprehension(r, cos, phi_array, num_angle, 2, f1);
                      /* reversed line_2_x [0] and [1] */
                      f1[2 + num_angle + 1] = cos(bin_edges[x + 1]) * r_min;
                      f1[2 + num_angle] = min(rect, r_max) * cos(bin_edges[x + 1]);
                      /* reversed arc_2_x */
                      arc_2_x = listcomprehension(r_min, cos, phi_array, num_angle, 0, NULL);
                      cleanup_and_set_error_if(arc_2_x == NULL, ERROR_MALLOC);
                      for (i = 0; i < num_angle; ++i)
                        {
                          f1[2 + num_angle + 2 + i] = arc_2_x[num_angle - 1 - i];
                        }
                      free(arc_2_x);
                      arc_2_x = NULL;

                      f2 = (double *)malloc((4 + 2 * num_angle) * sizeof(double));
                      cleanup_and_set_error_if(f2 == NULL, ERROR_MALLOC);
                      /* line_1_y[0] and [1] */
                      f2[0] = r_min * sin(bin_edges[x]);
                      f2[1] = min(rect, r_max) * sin(bin_edges[x]);
                      /*arc_1_y */
                      listcomprehension(r, sin, phi_array, num_angle, 2, f2);
                      /* reversed line_2_y [0] and [1] */
                      f2[2 + num_angle + 1] = r_min * sin(bin_edges[x + 1]);
                      f2[2 + num_angle] = min(rect, r_max) * sin(bin_edges[x + 1]);
                      /* reversed arc_2_y */
                      arc_2_y = listcomprehension(r_min, sin, phi_array, num_angle, 0, NULL);
                      cleanup_and_set_error_if(arc_2_y == NULL, ERROR_MALLOC);
                      for (i = 0; i < num_angle; ++i)
                        {
                          f2[2 + num_angle + 2 + i] = arc_2_y[num_angle - 1 - i];
                        }
                      free(arc_2_y);
                      arc_2_y = NULL;

                      if (draw_edges == 0)
                        {
                          gr_setfillintstyle(1);
                          gr_setfillcolorind(face_color);
                          gr_fillarea(4 + 2 * num_angle, f1, f2);
                        }

                      gr_setfillintstyle(0);
                      gr_setfillcolorind(edge_color);

                      gr_fillarea(4 + 2 * num_angle, f1, f2);

                      free(f1);
                      f1 = NULL;
                      free(f2);
                      f2 = NULL;
                      free(phi_array);
                      phi_array = NULL;
                    }
                  /* no rlim */
                  else
                    {
                      if (draw_edges == 0)
                        {
                          gr_setfillintstyle(1);
                          gr_setfillcolorind(face_color);
                          gr_fillarc(-rect, rect, -rect, rect, bin_edges[x] * convert, bin_edges[x + 1] * convert);
                        }
                      gr_setfillintstyle(0);
                      gr_setfillcolorind(edge_color);

                      gr_fillarc(-rect, rect, -rect, rect, bin_edges[x] * convert, bin_edges[x + 1] * convert);
                    }
                }
            } /* end no stairs condition */
          /* stairs without draw_edges (not compatible) */
          else if (draw_edges == 0 && colormap == NULL)
            {
              gr_setfillcolorind(0);
              gr_setlinecolorind(edge_color);
              gr_setlinewidth(2.3);

              r = pow((count / max), (num_bins * 2));
              liste = moivre(r, (2 * x), num_bins * 2);
              cleanup_and_set_error_if(liste == NULL, ERROR_MALLOC);
              liste2 = moivre(r, (2 * x + 2), (num_bins * 2));
              cleanup_and_set_error_if(liste2 == NULL, ERROR_MALLOC);
              rect = sqrt(*liste * *liste + *(liste + 1) * *(liste + 1));

              /*  no bin_edges */
              if (num_bin_edges == 0)
                {
                  *(mlist + x * 4) = *liste;
                  *(mlist + x * 4 + 1) = *(liste + 1);
                  *(mlist + x * 4 + 2) = *(liste2);
                  *(mlist + x * 4 + 3) = *(liste2 + 1);

                  if (rlim != NULL)
                    {
                      double temporary;
                      int i;
                      for (i = 0; i < 2; ++i)
                        {
                          temporary = fabs(sqrt(pow(mlist[x * 4 + 2 - i * 2], 2) + pow(mlist[x * 4 + 3 - i * 2], 2)));
                          if (temporary > r_max)
                            {
                              double factor = fabs(r_max / temporary);
                              mlist[x * 4 + 2 - i * 2] *= factor;
                              mlist[x * 4 + 3 - i * 2] *= factor;
                            }
                        }

                      if (rect > r_min)
                        {
                          gr_drawarc(-min(rect, r_max), min(rect, r_max), -min(rect, r_max), min(rect, r_max),
                                     x * (360.0 / num_bins), (x + 1) * 360.0 / num_bins);

                          gr_drawarc(-r_min, r_min, -r_min, r_min, x * (360.0 / num_bins),
                                     (x + 1) * (360.0 / num_bins));
                        }
                    }
                  /* no rlim */
                  else
                    {
                      gr_drawarc(-rect, rect, -rect, rect, x * (360.0 / num_bins), (x + 1) * (360.0 / num_bins));
                    }
                }
              /* with bin_edges */
              else
                {
                  /* rlim and bin_edges*/
                  if (rlim != NULL)
                    {
                      if (rect < r_min)
                        {
                          rectlist[x] = r_min;
                        }
                      else if (rect > r_max)
                        {
                          rectlist[x] = r_max;
                        }
                      else
                        {
                          rectlist[x] = rect;
                        }

                      if (rect > r_min)
                        {
                          gr_drawarc(-min(rect, r_max), min(rect, r_max), -min(rect, r_max), min(rect, r_max),
                                     bin_edges[x] * convert, bin_edges[x + 1] * convert);

                          gr_drawarc(-r_min, r_min, -r_min, r_min, bin_edges[x] * convert, bin_edges[x + 1] * convert);
                        }
                    }
                  /* no rlim */
                  else
                    {
                      *(rectlist + x) = rect;
                      if (x == num_bin_edges - 1)
                        {
                          break;
                        }

                      gr_drawarc(-rect, rect, -rect, rect, *(bin_edges + x) * convert, *(bin_edges + x + 1) * convert);
                    }
                }
            }

          if (phiflip == 0)
            outer += length;
          else
            {
              outer -= length;
            }
        } /* end of classes for loop */

      if (stairs != 0 && draw_edges == 0)
        {
          /* stairs without binedges, rlim */
          if (mlist != NULL && rlim == NULL && rectlist == NULL)
            {
              int s;
              double line_x[2];
              double line_y[2];
              for (s = 0; s < num_bins * 4; s += 2)
                {
                  if (s > 2 && s % 4 == 0)
                    {
                      line_x[0] = *(mlist + s);
                      line_x[1] = *(mlist + s - 2);
                      line_y[0] = *(mlist + s + 1);
                      line_y[1] = *(mlist + s - 1);

                      gr_polyline(2, line_x, line_y);
                    }
                }
              line_x[0] = *(mlist);
              line_x[1] = *(mlist + (num_bins - 1) * 4 + 2);
              line_y[0] = *(mlist + 1);
              line_y[1] = *(mlist + (num_bins - 1) * 4 + 3);
              gr_polyline(2, line_x, line_y);
            }

          /* stairs without bin_edges with rlim*/
          else if (mlist != NULL && rlim != NULL && rectlist == NULL)
            {
              double line_x[2], line_y[2];
              double rect1, rect2;
              for (x = 0; x < num_bins; ++x)
                {
                  if (x > 0)
                    {
                      rect1 = sqrt(pow(mlist[x * 4], 2) + pow(mlist[x * 4 + 1], 2));
                      rect2 = sqrt(pow(mlist[(x - 1) * 4 + 2], 2) + pow(mlist[(x - 1) * 4 + 3], 2));

                      if (rect1 < r_min && rect2 < r_min) continue;
                      if (rect1 < r_min)
                        {
                          mlist[4 * x] = r_min * cos(2 * M_PI / num_bins * x);
                          mlist[4 * x + 1] = r_min * sin(2 * M_PI / num_bins * x);
                        }
                      else if (rect2 < r_min)
                        {
                          mlist[(x - 1) * 4 + 2] = r_min * cos(2 * M_PI / num_bins * x);
                          mlist[(x - 1) * 4 + 3] = r_min * sin(2 * M_PI / num_bins * x);
                        }
                      line_x[0] = mlist[x * 4];
                      line_x[1] = mlist[(x - 1) * 4 + 2];
                      line_y[0] = mlist[x * 4 + 1];
                      line_y[1] = mlist[(x - 1) * 4 + 3];
                      gr_polyline(2, line_x, line_y);
                    }
                }
              line_x[0] = mlist[(num_bins - 1) * 4 + 2] = max(mlist[(num_bins - 1) * 4 + 2], r_min * cos(0));
              line_y[0] = mlist[(num_bins - 1) * 4 + 3] = max(mlist[(num_bins - 1) * 4 + 3], r_min * sin(0));
              line_x[1] = mlist[0] = max(mlist[0], r_min * cos(0));
              line_y[1] = mlist[1] = max(mlist[1], r_min * sin(0));

              gr_polyline(2, line_x, line_y);
            }

          /* stairs with binedges without rlim */
          else if (rectlist != NULL && rlim == NULL)
            {
              double startx = 0.0, starty = 0.0;
              double line_x[2], line_y[2];

              for (x = 0; x < num_bin_edges - 1; ++x)
                {
                  line_x[0] = startx;
                  line_x[1] = *(rectlist + x) * cos(*(bin_edges + x));
                  line_y[0] = starty;
                  line_y[1] = *(rectlist + x) * sin(*(bin_edges + x));

                  startx = *(rectlist + x) * cos(*(bin_edges + x + 1));
                  starty = *(rectlist + x) * sin(*(bin_edges + x + 1));

                  if (!(*bin_edges == 0.0 && *(bin_edges + num_bin_edges - 1) > 1.96 * M_PI) || x > 0)
                    {
                      gr_polyline(2, line_x, line_y);
                    }
                }

              if (*bin_edges == 0.0 && *(bin_edges + num_bin_edges - 1) > 1.96 * M_PI)
                {
                  line_x[0] = *rectlist * cos(*bin_edges);
                  line_x[1] = startx;
                  line_y[0] = *rectlist * sin(*bin_edges);
                  line_y[1] = starty;
                  gr_polyline(2, line_x, line_y);
                }
              else
                {
                  line_x[0] = *(rectlist + num_bin_edges - 2) * cos(*(bin_edges + num_bin_edges - 1));
                  line_x[1] = 0.0;
                  line_y[0] = *(rectlist + num_bin_edges - 2) * sin(*(bin_edges + num_bin_edges - 1));
                  line_y[1] = 0.0;
                  gr_polyline(2, line_x, line_y);
                }
            }

          /* stairs with bin_edges and rlim */
          else if (rectlist != NULL && rlim != NULL)
            {
              double startx = max(rectlist[0] * cos(bin_edges[0]), r_min * cos(bin_edges[0]));
              double starty = max(rectlist[0] * sin(bin_edges[0]), r_min * sin(bin_edges[0]));

              double line_x[2];
              double line_y[2];

              for (x = 0; x < num_bin_edges - 1; ++x)
                {
                  *line_x = startx;
                  *(line_x + 1) = *(rectlist + x) * cos(*(bin_edges + x));
                  *line_y = starty;
                  *(line_y + 1) = *(rectlist + x) * sin(*(bin_edges + x));


                  startx = *(rectlist + x) * cos(*(bin_edges + x + 1));
                  starty = *(rectlist + x) * sin(*(bin_edges + x + 1));

                  if (((phiflip == 0) &&
                       (!((*bin_edges > 0.0 && *bin_edges < 0.001) && *(bin_edges + num_bin_edges - 1) > 1.96 * M_PI) ||
                        x > 0)) ||
                      ((*bin_edges > 1.96 * M_PI &&
                        !(*(bin_edges + num_bin_edges - 1) > 0.0 && *(bin_edges + num_bin_edges - 1) < 0.001)) ||
                       x > 0))
                    {
                      gr_polyline(2, line_x, line_y);
                    }
                }

              if (*bin_edges == 0.0 && *(bin_edges + num_bin_edges - 1) > 1.96 * M_PI)
                {
                  *line_x = *rectlist * cos(*bin_edges);
                  *(line_x + 1) = rectlist[num_bin_edges - 2] * cos(bin_edges[num_bin_edges - 1]);
                  *line_y = *rectlist * sin(*bin_edges);
                  *(line_y + 1) = rectlist[num_bin_edges - 2] * sin(bin_edges[num_bin_edges - 1]);
                  gr_polyline(2, line_x, line_y);
                }
              else
                {
                  *line_x = *(rectlist + num_bin_edges - 2) * cos(*(bin_edges + num_bin_edges - 1));
                  *(line_x + 1) = r_min * cos(bin_edges[num_bin_edges - 1]);
                  *line_y = *(rectlist + num_bin_edges - 2) * sin(*(bin_edges + num_bin_edges - 1));
                  *(line_y + 1) = r_min * sin(bin_edges[num_bin_edges - 1]);

                  gr_polyline(2, line_x, line_y);

                  line_x[0] = r_min * cos(bin_edges[0]);
                  line_x[1] = rectlist[0] * cos(bin_edges[0]);
                  line_y[0] = r_min * sin(bin_edges[0]);
                  line_y[1] = rectlist[0] * sin(bin_edges[0]);

                  gr_polyline(2, line_x, line_y);
                }
            }
        }
    }


  gr_updatews();
  gr_updategks();
  gr_setresamplemethod(resample);

cleanup:
  free(mlist);
  free(rectlist);
  free(lineardata);
  free(bin_counts);
  if (freeable_bin_widths == 1)
    {
      free(bin_widths);
    }
  if (freeable_bin_edges == 1)
    {
      free(bin_edges);
    }
  if (freeable_angles == 1)
    {
      free(angles);
    }
  free(phi_array);
  free(r_min_list);
  free(r_min_list2);
  free(f1);
  free(f2);
  free(arc_2_x);
  free(arc_2_y);
  free(liste);
  free(liste2);
  free(colormap);

  return error;
}

error_t plot_trisurf(grm_args_t *subplot_args)
{
  grm_args_t **current_series;

  args_values(subplot_args, "series", "A", &current_series);
  while (*current_series != NULL)
    {
      double *x, *y, *z;
      unsigned int x_length, y_length, z_length;
      return_error_if(!args_first_value(*current_series, "x", "D", &x, &x_length), ERROR_PLOT_MISSING_DATA);
      return_error_if(!args_first_value(*current_series, "y", "D", &y, &y_length), ERROR_PLOT_MISSING_DATA);
      return_error_if(!args_first_value(*current_series, "z", "D", &z, &z_length), ERROR_PLOT_MISSING_DATA);
      return_error_if(x_length != y_length || x_length != z_length, ERROR_PLOT_COMPONENT_LENGTH_MISMATCH);
      gr_trisurface(x_length, x, y, z);
      ++current_series;
    }
  plot_draw_axes(subplot_args, 2);
  plot_draw_colorbar(subplot_args, 0.05, 256);

  return NO_ERROR;
}

error_t plot_tricont(grm_args_t *subplot_args)
{
  double z_min, z_max;
  double *levels;
  int num_levels;
  grm_args_t **current_series;
  int i;

  args_values(subplot_args, "_zlim", "dd", &z_min, &z_max);
  args_values(subplot_args, "levels", "i", &num_levels);
  levels = malloc(num_levels * sizeof(double));
  if (levels == NULL)
    {
      debug_print_malloc_error();
      return ERROR_MALLOC;
    }
  for (i = 0; i < num_levels; ++i)
    {
      levels[i] = z_min + ((1.0 * i) / (num_levels - 1)) * (z_max - z_min);
    }
  args_values(subplot_args, "series", "A", &current_series);
  while (*current_series != NULL)
    {
      double *x, *y, *z;
      unsigned int x_length, y_length, z_length;
      return_error_if(!args_first_value(*current_series, "x", "D", &x, &x_length), ERROR_PLOT_MISSING_DATA);
      return_error_if(!args_first_value(*current_series, "y", "D", &y, &y_length), ERROR_PLOT_MISSING_DATA);
      return_error_if(!args_first_value(*current_series, "z", "D", &z, &z_length), ERROR_PLOT_MISSING_DATA);
      return_error_if(x_length != y_length || x_length != z_length, ERROR_PLOT_COMPONENT_LENGTH_MISMATCH);
      gr_tricontour(x_length, x, y, z, num_levels, levels);
      ++current_series;
    }
  plot_draw_axes(subplot_args, 2);
  plot_draw_colorbar(subplot_args, 0.05, 256);
  free(levels);

  return NO_ERROR;
}

error_t plot_shade(grm_args_t *subplot_args)
{
  grm_args_t **current_shader;
  const char *data_component_names[] = {"x", "y", NULL};
  double *components[2];
  /* char *spec = ""; TODO: read spec from data! */
  int xform, xbins, ybins;
  double **current_component = components;
  const char **current_component_name = data_component_names;
  unsigned int point_count;

  args_values(subplot_args, "series", "A", &current_shader);
  while (*current_component_name != NULL)
    {
      args_first_value(*current_shader, *current_component_name, "D", current_component, &point_count);
      ++current_component_name;
      ++current_component;
    }
  if (!args_values(subplot_args, "xform", "i", &xform))
    {
      xform = 1;
    }
  if (!args_values(subplot_args, "xbins", "i", &xbins))
    {
      xbins = 100;
    }
  if (!args_values(subplot_args, "ybins", "i", &ybins))
    {
      ybins = 100;
    }
  gr_shadepoints(point_count, components[0], components[1], xform, xbins, ybins);

  return NO_ERROR;
}

error_t plot_raw(grm_args_t *plot_args)
{
  const char *base64_data = NULL;
  char *graphics_data = NULL;
  error_t error = NO_ERROR;

  cleanup_and_set_error_if(!args_values(plot_args, "raw", "s", &base64_data), ERROR_PLOT_MISSING_DATA);
  graphics_data = base64_decode(NULL, base64_data, NULL, &error);
  cleanup_if_error;
  gr_clearws();
  gr_drawgraphics(graphics_data);
  gr_updatews();

cleanup:
  if (graphics_data != NULL)
    {
      free(graphics_data);
    }

  return error;
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~ auxiliary drawing functions ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

error_t plot_draw_axes(grm_args_t *args, unsigned int pass)
{
  const char *kind = NULL;
  const double *viewport, *vp;
  double x_tick;
  double x_org_low, x_org_high;
  int x_major_count;
  double y_tick;
  double y_org_low, y_org_high;
  int y_major_count;
  double z_tick;
  double z_org_low, z_org_high;
  int z_major_count;
  double diag;
  double charheight;
  double ticksize;
  char *title;
  char *x_label, *y_label, *z_label;

  args_values(args, "kind", "s", &kind);
  args_values(args, "viewport", "D", &viewport);
  args_values(args, "vp", "D", &vp);
  args_values(args, "xtick", "d", &x_tick);
  args_values(args, "xorg", "dd", &x_org_low, &x_org_high);
  args_values(args, "xmajor", "i", &x_major_count);
  args_values(args, "ytick", "d", &y_tick);
  args_values(args, "yorg", "dd", &y_org_low, &y_org_high);
  args_values(args, "ymajor", "i", &y_major_count);

  gr_setlinecolorind(1);
  gr_setlinewidth(1);
  diag = sqrt((viewport[1] - viewport[0]) * (viewport[1] - viewport[0]) +
              (viewport[3] - viewport[2]) * (viewport[3] - viewport[2]));
  charheight = max(0.018 * diag, 0.012);
  gr_setcharheight(charheight);
  ticksize = 0.0075 * diag;
  if (str_equals_any(kind, 5, "wireframe", "surface", "plot3", "scatter3", "trisurf"))
    {
      args_values(args, "ztick", "d", &z_tick);
      args_values(args, "zorg", "dd", &z_org_low, &z_org_high);
      args_values(args, "zmajor", "i", &z_major_count);
      if (pass == 1)
        {
          gr_grid3d(x_tick, 0, z_tick, x_org_low, y_org_high, z_org_low, 2, 0, 2);
          gr_grid3d(0, y_tick, 0, x_org_low, y_org_high, z_org_low, 0, 2, 0);
        }
      else
        {
          gr_axes3d(x_tick, 0, z_tick, x_org_low, y_org_low, z_org_low, x_major_count, 0, z_major_count, -ticksize);
          gr_axes3d(0, y_tick, 0, x_org_high, y_org_low, z_org_low, 0, y_major_count, 0, ticksize);
        }
    }
  else if (!str_equals_any(kind, 2, "imshow", "isosurface"))
    {
      if (str_equals_any(kind, 2, "heatmap", "shade"))
        {
          ticksize = -ticksize;
        }
      if (!str_equals_any(kind, 1, "shade"))
        {
          gr_grid(x_tick, y_tick, 0, 0, x_major_count, y_major_count);
        }
      gr_axes(x_tick, y_tick, x_org_low, y_org_low, x_major_count, y_major_count, ticksize);
      gr_axes(x_tick, y_tick, x_org_high, y_org_high, -x_major_count, -y_major_count, -ticksize);
    }

  if (args_values(args, "title", "s", &title))
    {
      gr_savestate();
      gr_settextalign(GKS_K_TEXT_HALIGN_CENTER, GKS_K_TEXT_VALIGN_TOP);
      gr_textext(0.5 * (viewport[0] + viewport[1]), vp[3], title);
      gr_restorestate();
    }

  if (str_equals_any(kind, 5, "wireframe", "surface", "plot3", "scatter3", "trisurf"))
    {
      if (args_values(args, "xlabel", "s", &x_label) && args_values(args, "ylabel", "s", &y_label) &&
          args_values(args, "zlabel", "s", &z_label))
        {
          gr_titles3d(x_label, y_label, z_label);
        }
    }
  else
    {
      if (args_values(args, "xlabel", "s", &x_label))
        {
          gr_savestate();
          gr_settextalign(GKS_K_TEXT_HALIGN_CENTER, GKS_K_TEXT_VALIGN_BOTTOM);
          gr_textext(0.5 * (viewport[0] + viewport[1]), vp[2] + 0.5 * charheight, x_label);
          gr_restorestate();
        }
      if (args_values(args, "ylabel", "s", &y_label))
        {
          gr_savestate();
          gr_settextalign(GKS_K_TEXT_HALIGN_CENTER, GKS_K_TEXT_VALIGN_TOP);
          gr_setcharup(-1, 0);
          gr_textext(vp[0] + 0.5 * charheight, 0.5 * (viewport[2] + viewport[3]), y_label);
          gr_restorestate();
        }
    }
  if (strcmp("barplot", kind) == 0)
    {
      grm_args_t **current_series;
      args_values(args, "series", "A", &current_series);
      char **xnotations = NULL;
      unsigned int xnotations_length;
      int i;
      if (args_first_value(args, "xnotations", "S", &xnotations, &xnotations_length))
        {
          double x1, x2;
          double *y;
          unsigned int y_length;
          const double *window;
          return_error_if(!args_first_value(*current_series, "y", "D", &y, &y_length), ERROR_PLOT_MISSING_DATA);
          return_error_if(xnotations_length != y_length, ERROR_PLOT_COMPONENT_LENGTH_MISMATCH);
          args_values(args, "window", "D", &window);
          gr_setcharheight(charheight);
          gr_settextalign(2, 1);
          for (i = 1; i <= y_length; i++)
            {
              x1 = viewport[0] + ((viewport[1] - viewport[0]) * i) / (window[1] - window[0]);
              x2 = viewport[2] - 0.5 * charheight;
              gr_textext(x1, x2, xnotations[i - 1]);
            }
        }
    }

  return NO_ERROR;
}

error_t plot_draw_polar_axes(grm_args_t *args)
{
  const double *window, *viewport, *vp;
  double diag;
  double charheight;
  double r_min, r_max;
  double tick;
  double x[2], y[2];
  int i, n;
  double alpha;
  char text_buffer[PLOT_POLAR_AXES_TEXT_BUFFER];
  char *kind;
  int angle_ticks, rings;
  int phiflip = 0;
  double interval;
  char *title;


  args_values(args, "viewport", "D", &viewport);
  args_values(args, "vp", "D", &vp);
  diag = sqrt((viewport[1] - viewport[0]) * (viewport[1] - viewport[0]) +
              (viewport[3] - viewport[2]) * (viewport[3] - viewport[2]));
  charheight = max(0.018 * diag, 0.012);

  args_values(args, "window", "D", &window);
  r_min = window[2];
  r_max = window[3];

  if (args_values(args, "angle_ticks", "i", &angle_ticks) == 0)
    {
      angle_ticks = 12;
    }
  if (args_values(args, "rings", "i", &rings) == 0)
    {
      rings = 4;
    }

  args_values(args, "kind", "s", &kind);


  gr_savestate();
  gr_setcharheight(charheight);
  gr_setlinetype(GKS_K_LINETYPE_SOLID);


  if (strcmp(kind, "polar_histogram") == 0)
    {
      const char *norm;
      r_min = 0.0;
      if (args_values(args, "normalization", "s", &norm) == 0)
        {
          norm = "count";
        }

      args_values(args, "r_max", "d", &r_max);

      if (str_equals_any(norm, 2, "count", "cumcount"))
        {
          tick = 1.5 * gr_tick(r_min, r_max);
        }
      else if (str_equals_any(norm, 2, "pdf", "probability"))
        {
          tick = 1.5 * gr_tick(r_min, r_max);
        }
      else if (strcmp(norm, "countdensity") == 0)
        {
          tick = 1.5 * gr_tick(r_min, r_max);
        }

      else if (strcmp(norm, "cdf") == 0)
        {
          tick = 1.0 / rings;
        }
    }
  else
    {
      tick = 0.5 * gr_tick(r_min, r_max);
    }

  n = rings;
  gr_setlinecolorind(88);
  if (args_values(args, "phiflip", "i", &phiflip) == 0) phiflip = 0;
  for (i = 0; i <= n; i++)
    {
      double r = (i * 1.0) / n;
      gr_drawarc(-r, r, -r, r, 0, 180);
      gr_drawarc(-r, r, -r, r, 180, 360);
      gr_settextalign(GKS_K_TEXT_HALIGN_LEFT, GKS_K_TEXT_VALIGN_HALF);
      x[0] = 0.05;
      y[0] = r;
      gr_wctondc(x, y);
      snprintf(text_buffer, PLOT_POLAR_AXES_TEXT_BUFFER, "%g", r_min + i * tick);
      gr_text(x[0], y[0], text_buffer);
    }
  if (strcmp(kind, "polar_histogram") == 0)
    {
      grm_args_push(args, "r_max", "d", r_min + n * tick);
    }
  interval = 360.0 / angle_ticks;
  for (alpha = 0.0; alpha < 360; alpha += interval)
    {
      x[0] = cos(alpha * M_PI / 180.0);
      y[0] = sin(alpha * M_PI / 180.0);
      x[1] = 0.0;
      y[1] = 0.0;
      gr_polyline(2, x, y);
      gr_settextalign(GKS_K_TEXT_HALIGN_CENTER, GKS_K_TEXT_VALIGN_HALF);
      x[0] *= 1.1;
      y[0] *= 1.1;
      gr_wctondc(x, y);
      if (phiflip == 0)
        {
          snprintf(text_buffer, PLOT_POLAR_AXES_TEXT_BUFFER, "%d\xc2\xb0", (int)round(alpha));
        }
      else
        {
          if (alpha == 0.0)
            snprintf(text_buffer, PLOT_POLAR_AXES_TEXT_BUFFER, "%d\xc2\xb0", 0);
          else
            snprintf(text_buffer, PLOT_POLAR_AXES_TEXT_BUFFER, "%d\xc2\xb0", 330 - (int)round(alpha - interval));
        }
      gr_textext(x[0], y[0], text_buffer);
    }
  gr_restorestate();

  if (args_values(args, "title", "s", &title))
    {
      gr_savestate();
      gr_settextalign(GKS_K_TEXT_HALIGN_CENTER, GKS_K_TEXT_VALIGN_TOP);
      gr_textext(0.5 * (viewport[0] + viewport[1]), vp[3] - 0.02, title);
      gr_restorestate();
    }

  return NO_ERROR;
}

error_t plot_draw_legend(grm_args_t *subplot_args)
{
  const char **labels, **current_label;
  unsigned int num_labels, num_series;
  grm_args_t **current_series;
  const double *viewport;
  int location;
  double px, py, w, h;
  double tbx[4], tby[4];
  double legend_symbol_x[2], legend_symbol_y[2];


  return_error_if(!args_first_value(subplot_args, "labels", "S", &labels, &num_labels), ERROR_PLOT_MISSING_LABELS);
  logger((stderr, "Draw a legend with %d labels\n", num_labels));
  args_first_value(subplot_args, "series", "A", &current_series, &num_series);
  args_values(subplot_args, "viewport", "D", &viewport);
  args_values(subplot_args, "location", "i", &location);
  gr_savestate();
  gr_selntran(0);
  gr_setscale(0);
  w = 0;
  for (current_label = labels; *current_label != NULL; ++current_label)
    {
      gr_inqtextext(0, 0, *(char **)current_label, tbx, tby);
      w = max(w, tbx[2]);
    }

  h = (num_series + 1) * 0.03;
  if (int_equals_any(location, 3, 8, 9, 10))
    {
      px = 0.5 * (viewport[0] + viewport[1] - w);
    }
  else if (int_equals_any(location, 3, 2, 3, 6))
    {
      px = viewport[0] + 0.11;
    }
  else
    {
      px = viewport[1] - 0.05 - w;
    }
  if (int_equals_any(location, 4, 5, 6, 7, 10))
    {
      py = 0.5 * (viewport[2] + viewport[3] + h) - 0.03;
    }
  else if (int_equals_any(location, 3, 3, 4, 8))
    {
      py = viewport[2] + h;
    }
  else
    {
      py = viewport[3] - 0.06;
    }

  gr_setfillintstyle(GKS_K_INTSTYLE_SOLID);
  gr_setfillcolorind(0);
  gr_fillrect(px - 0.08, px + w + 0.02, py + 0.03, py - 0.03 * num_series);
  gr_setlinetype(GKS_K_INTSTYLE_SOLID);
  gr_setlinecolorind(1);
  gr_setlinewidth(1);
  gr_drawrect(px - 0.08, px + w + 0.02, py + 0.03, py - 0.03 * num_series);
  gr_uselinespec(" ");
  current_label = labels;
  while (*current_series != NULL)
    {
      char *spec;
      int mask;

      gr_savestate();
      args_values(*current_series, "spec", "s", &spec); /* `spec` is always set */
      mask = gr_uselinespec(spec);
      if (int_equals_any(mask, 5, 0, 1, 3, 4, 5))
        {
          legend_symbol_x[0] = px - 0.07;
          legend_symbol_x[1] = px - 0.01;
          legend_symbol_y[0] = py;
          legend_symbol_y[1] = py;
          gr_polyline(2, legend_symbol_x, legend_symbol_y);
        }
      if (mask & 2)
        {
          legend_symbol_x[0] = px - 0.06;
          legend_symbol_x[1] = px - 0.02;
          legend_symbol_y[0] = py;
          legend_symbol_y[1] = py;
          gr_polymarker(2, legend_symbol_x, legend_symbol_y);
        }
      gr_restorestate();
      gr_settextalign(GKS_K_TEXT_HALIGN_LEFT, GKS_K_TEXT_VALIGN_HALF);
      if (*current_label != NULL)
        {
          gr_textext(px, py, (char *)*current_label);
          ++current_label;
        }
      py -= 0.03;
      ++current_series;
    }
  gr_selntran(1);
  gr_restorestate();

  return NO_ERROR;
}

error_t plot_draw_colorbar(grm_args_t *subplot_args, double off, unsigned int colors)
{
  const double *viewport;
  double c_min, c_max;
  int *data;
  double diag, charheight;
  int scale, flip, options;
  unsigned int i;
  error_t error;

  gr_savestate();
  args_values(subplot_args, "viewport", "D", &viewport);
  /* TODO: What to do, if there is a `_clim` and a `_zlim`? Merge both together? */
  if (!args_values(subplot_args, "_clim", "dd", &c_min, &c_max))
    {
      error = args_values(subplot_args, "_zlim", "dd", &c_min, &c_max);
      if (!error)
        {
          return error;
        }
    }
  data = malloc(colors * sizeof(int));
  if (data == NULL)
    {
      debug_print_malloc_error();
      return ERROR_MALLOC;
    }
  for (i = 0; i < colors; ++i)
    {
      data[i] = 1000 + 255 * i / (colors - 1);
    }
  gr_inqscale(&options);
  if (args_values(subplot_args, "xflip", "i", &flip) && flip)
    {
      options = (options | GR_OPTION_FLIP_Y) & ~GR_OPTION_FLIP_X;
      gr_setscale(options);
    }
  else if (args_values(subplot_args, "yflip", "i", &flip) && flip)
    {
      options = options & ~GR_OPTION_FLIP_Y & ~GR_OPTION_FLIP_X;
      gr_setscale(options);
    }
  else
    {
      options = options & ~GR_OPTION_FLIP_X;
      gr_setscale(options);
    }
  gr_setwindow(0.0, 1.0, c_min, c_max);
  gr_setviewport(viewport[1] + 0.02 + off, viewport[1] + 0.05 + off, viewport[2], viewport[3]);
  gr_cellarray(0, 1, c_max, c_min, 1, colors, 1, 1, 1, colors, data);
  diag = sqrt((viewport[1] - viewport[0]) * (viewport[1] - viewport[0]) +
              (viewport[3] - viewport[2]) * (viewport[3] - viewport[2]));
  charheight = max(0.016 * diag, 0.012);
  gr_setcharheight(charheight);
  args_values(subplot_args, "scale", "i", &scale);
  if (scale & GR_OPTION_Z_LOG)
    {
      gr_setscale(GR_OPTION_Y_LOG);
      gr_axes(0, 2, 1, c_min, 0, 1, 0.005);
    }
  else
    {
      double c_tick = 0.5 * gr_tick(c_min, c_max);
      gr_axes(0, c_tick, 1, c_min, 0, 1, 0.005);
    }
  free(data);
  gr_restorestate();

  return NO_ERROR;
}

error_t extract_multi_type_argument(grm_args_t *error_container, const char *key, unsigned int x_length,
                                    unsigned int *downwards_length, unsigned int *upwards_length, double **downwards,
                                    double **upwards, double *downwards_flt, double *upwards_flt)
{
  arg_t *arg_ptr;
  args_value_iterator_t *value_it;
  unsigned int length;
  int i, *ii;

  arg_ptr = args_at(error_container, key);
  if (!arg_ptr)
    {
      return NO_ERROR;
    }
  if (strcmp(arg_ptr->value_format, "nDnD") == 0)
    {
      value_it = arg_value_iter(arg_ptr);
      ARGS_VALUE_ITERATOR_GET(value_it, *downwards_length, *downwards);
      ARGS_VALUE_ITERATOR_GET(value_it, *upwards_length, *upwards);
      args_value_iterator_delete(value_it);

      return_error_if(*downwards_length != *upwards_length || *downwards_length != x_length,
                      ERROR_PLOT_COMPONENT_LENGTH_MISMATCH);
    }
  else if (strcmp(arg_ptr->value_format, "nD") == 0)
    {
      return_error_if(!args_first_value(error_container, key, "D", downwards, downwards_length), ERROR_INTERNAL);
      /* Python encapsules all single elements into an array */
      if (*downwards_length == 1)
        {
          *downwards_flt = *upwards_flt = **downwards;
          *downwards = NULL;
          *downwards_length = 0;
          return NO_ERROR;
        }
      return_error_if(*downwards_length != x_length, ERROR_PLOT_COMPONENT_LENGTH_MISMATCH);
      *upwards = *downwards;
      *upwards_length = *downwards_length;
    }
  else if (strcmp(arg_ptr->value_format, "d") == 0)
    {
      return_error_if(!args_values(error_container, key, "d", downwards_flt), ERROR_INTERNAL);
      *upwards_flt = *downwards_flt;
    }
  else if (strcmp(arg_ptr->value_format, "nI") == 0)
    {
      return_error_if(!args_first_value(error_container, key, "nI", &ii, &length), ERROR_INTERNAL);
      return_error_if(length != 1, ERROR_PLOT_COMPONENT_LENGTH_MISMATCH);
      *upwards_flt = *downwards_flt = (double)ii[0];
    }
  else if (strcmp(arg_ptr->value_format, "i") == 0)
    {
      return_error_if(!args_values(error_container, key, "i", &i), ERROR_INTERNAL);
      *upwards_flt = *downwards_flt = (double)i;
    }
  return NO_ERROR;
}

error_t plot_draw_errorbars(grm_args_t *series_args, double *x, unsigned int x_length, double *y, char *kind)
{
  grm_args_t *error_container;
  arg_t *arg_ptr;
  error_t error;

  double *absolute_upwards, *absolute_downwards, *relative_upwards, *relative_downwards;
  double absolute_upwards_flt, relative_upwards_flt, absolute_downwards_flt, relative_downwards_flt;
  unsigned int upwards_length, downwards_length, i;
  int scale_options, color_upwardscap, color_downwardscap, color_errorbar;
  int is_barplot;

  double marker_size, xmin, xmax, ymin, ymax, tick, a, b, e_upwards, e_downwards, x_value;
  double line_x[2], line_y[2];
  absolute_upwards = absolute_downwards = relative_upwards = relative_downwards = NULL;
  absolute_upwards_flt = absolute_downwards_flt = relative_upwards_flt = relative_downwards_flt = FLT_MAX;
  is_barplot = strcmp(kind, "barplot") == 0 ? 1 : 0;

  arg_ptr = args_at(series_args, "error");
  if (!arg_ptr)
    {
      return NO_ERROR;
    }
  error_container = NULL;
  if (strcmp(arg_ptr->value_format, "a") == 0 || strcmp(arg_ptr->value_format, "nA") == 0)
    {
      return_error_if(!args_values(series_args, "error", "a", &error_container), ERROR_INTERNAL);

      error = extract_multi_type_argument(error_container, "absolute", x_length - is_barplot, &downwards_length,
                                          &upwards_length, &absolute_downwards, &absolute_upwards,
                                          &absolute_downwards_flt, &absolute_upwards_flt);
      return_if_error;
      error = extract_multi_type_argument(error_container, "relative", x_length - is_barplot, &downwards_length,
                                          &upwards_length, &relative_downwards, &relative_upwards,
                                          &relative_downwards_flt, &relative_upwards_flt);
      return_if_error;
    }
  else
    {
      error = extract_multi_type_argument(series_args, "error", x_length - is_barplot, &downwards_length,
                                          &upwards_length, &absolute_downwards, &absolute_upwards,
                                          &absolute_downwards_flt, &absolute_upwards_flt);
      return_if_error;
    }

  if (absolute_upwards == NULL && relative_upwards == NULL && absolute_upwards_flt == FLT_MAX &&
      relative_upwards_flt == FLT_MAX && absolute_downwards == NULL && relative_downwards == NULL &&
      absolute_downwards_flt == FLT_MAX && relative_downwards_flt == FLT_MAX)
    {
      return ERROR_PLOT_MISSING_DATA;
    }

  /* Getting GR options and sizes. See gr_verrorbars. */
  gr_savestate();
  gr_inqmarkersize(&marker_size);
  gr_inqwindow(&xmin, &xmax, &ymin, &ymax);
  gr_inqscale(&scale_options);
  tick = marker_size * 0.0075 * (xmax - xmin);
  a = (xmax - xmin) / log10(xmax / xmin);
  b = xmin - a * log10(xmin);

  gr_inqlinecolorind(&color_errorbar);
  color_upwardscap = color_downwardscap = color_errorbar;
  if (error_container != NULL)
    {
      args_values(error_container, "upwardscap_color", "i", &color_upwardscap);
      args_values(error_container, "downwardscap_color", "i", &color_downwardscap);
      args_values(error_container, "errorbar_color", "i", &color_errorbar);
    }

  /* Actual drawing of bars */
  e_upwards = e_downwards = FLT_MAX;
  for (i = 0; i < x_length - is_barplot; i++)
    {
      if (absolute_upwards != NULL || relative_upwards != NULL || absolute_upwards_flt != FLT_MAX ||
          relative_upwards_flt != FLT_MAX)
        {
          e_upwards =
              y[i] * (1. + (relative_upwards != NULL ? relative_upwards[i]
                                                     : (relative_upwards_flt != FLT_MAX ? relative_upwards_flt : 0))) +
              (absolute_upwards != NULL ? absolute_upwards[i]
                                        : (absolute_upwards_flt != FLT_MAX ? absolute_upwards_flt : 0.));
        }

      if (absolute_downwards != NULL || relative_downwards != NULL || absolute_downwards_flt != FLT_MAX ||
          relative_downwards_flt != FLT_MAX)
        {
          e_downwards =
              y[i] * (1. - (relative_downwards != NULL
                                ? relative_downwards[i]
                                : (relative_downwards_flt != FLT_MAX ? relative_downwards_flt : 0))) -
              (absolute_downwards != NULL ? absolute_downwards[i]
                                          : (absolute_downwards_flt != FLT_MAX ? absolute_downwards_flt : 0.));
        }

      /* See gr_verrorbars for reference */
      x_value = is_barplot ? (x[i] + x[i + 1]) / 2 : x[i];
      line_x[0] = X_LOG(X_LIN(x_value - tick, scale_options, xmin, xmax, a, b), scale_options, xmin, xmax, a, b);
      line_x[1] = X_LOG(X_LIN(x_value + tick, scale_options, xmin, xmax, a, b), scale_options, xmin, xmax, a, b);
      if (e_upwards != FLT_MAX && color_upwardscap >= 0)
        {
          line_y[0] = e_upwards;
          line_y[1] = e_upwards;
          gr_setlinecolorind(color_upwardscap);
          gr_polyline(2, line_x, line_y);
        }

      if (e_downwards != FLT_MAX && color_downwardscap >= 0)
        {
          line_y[0] = e_downwards;
          line_y[1] = e_downwards;
          gr_setlinecolorind(color_downwardscap);
          gr_polyline(2, line_x, line_y);
        }

      if (color_errorbar >= 0)
        {
          line_x[0] = x_value;
          line_x[1] = x_value;
          line_y[0] = e_upwards != FLT_MAX ? e_upwards : y[i];
          line_y[1] = e_downwards != FLT_MAX ? e_downwards : y[i];
          gr_setlinecolorind(color_errorbar);
          gr_polyline(2, line_x, line_y);
        }
    }
  gr_restorestate();

  return NO_ERROR;
}


/* ~~~~~~~~~~~~~~~~~~~~~~~~~ util ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

double find_max_step(unsigned int n, const double *x)
{
  double max_step = 0.0;
  unsigned int i;

  if (n < 2)
    {
      return 0.0;
    }
  for (i = 1; i < n; ++i)
    {
      max_step = max(x[i] - x[i - 1], max_step);
    }

  return max_step;
}

const char *next_fmt_key(const char *kind)
{
  static const char *saved_fmt = NULL;
  static char fmt_key[2] = {0, 0};

  if (kind != NULL)
    {
      string_map_at(fmt_map, kind, (char **)&saved_fmt);
    }
  if (saved_fmt == NULL)
    {
      return NULL;
    }
  fmt_key[0] = *saved_fmt;
  if (*saved_fmt != '\0')
    {
      ++saved_fmt;
    }

  return fmt_key;
}

const char *get_compatible_format(const char *key, const char *given_format)
{
  const char **valid_formats;
  char *reduced_given_format;
  const char **current_format_ptr;
  const char *compatible_format = NULL;
  /* First, get all valid formats */
  if (!string_array_map_at(type_map, key, (char ***)&valid_formats))
    {
      /* If the given key does not exist, there is no type constraint
       * -> simply return the same type that was given */
      return given_format;
    }
  /* Second, filter the given format -> remove `n` chars because they are irrelevant for the following tests */
  reduced_given_format = str_filter(given_format, "n");
  if (reduced_given_format == NULL)
    {
      debug_print_malloc_error();
      goto cleanup;
    }
  /* Third, iterate over all valid formats and check if
   * - there is an exact match
   * - a single format char is valid,
   *   the type matches (e.g. `d` and `D`) and
   *   a single value / array or multiple single values of the same type ar given (e.g. `dddd`)
   */
  current_format_ptr = valid_formats;
  while (*current_format_ptr != NULL)
    {
      if (strcmp(*current_format_ptr, reduced_given_format) == 0)
        {
          /* Found an exact match */
          compatible_format = *current_format_ptr;
          break;
        }
      else if ((strlen(*current_format_ptr) == 1) && (tolower(**current_format_ptr) == tolower(*reduced_given_format)))
        {
          /* The current format is a single format of the same base type as the given format */
          if (strlen(reduced_given_format) == 1 ||
              is_homogenous_string_of_char(reduced_given_format, tolower(*reduced_given_format)))
            {
              /* Either a single format of the same type or
               * a homogenous format string of single values of the same type
               * is given */
              compatible_format = *current_format_ptr;
              break;
            }
        }
      ++current_format_ptr;
    }

cleanup:
  free(reduced_given_format);

  /* The previous tests were not successful -> `NULL` indicates no compatible format */
  return compatible_format;
}

int get_id_from_args(const grm_args_t *args, int *plot_id, int *subplot_id, int *series_id)
{
  const char *combined_id;
  int _plot_id = -1, _subplot_id = 0, _series_id = 0;

  if (args_values(args, "id", "s", &combined_id))
    {
      const char *valid_id_delims = ":.";
      int *id_ptrs[4], **current_id_ptr;
      char *copied_id_str, *current_id_str;
      size_t segment_length;
      int is_last_segment;

      id_ptrs[0] = &_plot_id;
      id_ptrs[1] = &_subplot_id;
      id_ptrs[2] = &_series_id;
      id_ptrs[3] = NULL;
      if ((copied_id_str = gks_strdup(combined_id)) == NULL)
        {
          debug_print_malloc_error();
          return 0;
        }

      current_id_ptr = id_ptrs;
      current_id_str = copied_id_str;
      is_last_segment = 0;
      while (*current_id_ptr != NULL && !is_last_segment)
        {
          segment_length = strcspn(current_id_str, valid_id_delims);
          if (current_id_str[segment_length] == '\0')
            {
              is_last_segment = 1;
            }
          else
            {
              current_id_str[segment_length] = '\0';
            }
          if (*current_id_str != '\0')
            {
              if (!str_to_uint(current_id_str, (unsigned int *)*current_id_ptr))
                {
                  logger((stderr, "Got an invalid id \"%s\"\n", current_id_str));
                }
              else
                {
                  logger((stderr, "Read id: %d\n", **current_id_ptr));
                }
            }
          ++current_id_ptr;
          ++current_id_str;
        }

      free(copied_id_str);
    }
  else
    {
      args_values(args, "plot_id", "i", &_plot_id);
      args_values(args, "subplot_id", "i", &_subplot_id);
      args_values(args, "series_id", "i", &_series_id);
    }
  /* plot id `0` references the first plot object (implicit object) -> handle it as the first plot and shift all ids by
   * one */
  *plot_id = _plot_id + 1;
  *subplot_id = _subplot_id;
  *series_id = _series_id;

  return _plot_id > 0 || _subplot_id > 0 || _series_id > 0;
}

int get_figure_size(const grm_args_t *plot_args, int *pixel_width, int *pixel_height, double *metric_width,
                    double *metric_height)
{
  double display_metric_width, display_metric_height;
  int display_pixel_width, display_pixel_height;
  double dpm[2], dpi[2];
  int tmp_size_i[2], pixel_size[2];
  double tmp_size_d[2], metric_size[2];
  grm_args_ptr_t tmp_size_a[2];
  const char *tmp_size_s[2];
  int i;

  if (plot_args == NULL)
    {
      plot_args = active_plot_args;
    }

#ifdef __EMSCRIPTEN__
  display_metric_width = 0.16384;
  display_metric_height = 0.12288;
  display_pixel_width = 640;
  display_pixel_height = 480;
#else
  gr_inqdspsize(&display_metric_width, &display_metric_height, &display_pixel_width, &display_pixel_height);
#endif
  dpm[0] = display_pixel_width / display_metric_width;
  dpm[1] = display_pixel_height / display_metric_height;
  dpi[0] = dpm[0] * 0.0254;
  dpi[1] = dpm[1] * 0.0254;

  /* TODO: Overwork this calculation */
  if (args_values(plot_args, "figsize", "dd", &tmp_size_d[0], &tmp_size_d[1]))
    {
      for (i = 0; i < 2; ++i)
        {
          pixel_size[i] = (int)round(tmp_size_d[i] * dpi[i]);
          metric_size[i] = tmp_size_d[i] / 0.0254;
        }
    }
  else if (args_values(plot_args, "size", "dd", &tmp_size_d[0], &tmp_size_d[1]))
    {


      for (i = 0; i < 2; ++i)
        {
          pixel_size[i] = (int)round(tmp_size_d[i]);
          metric_size[i] = tmp_size_d[i] / dpm[i];
        }
    }
  else if (args_values(plot_args, "size", "ii", &tmp_size_i[0], &tmp_size_i[1]))
    {
      for (i = 0; i < 2; ++i)
        {
          pixel_size[i] = tmp_size_i[i];
          metric_size[i] = tmp_size_i[i] / dpm[i];
        }
    }
  else if (args_values(plot_args, "size", "aa", &tmp_size_a[0], &tmp_size_a[1]))
    {
      for (i = 0; i < 2; ++i)
        {
          double pixels_per_unit = 1;
          if (args_values(tmp_size_a[i], "unit", "s", &tmp_size_s[i]))
            {
              if (strcmp(tmp_size_s[i], "px") != 0)
                {
                  double meters_per_unit;
                  if (double_map_at(meters_per_unit_map, tmp_size_s[i], &meters_per_unit))
                    {
                      pixels_per_unit = meters_per_unit * dpm[i];
                    }
                  else
                    {
                      debug_print_error(("The unit %s is unknown.\n", tmp_size_s[i]));
                    }
                }
            }
          if (args_values(tmp_size_a[i], "value", "i", &tmp_size_i[i]))
            {
              tmp_size_d[i] = tmp_size_i[i] * pixels_per_unit;
            }
          else if (args_values(tmp_size_a[i], "value", "d", &tmp_size_d[i]))
            {
              tmp_size_d[i] = tmp_size_d[i] * pixels_per_unit;
            }
          else
            {
              /* If no value is given, fall back to default value */
              return 0;
            }
          pixel_size[i] = (int)round(tmp_size_d[i]);
          metric_size[i] = tmp_size_d[i] / dpm[i];
        }
    }
  else
    {
      /* If this branch is executed, there is an internal error (size has a default value if not set by the user) */
      return 0;
    }

  logger((stderr, "figure pixel size: (%d, %d)\n", pixel_size[0], pixel_size[1]));
  logger((stderr, "figure metric size: (%f, %f)\n", metric_size[0], metric_size[1]));
  logger((stderr, "device dpi: (%lf, %lf)\n", dpi[0], dpi[1]));

  if (pixel_width != NULL)
    {
      *pixel_width = pixel_size[0];
    }
  if (pixel_height != NULL)
    {
      *pixel_height = pixel_size[1];
    }
  if (metric_width != NULL)
    {
      *metric_width = metric_size[0];
    }
  if (metric_height != NULL)
    {
      *metric_height = metric_size[1];
    }

  return 1;
}

int get_focus_and_factor(const int x1, const int y1, const int x2, const int y2, const int keep_aspect_ratio,
                         double *factor_x, double *factor_y, double *focus_x, double *focus_y,
                         grm_args_t **subplot_args)
{
  double ndc_box_x[4], ndc_box_y[4];
  double ndc_left, ndc_top, ndc_right, ndc_bottom;
  const double *wswindow, *viewport;
  int width, height, max_width_height;

  get_figure_size(NULL, &width, &height, NULL, NULL);
  max_width_height = max(width, height);

  if (x1 <= x2)
    {
      ndc_left = (double)x1 / max_width_height;
      ndc_right = (double)x2 / max_width_height;
    }
  else
    {
      ndc_left = (double)x2 / max_width_height;
      ndc_right = (double)x1 / max_width_height;
    }
  if (y1 <= y2)
    {
      ndc_top = (double)(height - y1) / max_width_height;
      ndc_bottom = (double)(height - y2) / max_width_height;
    }
  else
    {
      ndc_top = (double)(height - y2) / max_width_height;
      ndc_bottom = (double)(height - y1) / max_width_height;
    }

  ndc_box_x[0] = ndc_left;
  ndc_box_y[0] = ndc_bottom;
  ndc_box_x[1] = ndc_right;
  ndc_box_y[1] = ndc_bottom;
  ndc_box_x[2] = ndc_left;
  ndc_box_y[2] = ndc_top;
  ndc_box_x[3] = ndc_right;
  ndc_box_y[3] = ndc_top;
  *subplot_args = get_subplot_from_ndc_points(array_size(ndc_box_x), ndc_box_x, ndc_box_y);
  if (*subplot_args == NULL)
    {
      return 0;
    }
  args_values(*subplot_args, "viewport", "D", &viewport);
  args_values(active_plot_args, "wswindow", "D", &wswindow);

  *factor_x = abs(x1 - x2) / (width * (viewport[1] - viewport[0]) / (wswindow[1] - wswindow[0]));
  *factor_y = abs(y1 - y2) / (height * (viewport[3] - viewport[2]) / (wswindow[3] - wswindow[2]));
  if (keep_aspect_ratio)
    {
      if (*factor_x <= *factor_y)
        {
          *factor_x = *factor_y;
          if (x1 > x2)
            {
              ndc_left = ndc_right - *factor_x * (viewport[1] - viewport[0]);
            }
        }
      else
        {
          *factor_y = *factor_x;
          if (y1 > y2)
            {
              ndc_top = ndc_bottom + *factor_y * (viewport[3] - viewport[2]);
            }
        }
    }
  *focus_x = (ndc_left - *factor_x * viewport[0]) / (1 - *factor_x) - (viewport[0] + viewport[1]) / 2.0;
  *focus_y = (ndc_top - *factor_y * viewport[3]) / (1 - *factor_y) - (viewport[2] + viewport[3]) / 2.0;
  return 1;
}
grm_args_t *get_subplot_from_ndc_point(double x, double y)
{
  grm_args_t **subplot_args;
  const double *viewport;

  args_values(active_plot_args, "subplots", "A", &subplot_args);
  while (*subplot_args != NULL)
    {
      if (args_values(*subplot_args, "viewport", "D", &viewport))
        {
          if (viewport[0] <= x && x <= viewport[1] && viewport[2] <= y && y <= viewport[3])
            {
              unsigned int array_index;
              args_values(*subplot_args, "array_index", "i", &array_index);
              logger((stderr, "Found subplot id \"%u\" for ndc point (%lf, %lf)\n", array_index + 1, x, y));

              return *subplot_args;
            }
        }
      ++subplot_args;
    }

  return NULL;
}

grm_args_t *get_subplot_from_ndc_points(unsigned int n, const double *x, const double *y)
{
  grm_args_t *subplot_args;
  unsigned int i;

  for (i = 0, subplot_args = NULL; i < n && subplot_args == NULL; ++i)
    {
      subplot_args = get_subplot_from_ndc_point(x[i], y[i]);
    }

  return subplot_args;
}

double *moivre(double r, int x, int n)
{
  double *result = (double *)malloc(2 * sizeof(double));
  if (result != NULL)
    {
      if (n != 0)
        {
          *result = pow(r, (1.0 / n)) * (cos(2.0 * x * M_PI / n));
          *(result + 1) = pow(r, (1.0 / n)) * (sin(2.0 * x * M_PI / n));
        }
      else
        {
          *result = 1.0;
          *(result + 1) = 0.0;
        }
    }
  return result;
}


/* like python list comprehension [factor * func(element) for element in list] saves values in result starting at start
 * index */

double *listcomprehension(double factor, double (*pFunction)(double), double *list, int num, int start, double *result)
{
  int i;
  if (result == NULL)
    {
      result = (double *)malloc(num * sizeof(double));
      if (result == NULL)
        {
          return NULL;
        }
    }

  for (i = 0; i < num; ++i)
    {
      result[start] = factor * (*pFunction)(list[i]);
      start++;
    }

  return result;
}

/*
 * mixes gr colormaps with size = size * size. If x and or y < 0
 * */
int *create_colormap(int x, int y, int size)
{
  int r, g, b, a;
  int outer, inner;
  int r1, g1, b1;
  int r2, g2, b2;
  int *colormap = NULL;
  if (x > 47 || y > 47)
    {
      logger((stderr, "values for the keyword \"colormap\" can not be greater than 47\n"));
      return NULL;
    }

  colormap = malloc(size * size * sizeof(int));
  if (colormap == NULL)
    {
      debug_print_malloc_error();
      return NULL;
    }
  if (x >= 0 && y < 0)
    {
      for (outer = 0; outer < size; outer++)
        {
          for (inner = 0; inner < size; inner++)
            {
              a = 255;
              r = ((cmap_h[x][(int)(inner * 255.0 / size)] >> 16) & 0xff);
              g = ((cmap_h[x][(int)(inner * 255.0 / size)] >> 8) & 0xff);
              b = (cmap_h[x][(int)(inner * 255.0 / size)] & 0xff);

              colormap[outer * size + inner] = (a << 24) + (b << 16) + (g << 8) + (r);
            }
        }
      return colormap;
    }

  if (x < 0 && y >= 0)
    {
      gr_setcolormap(y);
      for (outer = 0; outer < size; outer++)
        {
          for (inner = 0; inner < size; inner++)
            {
              a = 255;
              r = ((cmap_h[y][(int)(inner * 255.0 / size)] >> 16) & 0xff);
              g = ((cmap_h[y][(int)(inner * 255.0 / size)] >> 8) & 0xff);
              b = (cmap_h[y][(int)(inner * 255.0 / size)] & 0xff);

              colormap[inner * size + outer] = (a << 24) + (b << 16) + (g << 8) + (r);
            }
        }


      return colormap;
    }

  if ((x >= 0 && y >= 0) || (x < 0 && y < 0))
    {
      if (x < 0 && y < 0)
        {
          x = y = 0;
        }
      gr_setcolormap(x);
      for (outer = 0; outer < size; outer++)
        {
          for (inner = 0; inner < size; inner++)
            {
              a = 255;
              r1 = ((cmap_h[x][(int)(inner * 255.0 / size)] >> 16) & 0xff);
              g1 = ((cmap_h[x][(int)(inner * 255.0 / size)] >> 8) & 0xff);
              b1 = (cmap_h[x][(int)(inner * 255.0 / size)] & 0xff);

              r2 = ((cmap_h[y][(int)(outer * 255.0 / size)] >> 16) & 0xff);
              g2 = ((cmap_h[y][(int)(outer * 255.0 / size)] >> 8) & 0xff);
              b2 = (cmap_h[y][(int)(outer * 255.0 / size)] & 0xff);

              colormap[outer * size + inner] =
                  (a << 24) + (((b1 + b2) / 2) << 16) + (((g1 + g2) / 2) << 8) + ((r1 + r2) / 2);
            }
        }
      return colormap;
    }
  return NULL;
}


/*
 * Calculates the classes for polar histogram
 * */
error_t classes_polar_histogram(grm_args_t *subplot_args, double *r_max)
{
  unsigned int num_bins;
  double *theta = NULL;
  unsigned int length;
  char *norm;
  double *classes = NULL;

  double interval;
  double start;
  double *p;
  double max;
  double temp_max;

  double *bin_edges = NULL;
  double *bin_edges_buf = NULL;
  unsigned int num_bin_edges;

  double bin_width;

  double *bin_widths = NULL;

  int is_bin_counts;
  int *bin_counts = NULL;

  double false = -1;

  double *philim = NULL;
  unsigned int dummy;

  double *new_theta = NULL;
  double *new_edges = NULL;

  grm_args_t **series;

  error_t error = NO_ERROR;


  args_values(subplot_args, "series", "A", &series);


  /* get theta or bin_counts */
  if (args_values(*series, "bin_counts", "i", &is_bin_counts) == 0)
    {
      is_bin_counts = 0;
      args_first_value(*series, "x", "D", &theta, &length);
    }
  else
    {
      args_first_value(*series, "x", "I", &bin_counts, &length);
      is_bin_counts = 1;
      num_bins = length;
      grm_args_push(*series, "nbins", "i", num_bins);
    }

  if (args_first_value(*series, "philim", "D", &philim, &dummy) == 0)
    {
      philim = &false;
    }
  else
    {
      if (philim[0] < 0.0 || philim[1] > 2 * M_PI)
        {
          logger((stderr, "philim must be between 0 and 2 * pi\n"));
          cleanup_and_set_error(ERROR_PLOT_OUT_OF_RANGE);
        }
      if (philim[1] < philim[0])
        {
          double temp = philim[1];
          int phiflip;
          philim[1] = philim[0];
          philim[0] = temp;
          if (args_values(subplot_args, "phiflip", "i", &phiflip) == 0)
            {
              phiflip = 1;
              grm_args_push(subplot_args, "phiflip", "i", phiflip);
            }
          else
            {
              grm_args_push(subplot_args, "phiflip", "i", 0);
            }
        }
    }


  /* bin_edges and nbins */
  if (args_first_value(*series, "bin_edges", "D", &bin_edges, &num_bin_edges) == 0)
    {
      if (args_values(*series, "nbins", "i", &num_bins) == 0)
        {
          num_bins = min(12, (int)(length * 1.0 / 2) - 1);
          grm_args_push(*series, "nbins", "i", num_bins);
        }
      else
        {
          if (num_bins <= 0 || num_bins > 200)
            {
              num_bins = min(12, (int)(length * 1.0 / 2) - 1);
              grm_args_push(*series, "nbins", "i", num_bins);
            }
        }
      if (*philim == -1.0)
        num_bin_edges = 0;
      else
        {
          bin_edges = bin_edges_buf = (double *)malloc((num_bins + 1) * sizeof(double));
          cleanup_and_set_error_if(bin_edges == NULL, ERROR_MALLOC);
          linspace(philim[0], philim[1], num_bins + 1, bin_edges);
          num_bin_edges = num_bins + 1;
          grm_args_push(*series, "bin_edges", "nD", num_bin_edges, bin_edges);
        }
    }
  /* with bin_edges */
  else
    {
      /* no philim */
      if (*philim == -1)
        {

          /* filter bin_edges */
          int temp = 0;
          int i;
          new_edges = malloc(num_bin_edges * sizeof(double));
          cleanup_and_set_error_if(new_edges == NULL, ERROR_MALLOC);

          for (i = 0; i < num_bin_edges; ++i)
            {
              if (0.0 <= bin_edges[i] && bin_edges[i] <= 2 * M_PI)
                {
                  new_edges[temp] = bin_edges[i];
                  temp++;
                }
              else
                {
                  logger((stderr, "Only values between 0 and 2 * pi allowed\n"));
                  cleanup_and_set_error(ERROR_PLOT_OUT_OF_RANGE);
                }
            }
          if (num_bin_edges > temp)
            {
              num_bin_edges = temp;
              bin_edges = bin_edges_buf = (double *)realloc(new_edges, temp * sizeof(double));
              cleanup_and_set_error_if(bin_edges == NULL, ERROR_MALLOC);
            }
          else
            {
              bin_edges = new_edges;
            }
          num_bins = num_bin_edges - 1;
          grm_args_push(*series, "nbins", "i", num_bins);
        }
      /* with philim and binedges */
      else
        {
          /* filter bin_edges */
          int temp = 0;
          new_edges = malloc(num_bin_edges * sizeof(double));
          cleanup_and_set_error_if(new_edges == NULL, ERROR_MALLOC);

          int i;
          for (i = 0; i < num_bin_edges; ++i)
            {
              if (philim[0] <= bin_edges[i] && bin_edges[i] <= philim[1])
                {
                  new_edges[temp] = bin_edges[i];
                  temp++;
                }
            }
          if (temp > 1)
            {
              if (num_bin_edges > temp)
                {
                  num_bin_edges = temp;
                  bin_edges = bin_edges_buf = (double *)realloc(new_edges, temp * sizeof(double));
                }
              else
                {
                  bin_edges = new_edges;
                }
            }
          if (num_bin_edges == 1)
            {
              logger(
                  (stderr, "given philim and given bin_edges are not compatible --> filtered len(bin_edges) == 1\n"));
              cleanup_and_set_error(ERROR_PLOT_INCOMPATIBLE_ARGUMENTS);
            }
          else
            {
              num_bins = num_bin_edges - 1;
              grm_args_push(*series, "nbins", "i", num_bins);
              grm_args_push(*series, "bin_edges", "nD", num_bin_edges, bin_edges);
            }
        }
    }


  if (args_values(subplot_args, "normalization", "s", &norm) == 0)
    {
      norm = "count";
    }
  else
    {
      if (!str_equals_any(norm, 6, "count", "countdensity", "pdf", "probability", "cumcount", "cdf"))
        {
          logger((stderr, "Got keyword \"norm\"  with invalid value \"%s\"\n", norm));
          cleanup_and_set_error(ERROR_PLOT_NORMALIZATION);
        }
    }

  if (args_values(*series, "bin_width", "d", &bin_width) == 0)
    {
      if (num_bin_edges > 0)
        {
          int i;
          bin_widths = (double *)malloc((num_bins + 1) * sizeof(double));
          cleanup_and_set_error_if(bin_widths == NULL, ERROR_MALLOC);

          for (i = 1; i <= num_bin_edges - 1; ++i)
            {
              *(bin_widths + i - 1) = *(bin_edges + i) - *(bin_edges + i - 1);
            }
          grm_args_push(*series, "bin_widths", "nD", num_bins, bin_widths);
        }
      else
        {
          bin_width = 2 * M_PI / num_bins;
          grm_args_push(*series, "bin_width", "d", bin_width);
        }
    }
  /* bin_width is given*/
  else
    {
      int n = 0;
      int temp;

      if (num_bin_edges > 0 && *philim == -1.0)
        {
          int i;
          logger((stderr, "bin_width is not compatible with bin_edges\n"));
          cleanup_and_set_error(ERROR_PLOT_INCOMPATIBLE_ARGUMENTS);

          bin_widths = (double *)malloc(num_bins * sizeof(double));
          cleanup_and_set_error_if(bin_widths == NULL, ERROR_MALLOC);

          for (i = 1; i <= num_bin_edges - 1; ++i)
            {
              *(bin_widths + i - 1) = *(bin_edges + i) - *(bin_edges + i - 1);
            }
          grm_args_push(*series, "bin_widths", "nD", num_bins, bin_widths);
        }

      /* with philim */
      if (*philim != -1)
        {
          if (bin_width < 0 || bin_width > 2 * M_PI)
            {
              logger((stderr, "bin_width must be between 0 and 2 * Pi\n"));
              cleanup_and_set_error(ERROR_PLOT_OUT_OF_RANGE);
            }
          if (philim[1] - philim[0] < bin_width)
            {
              logger((stderr, "the given philim range does not work with the given bin_width\n"));
              cleanup_and_set_error(ERROR_PLOT_INCOMPATIBLE_ARGUMENTS);
            }
          else
            {
              n = (int)((philim[1] - philim[0]) / bin_width);
              if (is_bin_counts == 1)
                {
                  if (num_bins > n)
                    {
                      logger((stderr,
                              "bin_width does not work with this specific bin_count. Nbins do not fit bin_width\n"));
                      cleanup_and_set_error(ERROR_PLOT_INCOMPATIBLE_ARGUMENTS);
                    }
                  n = num_bins;
                }
              bin_edges = bin_edges_buf = (double *)malloc((n + 1) * sizeof(double));
              cleanup_and_set_error_if(bin_edges == NULL, ERROR_MALLOC);

              linspace(philim[0], n * bin_width, n + 1, bin_edges);
            }
        }
      /* without philim */
      else
        {
          if (bin_width <= 0 || bin_width > 2 * M_PI)
            {
              logger((stderr, "bin_width must be between 0 (exclusive) and 2 * Pi\n"));
              cleanup_and_set_error(ERROR_PLOT_OUT_OF_RANGE);
            }
          else if ((int)(2 * M_PI / bin_width) > 200)
            {
              n = 200;
              bin_width = 2 * M_PI / n;
            }
          n = (int)(2 * M_PI / bin_width);
          if (is_bin_counts == 1)
            {
              if (num_bins > n)
                {
                  logger(
                      (stderr, "bin_width does not work with this specific bin_count. Nbins do not fit bin_width\n"));
                  cleanup_and_set_error(ERROR_PLOT_INCOMPATIBLE_ARGUMENTS);
                }
              n = num_bins;
            }
          bin_edges = bin_edges_buf = (double *)malloc((n + 1) * sizeof(double));
          cleanup_and_set_error_if(bin_edges == NULL, ERROR_MALLOC);

          linspace(0, n * bin_width, n + 1, bin_edges);
        }
      grm_args_push(*series, "nbins", "i", n);
      num_bin_edges = n + 1;
      num_bins = n;
      grm_args_push(*series, "bin_edges", "nD", num_bin_edges, bin_edges);
      grm_args_push(*series, "bin_width", "d", bin_width);
      bin_widths = malloc(num_bins * sizeof(double));
      cleanup_and_set_error_if(bin_widths == NULL, ERROR_MALLOC);

      for (temp = 0; temp < num_bins; ++temp)
        {
          bin_widths[temp] = bin_width;
        }
      grm_args_push(*series, "bin_widths", "nD", num_bins, bin_widths);
    }


  /* is_bin_counts */
  if (is_bin_counts == 1)
    {
      double temp_max_bc = 0.0;
      int i;
      int total = 0;
      int j;
      int prev = 0;

      if (num_bin_edges > 0 && num_bins != num_bin_edges - 1)
        {
          logger((stderr, "Number of bin_edges must be number of bin_counts + 1\n"));
          cleanup_and_set_error(ERROR_PLOT_INCOMPATIBLE_ARGUMENTS);
        }

      /* total number of observations */
      for (i = 0; i < num_bins; ++i)
        {
          total += bin_counts[i];
        }
      for (i = 0; i < num_bins; ++i)
        {
          if (num_bin_edges > 0) bin_width = bin_widths[i];

          if (strcmp(norm, "pdf") == 0)
            {
              if (bin_counts[i] * 1.0 / (total * bin_width) > temp_max_bc)
                {
                  temp_max_bc = bin_counts[i] * 1.0 / (total * bin_width);
                }
            }
          else if (strcmp(norm, "countdensity") == 0)
            {
              if (bin_counts[i] * 1.0 / (bin_width) > temp_max_bc)
                {
                  temp_max_bc = bin_counts[i] * 1.0 / (bin_width);
                }
            }
          else
            {
              if (bin_counts[i] > temp_max_bc) temp_max_bc = bin_counts[i];
            }
        }

      /* double classes[num_bins][total]; */
      classes = (double *)malloc((num_bins * total) * sizeof(double));
      cleanup_and_set_error_if(classes == NULL, ERROR_MALLOC);

      length = (int)temp_max_bc;
      p = classes;

      for (i = 0; i < (num_bins * total); ++i)
        {
          *p = -1.0;
          p++;
        }

      /* fill classes with bin counts */
      for (i = 0; i < num_bins; ++i)
        {
          p = classes + i * total;
          if (str_equals_any(norm, 2, "cdf", "cumcount"))
            {
              prev += bin_counts[i];
              for (j = 0; j < prev; ++j)
                {
                  *p = 1.0;
                  ++p;
                }
            }

          else
            {
              for (j = 0; j < bin_counts[i]; ++j)
                {
                  *p = 1.0;
                  ++p;
                }
            }
        }

      grm_args_push(*series, "classes", "nD", total * num_bins, classes);
      if (strcmp(norm, "probability") == 0)
        max = temp_max_bc * 1.0 / total;
      else if (strcmp(norm, "cdf") == 0)
        max = 1.0;
      else if (strcmp(norm, "cumcount") == 0)
        max = total * 1.0;
      else
        max = temp_max_bc;
    }

  /* no is_bin_counts */
  else
    {
      int x;

      /* no bin_edges */
      if (num_bin_edges == 0)
        {
          max = 0.0;
          start = 0;
          interval = 2 * M_PI / num_bins;
          if (str_equals_any(norm, 4, "count", "pdf", "countdensity", "probability"))
            {
              /*double classes[num_bins][length];*/
              classes = (double *)malloc(num_bins * length * sizeof(double));
              cleanup_and_set_error_if(classes == NULL, ERROR_MALLOC);
              p = classes;
              int i;
              for (i = 0; i < (num_bins * length); ++i)
                {
                  *p = -1.0;
                  p++;
                }
              for (x = 0; x < num_bins; ++x)
                {
                  int y;
                  p = classes + x * length;
                  temp_max = 0.0;
                  for (y = 0; y < length; ++y)
                    {
                      if (start <= theta[y] && theta[y] < (start + interval))
                        {
                          temp_max++;
                          *p = theta[y];
                          p++;
                          if (max < temp_max) max = temp_max;
                        }
                    }
                  start += interval;
                }
              grm_args_push(*series, "classes", "nD", num_bins * length, classes);
            }
          else if (str_equals_any(norm, 2, "cdf", "cumcount"))
            {
              /*double classes[num_bins][length];*/
              classes = (double *)malloc(num_bins * length * sizeof(double));
              cleanup_and_set_error_if(classes == NULL, ERROR_MALLOC);
              int i;
              int prev;
              p = classes;

              for (i = 0; i < (num_bins * length); ++i)
                {
                  *p = -1;
                  p++;
                }

              prev = 0;
              for (x = 0; x < num_bins; ++x)
                {
                  p = classes + x * length;

                  if (x > 0)
                    {
                      double *prev_list;
                      int y;
                      /*prev_list = classes[x - 1];*/
                      prev_list = classes + (x - 1) * length;
                      for (y = 0; y < prev; ++y)
                        {
                          *p = *prev_list;
                          p++;
                          prev_list++;
                        }
                    }
                  for (i = 0; i < length; ++i)
                    {
                      if (start <= theta[i] && theta[i] < (start + interval))
                        {
                          *p = theta[i];
                          p++;
                          prev++;
                        }
                    }
                  start += interval;
                }
              max = length;
              grm_args_push(*series, "classes", "nD", num_bins * length, classes);
            }

          if (str_equals_any(norm, 2, "probability", "cdf"))
            {
              max = max / length;
            }
          else if (strcmp(norm, "pdf") == 0)
            {
              max = max / (length * bin_width);
            }
        }
      /* bin_edges */
      else
        {
          /* filter theta list */
          int filter;
          int temp = 0;
          double bin_min = *bin_edges;
          double bin_max = bin_edges[num_bin_edges - 1];
          new_theta = (double *)malloc(length * sizeof(double));
          cleanup_and_set_error_if(new_theta == NULL, ERROR_MALLOC);
          for (filter = 0; filter < length; ++filter)
            {
              if (theta[filter] >= bin_min && theta[filter] < bin_max)
                {
                  new_theta[temp] = theta[filter];
                  temp++;
                }
            }
          theta = new_theta;
          length = temp;

          max = 0.0;
          if (str_equals_any(norm, 4, "count", "pdf", "countdensity", "probability"))
            {
              int a;
              /* double classes[num_bins][length]; */
              classes = (double *)malloc(num_bins * length * sizeof(double));
              cleanup_and_set_error_if(classes == NULL, ERROR_MALLOC);
              p = classes;
              for (a = 0; a < (num_bins * length); ++a)
                {
                  *p = -1;
                  p++;
                }
              for (x = 0; x < num_bins; ++x)
                {
                  int b;
                  p = classes + x * length;
                  temp_max = 0.0;
                  for (b = 0; b < length; ++b)
                    {
                      if (x == num_bin_edges - 1) break;
                      if (*bin_edges <= theta[b] && theta[b] < *(bin_edges + 1))
                        {
                          temp_max++;
                          *p = theta[b];
                          p++;
                        }
                    }
                  if (strcmp(norm, "pdf") == 0)
                    {
                      temp_max /= length * *(bin_widths + x);
                    }
                  else if (strcmp(norm, "countdensity") == 0)
                    {
                      temp_max /= *(bin_widths + x);
                    }

                  if (max < temp_max) max = temp_max;

                  bin_edges++;
                }

              grm_args_push(*series, "classes", "nD", num_bins * length, classes);
            }
          else if (str_equals_any(norm, 2, "cdf", "cumcount"))
            {
              int c;
              int prev;
              int d;
              classes = (double *)malloc(num_bins * length * sizeof(double));
              cleanup_and_set_error_if(classes == NULL, ERROR_MALLOC);
              p = classes;
              for (c = 0; c < (num_bins * length); ++c)
                {
                  *p = -1;
                  p++;
                }

              prev = 0;
              for (d = 0; d < num_bins; ++d)
                {
                  int i;
                  p = classes + d * num_bins;

                  if (d > 0)
                    {
                      double *prev_list;
                      int y;
                      prev_list = classes + (d - 1) * num_bins;

                      for (y = 0; y < prev; ++y)
                        {
                          *p = *prev_list;
                          p++;
                          prev_list++;
                        }
                    }
                  for (i = 0; i < length; ++i)
                    {
                      if (*bin_edges <= theta[i] && theta[i] < *(bin_edges + 1))
                        {
                          *p = theta[i];
                          p++;
                          prev++;
                        }
                    }
                  bin_edges++;
                }
              max = length;
              grm_args_push(*series, "classes", "nD", num_bins * length, classes);
            }

          if (str_equals_any(norm, 2, "probability", "cdf"))
            {
              max = max / length;
            }
        }
    }

  if (r_max != NULL)
    {
      *r_max = max;
    }

cleanup:
  free(bin_edges_buf);
  free(bin_widths);
  free(classes);
  free(new_theta);
  free(new_edges);

  return error;
}


/* ========================= methods ================================================================================ */

/* ------------------------- args set ------------------------------------------------------------------------------- */

DEFINE_SET_METHODS(args)

int args_set_entry_copy(args_set_entry_t *copy, args_set_const_entry_t entry)
{
  /* discard const because it is necessary to work on the object itself */
  /* TODO create two set types: copy and pointer version */
  *copy = (args_set_entry_t)entry;
  return 1;
}

void args_set_entry_delete(args_set_entry_t entry UNUSED) {}

size_t args_set_entry_hash(args_set_const_entry_t entry)
{
  return (size_t)entry;
}

int args_set_entry_equals(args_set_const_entry_t entry1, args_set_const_entry_t entry2)
{
  return entry1 == entry2;
}


/* ------------------------- string-to-plot_func map ---------------------------------------------------------------- */

DEFINE_MAP_METHODS(plot_func)

int plot_func_map_value_copy(plot_func_t *copy, const plot_func_t value)
{
  *copy = value;

  return 1;
}

void plot_func_map_value_delete(plot_func_t value UNUSED) {}


/* ------------------------- string-to-args_set map ----------------------------------------------------------------- */

DEFINE_MAP_METHODS(args_set)

int args_set_map_value_copy(args_set_t **copy, const args_set_t *value)
{
  /* discard const because it is necessary to work on the object itself */
  /* TODO create two map types: copy and pointer version */
  *copy = (args_set_t *)value;

  return 1;
}

void args_set_map_value_delete(args_set_t *value UNUSED) {}


#undef DEFINE_SET_METHODS
#undef DEFINE_MAP_METHODS


/* ######################### public implementation ################################################################## */

/* ========================= functions ============================================================================== */

/* ------------------------- plot ----------------------------------------------------------------------------------- */

void grm_finalize(void)
{
  if (plot_static_variables_initialized)
    {
      grm_args_delete(global_root_args);
      global_root_args = NULL;
      active_plot_args = NULL;
      active_plot_index = 0;
      event_queue_delete(event_queue);
      event_queue = NULL;
      double_map_delete(meters_per_unit_map);
      meters_per_unit_map = NULL;
      string_map_delete(fmt_map);
      fmt_map = NULL;
      plot_func_map_delete(plot_func_map);
      plot_func_map = NULL;
      string_map_delete(plot_valid_keys_map);
      plot_valid_keys_map = NULL;
      string_array_map_delete(type_map);
      type_map = NULL;
      plot_static_variables_initialized = 0;
    }
}

int grm_clear(void)
{
  if (plot_init_static_variables() != NO_ERROR)
    {
      return 0;
    }
  grm_args_clear(active_plot_args);
  if (plot_init_args_structure(active_plot_args, plot_hierarchy_names + 1, 1) != NO_ERROR)
    {
      return 0;
    }

  return 1;
}

unsigned int grm_max_plotid(void)
{
  unsigned int args_array_length = 0;

  if (args_first_value(global_root_args, "plots", "A", NULL, &args_array_length))
    {
      --args_array_length;
    }

  return args_array_length;
}

int grm_merge(const grm_args_t *args)
{
  return grm_merge_extended(args, 0, NULL);
}

int grm_merge_extended(const grm_args_t *args, int hold, const char *identificator)
{
  if (plot_init_static_variables() != NO_ERROR)
    {
      return 0;
    }
  if (args != NULL)
    {
      if (plot_merge_args(global_root_args, args, NULL, NULL, hold) != NO_ERROR)
        {
          return 0;
        }
    }

  process_events();
  event_queue_enqueue_merge_end_event(event_queue, identificator);
  process_events();

  return 1;
}

int grm_merge_hold(const grm_args_t *args)
{
  return grm_merge_extended(args, 1, NULL);
}

int grm_merge_named(const grm_args_t *args, const char *identificator)
{
  return grm_merge_extended(args, 0, identificator);
}

int grm_plot(const grm_args_t *args)
{
  grm_args_t **current_subplot_args;
  plot_func_t plot_func;
  const char *kind = NULL;
  if (!grm_merge(args))
    {
      return 0;
    }

  if (args_values(active_plot_args, "raw", "s", &current_subplot_args))
    {
      plot_raw(active_plot_args);
    }
  else
    {
      plot_set_attribute_defaults(active_plot_args);
      plot_pre_plot(active_plot_args);
      args_values(active_plot_args, "subplots", "A", &current_subplot_args);
      while (*current_subplot_args != NULL)
        {
          if (plot_pre_subplot(*current_subplot_args) != NO_ERROR)
            {
              return 0;
            }
          args_values(*current_subplot_args, "kind", "s", &kind);
          logger((stderr, "Got keyword \"kind\" with value \"%s\"\n", kind));
          if (!plot_func_map_at(plot_func_map, kind, &plot_func))
            {
              return 0;
            }
          if (plot_func(*current_subplot_args) != NO_ERROR)
            {
              return 0;
            };
          plot_post_subplot(*current_subplot_args);
          ++current_subplot_args;
        }
      plot_post_plot(active_plot_args);
    }

  process_events();

#ifndef NDEBUG
  logger((stderr, "root args after \"grm_plot\" (active_plot_index: %d):\n", active_plot_index - 1));
  if (logger_enabled())
    {
      grm_dump(global_root_args, stderr);
    }
#endif

  return 1;
}

int grm_switch(unsigned int id)
{
  grm_args_t **args_array = NULL;
  unsigned int args_array_length = 0;

  if (plot_init_static_variables() != NO_ERROR)
    {
      return 0;
    }

  if (plot_init_args_structure(global_root_args, plot_hierarchy_names, id + 1) != NO_ERROR)
    {
      return 0;
    }
  if (!args_first_value(global_root_args, "plots", "A", &args_array, &args_array_length))
    {
      return 0;
    }
  if (id + 1 > args_array_length)
    {
      return 0;
    }

  active_plot_index = id + 1;
  active_plot_args = args_array[id];

  return 1;
}
