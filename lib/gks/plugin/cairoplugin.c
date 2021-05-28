
#ifdef NO_FT
#ifndef NO_CAIRO
#define NO_CAIRO
#endif
#endif

#ifdef _WIN32
#include <windows.h>

#ifndef DLLEXPORT
#define DLLEXPORT __declspec(dllexport)
#endif
#endif

#include <stdio.h>

#ifndef NO_CAIRO
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#ifdef _MSC_VER
typedef __int64 int64_t;
#else
#include <stdint.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <math.h>
#include <time.h>

#include <cairo/cairo.h>
#ifndef NO_FT
#include <cairo/cairo-ft.h>
#endif
#ifndef NO_X11
#include <cairo/cairo-xlib.h>
#endif

#endif

#ifndef NO_X11

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Intrinsic.h>

#include <pthread.h>
#include <signal.h>
#endif

#if !defined(VMS) && !defined(_WIN32)
#include <unistd.h>
#endif

#ifndef NO_CAIRO
#include <jpeglib.h>

#ifndef NO_TIFF
#include <tiffio.h>
#endif
#endif

#include "gks.h"
#include "gkscore.h"


#ifdef __cplusplus
extern "C" {
#endif

DLLEXPORT void gks_cairoplugin(int fctid, int dx, int dy, int dimx, int *i_arr, int len_f_arr_1, double *f_arr_1,
                               int len_f_arr_2, double *f_arr_2, int len_c_arr, char *c_arr, void **ptr);

#ifdef __cplusplus
}
#endif

#ifndef NO_CAIRO

#define MAX_POINTS 2048
#define PATTERNS 120
#define HATCH_STYLE 108

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef MAXPATHLEN
#define MAXPATHLEN 1024
#endif

#define MAX_TNR 9

#define WC_to_NDC(xw, yw, tnr, xn, yn) \
  xn = a[tnr] * (xw) + b[tnr];         \
  yn = c[tnr] * (yw) + d[tnr]

#define WC_to_NDC_rel(xw, yw, tnr, xn, yn) \
  xn = a[tnr] * (xw);                      \
  yn = c[tnr] * (yw)

#define NDC_to_DC(xn, yn, xd, yd) \
  xd = (p->a * (xn) + p->b);      \
  yd = (p->c * (yn) + p->d)

#define CharXform(xrel, yrel, x, y)                  \
  x = cos(p->alpha) * (xrel)-sin(p->alpha) * (yrel); \
  y = sin(p->alpha) * (xrel) + cos(p->alpha) * (yrel);

#define nint(a) ((int)(a + 0.5))

#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif

#ifndef max
#define max(a, b) (((a) > (b)) ? (a) : (b))
#endif

#ifndef round
#define round(x) (((x) < 0) ? ceil((x)-.5) : floor((x) + .5))
#endif


/* set this flag so that the exit handler won't try to use Cairo X11 support */
static int exit_due_to_x11_support_ = 0;

static gks_state_list_t *gkss;

static double a[MAX_TNR], b[MAX_TNR], c[MAX_TNR], d[MAX_TNR];

typedef unsigned char Byte;
typedef unsigned long uLong;

typedef struct cairo_point_t
{
  double x, y;
} cairo_point;

typedef struct ws_state_list_t
{
  int conid, state, wtype;
  double mw, mh;
  int w, h, dpi;
  char *path;
  void *mem;
  int mem_resizable;
  double a, b, c, d;
  double window[4], viewport[4];
  double rgb[MAX_COLOR + 1][3];
  double transparency;
  int width, height;
  int color;
  double linewidth, nominal_size;
  double alpha, angle;
  int family, capheight;
  cairo_surface_t *surface;
  cairo_t *cr;
  cairo_point *points;
#ifndef NO_X11
  Display *dpy;
  Window win;
  pthread_t thread;
  pthread_mutex_t mutex;
  int run, done;
  Atom wmDeleteMessage;
  pthread_t master_thread;
#endif
  int npoints, max_points;
  int empty, current_page_written, page_counter;
  double rect[MAX_TNR][2][2];
  unsigned char *patterns;
  int pattern_counter, use_symbols;
  double dashes[10];
} ws_state_list;

static ws_state_list *p;

static int idle = 0;

static int predef_prec[] = {0, 1, 2, 2, 2, 2};

static int predef_ints[] = {0, 1, 3, 3, 3};

static int predef_styli[] = {1, 1, 1, 2, 3};

static void set_norm_xform(int tnr, double *wn, double *vp)
{
  a[tnr] = (vp[1] - vp[0]) / (wn[1] - wn[0]);
  b[tnr] = vp[0] - wn[0] * a[tnr];
  c[tnr] = (vp[3] - vp[2]) / (wn[3] - wn[2]);
  d[tnr] = vp[2] - wn[2] * c[tnr];

  NDC_to_DC(vp[0], vp[3], p->rect[tnr][0][0], p->rect[tnr][0][1]);
  NDC_to_DC(vp[1], vp[2], p->rect[tnr][1][0], p->rect[tnr][1][1]);
}

static void init_norm_xform(void)
{
  int tnr;

  for (tnr = 0; tnr < MAX_TNR; tnr++) set_norm_xform(tnr, gkss->window[tnr], gkss->viewport[tnr]);
}

static void set_xform(void)
{
  p->a = p->width / (p->window[1] - p->window[0]);
  p->b = -p->window[0] * p->a;
  p->c = p->height / (p->window[2] - p->window[3]);
  p->d = p->height - p->window[2] * p->c;
}

static void seg_xform(double *x, double *y)
{
  double xx;

  xx = *x * gkss->mat[0][0] + *y * gkss->mat[0][1] + gkss->mat[2][0];
  *y = *x * gkss->mat[1][0] + *y * gkss->mat[1][1] + gkss->mat[2][1];
  *x = xx;
}

static void seg_xform_rel(double *x, double *y)
{
  double xx;

  xx = *x * gkss->mat[0][0] + *y * gkss->mat[0][1];
  *y = *x * gkss->mat[1][0] + *y * gkss->mat[1][1];
  *x = xx;
}

static void resize(int width, int height)
{
  p->width = width;
  p->height = height;
  p->window[0] = p->window[2] = 0.0;
  p->window[1] = p->window[3] = 1.0;
  p->viewport[0] = p->viewport[2] = 0;
  p->viewport[1] = (double)p->width * p->mw / p->w;
  p->viewport[3] = (double)p->height * p->mh / p->h;

  set_xform();
  init_norm_xform();
}

static void set_color_rep(int color, double red, double green, double blue)
{
  if (color >= 0 && color < MAX_COLOR)
    {
      p->rgb[color][0] = red;
      p->rgb[color][1] = green;
      p->rgb[color][2] = blue;
    }
}

static void init_colors(void)
{
  int color;
  double red, green, blue;

  for (color = 0; color < MAX_COLOR; color++)
    {
      gks_inq_rgb(color, &red, &green, &blue);
      set_color_rep(color, red, green, blue);
    }
}

static void set_color(int index)
{
  cairo_set_source_rgba(p->cr, p->rgb[index][0], p->rgb[index][1], p->rgb[index][2], p->transparency);
}

