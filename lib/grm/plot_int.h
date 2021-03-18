#ifndef GRM_PLOT_INT_H_INCLUDED
#define GRM_PLOT_INT_H_INCLUDED

/* ######################### includes ############################################################################### */

#include "args_int.h"
#include "datatype/uint_map_int.h"
#include "error_int.h"
#include "event_int.h"
#include "plot.h"


/* ######################### internal interface ##################################################################### */

/* ========================= global varibales ======================================================================= */

/* ------------------------- plot ----------------------------------------------------------------------------------- */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~ args ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

extern grm_args_t *active_plot_args;


/* ~~~~~~~~~~~~~~~~~~~~~~~~~ event handling ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

extern event_queue_t *event_queue;


/* ~~~~~~~~~~~~~~~~~~~~~~~~~ plot clear ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

extern const char *plot_clear_exclude_keys[];


/* ========================= macros ================================================================================= */

/* ------------------------- plot ----------------------------------------------------------------------------------- */

#define ROOT_DEFAULT_APPEND_PLOTS 0
#define PLOT_DEFAULT_WIDTH 600.0
#define PLOT_DEFAULT_HEIGHT 450.0
#define PLOT_DEFAULT_KIND "line"
#define PLOT_DEFAULT_SPEC ""
#define PLOT_DEFAULT_CLEAR 1
#define PLOT_DEFAULT_UPDATE 1
#define PLOT_DEFAULT_LOCATION 1
#define PLOT_DEFAULT_SUBPLOT_MIN_X 0.0
#define PLOT_DEFAULT_SUBPLOT_MAX_X 1.0
#define PLOT_DEFAULT_SUBPLOT_MIN_Y 0.0
#define PLOT_DEFAULT_SUBPLOT_MAX_Y 1.0
#define PLOT_DEFAULT_XLOG 0
#define PLOT_DEFAULT_YLOG 0
#define PLOT_DEFAULT_ZLOG 0
#define PLOT_DEFAULT_XFLIP 0
#define PLOT_DEFAULT_YFLIP 0
#define PLOT_DEFAULT_ZFLIP 0
#define PLOT_DEFAULT_XGRID 1
#define PLOT_DEFAULT_YGRID 1
#define PLOT_DEFAULT_ZGRID 1
#define PLOT_DEFAULT_ADJUST_XLIM 1
#define PLOT_DEFAULT_ADJUST_YLIM 1
#define PLOT_DEFAULT_ADJUST_ZLIM 1
#define PLOT_DEFAULT_COLORMAP 44                                 /* VIRIDIS */
#define PLOT_DEFAULT_FONT 232                                    /* CMUSerif-Math */
#define PLOT_DEFAULT_FONT_PRECISION GKS_K_TEXT_PRECISION_OUTLINE /* hardware font rendering */
#define PLOT_DEFAULT_ROTATION 40
#define PLOT_DEFAULT_TILT 70
#define PLOT_DEFAULT_KEEP_ASPECT_RATIO 0
#define PLOT_DEFAULT_XLABEL ""
#define PLOT_DEFAULT_YLABEL ""
#define PLOT_DEFAULT_ZLABEL ""
#define PLOT_DEFAULT_STEP_WHERE "mid"
#define PLOT_DEFAULT_CONTOUR_LEVELS 20
#define PLOT_DEFAULT_HEXBIN_NBINS 40
#define PLOT_DEFAULT_TRICONT_LEVELS 20
#define SERIES_DEFAULT_SPEC ""
#define PLOT_POLAR_AXES_TEXT_BUFFER 40
#define PLOT_CONTOUR_GRIDIT_N 200
#define PLOT_WIREFRAME_GRIDIT_N 50
#define PLOT_SURFACE_GRIDIT_N 200


/* ========================= datatypes ============================================================================== */

/* ------------------------- plot ----------------------------------------------------------------------------------- */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~ kind to func ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

typedef error_t (*plot_func_t)(grm_args_t *args);


/* ~~~~~~~~~~~~~~~~~~~~~~~~~ options ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

typedef enum
{
  GR_OPTION_X_LOG = 1 << 0,
  GR_OPTION_Y_LOG = 1 << 1,
  GR_OPTION_Z_LOG = 1 << 2,
  GR_OPTION_FLIP_X = 1 << 3,
  GR_OPTION_FLIP_Y = 1 << 4,
  GR_OPTION_FLIP_Z = 1 << 5,
  GR_OPTION_LINES = 0,
  GR_OPTION_MESH = 1,
  GR_OPTION_FILLED_MESH = 2,
  GR_OPTION_Z_SHADED_MESH = 3,
  GR_OPTION_COLORED_MESH = 4,
  GR_OPTION_CELL_ARRAY = 5,
  GR_OPTION_SHADED_MESH = 6
} gr_option_t;


/* ========================= functions ============================================================================== */

/* ------------------------- plot ----------------------------------------------------------------------------------- */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~ general ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

error_t plot_init_static_variables(void);


