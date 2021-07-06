#ifndef _GKSCORE_H_
#define _GKSCORE_H_

#include <stddef.h>

#ifdef _WIN32

#include <windows.h> /* required for all Windows applications */
#define DLLEXPORT __declspec(dllexport)

#ifdef TEXT
#undef TEXT
#endif

#else

#ifndef DLLEXPORT
#define DLLEXPORT
#endif

#endif

#ifdef __cplusplus
extern "C" {
#endif

#define FEPS 1.0E-09

#define GRALGKS 3
#define GLIGKS 4
#define GKS5 5

#define MAX_WS 16      /* maximum number of workstations */
#define MAX_TNR 9      /* maximum number of normalization transformations */
#define MAX_COLOR 1256 /* maximum number of predefined colors */

#define FIX_COLORIND(c) (c) < 0 ? 0 : (c) < MAX_COLOR ? (c) : MAX_COLOR - 1

#define OPEN_GKS 0
#define CLOSE_GKS 1
#define OPEN_WS 2
#define CLOSE_WS 3
#define ACTIVATE_WS 4
#define DEACTIVATE_WS 5
#define CLEAR_WS 6
#define REDRAW_SEG_ON_WS 7
#define UPDATE_WS 8
#define SET_DEFERRAL_STATE 9
#define MESSAGE 10
#define ESCAPE 11
#define POLYLINE 12
#define POLYMARKER 13
#define TEXT 14
#define FILLAREA 15
#define CELLARRAY 16
#define GDP 17
#define SET_PLINE_INDEX 18
#define SET_PLINE_LINETYPE 19
#define SET_PLINE_LINEWIDTH 20
#define SET_PLINE_COLOR_INDEX 21
#define SET_PMARK_INDEX 22
#define SET_PMARK_TYPE 23
#define SET_PMARK_SIZE 24
#define SET_PMARK_COLOR_INDEX 25
#define SET_TEXT_INDEX 26
#define SET_TEXT_FONTPREC 27
#define SET_TEXT_EXPFAC 28
#define SET_TEXT_SPACING 29
#define SET_TEXT_COLOR_INDEX 30
#define SET_TEXT_HEIGHT 31
#define SET_TEXT_UPVEC 32
#define SET_TEXT_PATH 33
#define SET_TEXT_ALIGN 34
#define SET_FILL_INDEX 35
#define SET_FILL_INT_STYLE 36
#define SET_FILL_STYLE_INDEX 37
#define SET_FILL_COLOR_INDEX 38
#define SET_ASF 41
#define SET_COLOR_REP 48
#define SET_WINDOW 49
#define SET_VIEWPORT 50
#define SELECT_XFORM 52
#define SET_CLIPPING 53
#define SET_WS_WINDOW 54
#define SET_WS_VIEWPORT 55
#define CREATE_SEG 56
#define CLOSE_SEG 57
#define DELETE_SEG 58
#define ASSOC_SEG_WITH_WS 61
#define COPY_SEG_TO_WS 62
#define SET_SEG_XFORM 64
#define INITIALIZE_LOCATOR 69
#define REQUEST_LOCATOR 81
#define REQUEST_STROKE 82
#define REQUEST_CHOICE 84
#define REQUEST_STRING 86
#define GET_ITEM 102
#define READ_ITEM 103
#define INTERPRET_ITEM 104
#define EVAL_XFORM_MATRIX 105
#define SET_ENCODING 106
#define INQ_ENCODING 107
#define SET_RESAMPLE_METHOD 108

#define SET_TEXT_SLANT 200
#define DRAW_IMAGE 201
#define SET_SHADOW 202
#define SET_TRANSPARENCY 203
#define SET_COORD_XFORM 204
#define CONFIGURE_WS 205
#define SET_BORDER_WIDTH 206
#define SET_BORDER_COLOR_INDEX 207
#define SELECT_CLIP_XFORM 208
#define INQ_WS_STATE 209

#define BEGIN_SELECTION 250
#define END_SELECTION 251
#define MOVE_SELECTION 252
#define RESIZE_SELECTION 253
#define INQ_BBOX 254

#define ENCODING_LATIN1 300
#define ENCODING_UTF8 301

#define MAX_ATTRIBUTE_FCTID SET_WS_VIEWPORT /* maximum function ID for setting an attribute */

typedef struct
{
  int lindex;
  int ltype;
  double lwidth;
  int plcoli;
  int mindex;
  int mtype;
  double mszsc;
  int pmcoli;
  int tindex;
  int txfont, txprec;
  double chxp;
  double chsp;
  int txcoli;
  double chh;
  double chup[2];
  int txp;
  int txal[2];
  int findex;
  int ints;
  int styli;
  int facoli;
  double window[MAX_TNR][4], viewport[MAX_TNR][4];
  int cntnr, clip, opsg;
  double mat[3][2];
  int asf[13];
  int wiss, version;
  int fontfile;
  int input_encoding;
  double txslant;
  double shoff[2];
  double blur;
  double alpha;
  double a[MAX_TNR], b[MAX_TNR], c[MAX_TNR], d[MAX_TNR];
  unsigned int resample_method;
  double bwidth;
  int bcoli;
  int clip_tnr;
  double aspect_ratio;
  char *(*callback)(const char *);
} gks_state_list_t;

typedef struct gks_list
{
  int item;
  struct gks_list *next;
  void *ptr;
} gks_list_t;

typedef struct
{
  int wkid;
  char *path;
  int wtype;
  int conid;
  void *ptr;
  double vp[4];
} ws_list_t;

typedef struct
{
  int wtype;
  int dcunit;
  double sizex, sizey;
  int unitsx, unitsy;
  int wscat;
  char *type;
  char *env;
} ws_descr_t;

typedef struct
{
  int state;
  char *buffer;
  int size, nbytes, position;
  int empty;
} gks_display_list_t;

typedef struct
{
  int left, right;
  int size;
  int bottom, base, cap, top;
  int length;
  int coord[124][2];
} stroke_data_t;

typedef struct
{
  int width;
  int height;
  double device_pixel_ratio;
} gks_ws_state_t;

int gks_open_font(void);
void gks_lookup_font(int fd, int version, int font, int chr, stroke_data_t *buffer);
void gks_close_font(int fd);

void gks_lookup_afm(int font, int chr, stroke_data_t *buffer);

DLLEXPORT char *gks_malloc(int size);
DLLEXPORT char *gks_realloc(void *ptr, int size);
DLLEXPORT void gks_free(void *ptr);

DLLEXPORT char *gks_strdup(const char *str);

void gks_perror(const char *, ...);
void gks_fatal_error(const char *, ...);
const char *gks_function_name(int routine);
void gks_report_error(int routine, int errnum);

DLLEXPORT void gks_resample(const unsigned char *source_image, unsigned char *target_image, size_t source_width,
                            size_t source_height, size_t target_width, size_t target_height, size_t stride, int swapx,
                            int swapy, unsigned int resample_method);
void gks_init_core(gks_state_list_t *list);
gks_list_t *gks_list_find(gks_list_t *list, int element);
gks_list_t *gks_list_add(gks_list_t *list, int element, void *ptr);
gks_list_t *gks_list_del(gks_list_t *list, int element);
void gks_list_free(gks_list_t *list);
void gks_inq_pattern_array(int index, int *pa);
void gks_set_pattern_array(int index, int *pa);
void gks_inq_rgb(int index, double *red, double *green, double *blue);
void gks_set_rgb(int index, double red, double green, double blue);
void gks_inq_pixel(int index, int *pixel);
void gks_set_pixel(int index, int pixel);
void gks_fit_ws_viewport(double *viewport, double xmax, double ymax, double margin);
void gks_set_norm_xform(int tnr, double *window, double *viewport);
void gks_set_xform_matrix(double tran[3][2]);
void gks_seg_xform(double *x, double *y);
void gks_WC_to_NDC(int tnr, double *x, double *y);
void gks_NDC_to_WC(int tnr, double *x, double *y);
void gks_set_dev_xform(gks_state_list_t *s, double *window, double *viewport);
void gks_inq_dev_xform(double *window, double *viewport);
void gks_set_chr_xform(void);
void gks_chr_height(double *height);
void gks_get_dash(int ltype, double scale, char *dash);
void gks_get_dash_list(int ltype, double scale, int list[10]);
void gks_move(double x, double y, void (*move)(double x, double y));
void gks_dash(double x, double y, void (*move)(double x, double y), void (*draw)(double x, double y));
void gks_emul_polyline(int n, double *px, double *py, int ltype, int tnr, void (*move)(double x, double y),
                       void (*draw)(double x, double y));
void gks_emul_polymarker(int n, double *px, double *py, void (*marker)(double x, double y, int mtype));
void gks_emul_text(double px, double py, int nchars, char *chars,
                   void (*polyline)(int n, double *px, double *py, int ltype, int tnr),
                   void (*fillarea)(int n, double *px, double *py, int tnr));
void gks_emul_fillarea(int n, double *px, double *py, int tnr,
                       void (*polyline)(int n, double *px, double *py, int ltype, int tnr), double yres);
void gks_util_inq_text_extent(double px, double py, char *chars, int nchars, double *cpx, double *cpy, double tx[4],
                              double ty[4]);
int gks_get_ws_type(void);
int gks_base64(unsigned char *src, size_t srclength, char *target, size_t targsize);
DLLEXPORT const char *gks_getenv(const char *env);
void gks_iso2utf(unsigned char c, char *utf, size_t *len);
void gks_symbol2utf(unsigned char c, char *utf, size_t *len);
void gks_input2utf8(const char *input_str, char *utf8_str, int input_encoding);
void gks_utf82latin1(const char *utf8_str, char *latin1_str);
int *gks_resize(int *image, int width, int height, int w, int h);
void gks_filepath(char *path, char *defpath, const char *type, int page, int index);
void gks_adjust_cellarray(double *qx, double *qy, double *rx, double *ry, int *scol, int *srow, int *ncol, int *nrow,
                          int dimx, int dimy);

DLLEXPORT void gks_dl_write_item(gks_display_list_t *d, int fctid, int dx, int dy, int dimx, int *ia, int lr1,
                                 double *r1, int lr2, double *r2, int lc, char *c, gks_state_list_t *gkss);

void gks_wiss_dispatch(int fctid, int wkid, int segn);

#ifndef EMSCRIPTEN

void gks_drv_mo(int fctid, int dx, int dy, int dimx, int *i_arr, int len_f_arr_1, double *f_arr_1, int len_f_arr_2,
                double *f_arr_2, int len_c_arr, char *c_arr, void **ptr);

void gks_drv_mi(int fctid, int dx, int dy, int dimx, int *i_arr, int len_f_arr_1, double *f_arr_1, int len_f_arr_2,
                double *f_arr_2, int len_c_arr, char *c_arr, void **ptr);

void gks_drv_wiss(int fctid, int dx, int dy, int dimx, int *i_arr, int len_f_arr_1, double *f_arr_1, int len_f_arr_2,
                  double *f_arr_2, int len_c_arr, char *c_arr, void **ptr);

void gks_drv_win(int fctid, int dx, int dy, int dimx, int *i_arr, int len_f_arr_1, double *f_arr_1, int len_f_arr_2,
                 double *f_arr_2, int len_c_arr, char *c_arr, void **ptr);

void gks_drv_ps(int fctid, int dx, int dy, int dimx, int *i_arr, int len_f_arr_1, double *f_arr_1, int len_f_arr_2,
                double *f_arr_2, int len_c_arr, char *c_arr, void **ptr);

void gks_drv_pdf(int fctid, int dx, int dy, int dimx, int *i_arr, int len_f_arr_1, double *f_arr_1, int len_f_arr_2,
                 double *f_arr_2, int len_c_arr, char *c_arr, void **ptr);

DLLEXPORT void gks_x11_plugin(int fctid, int dx, int dy, int dimx, int *i_arr, int len_f_arr_1, double *f_arr_1,
                              int len_f_arr_2, double *f_arr_2, int len_c_arr, char *c_arr, void **ptr);

void gks_drv_socket(int fctid, int dx, int dy, int dimx, int *i_arr, int len_f_arr_1, double *f_arr_1, int len_f_arr_2,
                    double *f_arr_2, int len_c_arr, char *c_arr, void **ptr);

void gks_drv_plugin(int fctid, int dx, int dy, int dimx, int *i_arr, int len_f_arr_1, double *f_arr_1, int len_f_arr_2,
                    double *f_arr_2, int len_c_arr, char *c_arr, void **ptr);

DLLEXPORT void gks_gs_plugin(int fctid, int dx, int dy, int dimx, int *i_arr, int len_f_arr_1, double *f_arr_1,
                             int len_f_arr_2, double *f_arr_2, int len_c_arr, char *c_arr, void **ptr);

DLLEXPORT void gks_gtk_plugin(int fctid, int dx, int dy, int dimx, int *i_arr, int len_f_arr_1, double *f_arr_1,
                              int len_f_arr_2, double *f_arr_2, int len_c_arr, char *c_arr, void **ptr);

DLLEXPORT void gks_wx_plugin(int fctid, int dx, int dy, int dimx, int *i_arr, int len_f_arr_1, double *f_arr_1,
                             int len_f_arr_2, double *f_arr_2, int len_c_arr, char *c_arr, void **ptr);

DLLEXPORT void gks_qt_plugin(int fctid, int dx, int dy, int dimx, int *i_arr, int len_f_arr_1, double *f_arr_1,
                             int len_f_arr_2, double *f_arr_2, int len_c_arr, char *c_arr, void **ptr);

DLLEXPORT void gks_svg_plugin(int fctid, int dx, int dy, int dimx, int *i_arr, int len_f_arr_1, double *f_arr_1,
                              int len_f_arr_2, double *f_arr_2, int len_c_arr, char *c_arr, void **ptr);

DLLEXPORT void gks_wmf_plugin(int fctid, int dx, int dy, int dimx, int *i_arr, int len_f_arr_1, double *f_arr_1,
                              int len_f_arr_2, double *f_arr_2, int len_c_arr, char *c_arr, void **ptr);

DLLEXPORT void gks_quartz_plugin(int fctid, int dx, int dy, int dimx, int *i_arr, int len_f_arr_1, double *f_arr_1,
                                 int len_f_arr_2, double *f_arr_2, int len_c_arr, char *c_arr, void **ptr);

DLLEXPORT void gks_gl_plugin(int fctid, int dx, int dy, int dimx, int *i_arr, int len_f_arr_1, double *f_arr_1,
                             int len_f_arr_2, double *f_arr_2, int len_c_arr, char *c_arr, void **ptr);

DLLEXPORT void gks_cairo_plugin(int fctid, int dx, int dy, int dimx, int *i_arr, int len_f_arr_1, double *f_arr_1,
                                int len_f_arr_2, double *f_arr_2, int len_c_arr, char *c_arr, void **ptr);

DLLEXPORT void gks_zmq_plugin(int fctid, int dx, int dy, int dimx, int *i_arr, int len_f_arr_1, double *f_arr_1,
                              int len_f_arr_2, double *f_arr_2, int len_c_arr, char *c_arr, void **ptr);

DLLEXPORT void gks_pgf_plugin(int fctid, int dx, int dy, int dimx, int *i_arr, int len_f_arr_1, double *f_arr_1,
                              int len_f_arr_2, double *f_arr_2, int len_c_arr, char *c_arr, void **ptr);

DLLEXPORT void gks_video_plugin(int fctid, int dx, int dy, int dimx, int *i_arr, int len_f_arr_1, double *f_arr_1,
                                int len_f_arr_2, double *f_arr_2, int len_c_arr, char *c_arr, void **ptr);

#else

void gks_drv_js(int fctid, int dx, int dy, int dimx, int *i_arr, int len_f_arr_1, double *f_arr_1, int len_f_arr_2,
                double *f_arr_2, int len_c_arr, char *c_arr, void **ptr);

#endif

void gks_compress(int bits, unsigned char *in, int in_len, unsigned char *out, int *out_len);

int gks_open_file(const char *path, const char *mode);
int gks_read_file(int fd, void *buf, int count);
int gks_write_file(int fd, void *buf, int count);
int gks_close_file(int fd);

int gks_ft_init(void);
int *gks_ft_render(int *x, int *y, int *width, int *height, gks_state_list_t *gkss, const char *text, int length);
unsigned char *gks_ft_get_bitmap(int *x, int *y, int *width, int *height, gks_state_list_t *gkss, const char *text,
                                 int length);
void *gks_ft_get_face(int);
DLLEXPORT int gks_ft_get_metrics(int font, double fontsize, unsigned int codepoint, unsigned int dpi, double *width,
                                 double *height, double *depth, double *advance, double *bearing, double *xmin,
                                 double *xmax, double *ymin, double *ymax);
DLLEXPORT double gks_ft_get_kerning(int font, double fontsize, unsigned int dpi, unsigned int first_codepoint,
                                    unsigned int second_codepoint);
void gks_ft_terminate(void);
void gks_ft_text(double x, double y, char *text, gks_state_list_t *gkss,
                 void (*gdp)(int, double *, double *, int, int, int *));
void gks_ft_inq_text_extent(double x, double y, char *text, gks_state_list_t *gkss,
                            void (*gdp)(int, double *, double *, int, int, int *), double *bx, double *by);
DLLEXPORT void gks_ft_text3d(double x, double y, double z, char *text, int axis, gks_state_list_t *gkss,
                             double heightFactor, double *scaleFactors,
                             void (*gdp)(int, double *, double *, int, int, int *),
                             void (*wc3towc)(double *, double *, double *));
DLLEXPORT void gks_ft_inq_text3d_extent(double x, double y, double z, char *text, int axis, gks_state_list_t *gkss,
                                        double heightFactor, double *scaleFactors,
                                        void (*gdp)(int, double *, double *, int, int, int *),
                                        void (*wc3towc)(double *, double *, double *), double *bx, double *by);
DLLEXPORT void gks_ft_set_bearing_x_direction(int);
DLLEXPORT void gks_ft_inq_bearing_x_direction(int *);
DLLEXPORT int gks_ft_load_user_font(char *font, int ignore_file_not_found);

DLLEXPORT void gks_set_encoding(int encoding);
DLLEXPORT void gks_inq_encoding(int *encoding);

DLLEXPORT void gks_set_callback(char *(*callback)(const char *));

#ifdef __cplusplus
}
#endif

#endif