static void draw_marker(double xn, double yn, int mtype, double mscale, int mcolor)
{
  double x, y;
  double scale, xr, yr, x1, x2, y1, y2;
  int pc, op, r, i;

#include "marker.h"

  mscale *= p->nominal_size;
  r = (int)(3 * mscale);
  scale = 0.01 * mscale / 3.0;

  xr = r;
  yr = 0;
  seg_xform_rel(&xr, &yr);
  r = nint(sqrt(xr * xr + yr * yr));

  NDC_to_DC(xn, yn, x, y);

  pc = 0;
  mtype = (r > 0) ? mtype + marker_off : marker_off + 1;

  cairo_set_dash(p->cr, p->dashes, 0, 0);

  do
    {
      op = marker[mtype][pc];
      switch (op)
        {
        case 1: /* point */
          cairo_set_line_cap(p->cr, CAIRO_LINE_CAP_BUTT);
          cairo_set_line_width(p->cr, p->nominal_size);
          set_color(mcolor);
          cairo_rectangle(p->cr, round(x), round(y), 1.0, 1.0);
          cairo_fill(p->cr);
          break;

        case 2: /* line */
          x1 = scale * marker[mtype][pc + 1];
          y1 = scale * marker[mtype][pc + 2];
          seg_xform_rel(&x1, &y1);

          x2 = scale * marker[mtype][pc + 2 + 1];
          y2 = scale * marker[mtype][pc + 2 + 2];
          seg_xform_rel(&x2, &y2);

          cairo_set_line_cap(p->cr, CAIRO_LINE_CAP_BUTT);
          cairo_set_line_width(p->cr, p->nominal_size);
          set_color(mcolor);
          cairo_move_to(p->cr, x - x1, y - y1);
          cairo_line_to(p->cr, x - x2, y - y2);
          cairo_stroke(p->cr);
          pc += 4;
          break;

        case 3: /* polyline */
        case 4: /* filled polygon */
        case 5: /* hollow polygon */
          xr = scale * marker[mtype][pc + 2];
          yr = -scale * marker[mtype][pc + 3];
          seg_xform_rel(&xr, &yr);

          cairo_set_line_cap(p->cr, CAIRO_LINE_CAP_BUTT);
          cairo_set_line_join(p->cr, CAIRO_LINE_JOIN_ROUND);
          cairo_set_line_width(p->cr, p->nominal_size);
          set_color(mcolor);
          cairo_move_to(p->cr, x - xr, y + yr);
          for (i = 1; i < marker[mtype][pc + 1]; i++)
            {
              xr = scale * marker[mtype][pc + 2 + 2 * i];
              yr = -scale * marker[mtype][pc + 3 + 2 * i];
              seg_xform_rel(&xr, &yr);

              cairo_line_to(p->cr, x - xr, y + yr);
            }
          cairo_close_path(p->cr);

          if (op == 4)
            {
              if (gkss->bcoli != gkss->pmcoli)
                {
                  cairo_fill_preserve(p->cr);
                  set_color(gkss->bcoli);
                  cairo_set_line_width(p->cr, gkss->bwidth * p->nominal_size);
                  cairo_stroke(p->cr);
                }
              else
                cairo_fill(p->cr);
            }
          else
            cairo_stroke(p->cr);

          pc += 1 + 2 * marker[mtype][pc + 1];
          break;

        case 6: /* arc */
        case 7: /* filled arc */
        case 8: /* hollow arc */
          cairo_arc(p->cr, x, y, r * 1.0, 0, 2 * M_PI);

          set_color(mcolor);
          if (op == 7)
            {
              if (gkss->bcoli != gkss->pmcoli)
                {
                  cairo_fill_preserve(p->cr);
                  set_color(gkss->bcoli);
                  cairo_set_line_width(p->cr, gkss->bwidth * p->nominal_size);
                  cairo_stroke(p->cr);
                }
              else
                cairo_fill(p->cr);
            }
          else
            cairo_stroke(p->cr);
          break;

        default:
          break;
        }
      pc++;
    }
  while (op != 0);
}

static void polymarker(int n, double *px, double *py)
{
  int mk_type, mk_color, i;
  double mk_size, x, y;

  mk_type = gkss->asf[3] ? gkss->mtype : gkss->mindex;
  mk_size = gkss->asf[4] ? gkss->mszsc : 1;
  mk_color = gkss->asf[5] ? gkss->pmcoli : 1;

  p->linewidth = p->nominal_size;

  for (i = 0; i < n; i++)
    {
      WC_to_NDC(px[i], py[i], gkss->cntnr, x, y);
      seg_xform(&x, &y);

      draw_marker(x, y, mk_type, mk_size, mk_color);
    }
}

static void stroke(void)
{
  int i;

  cairo_move_to(p->cr, p->points[0].x, p->points[0].y);
  for (i = 1; i < p->npoints; i++)
    {
      cairo_line_to(p->cr, p->points[i].x, p->points[i].y);
    }
  cairo_stroke(p->cr);

  p->npoints = 0;
}

static void move(double x, double y)
{
  if (p->npoints > 0) stroke();

  NDC_to_DC(x, y, p->points[p->npoints].x, p->points[p->npoints].y);
  p->npoints++;
}

static void draw(double x, double y)
{
  NDC_to_DC(x, y, p->points[p->npoints].x, p->points[p->npoints].y);
  p->npoints++;
}

static void line_routine(int n, double *px, double *py, int linetype, int tnr)
{
  double x, y, x0, y0, xi, yi;
  int i;

  WC_to_NDC(px[0], py[0], tnr, x, y);
  seg_xform(&x, &y);
  NDC_to_DC(x, y, x0, y0);

  cairo_set_line_cap(p->cr, CAIRO_LINE_CAP_BUTT);
  cairo_set_line_join(p->cr, CAIRO_LINE_JOIN_ROUND);
  cairo_set_line_width(p->cr, p->linewidth);

  cairo_move_to(p->cr, x0, y0);

  for (i = 1; i < n; i++)
    {
      WC_to_NDC(px[i], py[i], tnr, x, y);
      seg_xform(&x, &y);
      NDC_to_DC(x, y, xi, yi);

      cairo_line_to(p->cr, xi, yi);
    }
  cairo_stroke(p->cr);
}

static void fill_routine(int n, double *px, double *py, int tnr)
{
  int i, j, k;
  double x, y, ix, iy;
  int fl_inter, fl_style, fl_color, size;
  int gks_pattern[33];
  cairo_format_t format = CAIRO_FORMAT_ARGB32;
  int stride = cairo_format_stride_for_width(format, 8);
  cairo_pattern_t *pattern;
  cairo_surface_t *image;
  cairo_matrix_t pattern_matrix;

  WC_to_NDC(px[0], py[0], tnr, x, y);
  seg_xform(&x, &y);
  NDC_to_DC(x, y, ix, iy);

  cairo_set_dash(p->cr, p->dashes, 0, 0);

  cairo_move_to(p->cr, ix, iy);

  for (i = 1; i < n; i++)
    {
      WC_to_NDC(px[i], py[i], tnr, x, y);
      seg_xform(&x, &y);
      NDC_to_DC(x, y, ix, iy);
      cairo_line_to(p->cr, ix, iy);
    }

  cairo_close_path(p->cr);

  fl_inter = gkss->asf[10] ? gkss->ints : predef_ints[gkss->findex - 1];
  fl_color = gkss->asf[12] ? gkss->facoli : 1;

  if (fl_inter == GKS_K_INTSTYLE_PATTERN || fl_inter == GKS_K_INTSTYLE_HATCH)
    {
      fl_style = gkss->asf[11] ? gkss->styli : predef_styli[gkss->findex - 1];
      if (fl_inter == GKS_K_INTSTYLE_HATCH)
        {
          fl_style += HATCH_STYLE;
        }
      if (fl_style >= PATTERNS)
        {
          fl_style = 1;
        }
      gks_inq_pattern_array(fl_style, gks_pattern);
      size = gks_pattern[0];

      p->patterns = (unsigned char *)gks_realloc(p->patterns, size * 8 * 4 * sizeof(unsigned char));
      memset(p->patterns, 0, size * 8 * 4 * sizeof(unsigned char));

      for (j = 1; j < size + 1; j++)
        {
          for (i = 0; i < 8; i++)
            {
              k = (1 << i) & gks_pattern[j];
              if (!(k))
                {
                  int p_ind = ((i + 7) % 8) + ((j - 1 + (size - 1)) % size) * 8;
                  p->patterns[p_ind * 4 + 3] = (unsigned char)(255 * p->transparency);
                  p->patterns[p_ind * 4 + 2] = (unsigned char)(255 * p->rgb[fl_color][0] * p->transparency);
                  p->patterns[p_ind * 4 + 1] = (unsigned char)(255 * p->rgb[fl_color][1] * p->transparency);
                  p->patterns[p_ind * 4 + 0] = (unsigned char)(255 * p->rgb[fl_color][2] * p->transparency);
                }
            }
        }

      image = cairo_image_surface_create_for_data(p->patterns, format, 8, size, stride);

      pattern = cairo_pattern_create_for_surface(image);
      cairo_pattern_set_extend(pattern, CAIRO_EXTEND_REPEAT);
      cairo_pattern_set_filter(pattern, CAIRO_FILTER_NEAREST);
      cairo_matrix_init_scale(&pattern_matrix, 500.0 / min(p->width, p->height), 500.0 / min(p->width, p->height));
      cairo_pattern_set_matrix(pattern, &pattern_matrix);

      cairo_set_source(p->cr, pattern);
    }

  if (fl_inter == GKS_K_INTSTYLE_PATTERN || fl_inter == GKS_K_INTSTYLE_HATCH || fl_inter == GKS_K_INTSTYLE_SOLID)
    {
      cairo_fill(p->cr);
    }
  else
    {
      cairo_stroke(p->cr);
    }
}

static void fillarea(int n, double *px, double *py)
{
  int fl_color;

  p->linewidth = p->nominal_size;

  fl_color = gkss->asf[12] ? gkss->facoli : 1;
  set_color(fl_color);

  cairo_set_fill_rule(p->cr, CAIRO_FILL_RULE_EVEN_ODD);
  fill_routine(n, px, py, gkss->cntnr);
  cairo_set_fill_rule(p->cr, CAIRO_FILL_RULE_WINDING);
}

