#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <math.h>
#include <png.h>

#if !defined(VMS) && !defined(_WIN32)
#include <unistd.h>
#endif

#include "gks.h"
#include "gkscore.h"

#define MEMORY_INCREMENT 32768

#define MAX_POINTS 2048
#define PATTERNS 120
#define HATCH_STYLE 108

#define MWIDTH 0.254
#define MHEIGHT 0.1905
#define WIDTH 4096
#define HEIGHT 3072

#define NOMINAL_POINTSIZE 4

#define DrawBorder 0

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef MAXPATHLEN
#define MAXPATHLEN 1024
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32

#include <windows.h>
#ifndef DLLEXPORT
#define DLLEXPORT __declspec(dllexport)
#endif

#endif

DLLEXPORT void gks_svgplugin(int fctid, int dx, int dy, int dimx, int *i_arr, int len_f_arr_1, double *f_arr_1,
                             int len_f_arr_2, double *f_arr_2, int len_c_arr, char *c_arr, void **ptr);

#ifdef __cplusplus
}
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

#define CharXform(xrel, yrel, x, y)              \
  x = cos(p->phi) * (xrel)-sin(p->phi) * (yrel); \
  y = sin(p->phi) * (xrel) + cos(p->phi) * (yrel);

#define nint(a) ((int)(a + 0.5))

#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif

#ifndef max
#define max(a, b) (((a) > (b)) ? (a) : (b))
#endif

#define MAX_CLIP_RECTS 64

static gks_state_list_t *gkss;

static double a[MAX_TNR], b[MAX_TNR], c[MAX_TNR], d[MAX_TNR];

typedef struct SVG_stream_t
{
  unsigned char *buffer;
  unsigned long size, length;
} SVG_stream;

typedef struct SVG_point_t
{
  double x, y;
} SVG_point;

typedef struct SVG_clip_rect_t
{
  int x, y, width, height;
} SVG_clip_rect;

typedef struct ws_state_list_t
{
  int conid, state, wtype;
  char *path;
  double a, b, c, d;
  double window[4], viewport[4];
  unsigned char rgb[MAX_COLOR + 1][3];
  int width, height;
  int color;
  double linewidth, nominal_size;
  double phi, angle;
  int family, capheight;
  int pattern, pattern_count;
  SVG_stream *stream;
  SVG_point *points;
  int npoints, max_points;
  int empty, page_counter, offset;
  SVG_clip_rect *cr;
  int clip_index, rect_index, max_clip_rects;
  double transparency;
} ws_state_list;

static ws_state_list *p;

static const char *fonts[] = {"Times New Roman,TimesNewRoman,Times,Baskerville,Georgia,serif",
                              "Arial,Helvetica Neue,Helvetica,sans-serif",
                              "Courier,Courier New,Lucida Sans Typewriter,Lucida Typewriter,monospace",
                              "Symbol",
                              "Bookman Old Style,Bookman,Georgia,serif",
                              "Century Schoolbook,Georgia,serif",
                              "Century Gothic,CenturyGothic,AppleGothic,sans-serif",
                              "Book Antiqua,Palatino,Palatino Linotype,Palatino LT STD,Georgia,serif"};

static double capheights[29] = {0.662, 0.660, 0.681, 0.662, 0.729, 0.729, 0.729, 0.729, 0.583, 0.583,
                                0.583, 0.583, 0.667, 0.681, 0.681, 0.681, 0.681, 0.722, 0.722, 0.722,
                                0.722, 0.739, 0.739, 0.739, 0.739, 0.694, 0.693, 0.683, 0.683};

static int map[32] = {22, 9,  5, 14, 18, 26, 13, 1, 24, 11, 7, 16, 20, 28, 13, 3,
                      23, 10, 6, 15, 19, 27, 13, 2, 25, 12, 8, 17, 21, 29, 13, 4};

static double xfac[4] = {0, 0, -0.5, -1};

static double yfac[6] = {0, -1.2, -1, -0.5, 0, 0.2};

static int predef_font[] = {1, 1, 1, -2, -3, -4};

static int predef_prec[] = {0, 1, 2, 2, 2, 2};

static int predef_ints[] = {0, 1, 3, 3, 3};

static int predef_styli[] = {1, 1, 1, 2, 3};

static int path_id = -1;

static void svg_memcpy(SVG_stream *p, char *s, size_t n)
{
  if (p->length + n >= p->size)
    {
      while (p->length + n >= p->size) p->size += MEMORY_INCREMENT;
      p->buffer = (unsigned char *)realloc(p->buffer, p->size);
    }

  memmove(p->buffer + p->length, s, n);
  p->length += n;
}

static void svg_printf(SVG_stream *p, const char *args, ...)
{
  va_list ap;
  char fmt[BUFSIZ], s[BUFSIZ];

  strcpy(fmt, args);

  va_start(ap, args);
  vsprintf(s, fmt, ap);
  va_end(ap);

  svg_memcpy(p, s, strlen(s));
}

static SVG_stream *svg_alloc_stream(void)
{
  SVG_stream *p;

  p = (SVG_stream *)calloc(1, sizeof(SVG_stream));
  p->buffer = NULL;
  p->size = p->length = 0;

  return p;
}