/* ~~~~~~~~~~~~~~~~~~~~~~~~~ plot arguments ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

error_t plot_merge_args(grm_args_t *args, const grm_args_t *merge_args, const char **hierarchy_name_ptr,
                        uint_map_t *hierarchy_to_id, int hold_always);
error_t plot_init_arg_structure(arg_t *arg, const char **hierarchy_name_ptr, unsigned int next_hierarchy_level_max_id);
error_t plot_init_args_structure(grm_args_t *args, const char **hierarchy_name_ptr,
                                 unsigned int next_hierarchy_level_max_id);
void plot_set_flag_defaults(void);
void plot_set_attribute_defaults(grm_args_t *subplot_args);
void plot_pre_plot(grm_args_t *plot_args);
void plot_set_text_encoding(void);
void plot_process_wswindow_wsviewport(grm_args_t *plot_args);
error_t plot_pre_subplot(grm_args_t *subplot_args);
void plot_process_colormap(grm_args_t *subplot_args);
void plot_process_font(grm_args_t *subplot_args);
void plot_process_viewport(grm_args_t *subplot_args);
void plot_process_window(grm_args_t *subplot_args);
error_t plot_store_coordinate_ranges(grm_args_t *subplot_args);
void plot_post_plot(grm_args_t *plot_args);
void plot_restore_text_encoding(void);
void plot_post_subplot(grm_args_t *subplot_args);
error_t plot_get_args_in_hierarchy(grm_args_t *args, const char **hierarchy_name_start_ptr, const char *key,
                                   uint_map_t *hierarchy_to_id, const grm_args_t **found_args,
                                   const char ***found_hierarchy_ptr);


/* ~~~~~~~~~~~~~~~~~~~~~~~~~ plotting ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

error_t plot_line(grm_args_t *subplot_args);
error_t plot_step(grm_args_t *subplot_args);
error_t plot_scatter(grm_args_t *subplot_args);
error_t plot_quiver(grm_args_t *subplot_args);
error_t plot_stem(grm_args_t *subplot_args);
error_t plot_hist(grm_args_t *subplot_args);
error_t plot_barplot(grm_args_t *subplot_args);
error_t plot_contour(grm_args_t *subplot_args);
error_t plot_contourf(grm_args_t *subplot_args);
error_t plot_hexbin(grm_args_t *subplot_args);
error_t plot_heatmap(grm_args_t *subplot_args);
error_t plot_wireframe(grm_args_t *subplot_args);
error_t plot_surface(grm_args_t *subplot_args);
error_t plot_plot3(grm_args_t *subplot_args);
error_t plot_scatter3(grm_args_t *subplot_args);
error_t plot_imshow(grm_args_t *subplot_args);
error_t plot_isosurface(grm_args_t *subplot_args);
error_t plot_polar(grm_args_t *subplot_args);
error_t plot_polar_histogram(grm_args_t *subplot_args);
error_t plot_pie(grm_args_t *subplot_args);
error_t plot_trisurf(grm_args_t *subplot_args);
error_t plot_tricont(grm_args_t *subplot_args);
error_t plot_shade(grm_args_t *subplot_args);
error_t plot_raw(grm_args_t *subplot_args);


/* ~~~~~~~~~~~~~~~~~~~~~~~~~ auxiliary drawing functions ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

error_t plot_draw_axes(grm_args_t *args, unsigned int pass);
error_t plot_draw_polar_axes(grm_args_t *args);
error_t plot_draw_legend(grm_args_t *args);
error_t plot_draw_pie_legend(grm_args_t *args);
error_t plot_draw_colorbar(grm_args_t *args, double off, unsigned int colors);
error_t plot_draw_errorbars(grm_args_t *series_args, double *x, unsigned int x_length, double *y, char *kind);


/* ~~~~~~~~~~~~~~~~~~~~~~~~~ util ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

double find_max_step(unsigned int n, const double *x);
double *normalize(unsigned int n, const double *x);
unsigned int *normalize_int(unsigned int n, const double *x, unsigned int sum);
const char *next_fmt_key(const char *fmt) UNUSED;
const char *get_compatible_format(const char *key, const char *given_format);
int get_id_from_args(const grm_args_t *args, int *plot_id, int *subplot_id, int *series_id);
int get_figure_size(const grm_args_t *plot_args, int *pixel_width, int *pixel_height, double *metric_width,
                    double *metric_height);
int get_focus_and_factor(const int top, const int right, const int bottom, const int left, const int keep_aspect_ratio,
                         double *factor_x, double *factor_y, double *focus_x, double *focus_y,
                         grm_args_t **subplot_args);
grm_args_t *get_subplot_from_ndc_point(double x, double y);
grm_args_t *get_subplot_from_ndc_points(unsigned int n, const double *x, const double *y);
double *moivre(double r, int x, int n);
double *listcomprehension(double count, double (*pFunction)(double), double *pDouble, int num, int start,
                          double *result);
int *create_colormap(int x, int y, int size);
error_t classes_polar_histogram(grm_args_t *subplot_args, double *r_max);
double get_lightness_from_rbg(double r, double g, double b);
void set_text_color_for_background(double r, double g, double b);


#endif /* ifndef GRM_PLOT_INT_H_INCLUDED */