static void polyline(int n, double *px, double *py)
{
  int ln_type, ln_color, i;
  double ln_width;
  int gks_dashes[10];

  if (n > p->max_points)
    {
      p->points = (cairo_point *)gks_realloc(p->points, n * sizeof(cairo_point));
      p->max_points = n;
    }

  ln_type = gkss->asf[0] ? gkss->ltype : gkss->lindex;
  ln_width = gkss->asf[1] ? gkss->lwidth : 1;
  ln_color = gkss->asf[2] ? gkss->plcoli : 1;

  cairo_set_line_cap(p->cr, CAIRO_LINE_CAP_BUTT);
  cairo_set_line_join(p->cr, CAIRO_LINE_JOIN_ROUND);
  p->linewidth = ln_width * p->nominal_size;
  cairo_set_line_width(p->cr, p->linewidth);

  p->color = ln_color;
  set_color(ln_color);

  gks_get_dash_list(ln_type, ln_width, gks_dashes);
  for (i = 0; i < gks_dashes[0]; i++) p->dashes[i] = gks_dashes[i + 1] * min(p->width, p->height) / 500.0;
  cairo_set_dash(p->cr, p->dashes, gks_dashes[0], 0);

  gks_set_dev_xform(gkss, p->window, p->viewport);
  gks_emul_polyline(n, px, py, ln_type, gkss->cntnr, move, draw);

  if (p->npoints > 0) stroke();
}

static void text_routine(double x, double y, int nchars, char *chars)
{
  cairo_surface_t *image;
  int i, j;
  int stride;
  int width = p->width;
  int height = p->height;
  int px, py;
  unsigned char *alpha_pixels;
  unsigned char *bgra_pixels;
  double red, green, blue;

  NDC_to_DC(x, y, px, py);
  py = p->height - py;

  alpha_pixels = gks_ft_get_bitmap(&px, &py, &width, &height, gkss, chars, nchars);
  stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, width);
  bgra_pixels = (unsigned char *)gks_malloc(4 * height * stride);

  gks_inq_rgb(p->color, &red, &green, &blue);
  for (i = 0; i < height; i++)
    {
      for (j = 0; j < width; j++)
        {
          double alpha = alpha_pixels[i * width + j];
          bgra_pixels[i * stride + j * 4 + 0] = blue * alpha;
          bgra_pixels[i * stride + j * 4 + 1] = green * alpha;
          bgra_pixels[i * stride + j * 4 + 2] = red * alpha;
          bgra_pixels[i * stride + j * 4 + 3] = alpha;
        }
    }
  image = cairo_image_surface_create_for_data(bgra_pixels, CAIRO_FORMAT_ARGB32, width, height, stride);
  cairo_set_source_surface(p->cr, image, px, p->height - py - height);
  cairo_paint(p->cr);
  cairo_surface_destroy(image);
  gks_free(bgra_pixels);
  gks_free(alpha_pixels);
}


static void text(double px, double py, int nchars, char *chars)
{
  int tx_prec, tx_color;
  double x, y;

  tx_prec = gkss->asf[6] ? gkss->txprec : predef_prec[gkss->tindex - 1];
  tx_color = gkss->asf[9] ? gkss->txcoli : 1;

  p->color = tx_color;

  cairo_set_dash(p->cr, p->dashes, 0, 0);
  set_color(tx_color);

  if (tx_prec == GKS_K_TEXT_PRECISION_STRING)
    {
      WC_to_NDC(px, py, gkss->cntnr, x, y);
      seg_xform(&x, &y);

      text_routine(x, y, nchars, chars);
    }
  else
    {
      p->linewidth = p->nominal_size;
      gks_emul_text(px, py, nchars, chars, line_routine, fill_routine);
    }
}

static void cellarray(double xmin, double xmax, double ymin, double ymax, int dx, int dy, int dimx, int *colia,
                      int true_color)
{
  double x1, y1, x2, y2, x, y;
  int ix1, ix2, iy1, iy2;
  int width, height;
  double red, green, blue, alpha;
  int i, j, ix, iy, ind;
  int swapx, swapy;
  int stride;
  unsigned char *data;
  cairo_surface_t *image;

  WC_to_NDC(xmin, ymax, gkss->cntnr, x1, y1);
  seg_xform(&x1, &y1);
  NDC_to_DC(x1, y1, ix1, iy1);

  WC_to_NDC(xmax, ymin, gkss->cntnr, x2, y2);
  seg_xform(&x2, &y2);
  NDC_to_DC(x2, y2, ix2, iy2);

  width = abs(ix2 - ix1);
  height = abs(iy2 - iy1);
  if (width == 0 || height == 0) return;
  x = min(ix1, ix2);
  y = min(iy1, iy2);

  swapx = ix1 > ix2;
  swapy = iy1 < iy2;
  stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, width);
  data = (unsigned char *)gks_malloc(stride * height);

  if (true_color)
    {
      gks_resample((unsigned char *)colia, data, (size_t)dx, (size_t)dy, (size_t)width, (size_t)height, (size_t)dimx,
                   swapx, swapy, gkss->resample_method);
      for (i = width - 1; i >= 0; i--)
        {
          for (j = height - 1; j >= 0; j--)
            {
              red = data[(j * width + i) * 4 + 0];
              green = data[(j * width + i) * 4 + 1];
              blue = data[(j * width + i) * 4 + 2];
              alpha = data[(j * width + i) * 4 + 3] * p->transparency;
              /* ARGB32 format requires pre-multiplied alpha */
              data[j * stride + i * 4 + 0] = (unsigned char)(blue * alpha / 255);
              data[j * stride + i * 4 + 1] = (unsigned char)(green * alpha / 255);
              data[j * stride + i * 4 + 2] = (unsigned char)(red * alpha / 255);
              data[j * stride + i * 4 + 3] = (unsigned char)alpha;
            }
        }
    }
  else
    {
      for (j = 0; j < height; j++)
        {
          iy = dy * j / height;
          if (swapy)
            {
              iy = dy - 1 - iy;
            }
          for (i = 0; i < width; i++)
            {
              ix = dx * i / width;
              if (swapx)
                {
                  ix = dx - 1 - ix;
                }
              ind = colia[iy * dimx + ix];
              ind = FIX_COLORIND(ind);
              red = 255 * p->rgb[ind][0];
              green = 255 * p->rgb[ind][1];
              blue = 255 * p->rgb[ind][2];
              alpha = 255 * p->transparency;
              /* ARGB32 format requires pre-multiplied alpha */
              data[j * stride + i * 4 + 0] = (unsigned char)(blue * alpha / 255);
              data[j * stride + i * 4 + 1] = (unsigned char)(green * alpha / 255);
              data[j * stride + i * 4 + 2] = (unsigned char)(red * alpha / 255);
              data[j * stride + i * 4 + 3] = (unsigned char)alpha;
            }
        }
    }

  image = cairo_image_surface_create_for_data(data, CAIRO_FORMAT_ARGB32, width, height, stride);
  cairo_set_source_surface(p->cr, image, x, y);
  cairo_paint(p->cr);
  cairo_surface_destroy(image);
  free(data);
}

static void set_clip_rect(int tnr)
{
  cairo_reset_clip(p->cr);

  if (gkss->clip_tnr != 0 || gkss->clip == GKS_K_CLIP)
    {
      if (gkss->clip_tnr != 0)
        {
          tnr = gkss->clip_tnr;
        }
      cairo_rectangle(p->cr, p->rect[tnr][0][0], p->rect[tnr][0][1], p->rect[tnr][1][0] - p->rect[tnr][0][0],
                      p->rect[tnr][1][1] - p->rect[tnr][0][1]);
      cairo_clip(p->cr);
    }
}

static void set_clipping(int index)
{
  gkss->clip = index;
  set_clip_rect(gkss->cntnr);
}

static void lock(void)
{
#ifndef NO_X11
  if (p->wtype == 141) pthread_mutex_lock(&p->mutex);
#endif
}

static void unlock(void)
{
#ifndef NO_X11
  if (p->wtype == 141) pthread_mutex_unlock(&p->mutex);
#endif
}

#ifndef NO_X11

static void *event_loop(void *arg)
{
  ws_state_list *p = (ws_state_list *)arg;
  XEvent event;

  p->run = 1;
  while (p->run)
    {
      usleep(10000);

      if (idle && p->run)
        {
          if (pthread_mutex_trylock(&p->mutex) == 0)
            {
              while (XPending(p->dpy))
                {
                  XNextEvent(p->dpy, &event);
                  if (event.type == ClientMessage)
                    {
                      if ((Atom)event.xclient.data.l[0] == p->wmDeleteMessage)
                        {
#ifdef SIGUSR1
                          pthread_kill(p->master_thread, SIGUSR1);
#endif
                          p->run = 0;
                        }
                    }
                  else if (event.type == ConfigureNotify)
                    {
                      cairo_xlib_surface_set_size(p->surface, event.xconfigure.width, event.xconfigure.height);
                    }
                }
              pthread_mutex_unlock(&p->mutex);
            }
        }
    }
  p->done = 1;

  pthread_exit(0);
}