static void set_norm_xform(int tnr, double *wn, double *vp)
{
  a[tnr] = (vp[1] - vp[0]) / (wn[1] - wn[0]);
  b[tnr] = vp[0] - wn[0] * a[tnr];
  c[tnr] = (vp[3] - vp[2]) / (wn[3] - wn[2]);
  d[tnr] = vp[2] - wn[2] * c[tnr];
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

static void set_color_rep(int color, double red, double green, double blue)
{
  if (color >= 0 && color < MAX_COLOR)
    {
      p->rgb[color][0] = (unsigned char)(red * 255);
      p->rgb[color][1] = (unsigned char)(green * 255);
      p->rgb[color][2] = (unsigned char)(blue * 255);
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

static void init_clip_rects(void)
{
  int i;
  p->clip_index = 0;
  if (path_id < 0)
    {
      srand(time(NULL));
      path_id = rand() % 100;
    }
  else
    path_id = (path_id + 1) % 100;
  for (i = 0; i < p->max_clip_rects; i++)
    {
      p->cr[i].x = p->cr[i].y = -1;
      p->cr[i].width = p->cr[i].height = 0;
    }
}

typedef struct WriteCallbackData_t
{
  png_bytep data_ptr;
  png_size_t size;
  png_size_t capacity;
} WriteCallbackData;

static WriteCallbackData current_write_data;

static void write_callback(png_structp png_ptr, png_bytep data, png_size_t num_bytes)
{
  WriteCallbackData *write_data = (WriteCallbackData *)png_get_io_ptr(png_ptr);
  png_size_t size_increment = 1000000;
  if (num_bytes > size_increment)
    {
      size_increment = num_bytes;
    }
  if (!write_data->data_ptr)
    {
      write_data->data_ptr = (png_bytep)gks_malloc(size_increment);
      write_data->size = 0;
      write_data->capacity = size_increment;
    }
  if (write_data->size + num_bytes > write_data->capacity)
    {
      write_data->data_ptr = (png_bytep)gks_realloc(write_data->data_ptr, write_data->capacity + size_increment);
      write_data->capacity += size_increment;
    }
  memcpy(write_data->data_ptr + write_data->size, data, num_bytes);
  write_data->size += num_bytes;
}

static void flush_callback(png_structp png_ptr)
{
  (void)png_ptr;
}

static void create_pattern(void)
{
  int i, j, height;
  int parray[33];
  png_byte bit_depth = 8;
  png_byte color_type = PNG_COLOR_TYPE_RGB_ALPHA;
  png_structp png_ptr;
  png_infop info_ptr;
  png_bytep *row_pointers;

  row_pointers = (png_bytep *)malloc(sizeof(png_bytep) * 8);
  for (i = 0; i < 8; i++)
    {
      row_pointers[i] = (png_byte *)malloc(sizeof(png_byte) * 4 * 8);
    }
  gks_inq_pattern_array(p->pattern, parray);
  height = (*parray == 32) ? 16 : (*parray == 4) ? 8 : *parray;
  for (j = *parray; j < height; j++)
    {
      parray[j + 1] = parray[j % *parray + 1];
    }
  for (j = 0; j < 8; ++j)
    {
      png_byte *row = row_pointers[j];
      png_byte *ptr = row;
      for (i = 0; i < 8; i++)
        {
          ptr[4 * i + 0] = parray[j + 1] & (1 << i) ? 255 : p->rgb[p->color][0];
          ptr[4 * i + 1] = parray[j + 1] & (1 << i) ? 255 : p->rgb[p->color][1];
          ptr[4 * i + 2] = parray[j + 1] & (1 << i) ? 255 : p->rgb[p->color][2];
          ptr[4 * i + 3] = 255 * p->transparency;
        }
    }
  current_write_data.data_ptr = NULL;
  current_write_data.size = 0;
  current_write_data.capacity = 0;
  png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  info_ptr = png_create_info_struct(png_ptr);
  png_set_write_fn(png_ptr, &current_write_data, write_callback, flush_callback);
  png_set_IHDR(png_ptr, info_ptr, 8, 8, bit_depth, color_type, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE,
               PNG_FILTER_TYPE_DEFAULT);
  png_write_info(png_ptr, info_ptr);
  png_write_image(png_ptr, row_pointers);
  png_write_end(png_ptr, NULL);
  for (j = 0; j < 8; ++j)
    {
      free(row_pointers[j]);
    }
  free(row_pointers);
  png_destroy_write_struct(&png_ptr, &info_ptr);
}

static void resize_window(void)
{
  p->width = nint((p->viewport[1] - p->viewport[0]) / MWIDTH * WIDTH);
  p->height = nint((p->viewport[3] - p->viewport[2]) / MHEIGHT * HEIGHT);
  p->nominal_size = min(p->width, p->height) / 500.0;
}

static void draw_marker(double xn, double yn, int mtype, double mscale, int mcolor)
{
  int r, i;
  double scale, x, y, xr, yr;
  int pc, op, color;

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

  do
    {
      op = marker[mtype][pc];
      switch (op)
        {
        case 1: /* point */
          svg_printf(p->stream,
                     "<circle clip-path=\"url(#clip%02d%d)\" "
                     "style=\"fill:#%02x%02x%02x; stroke:none; fill-opacity:%g\" "
                     "cx=\"%g\" cy=\"%g\" r=\"%d\"/>\n",
                     path_id, p->rect_index, p->rgb[mcolor][0], p->rgb[mcolor][1], p->rgb[mcolor][2], p->transparency,
                     x, y, NOMINAL_POINTSIZE / 2);
          break;

        case 2: /* line */
          for (i = 0; i < 2; i++)
            {
              xr = scale * marker[mtype][pc + 2 * i + 1];
              yr = -scale * marker[mtype][pc + 2 * i + 2];
              seg_xform_rel(&xr, &yr);
              if (i == 0)
                svg_printf(p->stream,
                           "<line clip-path=\"url(#clip%02d%d)\" "
                           "x1=\"%g\" y1=\"%g\" ",
                           path_id, p->rect_index, x - xr, y + yr);
              else
                svg_printf(p->stream,
                           "x2=\"%g\" y2=\"%g\" "
                           "style=\"stroke:#%02x%02x%02x; stroke-width:%g; stroke-opacity:%g\"/>\n",
                           x - xr, y + yr, p->rgb[mcolor][0], p->rgb[mcolor][1], p->rgb[mcolor][2], p->linewidth,
                           p->transparency);
            }
          pc += 4;
          break;

        case 3: /* polyline */
          svg_printf(p->stream,
                     "<polyline clip-path=\"url(#clip%02d%d)\" "
                     "style=\"stroke:#%02x%02x%02x; stroke-width:%g; stroke-opacity:%g; fill:none\" "
                     "points=\"\n  ",
                     path_id, p->rect_index, p->rgb[mcolor][0], p->rgb[mcolor][1], p->rgb[mcolor][2], p->linewidth,
                     p->transparency);
          for (i = 0; i < marker[mtype][pc + 1]; i++)
            {
              xr = scale * marker[mtype][pc + 2 + 2 * i];
              yr = -scale * marker[mtype][pc + 3 + 2 * i];
              seg_xform_rel(&xr, &yr);
              svg_printf(p->stream, "%g,%g ", x - xr, y + yr);
              if (!((i + 1) % 10))
                {
                  svg_printf(p->stream, "\n  ");
                }
            }
          svg_printf(p->stream, "\n  \"/>\n");
          pc += 1 + 2 * marker[mtype][pc + 1];
          break;

        case 4: /* filled polygon */
        case 5: /* hollow polygon */
          color = op == 4 ? mcolor : 0;
          svg_printf(p->stream, "<path clip-path=\"url(#clip%02d%d)\" d=\"", path_id, p->rect_index);
          for (i = 0; i < marker[mtype][pc + 1]; i++)
            {
              xr = scale * marker[mtype][pc + 2 + 2 * i];
              yr = -scale * marker[mtype][pc + 3 + 2 * i];
              seg_xform_rel(&xr, &yr);
              svg_printf(p->stream, "%c%g %g ", i == 0 ? 'M' : 'L', x - xr, y + yr);
            }
          svg_printf(p->stream, "Z\" fill=\"#%02x%02x%02x\" fill-rule=\"evenodd\" fill-opacity=\"%g\" ",
                     p->rgb[color][0], p->rgb[color][1], p->rgb[color][2], p->transparency);
          if (op == 4 && gkss->bcoli != gkss->pmcoli)
            svg_printf(p->stream, "stroke=\"#%02x%02x%02x\" stroke-opacity=\"%g\" stroke-width=\"%g\"",
                       p->rgb[gkss->bcoli][0], p->rgb[gkss->bcoli][1], p->rgb[gkss->bcoli][2], p->transparency,
                       gkss->bwidth * p->nominal_size);
          else
            svg_printf(p->stream, "stroke=\"none\"");
          svg_printf(p->stream, "/>\n");
          pc += 1 + 2 * marker[mtype][pc + 1];
          break;

        case 6: /* arc */
          svg_printf(p->stream,
                     "<circle clip-path=\"url(#clip%02d%d)\" "
                     "style=\"fill:none; stroke:#%02x%02x%02x; stroke-width:%g; stroke-opacity:%g\" "
                     "cx=\"%g\" cy=\"%g\" r=\"%d\"/>\n",
                     path_id, p->rect_index, p->rgb[mcolor][0], p->rgb[mcolor][1], p->rgb[mcolor][2], p->linewidth,
                     p->transparency, x, y, r);
          break;

        case 7: /* filled arc */
        case 8: /* hollow arc */
          color = op == 7 ? mcolor : 0;
          svg_printf(p->stream,
                     "<circle clip-path=\"url(#clip%02d%d)\" "
                     "cx=\"%g\" cy=\"%g\" r=\"%d\"",
                     path_id, p->rect_index, x, y, r);
          svg_printf(p->stream, " fill=\"#%02x%02x%02x\" fill-rule=\"evenodd\" fill-opacity=\"%g\" ", p->rgb[color][0],
                     p->rgb[color][1], p->rgb[color][2], p->transparency);
          if (op == 7 && gkss->bcoli != gkss->pmcoli)
            svg_printf(p->stream, "stroke=\"#%02x%02x%02x\" stroke-opacity=\"%g\" stroke-width=\"%g\"",
                       p->rgb[gkss->bcoli][0], p->rgb[gkss->bcoli][1], p->rgb[gkss->bcoli][2], p->transparency,
                       gkss->bwidth * p->nominal_size);
          else
            svg_printf(p->stream, "stroke=\"none\"");
          svg_printf(p->stream, "/>\n");
          break;
        }
      pc++;
    }
  while (op != 0);
}

static void marker_routine(int n, double *px, double *py, int mtype, double mscale, int mcolor)
{
  double x, y;
  double *clrt = gkss->viewport[gkss->cntnr];
  int i, draw;

  p->linewidth = p->nominal_size;

  for (i = 0; i < n; i++)
    {
      WC_to_NDC(px[i], py[i], gkss->cntnr, x, y);
      seg_xform(&x, &y);

      if (gkss->clip == GKS_K_CLIP)
        draw = (x >= clrt[0] && x <= clrt[1] && y >= clrt[2] && y <= clrt[3]);
      else
        draw = 1;

      if (draw) draw_marker(x, y, mtype, mscale, mcolor);
    }
}

static void polymarker(int n, double *px, double *py)
{
  int mk_type, mk_color;
  double mk_size;

  mk_type = gkss->asf[3] ? gkss->mtype : gkss->mindex;
  mk_size = gkss->asf[4] ? gkss->mszsc : 1;
  mk_color = gkss->asf[5] ? gkss->pmcoli : 1;

  marker_routine(n, px, py, mk_type, mk_size, mk_color);
}

static void stroke(void)
{
  int i;

  svg_printf(p->stream,
             "<polyline clip-path=\"url(#clip%02d%d)\" style=\""
             "stroke:#%02x%02x%02x; stroke-width:%g; stroke-opacity:%g; fill:none\" ",
             path_id, p->rect_index, p->rgb[p->color][0], p->rgb[p->color][1], p->rgb[p->color][2], p->linewidth,
             p->transparency);
  svg_printf(p->stream, "points=\"\n  ");
  for (i = 0; i < p->npoints; i++)
    {
      svg_printf(p->stream, "%g,%g ", p->points[i].x, p->points[i].y);
      if (!((i + 1) % 10))
        {
          svg_printf(p->stream, "\n  ");
        }
    }
  svg_printf(p->stream, "\n  \"/>\n");

  p->npoints = 0;
}

static void line_routine(int n, double *px, double *py, int linetype, int tnr)
{
  double x, y;
  int i, len;
  double x0, y0, xi, yi, xim1, yim1;
  int dash_list[10];
  char s[100], buf[20];

  WC_to_NDC(px[0], py[0], tnr, x, y);
  seg_xform(&x, &y);
  NDC_to_DC(x, y, x0, y0);

  svg_printf(p->stream,
             "<polyline clip-path=\"url(#clip%02d%d)\" style=\""
             "stroke:#%02x%02x%02x; stroke-width:%g; stroke-opacity:%g; fill:none\" ",
             path_id, p->rect_index, p->rgb[p->color][0], p->rgb[p->color][1], p->rgb[p->color][2], p->linewidth,
             p->transparency);
  if (linetype < 0 || linetype > 1)
    {
      gks_get_dash_list(linetype, 0.5 * p->linewidth, dash_list);
      len = dash_list[0];
      *s = '\0';
      for (i = 1; i <= len; i++)
        {
          sprintf(buf, "%d%s", dash_list[i], i < len ? ", " : "");
          strcat(s, buf);
        }
      svg_printf(p->stream, "stroke-dasharray=\"%s\" ", s);
    }
  svg_printf(p->stream, "points=\"\n  %g,%g ", x0, y0);

  xim1 = x0;
  yim1 = y0;
  for (i = 1; i < n; i++)
    {
      WC_to_NDC(px[i], py[i], tnr, x, y);
      seg_xform(&x, &y);
      NDC_to_DC(x, y, xi, yi);

      if (i == 1 || xi != xim1 || yi != yim1)
        {
          svg_printf(p->stream, "%g,%g ", xi, yi);
          xim1 = xi;
          yim1 = yi;
        }
      if (!((i + 1) % 10))
        {
          svg_printf(p->stream, "\n  ");
        }
    }
  if (linetype == 0) svg_printf(p->stream, "%g,%g", x0, y0);
  svg_printf(p->stream, "\n  \"/>\n");
}

static void fill_routine(int n, double *px, double *py, int tnr)
{
  int i, j, nan_found = 0;
  double x, y, ix, iy;
  char *s, line[80];
  size_t slen;

  const char *hatch_paths[] = {/* none */
                               "",
                               /* dense vertical */
                               "M0.5,-4 l0,16 M4.5,-4 l0,16",
                               /* dense horizontal */
                               "M-4,0.5 l16,0 M-4,4.5 l16,0",
                               /* dense upward diagonal */
                               "M-4,4 l8,-8 M4,12 l8,-8 M-4,12 l16,-16 M-2,14 l16,-16 M-2,6 l16,-16",
                               /* dense downward diagonal */
                               "M-4,4 l8,8 M-4,-4 l16,16 M4,-4 l8,8 M-2,2 l8,8 M-2,-6 l16,16",
                               /* dense cross */
                               "M-4,0.5 l16,0 M-4,4.5 l16,0 M0.5,-4 l0,16 M4.5,-4 l0,16",
                               /* sparse cross diagonal */
                               "M-4,4 l8,-8 M4,12 l8,-8 M-4,12 l16,-16 M-4,4 l8,8 M-4,-4 l16,16 M4,-4 l8,8",
                               /* sparse vertical */
                               "M3.5,-4 l0,16",
                               /* sparse horizontal */
                               "M-4,4.5 l16,0",
                               /* sparse upward diagonal */
                               "M-4,4 l8,-8 M4,12 l8,-8 M-4,12 l16,-16",
                               /* sparse downward diagonal */
                               "M-4,4 l8,8 M-4,-4 l16,16 M4,-4 l8,8",
                               /* sparse cross */
                               "M-4,4.5 l16,0 M3.5,-4 l0,16"};

  if (p->pattern)
    {
      p->pattern_count++;
      if (p->pattern > HATCH_STYLE && p->pattern - HATCH_STYLE < 12 && *hatch_paths[p->pattern - HATCH_STYLE])
        {
          svg_printf(p->stream,
                     "<defs>\n  <pattern id=\"pattern%d\" patternUnits=\"userSpaceOn"
                     "Use\" x=\"0\" y=\"0\" width=\"%d\" height=\"%d\">\n"
                     "<g transform=\"scale(%d)\">"
                     "<path d=\"%s\" style=\"stroke:#%02x%02x%02x; stroke-width:1; stroke-opacity:%g\"/>"
                     "</g>",
                     p->pattern_count, 8 * NOMINAL_POINTSIZE, 8 * NOMINAL_POINTSIZE, NOMINAL_POINTSIZE,
                     hatch_paths[p->pattern - HATCH_STYLE], p->rgb[p->color][0], p->rgb[p->color][1],
                     p->rgb[p->color][2], p->transparency);
          svg_printf(p->stream, "</pattern>\n</defs>\n");
        }
      else
        {
          create_pattern();
          svg_printf(p->stream,
                     "<defs>\n  <pattern id=\"pattern%d\" patternUnits=\"userSpaceOn"
                     "Use\" x=\"0\" y=\"0\" width=\"%d\" height=\"%d\">\n"
                     "<image width=\"%d\" height=\"%d\" image-rendering=\"optimizeSpeed\" "
                     "xlink:href=\"data:image/png;base64,\n",
                     p->pattern_count, 8 * NOMINAL_POINTSIZE, 8 * NOMINAL_POINTSIZE, 8 * NOMINAL_POINTSIZE,
                     8 * NOMINAL_POINTSIZE);
          slen = current_write_data.size * 4 / 3 + 4;
          s = (char *)gks_malloc(slen);
          gks_base64(current_write_data.data_ptr, current_write_data.size, s, slen);
          gks_free(current_write_data.data_ptr);
          i = j = 0;
          while (s[j])
            {
              line[i++] = s[j++];
              if (i == 76 || s[j] == '\0')
                {
                  line[i] = '\0';
                  svg_printf(p->stream, "%s\n", line);
                  i = 0;
                }
            }
          free(s);
          svg_printf(p->stream, "\"/>\n  </pattern>\n</defs>\n");
        }
    }

  svg_printf(p->stream, "<path clip-path=\"url(#clip%02d%d)\" d=\"\n", path_id, p->rect_index);
  for (i = 0; i < n; i++)
    {
      if (px[i] != px[i] && py[i] != py[i])
        {
          nan_found = 1;
          continue;
        }
      WC_to_NDC(px[i], py[i], tnr, x, y);
      seg_xform(&x, &y);
      NDC_to_DC(x, y, ix, iy);

      if (i == 0 || nan_found)
        {
          svg_printf(p->stream, "M%g %g ", ix, iy);
          nan_found = 0;
        }
      else
        {
          svg_printf(p->stream, "L%g %g ", ix, iy);
        }
      if (!((i + 1) % 10))
        {
          svg_printf(p->stream, "\n  ");
        }
    }
  if (p->pattern)
    svg_printf(p->stream, " Z\n  \" fill=\"url(#pattern%d)\"", p->pattern_count);
  else
    svg_printf(p->stream, " Z\n  \" fill=\"#%02x%02x%02x\" fill-rule=\"evenodd\" fill-opacity=\"%g\"",
               p->rgb[p->color][0], p->rgb[p->color][1], p->rgb[p->color][2], p->transparency);
  svg_printf(p->stream, "/>\n");
}

static void fillarea(int n, double *px, double *py)
{
  int fl_inter, fl_style, fl_color;

  fl_inter = gkss->asf[10] ? gkss->ints : predef_ints[gkss->findex - 1];
  fl_style = gkss->asf[11] ? gkss->styli : predef_styli[gkss->findex - 1];
  fl_color = gkss->asf[12] ? gkss->facoli : 1;

  p->pattern = 0;
  if (fl_inter == GKS_K_INTSTYLE_HOLLOW)
    {
      p->color = fl_color;
      p->linewidth = p->nominal_size;
      line_routine(n, px, py, DrawBorder, gkss->cntnr);
    }
  else if (fl_inter == GKS_K_INTSTYLE_SOLID)
    {
      p->color = fl_color;
      fill_routine(n, px, py, gkss->cntnr);
    }
  else if (fl_inter == GKS_K_INTSTYLE_PATTERN || fl_inter == GKS_K_INTSTYLE_HATCH)
    {
      p->color = fl_color;
      if (fl_inter == GKS_K_INTSTYLE_HATCH) fl_style += HATCH_STYLE;
      if (fl_style >= PATTERNS) fl_style = 1;
      p->pattern = fl_style;
      fill_routine(n, px, py, gkss->cntnr);
    }
}

static void polyline(int n, double *px, double *py)
{
  int ln_type, ln_color;
  double ln_width;

  if (n > p->max_points)
    {
      p->points = (SVG_point *)realloc(p->points, n * sizeof(SVG_point));
      p->max_points = n;
    }

  ln_type = gkss->asf[0] ? gkss->ltype : gkss->lindex;
  ln_width = gkss->asf[1] ? gkss->lwidth : 1;
  ln_color = gkss->asf[2] ? gkss->plcoli : 1;

  p->linewidth = ln_width * p->nominal_size;
  p->color = ln_color;

  line_routine(n, px, py, ln_type, gkss->cntnr);

  if (p->npoints > 0) stroke();
}

static void text_routine(double x, double y, int nchars, char *chars)
{
  double xstart, ystart;
  int width, height, ch;
  double xrel, yrel, ax, ay;
  int i;
  char utf[4];
  size_t len;

  NDC_to_DC(x, y, xstart, ystart);

  width = 0;
  height = p->capheight;

  xrel = width * xfac[gkss->txal[0]];
  yrel = height * yfac[gkss->txal[1]];
  CharXform(xrel, yrel, ax, ay);
  xstart += ax;
  ystart -= ay;

  if (gkss->txal[0] == GKS_K_TEXT_HALIGN_CENTER)
    svg_printf(p->stream, "text-anchor:middle;");
  else if (gkss->txal[0] == GKS_K_TEXT_HALIGN_RIGHT)
    svg_printf(p->stream, "text-anchor:end;");
  else
    svg_printf(p->stream, "text-anchor:start;");
  svg_printf(p->stream, "\" ");
  svg_printf(p->stream, "transform=\"rotate(%.4g", p->angle);
  svg_printf(p->stream, ", %g, %g)\" ", xstart, ystart);
  svg_printf(p->stream, "x=\"%g\" y=\"%g\"", xstart, ystart);
  svg_printf(p->stream, ">");

  for (i = 0; i < nchars; ++i)
    {
      ch = (chars[i] < 0) ? chars[i] + 256 : chars[i];
      switch (ch)
        {
        case '&':
          svg_printf(p->stream, "&amp;");
          break;
        case '<':
          svg_printf(p->stream, "&lt;");
          break;
        case '>':
          svg_printf(p->stream, "&gt;");
          break;
        case '"':
          svg_printf(p->stream, "&quot;");
          break;
        case '\'':
          svg_printf(p->stream, "&apos;");
          break;
        default:
          if (p->family == 3)
            gks_symbol2utf(ch, utf, &len);
          else
            {
              *utf = ch;
              len = 1;
            }
          utf[len] = '\0';
          svg_printf(p->stream, "%s", utf);
          break;
        }
    }
  svg_printf(p->stream, "<");
}

static void set_font(int font)
{
  double scale, ux, uy, angle;
  int size;
  double width, height, capheight;
  int bold, italic;

  font = abs(font);
  if (font >= 101 && font <= 129)
    font -= 100;
  else if (font >= 1 && font <= 32)
    font = map[font - 1];
  else
    font = 9;

  WC_to_NDC_rel(gkss->chup[0], gkss->chup[1], gkss->cntnr, ux, uy);
  seg_xform_rel(&ux, &uy);

  p->phi = -atan2(ux, uy);
  angle = p->phi * 180 / M_PI;
  if (angle < 0) angle += 360;
  p->angle = -angle;

  scale = sqrt(gkss->chup[0] * gkss->chup[0] + gkss->chup[1] * gkss->chup[1]);
  ux = gkss->chup[0] / scale * gkss->chh;
  uy = gkss->chup[1] / scale * gkss->chh;
  WC_to_NDC_rel(ux, uy, gkss->cntnr, ux, uy);

  width = 0;
  height = sqrt(ux * ux + uy * uy);
  seg_xform_rel(&width, &height);

  height = sqrt(width * width + height * height);
  capheight = nint(height * (fabs(p->c) + 1));
  p->capheight = nint(capheight);

  size = nint(capheight / capheights[font - 1]);
  if (font > 13) font += 3;
  p->family = (font - 1) / 4;
  bold = (font % 4 == 1 || font % 4 == 2) ? 0 : 1;
  italic = (font % 4 == 2 || font % 4 == 0);

  svg_printf(p->stream, "font-family:%s; font-size:%dpx; ", fonts[p->family], size);
  if (bold && italic)
    svg_printf(p->stream, "font-style:italic; font-weight:bold; ");
  else if (italic)
    svg_printf(p->stream, "font-style:italic; ");
  else if (bold)
    svg_printf(p->stream, "font-weight:bold; ");
}

static void text(double px, double py, int nchars, char *chars)
{
  int tx_font, tx_prec, tx_color;
  double x, y;

  tx_font = gkss->asf[6] ? gkss->txfont : predef_font[gkss->tindex - 1];
  tx_prec = gkss->asf[6] ? gkss->txprec : predef_prec[gkss->tindex - 1];
  tx_color = gkss->asf[9] ? gkss->txcoli : 1;

  p->color = tx_color;
  if (tx_prec == GKS_K_TEXT_PRECISION_STRING)
    {
      svg_printf(p->stream,
                 "<g clip-path=\"url(#clip%02d%d)\">\n<text style=\""
                 "fill:#%02x%02x%02x; fill-opacity:%g; ",
                 path_id, p->rect_index, p->rgb[tx_color][0], p->rgb[tx_color][1], p->rgb[tx_color][2],
                 p->transparency);
      set_font(tx_font);

      WC_to_NDC(px, py, gkss->cntnr, x, y);
      seg_xform(&x, &y);

      text_routine(x, y, nchars, chars);
      svg_printf(p->stream, "/text>\n</g>\n");
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
  double x1, y1, x2, y2;
  double ix1, ix2, iy1, iy2;
  int x, y, width, height;
  int red, green, blue, alpha;
  int i, j, ix, iy, ind, rgb;
  int swapx, swapy;
  png_byte bit_depth = 8;
  png_byte color_type = PNG_COLOR_TYPE_RGBA;
  png_structp png_ptr;
  png_infop info_ptr;
  png_bytep *row_pointers;
  char *s, line[80];
  size_t slen;

  WC_to_NDC(xmin, ymax, gkss->cntnr, x1, y1);
  seg_xform(&x1, &y1);
  NDC_to_DC(x1, y1, ix1, iy1);

  WC_to_NDC(xmax, ymin, gkss->cntnr, x2, y2);
  seg_xform(&x2, &y2);
  NDC_to_DC(x2, y2, ix2, iy2);

  width = nint(fabs(ix2 - ix1));
  height = nint(fabs(iy2 - iy1));
  if (width == 0 || height == 0) return;
  x = nint(min(ix1, ix2));
  y = nint(min(iy1, iy2));

  swapx = ix1 > ix2;
  swapy = iy1 < iy2;

  row_pointers = (png_bytep *)malloc(sizeof(png_bytep) * height);
  for (j = 0; j < height; ++j)
    {
      row_pointers[j] = (png_byte *)malloc(width * 4);
    }
  for (j = 0; j < height; j++)
    {
      png_byte *row = row_pointers[j];
      iy = dy * j / height;
      if (swapy) iy = dy - 1 - iy;
      for (i = 0; i < width; i++)
        {
          png_byte *ptr = &(row[i * 4]);
          ix = dx * i / width;
          if (swapx) ix = dx - 1 - ix;
          if (!true_color)
            {
              ind = colia[iy * dimx + ix];
              ind = FIX_COLORIND(ind);
              red = p->rgb[ind][0];
              green = p->rgb[ind][1];
              blue = p->rgb[ind][2];
              alpha = 0xff;
            }
          else
            {
              rgb = colia[iy * dimx + ix];
              red = (rgb & 0xff);
              green = (rgb & 0xff00) >> 8;
              blue = (rgb & 0xff0000) >> 16;
              alpha = (rgb & 0xff000000) >> 24;
            }
          ptr[0] = red;
          ptr[1] = green;
          ptr[2] = blue;
          ptr[3] = alpha;
        }
    }

  current_write_data.data_ptr = NULL;
  current_write_data.size = 0;
  current_write_data.capacity = 0;
  png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  info_ptr = png_create_info_struct(png_ptr);
  png_set_write_fn(png_ptr, &current_write_data, write_callback, flush_callback);
  png_set_IHDR(png_ptr, info_ptr, width, height, bit_depth, color_type, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE,
               PNG_FILTER_TYPE_DEFAULT);
  png_write_info(png_ptr, info_ptr);
  png_write_image(png_ptr, row_pointers);
  png_write_end(png_ptr, NULL);
  for (j = 0; j < height; ++j)
    {
      free(row_pointers[j]);
    }
  free(row_pointers);
  png_destroy_write_struct(&png_ptr, &info_ptr);
  slen = current_write_data.size * 4 / 3 + 4;
  s = (char *)gks_malloc(slen);
  gks_base64(current_write_data.data_ptr, current_write_data.size, s, slen);
  gks_free(current_write_data.data_ptr);
  svg_printf(p->stream,
             "<g clip-path=\"url(#clip%02d%d)\">\n"
             "<image width=\"%d\" height=\"%d\" "
             "xlink:href=\"data:image/png;base64,\n",
             path_id, p->rect_index, width, height);
  i = j = 0;
  while (s[j])
    {
      line[i++] = s[j++];
      if (i == 76 || s[j] == '\0')
        {
          line[i] = '\0';
          svg_printf(p->stream, "%s\n", line);
          i = 0;
        }
    }
  svg_printf(p->stream, "\" transform=\"translate(%d, %d)\"/>\n</g>\n", x, y);
  free(s);
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
  int large_arc_flag, sweep_flag;
  unsigned int in_path = 0;

  p->color = gkss->facoli;

  j = 0;
  for (i = 0; i < nc; ++i)
    {
      if (!in_path)
        {
          if (i != 0)
            svg_printf(p->stream, "<path clip-path=\"url(#clip%02d%d)\" d=\"M%g %g", path_id, p->rect_index, start_x,
                       start_y);
          else
            svg_printf(p->stream, "<path clip-path=\"url(#clip%02d%d)\" d=\"", path_id, p->rect_index);

          in_path = 1;
        }
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
          svg_printf(p->stream, "M%g %g ", x[0], y[0]);
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
          svg_printf(p->stream, "L%g %g ", x[0], y[0]);
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
          cur_x = x[1];
          cur_y = y[1];
          to_DC(2, x, y);
          svg_printf(p->stream, "Q%g %g %g %g ", x[0], y[0], x[1], y[1]);
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
          svg_printf(p->stream, "C%g %g %g %g %g %g ", x[0], y[0], x[1], y[1], x[2], y[2]);
          j += 3;
          break;
        case 'A':
        case 'a':
          {
            double rx, ry, cx, cy;
            rx = fabs(px[j]);
            ry = fabs(py[j]);
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
            x[2] = cur_x;
            y[2] = cur_y;
          }
          to_DC(3, x, y);

          w = (x[1] - x[0]) * 0.5;
          h = (y[1] - y[0]) * 0.5;

          sweep_flag = a1 > a2;
          while (fabs(a2 - a1) >= 2 * M_PI)
            {
              if (sweep_flag)
                {
                  a1 -= M_PI;
                }
              else
                {
                  a1 += M_PI;
                }
              double mx = x[0] + w + cos(a1) * w;
              double my = y[0] + h + sin(a1) * h;
              svg_printf(p->stream, "A%g %g 0 1 %d %g %g ", w, h, sweep_flag, mx, my);
            }
          large_arc_flag = fabs(a2 - a1) >= M_PI;
          svg_printf(p->stream, "A%g %g 0 %d %d %g %g ", w, h, large_arc_flag, sweep_flag, x[2], y[2]);
          j += 3;
          break;
        case 'S': /* stroke */
          svg_printf(p->stream,
                     "\" fill=\"none\" stroke=\"#%02x%02x%02x\" stroke-opacity=\"%g\" stroke-width=\"%g\" />",
                     p->rgb[gkss->bcoli][0], p->rgb[gkss->bcoli][1], p->rgb[gkss->bcoli][2], p->transparency,
                     gkss->bwidth * p->nominal_size);
          in_path = 0;
          break;
        case 's': /* close and stroke */
          svg_printf(p->stream,
                     "Z\" fill=\"none\" stroke=\"#%02x%02x%02x\" stroke-opacity=\"%g\" stroke-width=\"%g\" />",
                     p->rgb[gkss->bcoli][0], p->rgb[gkss->bcoli][1], p->rgb[gkss->bcoli][2], p->transparency,
                     gkss->bwidth * p->nominal_size);
          cur_x = start_x;
          cur_y = start_y;
          in_path = 0;
          break;
        case 'f': /* fill */
          svg_printf(p->stream, "Z\" fill=\"#%02x%02x%02x\" fill-rule=\"evenodd\" fill-opacity=\"%g\" />",
                     p->rgb[p->color][0], p->rgb[p->color][1], p->rgb[p->color][2], p->transparency);
          cur_x = start_x;
          cur_y = start_y;
          in_path = 0;
          break;
        case 'F': /* fill and stroke */
          svg_printf(p->stream,
                     "Z\" fill=\"#%02x%02x%02x\" fill-rule=\"evenodd\" fill-opacity=\"%g\" stroke=\"#%02x%02x%02x\" "
                     "stroke-opacity=\"%g\" stroke-width=\"%g\" />",
                     p->rgb[p->color][0], p->rgb[p->color][1], p->rgb[p->color][2], p->transparency,
                     p->rgb[gkss->bcoli][0], p->rgb[gkss->bcoli][1], p->rgb[gkss->bcoli][2], p->transparency,
                     gkss->bwidth * p->nominal_size);
          cur_x = start_x;
          cur_y = start_y;
          in_path = 0;
          break;
        case 'Z': /* close */
          svg_printf(p->stream, "Z ");
          cur_x = start_x;
          cur_y = start_y;
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
  int i, j = 0, rgba, red, green, blue;
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

      line_width = 0.01 * attributes[j++];
      rgba = attributes[j++];
      red = rgba & 0xff;
      green = (rgba >> 8) & 0xff;
      blue = (rgba >> 16) & 0xff;

      svg_printf(p->stream, "<path clip-path=\"url(#clip%02d%d)\" d=\"", path_id, p->rect_index);
      svg_printf(p->stream, "M%g %g L%g %g ", xim1, yim1, xi, yi);
      svg_printf(p->stream,
                 "\" fill=\"none\" stroke=\"#%02x%02x%02x\" stroke-opacity=\"%g\" stroke-width=\"%g\" "
                 "stroke-linecap=\"round\" stroke-linejoin=\"miter\" />",
                 red, green, blue, p->transparency, line_width * p->nominal_size);
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
      p->rgb[mk_color][0] = rgba & 0xff;
      p->rgb[mk_color][1] = (rgba >> 8) & 0xff;
      p->rgb[mk_color][2] = (rgba >> 16) & 0xff;

      draw_marker(x, y, mk_type, mk_size, mk_color);
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
    default:
      gks_perror("invalid drawing primitive ('%d')", primid);
      exit(1);
    }
}

static void set_clip_path(int tnr)
{
  double *vp;
  int x, y, width, height;
  double cxl, cxr, cyb, cyt;
  int i, found = 0, index;

  if (gkss->clip_tnr != 0)
    vp = gkss->viewport[gkss->clip_tnr];
  else if (gkss->clip == GKS_K_CLIP)
    vp = gkss->viewport[tnr];
  else
    vp = gkss->viewport[0];

  NDC_to_DC(vp[0], vp[2], cxl, cyb);
  NDC_to_DC(vp[1], vp[3], cxr, cyt);

  x = (int)cxl;
  y = (int)cyt;
  width = nint(cxr - cxl);
  height = nint(cyb - cyt);

  x = max(0, x);
  width = min(p->width, width + 1);
  y = max(0, y);
  height = min(p->height, height + 1);

  for (i = 0; i < p->clip_index && !found; i++)
    {
      if (x == p->cr[i].x && y == p->cr[i].y && width == p->cr[i].width && height == p->cr[i].height)
        {
          found = 1;
          index = i;
        }
    }
  if (found)
    {
      p->rect_index = index;
    }
  else
    {
      p->cr[p->clip_index].x = x;
      p->cr[p->clip_index].y = y;
      p->cr[p->clip_index].width = width;
      p->cr[p->clip_index].height = height;
      p->rect_index = p->clip_index;
      svg_printf(p->stream,
                 "<defs>\n  <clipPath id=\"clip%02d%d\">\n    <rect"
                 " x=\"%d\" y=\"%d\" width=\"%d\" height=\"%d\"/>\n  </clip"
                 "Path>\n</defs>\n",
                 path_id, p->rect_index, x, y, width, height);
      p->clip_index++;
      if (p->clip_index == MAX_CLIP_RECTS)
        {
          p->max_clip_rects += MAX_CLIP_RECTS;
          p->cr = (SVG_clip_rect *)gks_realloc(p->cr, p->max_clip_rects * sizeof(SVG_clip_rect));
        }
    }
}

static void write_page(void)
{
  char path[MAXPATHLEN], buf[256];
  int fd;

  p->page_counter++;

  if (p->conid == 0)
    {
      gks_filepath(path, p->path, "svg", p->page_counter, 0);
      fd = gks_open_file(path, "w");
    }
  else
    fd = p->conid;

  if (fd >= 0)
    {
      sprintf(buf,
              "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
              "<svg xmlns=\"http://www.w3.org/2000/svg\" "
              "xmlns:xlink=\"http://www.w3.org/1999/xlink\" "
              "width=\"%g\" height=\"%g\" viewBox=\"0 0 %d %d\">\n",
              p->width / 4.0, p->height / 4.0, p->width, p->height);
      gks_write_file(fd, buf, strlen(buf));
      gks_write_file(fd, p->stream->buffer, p->stream->length);
      sprintf(buf, "</svg>\n");
      gks_write_file(fd, buf, strlen(buf));
      if (fd != p->conid) gks_close_file(fd);

      p->stream->length = 0;
    }
  else
    {
      gks_perror("can't open SVG file");
      perror("open");
    }
}

#ifndef EMSCRIPTEN
void gks_svgplugin(
#else
void gks_drv_js(
#endif
    int fctid, int dx, int dy, int dimx, int *ia, int lr1, double *r1, int lr2, double *r2, int lc, char *chars,
    void **ptr)
{
  p = (ws_state_list *)*ptr;

  switch (fctid)
    {
      /* open workstation */
    case 2:
      gkss = (gks_state_list_t *)*ptr;

      gks_init_core(gkss);

      p = (ws_state_list *)calloc(1, sizeof(ws_state_list));

      p->conid = ia[1];
      p->path = chars;

      p->height = 2000;
      p->width = 2000;
      p->window[0] = p->window[2] = 0.0;
      p->window[1] = p->window[3] = 1.0;
      p->viewport[0] = p->viewport[2] = 0;
      p->viewport[1] = (double)p->width * MWIDTH / WIDTH;
      p->viewport[3] = (double)p->height * MHEIGHT / HEIGHT;
      p->nominal_size = 2000 / 500.0;

      p->stream = svg_alloc_stream();

      p->max_points = MAX_POINTS;
      p->points = (SVG_point *)gks_malloc(p->max_points * sizeof(SVG_point));
      p->npoints = 0;

      p->empty = 1;
      p->page_counter = 0;
      p->offset = 0;

      p->linewidth = p->nominal_size;
      p->transparency = 1.0;

      set_xform();
      init_norm_xform();
      init_colors();

      p->max_clip_rects = MAX_CLIP_RECTS;
      p->cr = (SVG_clip_rect *)gks_malloc(p->max_clip_rects * sizeof(SVG_clip_rect));

      init_clip_rects();
      set_clip_path(0);

      p->pattern_count = 0;

      *ptr = p;
      break;

      /* close workstation */
    case 3:
      if (!p->empty) write_page();

      free(p->cr);
      free(p->stream->buffer);
      free(p->stream);
      free(p->points);
      free(p);
      break;

      /* activate workstation */
    case 4:
      p->state = GKS_K_WS_ACTIVE;
      break;

      /* deactivate workstation */
    case 5:
      p->state = GKS_K_WS_INACTIVE;
      break;

      /* clear workstation */
    case 6:
      p->stream->length = 0;
      p->empty = 1;
      init_clip_rects();
      break;

      /* update workstation */
    case 8:
      if (ia[1] & GKS_K_WRITE_PAGE_FLAG)
        {
          if (!p->empty)
            {
              p->empty = 1;
              write_page();

              init_clip_rects();
            }
        }
      break;

      /* polyline */
    case 12:
      if (p->state == GKS_K_WS_ACTIVE)
        {
          polyline(ia[0], r1, r2);
          p->empty = 0;
        }
      break;

      /* polymarker */
    case 13:
      if (p->state == GKS_K_WS_ACTIVE)
        {
          polymarker(ia[0], r1, r2);
          p->empty = 0;
        }
      break;

      /* text */
    case 14:
      if (p->state == GKS_K_WS_ACTIVE)
        {
          text(r1[0], r2[0], strlen(chars), chars);
          p->empty = 0;
        }
      break;

      /* fill area */
    case 15:
      if (p->state == GKS_K_WS_ACTIVE)
        {
          fillarea(ia[0], r1, r2);
          p->empty = 0;
        }
      break;

      /* cell array */
    case 16:
    case DRAW_IMAGE:
      if (p->state == GKS_K_WS_ACTIVE)
        {
          int true_color = fctid == DRAW_IMAGE;

          cellarray(r1[0], r1[1], r2[0], r2[1], dx, dy, dimx, ia, true_color);
          p->empty = 0;
        }
      break;

      /* GDP */
    case 17:
      if (p->state == GKS_K_WS_ACTIVE)
        {
          gdp(ia[0], r1, r2, ia[1], ia[2], ia + 3);
          p->empty = 0;
        }
      break;

      /* set color representation */
    case 48:
      set_color_rep(ia[1], r1[0], r1[1], r1[2]);
      break;

    case 49:
      /* set window */
      set_norm_xform(*ia, gkss->window[*ia], gkss->viewport[*ia]);
      break;

    case 50:
      /* set viewport */
      set_norm_xform(*ia, gkss->window[*ia], gkss->viewport[*ia]);
      if (*ia == gkss->cntnr) set_clip_path(*ia);
      break;

    case 52:
      /* select normalization transformation */
    case 53:
      /* set clipping inidicator */
      set_clip_path(gkss->cntnr);
      break;

      /* set workstation window */
    case 54:
      p->window[0] = r1[0];
      p->window[1] = r1[1];
      p->window[2] = r2[0];
      p->window[3] = r2[1];

      set_xform();
      init_norm_xform();
      break;

      /* set workstation viewport */
    case 55:
      p->viewport[0] = r1[0];
      p->viewport[1] = r1[1];
      p->viewport[2] = r2[0];
      p->viewport[3] = r2[1];

      resize_window();
      set_xform();
      init_norm_xform();
      break;

    case 203:
      /* set transparency */
      p->transparency = r1[0];
      break;

    default:;
    }
}