static void create_window(void)
{
  int screen;
  pthread_attr_t attr;

  if (!(p->dpy = XOpenDisplay(NULL)))
    {
      gks_perror("Could not open display");
      exit(1);
    }

  screen = DefaultScreen(p->dpy);
  p->win = XCreateSimpleWindow(p->dpy, RootWindow(p->dpy, screen), 1, 1, p->width, p->height, 0,
                               BlackPixel(p->dpy, screen), WhitePixel(p->dpy, screen));

  p->master_thread = pthread_self();
  p->wmDeleteMessage = XInternAtom(p->dpy, "WM_DELETE_WINDOW", False);
  XSetWMProtocols(p->dpy, p->win, &p->wmDeleteMessage, 1);

  XStoreName(p->dpy, p->win, "GKS");
  XSelectInput(p->dpy, p->win, StructureNotifyMask | ExposureMask);
  XMapWindow(p->dpy, p->win);

  pthread_mutex_init(&p->mutex, NULL);
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
  if (pthread_create(&p->thread, &attr, event_loop, (void *)p)) perror("pthread_create");
  pthread_attr_destroy(&attr);
}

#endif

/**
 * Write an empty page or image.
 *
 * This is currently being ignored for most workstation types, but for memory
 * output this will ensure that the memory is initialized to white and, if
 * resizable memory is used, that the size is set correctly.
 */
static void write_empty_page(void)
{
  p->current_page_written = 1;
  if (p->wtype == 143 && p->mem)
    {
      int width = cairo_image_surface_get_width(p->surface);
      int height = cairo_image_surface_get_height(p->surface);
      unsigned char *mem;
      if (p->mem_resizable)
        {
          int *mem_info_ptr = (int *)p->mem;
          unsigned char **mem_ptr_ptr = (unsigned char **)(mem_info_ptr + 3);
          mem_info_ptr[0] = width;
          mem_info_ptr[1] = height;
          *mem_ptr_ptr = (unsigned char *)gks_realloc(*mem_ptr_ptr, width * height * 4);
          mem = *mem_ptr_ptr;
        }
      else
        {
          mem = p->mem;
        }
      memset(mem, 255, height * width * 4);
    }
}

static void open_page(void)
{
  char *env;
  exit_due_to_x11_support_ = 0;

  if (p->wtype == 141)
    {
#ifndef NO_X11
      create_window();
      p->surface = cairo_xlib_surface_create(p->dpy, p->win, DefaultVisual(p->dpy, 0), p->width, p->height);
#else
      gks_perror("Cairo X11 support not compiled in");
      exit_due_to_x11_support_ = 1;
      exit(1);
#endif
    }
  else if (p->wtype == 140 || p->wtype == 143 || p->wtype == 144 || p->wtype == 145 || p->wtype == 146 ||
           p->wtype == 150)
    {
      p->surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, p->width, p->height);
    }
  if (p->wtype == 142)
    {
      env = (char *)gks_getenv("GKS_CONID");
      if (!env) env = (char *)gks_getenv("GKSconid");

      if (env != NULL)
        sscanf(env, "%lu", (unsigned long *)&p->cr);
      else
        {
          gks_perror("can't obtain Gtk drawable");
          exit(1);
        }
    }
  else
    p->cr = cairo_create(p->surface);

  write_empty_page();
}

static void close_page(void)
{
  if (p->wtype != 142)
    {
      cairo_destroy(p->cr);
      cairo_surface_destroy(p->surface);
    }
#ifndef NO_X11
  if (p->wtype == 141)
    {
      if (p->run)
        {
          /* pthread_join didn't work for some reason */
          p->done = 0;
          while (!p->done)
            {
              p->run = 0;
              usleep(10000);
            }
        }
      XDestroyWindow(p->dpy, p->win);
      XCloseDisplay(p->dpy);
    }
#endif
}

#define ON_INHEAP 1

typedef struct oct_node_t oct_node_t, *oct_node;

struct oct_node_t
{
  int64_t r, g, b; /* sum of all child node colors */
  int count, heap_idx;
  unsigned char n_kids, kid_idx, flags, depth;
  oct_node kids[8], parent;
};

typedef struct
{
  int alloc, n;
  oct_node *buf;
} node_heap;

static int cmp_node(oct_node a, oct_node b)
{
  int ac, bc;

  if (a->n_kids < b->n_kids) return -1;
  if (a->n_kids > b->n_kids) return 1;

  ac = a->count >> a->depth;
  bc = b->count >> b->depth;

  return ac < bc ? -1 : ac > bc;
}

static void down_heap(node_heap *h, oct_node p)
{
  int n = p->heap_idx, m;

  while (1)
    {
      m = n * 2;
      if (m >= h->n) break;
      if (m + 1 < h->n && cmp_node(h->buf[m], h->buf[m + 1]) > 0) m++;

      if (cmp_node(p, h->buf[m]) <= 0) break;

      h->buf[n] = h->buf[m];
      h->buf[n]->heap_idx = n;
      n = m;
    }
  h->buf[n] = p;
  p->heap_idx = n;
}

static void up_heap(node_heap *h, oct_node p)
{
  int n = p->heap_idx;
  oct_node prev;

  while (n > 1)
    {
      prev = h->buf[n / 2];
      if (cmp_node(p, prev) >= 0) break;

      h->buf[n] = prev;
      prev->heap_idx = n;
      n /= 2;
    }
  h->buf[n] = p;
  p->heap_idx = n;
}

static void heap_add(node_heap *h, oct_node p)
{
  if ((p->flags & ON_INHEAP))
    {
      down_heap(h, p);
      up_heap(h, p);
      return;
    }

  p->flags |= ON_INHEAP;
  if (!h->n) h->n = 1;
  if (h->n >= h->alloc)
    {
      while (h->n >= h->alloc) h->alloc += 1024;
      h->buf = (oct_node *)gks_realloc(h->buf, sizeof(oct_node) * h->alloc);
    }

  p->heap_idx = h->n;
  h->buf[h->n++] = p;
  up_heap(h, p);
}

static oct_node pop_heap(node_heap *h)
{
  oct_node ret;

  if (h->n <= 1) return NULL;

  ret = h->buf[1];
  h->buf[1] = h->buf[--h->n];

  h->buf[h->n] = NULL;

  h->buf[1]->heap_idx = 1;
  down_heap(h, h->buf[1]);

  return ret;
}

static oct_node pool = NULL;

static oct_node node_new(unsigned char idx, unsigned char depth, oct_node p)
{
  static int len = 0;
  oct_node x, n;

  if (len <= 1 || pool == NULL)
    {
      n = (oct_node)gks_malloc(sizeof(oct_node_t) * 2048);
      n->parent = pool;
      pool = n;
      len = 2047;
    }

  x = pool + len--;
  x->kid_idx = idx;
  x->depth = depth;
  x->parent = p;
  if (p) p->n_kids++;

  return x;
}

static void node_free()
{
  oct_node p;

  while (pool)
    {
      p = pool->parent;
      free(pool);
      pool = p;
    }
}

static oct_node node_insert(oct_node root, unsigned char *pix)
{
  unsigned char i, bit, depth = 0;

  for (bit = 1 << 7; ++depth < 8; bit >>= 1)
    {
      i = !!(pix[1] & bit) * 4 + !!(pix[0] & bit) * 2 + !!(pix[2] & bit);
      if (!root->kids[i]) root->kids[i] = node_new(i, depth, root);

      root = root->kids[i];
    }

  root->r += pix[0];
  root->g += pix[1];
  root->b += pix[2];
  root->count++;

  return root;
}

static oct_node node_fold(oct_node p)
{
  oct_node q;

  if (p->n_kids) abort();
  q = p->parent;
  q->count += p->count;

  q->r += p->r;
  q->g += p->g;
  q->b += p->b;
  q->n_kids--;
  q->kids[p->kid_idx] = NULL;

  return q;
}

static int color_replace(oct_node root, unsigned char *pix)
{
  unsigned char i, bit;

  for (bit = 1 << 7; bit; bit >>= 1)
    {
      i = !!(pix[1] & bit) * 4 + !!(pix[0] & bit) * 2 + !!(pix[2] & bit);
      if (!root->kids[i]) break;
      root = root->kids[i];
    }

  pix[0] = root->r;
  pix[1] = root->g;
  pix[2] = root->b;

  return root->heap_idx;
}

#define N_COLORS 256

#define WRITE_SIXEL_DATA                                     \
  if (cache == -1)                                           \
    c = 0x3f;                                                \
  else                                                       \
    {                                                        \
      c = 0x3f + n;                                          \
      if (slots[cache] == 0)                                 \
        {                                                    \
          r = palette[cache * 3 - 3] * 100 / 256;            \
          g = palette[cache * 3 - 2] * 100 / 256;            \
          b = palette[cache * 3 - 1] * 100 / 256;            \
          slots[cache] = 1;                                  \
          fprintf(stream, "#%d;2;%d;%d;%d", cache, r, g, b); \
        }                                                    \
      fprintf(stream, "#%d", cache);                         \
    }                                                        \
  if (count < 3)                                             \
    for (i = 0; i < count; i++) fprintf(stream, "%c", c);    \
  else                                                       \
    fprintf(stream, "!%d%c", count, c);

static void write_sixels(char *path, int width, int height, int *palette, int *data)
{
  int i, slots[257];
  FILE *stream;
  int n, x, y, p, cache, count, c, color;
  int r, g, b;

  for (i = 0; i <= 256; i++) slots[i] = 0;

  stream = fopen(path, "w");

  fprintf(stream, "%c%s", 0x1b, "P");
  fprintf(stream, "%d;%d;%dq\"1;1;%d;%d", 7, 1, 75, width, height);

  n = 1;
  for (y = 0; y < height; y++)
    {
      p = y * width;
      cache = data[p];
      count = 1;
      c = -1;
      for (x = 0; x < width; x++)
        {
          color = data[p + x];
          if (color == cache)
            count += 1;
          else
            {
              WRITE_SIXEL_DATA;
              count = 1;
              cache = color;
            }
        }
      if (c != -1 && count > 1)
        {
          WRITE_SIXEL_DATA;
        }
      if (n == 32)
        {
          n = 1;
          fprintf(stream, "-");
        }
      else
        {
          n <<= 1;
          fprintf(stream, "$");
        }
    }
  fprintf(stream, "%c\\", 0x1b);
  fclose(stream);
}

static void write_to_six(char *path, int width, int height, unsigned char *data)
{
  unsigned char *pix = data;
  oct_node root, got;
  int i, j;
  node_heap heap = {0, 0, NULL};
  double c;
  int r, g, b, *palette, *ca;

  root = node_new(0, 0, NULL);
  for (i = 0; i < width * height; i++, pix += 4) heap_add(&heap, node_insert(root, pix));

  while (heap.n > N_COLORS + 1) heap_add(&heap, node_fold(pop_heap(&heap)));

  palette = (int *)gks_malloc(heap.n * 3 * sizeof(int));
  for (i = 1, j = 0; i < heap.n; i++)
    {
      got = heap.buf[i];
      c = got->count;
      got->r = got->r / c + 0.5;
      got->g = got->g / c + 0.5;
      got->b = got->b / c + 0.5;
      r = (int)got->r & 0xff;
      g = (int)got->g & 0xff;
      b = (int)got->b & 0xff;
      palette[j++] = r;
      palette[j++] = g;
      palette[j++] = b;
    }

  ca = (int *)gks_malloc(width * height * sizeof(int));
  for (i = 0, pix = data; i < width * height; i++, pix += 4) ca[i] = color_replace(root, pix);

  write_sixels(path, width, height, palette, ca);

  node_free();
  free(heap.buf);
}

static void write_page(void)
{
  char path[MAXPATHLEN];
  unsigned char *data, *pix;
  int width, height, stride;
  double alpha;
  int i, j, k, l, bg[3] = {255, 255, 255};

  p->current_page_written = 1;
  p->page_counter++;

  cairo_show_page(p->cr);

  if (p->wtype == 140)
    {
      gks_filepath(path, p->path, "png", p->page_counter, 0);
      cairo_surface_write_to_png(p->surface, path);
    }
#ifndef NO_X11
  else if (p->wtype == 141)
    XSync(p->dpy, False);
#endif
  else if (p->wtype == 143)
    {
      cairo_surface_flush(p->surface);
      data = cairo_image_surface_get_data(p->surface);
      width = cairo_image_surface_get_width(p->surface);
      height = cairo_image_surface_get_height(p->surface);
      stride = cairo_image_surface_get_stride(p->surface);
      if (p->mem)
        {
          unsigned char *mem;
          if (p->mem_resizable)
            {
              int *mem_info_ptr = (int *)p->mem;
              unsigned char **mem_ptr_ptr = (unsigned char **)(mem_info_ptr + 3);
              mem_info_ptr[0] = width;
              mem_info_ptr[1] = height;
              *mem_ptr_ptr = (unsigned char *)gks_realloc(*mem_ptr_ptr, width * height * 4);
              mem = *mem_ptr_ptr;
            }
          else
            {
              mem = p->mem;
            }
          for (j = 0; j < height; j++)
            {
              for (i = 0; i < width; i++)
                {
                  /* Reverse alpha pre-multiplication */
                  double alpha = data[j * stride + i * 4 + 3];
                  double red = data[j * stride + i * 4 + 2] * 255.0 / alpha;
                  double green = data[j * stride + i * 4 + 1] * 255.0 / alpha;
                  double blue = data[j * stride + i * 4 + 0] * 255.0 / alpha;
                  if (red > 255)
                    {
                      red = 255;
                    }
                  if (green > 255)
                    {
                      green = 255;
                    }
                  if (blue > 255)
                    {
                      blue = 255;
                    }
                  mem[j * width * 4 + i * 4 + 0] = (unsigned char)red;
                  mem[j * width * 4 + i * 4 + 1] = (unsigned char)green;
                  mem[j * width * 4 + i * 4 + 2] = (unsigned char)blue;
                  mem[j * width * 4 + i * 4 + 3] = (unsigned char)alpha;
                }
            }
        }
    }
  else if (p->wtype == 144)
    {
      FILE *fp;

      gks_filepath(path, p->path, "jpg", p->page_counter, 0);
      fp = fopen(path, "wb");
      if (!fp)
        {
          fprintf(stderr, "GKS: Failed to open file: %s\n", path);
        }
      else
        {
          cairo_surface_flush(p->surface);
          unsigned char *data = cairo_image_surface_get_data(p->surface);
          int width = cairo_image_surface_get_width(p->surface);
          int height = cairo_image_surface_get_height(p->surface);
          int stride = cairo_image_surface_get_stride(p->surface);
          unsigned char *row = (unsigned char *)gks_malloc(width * 3);

          struct jpeg_compress_struct cinfo;
          struct jpeg_error_mgr jerr;

          cinfo.err = jpeg_std_error(&jerr);
          jpeg_create_compress(&cinfo);
          jpeg_stdio_dest(&cinfo, fp);

          cinfo.image_width = width;
          cinfo.image_height = height;
          cinfo.input_components = 3;
          cinfo.in_color_space = JCS_RGB;
          jpeg_set_defaults(&cinfo);
          jpeg_set_quality(&cinfo, 100, 1);

          jpeg_start_compress(&cinfo, 1);
          while (cinfo.next_scanline < cinfo.image_height)
            {
              int i, j;
              for (i = 0; i < width; i++)
                {
                  double alpha = data[cinfo.next_scanline * stride + i * 4 + 3] / 255.0;
                  for (j = 0; j < 3; j++)
                    {
                      int component = data[cinfo.next_scanline * stride + i * 4 + (2 - j)];
                      component = bg[j] * (1 - alpha) + component * alpha + 0.5;
                      if (component > 255)
                        {
                          component = 255;
                        }
                      row[i * 3 + j] = component;
                    }
                }
              jpeg_write_scanlines(&cinfo, &row, 1);
            }
          jpeg_finish_compress(&cinfo);
          jpeg_destroy_compress(&cinfo);
          fclose(fp);
          gks_free(row);
        }
    }
  else if (p->wtype == 145)
    {
      FILE *fp;

      gks_filepath(path, p->path, "bmp", p->page_counter, 0);
      fp = fopen(path, "wb");
      if (!fp)
        {
          fprintf(stderr, "GKS: Failed to open file: %s\n", path);
        }
      else
        {
          cairo_surface_flush(p->surface);
          data = cairo_image_surface_get_data(p->surface);
          width = cairo_image_surface_get_width(p->surface);
          height = cairo_image_surface_get_height(p->surface);
          stride = cairo_image_surface_get_stride(p->surface);
          int padding = width % 4;
          int bmp_stride = 3 * width + padding;
          unsigned int file_size = 54 + bmp_stride * height;
          unsigned char *row = (unsigned char *)gks_malloc(bmp_stride);
          unsigned char header[54] = {0};
          header[0] = 'B';
          header[1] = 'M';
          header[2] = file_size / (1 << 0);
          header[3] = file_size / (1 << 8);
          header[4] = file_size / (1 << 16);
          header[5] = file_size / (1 << 24);
          header[10] = 54;
          header[14] = 40;
          header[18] = width / (1 << 0);
          header[19] = width / (1 << 8);
          header[20] = width / (1 << 16);
          header[21] = width / (1 << 24);
          header[22] = height / (1 << 0);
          header[23] = height / (1 << 8);
          header[24] = height / (1 << 16);
          header[25] = height / (1 << 24);
          header[26] = 1;
          header[28] = 24;
          fwrite(header, 1, 54, fp);

          for (i = width; i < bmp_stride; i++)
            {
              row[i] = 0;
            }
          for (i = 0, i = 0; i < height; i++)
            {
              for (j = 0; j < width; j++)
                {
                  double alpha = data[(height - i - 1) * stride + j * 4 + 3] / 255.0;
                  for (k = 0; k < 3; k++)
                    {
                      int component = data[stride * (height - i - 1) + j * 4 + k];
                      component = bg[k] * (1 - alpha) + component * alpha + 0.5;
                      if (component > 255)
                        {
                          component = 255;
                        }
                      row[j * 3 + k] = component;
                    }
                }
              fwrite(row, bmp_stride, 1, fp);
            }
          fclose(fp);
          gks_free(row);
        }
    }
  else if (p->wtype == 146)
    {
#ifdef NO_TIFF
      gks_perror("Cairo TIFF support not compiled in");
#else
      TIFF *fp;

      gks_filepath(path, p->path, "tif", p->page_counter, 0);
      fp = TIFFOpen(path, "w");
      if (!fp)
        {
          fprintf(stderr, "GKS: Failed to open file: %s\n", path);
        }
      else
        {
          time_t current_datetime = time(NULL);
          cairo_surface_flush(p->surface);
          data = cairo_image_surface_get_data(p->surface);
          width = cairo_image_surface_get_width(p->surface);
          height = cairo_image_surface_get_height(p->surface);
          stride = cairo_image_surface_get_stride(p->surface);
          pix = (unsigned char *)gks_malloc(stride);
          TIFFSetField(fp, TIFFTAG_IMAGEWIDTH, width);
          TIFFSetField(fp, TIFFTAG_IMAGELENGTH, height);
          TIFFSetField(fp, TIFFTAG_SAMPLESPERPIXEL, 4);
          TIFFSetField(fp, TIFFTAG_BITSPERSAMPLE, 8);
          TIFFSetField(fp, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
          TIFFSetField(fp, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
          TIFFSetField(fp, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
          TIFFSetField(fp, TIFFTAG_COMPRESSION, COMPRESSION_DEFLATE);
          TIFFSetField(fp, TIFFTAG_ROWSPERSTRIP, TIFFDefaultStripSize(fp, stride));
          TIFFSetField(fp, TIFFTAG_SOFTWARE, "GKS Cairo Plugin");
          TIFFSetField(fp, TIFFTAG_XRESOLUTION, p->w * 0.0254 / p->mw);
          TIFFSetField(fp, TIFFTAG_YRESOLUTION, p->h * 0.0254 / p->mh);
          TIFFSetField(fp, TIFFTAG_RESOLUTIONUNIT, RESUNIT_INCH);
          if (current_datetime != -1)
            {
              struct tm *current_datetime_struct = localtime(&current_datetime);
              if (current_datetime_struct)
                {
                  char current_datetime_str[20] = {0};
                  if (strftime(current_datetime_str, 20, "%Y:%m:%d %H:%M:%S", current_datetime_struct))
                    {
                      TIFFSetField(fp, TIFFTAG_DATETIME, current_datetime_str);
                    }
                }
            }
          for (i = 0; i < height; i++)
            {
              memcpy(pix, data + i * stride, stride);
              for (j = 0; j < width; j++)
                {
                  int tmp;
                  tmp = pix[4 * j + 0];
                  pix[4 * j + 0] = pix[4 * j + 2];
                  pix[4 * j + 2] = tmp;
                }
              if (TIFFWriteScanline(fp, pix, i, 0) < 0)
                {
                  break;
                }
            }
          free(pix);
          if (i != height)
            {
              fprintf(stderr, "GKS: Failed to write file: %s\n", path);
            }
          TIFFClose(fp);
        }
#endif
    }
  else if (p->wtype == 150)
    {
      cairo_surface_flush(p->surface);

      data = cairo_image_surface_get_data(p->surface);
      width = cairo_image_surface_get_width(p->surface);
      height = cairo_image_surface_get_height(p->surface);
      stride = cairo_image_surface_get_stride(p->surface);

      pix = (unsigned char *)gks_malloc(width * height * 4);
      l = 0;
      for (j = 0; j < height; j++)
        {
          for (i = 0; i < width; i++)
            {
              k = j * stride + i * 4;
              alpha = data[k + 3] / 255.0;
              pix[l++] = (int)(data[k + 2] * alpha + bg[0] * (1 - alpha) + 0.5);
              pix[l++] = (int)(data[k + 1] * alpha + bg[1] * (1 - alpha) + 0.5);
              pix[l++] = (int)(data[k + 0] * alpha + bg[2] * (1 - alpha) + 0.5);
              pix[l++] = 255;
            }
        }
      gks_filepath(path, p->path, "six", p->page_counter, 0);
      write_to_six(path, width, height, pix);
      free(pix);
    }
}

static void select_xform(int tnr)
{
  gkss->cntnr = tnr;
  set_clip_rect(tnr);
}

static void set_transparency(double alpha)
{
  p->transparency = alpha;
}

void set_window(int tnr, double xmin, double xmax, double ymin, double ymax)
{
  gkss->window[tnr][0] = xmin;
  gkss->window[tnr][1] = xmax;
  gkss->window[tnr][2] = ymin;
  gkss->window[tnr][3] = ymax;

  set_xform();
  set_norm_xform(tnr, gkss->window[tnr], gkss->viewport[tnr]);
  gks_set_norm_xform(tnr, gkss->window[tnr], gkss->viewport[tnr]);
}

static void set_viewport(int tnr, double xmin, double xmax, double ymin, double ymax)
{
  gkss->viewport[tnr][0] = xmin;
  gkss->viewport[tnr][1] = xmax;
  gkss->viewport[tnr][2] = ymin;
  gkss->viewport[tnr][3] = ymax;

  set_norm_xform(tnr, gkss->window[tnr], gkss->viewport[tnr]);
  gks_set_norm_xform(tnr, gkss->window[tnr], gkss->viewport[tnr]);
  if (tnr == gkss->cntnr)
    {
      set_clip_rect(tnr);
    }
}

static void to_DC(int n, double *x, double *y)
{
  int i;
  double xn, yn;

  for (i = 0; i < n; i++)
    {
      WC_to_NDC(x[i], y[i], gkss->cntnr, xn, yn);
      seg_xform(&xn, &yn);
      NDC_to_DC(xn, yn, x[i], y[i]);
    }
}

static void draw_path(int n, double *px, double *py, int nc, int *codes)
{
  int i, j;
  double x[3], y[3], w, h, a1, a2;
  double cur_x = 0, cur_y = 0, start_x = 0, start_y = 0;

  cairo_new_path(p->cr);
  cairo_set_line_cap(p->cr, CAIRO_LINE_CAP_BUTT);
  cairo_set_line_join(p->cr, CAIRO_LINE_JOIN_ROUND);
  cairo_set_line_width(p->cr, gkss->bwidth * p->nominal_size);

  j = 0;
  for (i = 0; i < nc; ++i)
    {
      switch (codes[i])
        {
        case 'M':
        case 'm':
          x[0] = px[j];
          y[0] = py[j];
          if (codes[i] == 'm')
            {
              x[0] += cur_x;
              y[0] += cur_y;
            }
          start_x = cur_x = x[0];
          start_y = cur_y = y[0];
          to_DC(1, x, y);
          cairo_move_to(p->cr, x[0], y[0]);
          j += 1;
          break;
        case 'L':
        case 'l':
          x[0] = px[j];
          y[0] = py[j];
          if (codes[i] == 'l')
            {
              x[0] += cur_x;
              y[0] += cur_y;
            }
          cur_x = x[0];
          cur_y = y[0];
          to_DC(1, x, y);
          cairo_line_to(p->cr, x[0], y[0]);
          j += 1;
          break;
        case 'Q':
        case 'q':
          x[0] = px[j];
          y[0] = py[j];
          if (codes[i] == 'q')
            {
              x[0] += cur_x;
              y[0] += cur_y;
            }
          x[1] = px[j + 1];
          y[1] = py[j + 1];
          if (codes[i] == 'q')
            {
              x[1] += cur_x;
              y[1] += cur_y;
            }
          x[2] = cur_x;
          y[2] = cur_y;
          cur_x = x[1];
          cur_y = y[1];
          to_DC(3, x, y);
          cairo_curve_to(p->cr, 2.0 / 3.0 * x[0] + 1.0 / 3.0 * x[2], 2.0 / 3.0 * y[0] + 1.0 / 3.0 * y[2],
                         2.0 / 3.0 * x[0] + 1.0 / 3.0 * x[1], 2.0 / 3.0 * y[0] + 1.0 / 3.0 * y[1], x[1], y[1]);
          j += 2;
          break;
        case 'C':
        case 'c':
          x[0] = px[j];
          y[0] = py[j];
          if (codes[i] == 'c')
            {
              x[0] += cur_x;
              y[0] += cur_y;
            }
          x[1] = px[j + 1];
          y[1] = py[j + 1];
          if (codes[i] == 'c')
            {
              x[1] += cur_x;
              y[1] += cur_y;
            }
          x[2] = px[j + 2];
          y[2] = py[j + 2];
          if (codes[i] == 'c')
            {
              x[2] += cur_x;
              y[2] += cur_y;
            }
          cur_x = x[2];
          cur_y = y[2];
          to_DC(3, x, y);
          cairo_curve_to(p->cr, x[0], y[0], x[1], y[1], x[2], y[2]);
          j += 3;
          break;
        case 'A':
        case 'a':
          {
            double rx, ry, cx, cy;
            rx = fabs(px[j]);
            ry = fabs(py[j]);
            if (rx == 0 && ry == 0)
              {
                break;
              }
            a1 = px[j + 1];
            a2 = py[j + 1];
            cx = cur_x - rx * cos(a1);
            cy = cur_y - ry * sin(a1);
            x[0] = cx - rx;
            y[0] = cy - ry;
            x[1] = cx + rx;
            y[1] = cy + ry;
            cur_x = cx + rx * cos(a2);
            cur_y = cy + ry * sin(a2);
          }
          to_DC(2, x, y);
          w = x[1] - x[0];
          h = y[1] - y[0];

          cairo_save(p->cr);
          cairo_translate(p->cr, x[0] + 0.5 * w, y[0] + 0.5 * h);
          cairo_scale(p->cr, 1., h / w);
          if (a1 < a2)
            {
              cairo_arc(p->cr, 0., 0., w * 0.5, a1, a2);
            }
          else
            {
              cairo_arc_negative(p->cr, 0., 0., w * 0.5, a1, a2);
            }
          cairo_restore(p->cr);
          j += 3;
          break;
        case 's': /* close and stroke */
          cairo_close_path(p->cr);
          cur_x = start_x;
          cur_y = start_y;
          set_color(gkss->bcoli);
          cairo_stroke(p->cr);
          break;
        case 'S': /* stroke */
          set_color(gkss->bcoli);
          cairo_stroke(p->cr);
          break;
        case 'F': /* fill and stroke */
          cairo_close_path(p->cr);
          cur_x = start_x;
          cur_y = start_y;
          set_color(gkss->facoli);
          cairo_fill_preserve(p->cr);
          set_color(gkss->bcoli);
          cairo_stroke(p->cr);
          break;
        case 'f': /* fill */
          cairo_close_path(p->cr);
          cur_x = start_x;
          cur_y = start_y;
          set_color(gkss->facoli);
          cairo_fill(p->cr);
          break;
        case 'Z': /* close */
          cairo_close_path(p->cr);
          cur_x = start_x;
          cur_y = start_y;
          break;
        case '\0':
          break;
        default:
          gks_perror("invalid path code ('%c')", codes[i]);
          exit(1);
        }
    }
}

static void draw_lines(int n, double *px, double *py, int *attributes)
{
  int i, j = 0, rgba, line_color = MAX_COLOR;
  double x, y, xim1, yim1, xi, yi;
  double line_width;

  WC_to_NDC(px[0], py[0], gkss->cntnr, x, y);
  seg_xform(&x, &y);
  NDC_to_DC(x, y, xi, yi);

  for (i = 1; i < n; i++)
    {
      xim1 = xi;
      yim1 = yi;
      WC_to_NDC(px[i], py[i], gkss->cntnr, x, y);
      seg_xform(&x, &y);
      NDC_to_DC(x, y, xi, yi);

      cairo_set_line_cap(p->cr, CAIRO_LINE_CAP_ROUND);
      line_width = 0.01 * attributes[j++];
      cairo_set_line_width(p->cr, line_width * p->nominal_size);
      rgba = attributes[j++];
      p->rgb[line_color][0] = (rgba & 0xff) / 255.0;
      p->rgb[line_color][1] = ((rgba >> 8) & 0xff) / 255.0;
      p->rgb[line_color][2] = ((rgba >> 16) & 0xff) / 255.0;

      set_color(line_color);

      cairo_move_to(p->cr, xim1, yim1);
      cairo_line_to(p->cr, xi, yi);
      cairo_stroke(p->cr);
    }
}

static void draw_markers(int n, double *px, double *py, int *attributes)
{
  int i, j = 0, rgba;
  int mk_type, mk_color = MAX_COLOR;
  double mk_size, x, y;

  mk_type = gkss->asf[3] ? gkss->mtype : gkss->mindex;

  for (i = 0; i < n; i++)
    {
      WC_to_NDC(px[i], py[i], gkss->cntnr, x, y);
      seg_xform(&x, &y);

      mk_size = 0.01 * attributes[j++];
      rgba = attributes[j++];
      p->rgb[mk_color][0] = (rgba & 0xff) / 255.0;
      p->rgb[mk_color][1] = ((rgba >> 8) & 0xff) / 255.0;
      p->rgb[mk_color][2] = ((rgba >> 16) & 0xff) / 255.0;

      set_color(mk_color);

      draw_marker(x, y, mk_type, mk_size, mk_color);
    }
}

static void draw_triangles(int n, double *px, double *py, int ntri, int *tri)
{
  double x, y;
  int i, j, k, rgba, line_color = MAX_COLOR;
  cairo_point triangle[3];

  if (n > p->max_points)
    {
      p->points = (cairo_point *)gks_realloc(p->points, n * sizeof(cairo_point));
      p->max_points = n;
    }

  for (i = 0; i < n; ++i)
    {
      WC_to_NDC(px[i], py[i], gkss->cntnr, x, y);
      seg_xform(&x, &y);
      NDC_to_DC(x, y, p->points[i].x, p->points[i].y);
    }

  j = 0;
  for (i = 0; i < ntri / 4; ++i)
    {
      for (k = 0; k < 3; ++k)
        {
          triangle[k].x = p->points[tri[j] - 1].x;
          triangle[k].y = p->points[tri[j] - 1].y;
          j++;
        }

      cairo_set_line_cap(p->cr, CAIRO_LINE_CAP_BUTT);
      cairo_set_line_join(p->cr, CAIRO_LINE_JOIN_ROUND);
      cairo_set_line_width(p->cr, gkss->lwidth * p->nominal_size);
      rgba = tri[j++];
      p->rgb[line_color][0] = (rgba & 0xff) / 255.0;
      p->rgb[line_color][1] = ((rgba >> 8) & 0xff) / 255.0;
      p->rgb[line_color][2] = ((rgba >> 16) & 0xff) / 255.0;

      set_color(line_color);
      cairo_set_line_cap(p->cr, CAIRO_LINE_CAP_ROUND);

      cairo_move_to(p->cr, triangle[0].x, triangle[0].y);
      for (k = 1; k < 3; k++)
        {
          cairo_line_to(p->cr, triangle[k].x, triangle[k].y);
        }
      cairo_close_path(p->cr);
      cairo_stroke(p->cr);
    }
}

static void gdp(int n, double *px, double *py, int primid, int nc, int *codes)
{
  switch (primid)
    {
    case GKS_K_GDP_DRAW_PATH:
      draw_path(n, px, py, nc, codes);
      break;
    case GKS_K_GDP_DRAW_LINES:
      draw_lines(n, px, py, codes);
      break;
    case GKS_K_GDP_DRAW_MARKERS:
      draw_markers(n, px, py, codes);
      break;
    case GKS_K_GDP_DRAW_TRIANGLES:
      draw_triangles(n, px, py, nc, codes);
      break;
    default:
      gks_perror("invalid drawing primitive ('%d')", primid);
      exit(1);
    }
}

void gks_cairoplugin(int fctid, int dx, int dy, int dimx, int *ia, int lr1, double *r1, int lr2, double *r2, int lc,
                     char *chars, void **ptr)
{
  p = (ws_state_list *)*ptr;

  idle = 0;

  switch (fctid)
    {
    case 2:
      /* open workstation */
      gkss = (gks_state_list_t *)*ptr;

      gks_init_core(gkss);

      p = (ws_state_list *)gks_malloc(sizeof(ws_state_list));

      p->conid = ia[1];
      p->path = chars;
      p->wtype = ia[2];
      p->mem = NULL;

      if (p->wtype == 140 || p->wtype == 144 || p->wtype == 145 || p->wtype == 146)
        {
          p->mw = 0.28575;
          p->mh = 0.19685;
          p->w = 6750;
          p->h = 4650;
          p->dpi = 600;
          resize(2400, 2400);
          p->nominal_size = 2400 / 500.0;
        }
      else if (p->wtype == 143)
        {
          int width = 0;
          int height = 0;
          int symbols_read = 0;
          int characters_read = 0;
          void *mem_ptr = NULL;
          char *path = p->path;

          if (!path)
            {
              fprintf(stderr, "Missing mem path. Expected !<width>x<height>@<pointer>.mem\n");
              exit(1);
            }
          symbols_read = sscanf(path, "!resizable@%p.mem%n", &mem_ptr, &characters_read);
          if (symbols_read == 1 && path[characters_read] == 0 && mem_ptr != NULL)
            {
              p->mem_resizable = 1;
              width = ((int *)mem_ptr)[0];
              height = ((int *)mem_ptr)[1];
              p->dpi = ((int *)mem_ptr)[2];
              if (width <= 0 || height <= 0 || p->dpi <= 0)
                {
                  width = 2400;
                  height = 2400;
                  p->dpi = 600;
                }
            }
          else
            {
              p->mem_resizable = 0;
              symbols_read = sscanf(path, "!%dx%d@%p.mem%n", &width, &height, &mem_ptr, &characters_read);
              if (symbols_read != 3 || path[characters_read] != 0 || width <= 0 || height <= 0 || mem_ptr == NULL)
                {
                  fprintf(stderr, "Failed to parse mem path. Expected !<width>x<height>@<pointer>.mem, but found %s\n",
                          p->path);
                  exit(1);
                }
            }
          p->mem = (unsigned char *)mem_ptr;
          p->w = 6750;
          p->h = 4650;
          p->mw = p->w * 2.54 / 100 / p->dpi;
          p->mh = p->h * 2.54 / 100 / p->dpi;
          resize(width, height);
          p->nominal_size = min(width, height) / 500.0;
        }
      else if (p->wtype == 150)
        {
          p->mw = 0.20320;
          p->mh = 0.15240;
          p->w = 560;
          p->h = 420;
          p->dpi = 100;
          resize(400, 400);
          p->nominal_size = 0.8;
        }
      else
        {
          p->mw = 0.25400;
          p->mh = 0.19050;
          p->w = 1024;
          p->h = 768;
          p->dpi = 100;
          resize(500, 500);
          p->nominal_size = 1;
        }

      p->max_points = MAX_POINTS;
      p->points = (cairo_point *)gks_malloc(p->max_points * sizeof(cairo_point));
      p->npoints = 0;

      p->empty = 1;
      p->current_page_written = 1;
      p->page_counter = 0;

      p->transparency = 1.0;
      p->linewidth = p->nominal_size;

      init_colors();

      open_page();

      p->patterns = NULL;

      *ptr = p;
      break;

    case 3:
      /* close workstation */
      if ((p->wtype != 141 || !exit_due_to_x11_support_) && (p->wtype != 143 || p->mem != NULL))
        {
          if (!p->current_page_written)
            {
              if (p->empty)
                {
                  write_empty_page();
                }
              else
                {
                  write_page();
                }
            }

          close_page();
          free(p->patterns);
          free(p->points);
          free(p);
        }
      break;

    case 4:
      /* activate workstation */
      p->state = GKS_K_WS_ACTIVE;
      break;

    case 5:
      /* deactivate workstation */
      p->state = GKS_K_WS_INACTIVE;
      break;

    case 6:
      /* clear workstation */
      lock();
      cairo_save(p->cr);
      cairo_reset_clip(p->cr);
      cairo_set_operator(p->cr, CAIRO_OPERATOR_CLEAR);
      cairo_paint(p->cr);
      cairo_restore(p->cr);
      p->empty = 1;
      p->current_page_written = 0;
      unlock();
      break;

    case 8:
      /* update workstation */
      if (ia[1] & GKS_K_WRITE_PAGE_FLAG)
        {
          lock();
          if (!p->empty)
            {
              write_page();
            }
          else
            {
              write_empty_page();
            }
          unlock();
        }
      break;

    case 12:
      /* polyline */
      if (p->state == GKS_K_WS_ACTIVE)
        {
          lock();
          polyline(ia[0], r1, r2);
          p->empty = 0;
          p->current_page_written = 0;
          unlock();
        }
      break;

    case 13:
      /* polymarker */
      if (p->state == GKS_K_WS_ACTIVE)
        {
          lock();
          polymarker(ia[0], r1, r2);
          p->empty = 0;
          p->current_page_written = 0;
          unlock();
        }
      break;

    case 14:
      /* text */
      if (p->state == GKS_K_WS_ACTIVE)
        {
          lock();
          text(r1[0], r2[0], strlen(chars), chars);
          p->empty = 0;
          p->current_page_written = 0;
          unlock();
        }
      break;

    case 15:
      /* fill area */
      if (p->state == GKS_K_WS_ACTIVE)
        {
          lock();
          fillarea(ia[0], r1, r2);
          p->empty = 0;
          p->current_page_written = 0;
          unlock();
        }
      break;

    case 16:
    case DRAW_IMAGE:
      /* cell array */
      if (p->state == GKS_K_WS_ACTIVE)
        {
          int true_color = fctid == DRAW_IMAGE;

          lock();
          cellarray(r1[0], r1[1], r2[0], r2[1], dx, dy, dimx, ia, true_color);
          p->empty = 0;
          p->current_page_written = 0;
          unlock();
        }
      break;

    case 17:
      if (p->state == GKS_K_WS_ACTIVE)
        {
          gdp(ia[0], r1, r2, ia[1], ia[2], ia + 3);
          p->empty = 0;
          p->current_page_written = 0;
        }
      break;

    case 48:
      /* set color representation */
      if (p->state == GKS_K_WS_ACTIVE)
        {
          lock();
          set_color_rep(ia[1], r1[0], r1[1], r1[2]);
          unlock();
        }
      break;

    case 49:
      /* set window */
      lock();
      set_window(ia[0], r1[0], r1[1], r2[0], r2[1]);
      unlock();
      break;

    case 50:
      /* set viewport */
      lock();
      set_viewport(ia[0], r1[0], r1[1], r2[0], r2[1]);
      unlock();
      break;

    case 52:
      /* select normalization transformation */
      lock();
      select_xform(ia[0]);
      unlock();
      break;

    case 53:
      /* set clipping inidicator */
      lock();
      set_clipping(ia[0]);
      unlock();
      break;

    case 54:
      /* set workstation window */
      lock();
      p->window[0] = r1[0];
      p->window[1] = r1[1];
      p->window[2] = r2[0];
      p->window[3] = r2[1];

      set_xform();
      init_norm_xform();
      unlock();
      break;

    case 55:
      /* set workstation viewport */
      lock();
      if (p->viewport[0] != 0 || p->viewport[1] != r1[1] - r1[0] || p->viewport[2] != 0 ||
          p->viewport[3] != r2[1] - r2[0])
        {
          p->viewport[0] = 0;
          p->viewport[1] = r1[1] - r1[0];
          p->viewport[2] = 0;
          p->viewport[3] = r2[1] - r2[0];

          if (p->wtype != 143 || p->mem_resizable)
            {
              p->width = p->viewport[1] * p->w / p->mw;
              p->height = p->viewport[3] * p->h / p->mh;
              p->nominal_size = min(p->width, p->height) / 500.0;
            }
          close_page();
          open_page();

          set_xform();
          init_norm_xform();
          set_clip_rect(gkss->cntnr);
        }
      unlock();
      break;

    case 203:
      /* set transparency */
      lock();
      set_transparency(r1[0]);
      unlock();
      break;

    default:;
    }

  idle = 1;
}

#else

void gks_cairoplugin(int fctid, int dx, int dy, int dimx, int *ia, int lr1, double *r1, int lr2, double *r2, int lc,
                     char *chars, void **ptr)
{
  if (fctid == 2)
    {
      gks_perror("Cairo support not compiled in");
      ia[0] = 0;
    }
}

#endif
