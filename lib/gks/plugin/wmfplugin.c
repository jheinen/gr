#ifdef __unix__
#define _POSIX_C_SOURCE 1
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>

#include "gks.h"
#include "gkscore.h"

#define MEMORY_INCREMENT 32768

#define MAX_POINTS 2048
#define PATTERNS 120
#define HATCH_STYLE 108

#define MWIDTH 0.254
#define MHEIGHT 0.1905
#define WIDTH 1024
#define HEIGHT 768

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

DLLEXPORT void gks_wmfplugin(int fctid, int dx, int dy, int dimx, int *i_arr, int len_f_arr_1, double *f_arr_1,
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
  xd = (int)(p->a * (xn) + p->b); \
  yd = (int)(p->c * (yn) + p->d)

#define CharXform(xrel, yrel, x, y)                  \
  x = cos(p->alpha) * (xrel)-sin(p->alpha) * (yrel); \
  y = sin(p->alpha) * (xrel) + cos(p->alpha) * (yrel);

#define nint(a) ((int)(a + 0.5))

#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif

#define WMF_STRETCHDIB 0x0f43
#define WMF_EXTTEXTOUT 0x0a32
#define WMF_POLYGON 0x0324
#define WMF_POLYLINE 0x0325
#define WMF_ELLIPSE 0x0418
#define WMF_SELECTCLIPREGION 0x012c
#define WMF_CREATEFONTINDIRECT 0x02fb
#define WMF_TEXTCOLOR 0x0209
#define WMF_TEXTALIGN 0x012e
#define WMF_DELETEOBJECT 0x01f0
#define WMF_CREATEBRUSHINDIRECT 0x02fc
#define WMF_SELECTOBJECT 0x012d
#define WMF_CREATEPENINDIRECT 0x02fa
#define WMF_SETMAPMODE 0x0103
#define WMF_SETWINDOWORG 0x020b
#define WMF_SETWINDOWEXT 0x020c
#define WMF_SETBKMODE 0x0102
#define WMF_INTERSECTRECTCLIP 0x0416
#define WMF_DIBCREATEPATTERNBRUSH 0x0142
#define WMF_EOF 0x0003

#define SIZE_BYTE 1
#define SIZE_SHORT 2
#define SIZE_LONG 4

static gks_state_list_t *gkss;

static double a[MAX_TNR], b[MAX_TNR], c[MAX_TNR], d[MAX_TNR];

typedef struct WMF_stream_t
{
  unsigned char *buffer;
  unsigned long size, length;
} WMF_stream;

typedef struct WMF_point_t
{
  int x, y;
} WMF_point;

typedef struct ws_state_list_t
{
  int conid, state, wtype;
  char *path;
  double a, b, c, d;
  double window[4], viewport[4];
  char rgb[MAX_COLOR][7];
  int red[MAX_COLOR];
  int green[MAX_COLOR];
  int blue[MAX_COLOR];
  int width, height;
  int color, linewidth;
  double alpha;
  int capheight, angle;
  int pattern;
  WMF_stream *stream;
  WMF_point *points;
  int npoints, max_points;
  int empty, page_counter, offset;
  int cxl[MAX_TNR], cxr[MAX_TNR], cyb[MAX_TNR], cyt[MAX_TNR];
  int clip_index, path_index, path_counter;
  int maxrecord;
} ws_state_list;

static ws_state_list *p;

static const char *fonts[] = {"Times New Roman",    "Arial",          "Courier New", "Symbol", "Bookman Old Style",
                              "Century Schoolbook", "Century Gothic", "Book Antiqua"};

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

static void wmf_memcpy(WMF_stream *p, int src, size_t n)
{
  unsigned char s[4];

  if (p->length + n >= p->size)
    {
      while (p->length + n >= p->size) p->size += MEMORY_INCREMENT;
      p->buffer = (unsigned char *)realloc(p->buffer, p->size);
    }

  switch (n)
    {
    case SIZE_BYTE:
      s[0] = src & 0xff;
      break;
    case SIZE_SHORT:
      s[0] = src & 0xff;
      s[1] = (src >> 8) & 0xff;
      break;
    case SIZE_LONG:
      s[0] = src & 0xff;
      s[1] = (src >> 8) & 0xff;
      s[2] = (src >> 16) & 0xff;
      s[3] = (src >> 24) & 0xff;
      break;
    }

  memmove(p->buffer + p->length, s, n);
  p->length += n;
}

static WMF_stream *wmf_alloc_stream(void)
{
  WMF_stream *p;

  p = (WMF_stream *)calloc(1, sizeof(WMF_stream));
  p->buffer = NULL;
  p->size = p->length = 0;

  return p;
}

static void wmf_setmapmode(void)
{
  wmf_memcpy(p->stream, 4, SIZE_LONG);
  wmf_memcpy(p->stream, WMF_SETMAPMODE, SIZE_SHORT);
  wmf_memcpy(p->stream, 8, SIZE_SHORT);
  if (p->maxrecord < 4)
    {
      p->maxrecord = 4;
    }
}

static void wmf_setwindoworg(void)
{
  wmf_memcpy(p->stream, 5, SIZE_LONG);
  wmf_memcpy(p->stream, WMF_SETWINDOWORG, SIZE_SHORT);
  wmf_memcpy(p->stream, 0, SIZE_SHORT);
  wmf_memcpy(p->stream, 0, SIZE_SHORT);
  if (p->maxrecord < 5)
    {
      p->maxrecord = 5;
    }
}

static void wmf_setwindowext(void)
{
  wmf_memcpy(p->stream, 5, SIZE_LONG);
  wmf_memcpy(p->stream, WMF_SETWINDOWEXT, SIZE_SHORT);
  wmf_memcpy(p->stream, p->height, SIZE_SHORT);
  wmf_memcpy(p->stream, p->width, SIZE_SHORT);
  if (p->maxrecord < 5)
    {
      p->maxrecord = 5;
    }
}

static void wmf_setbkmode(void)
{
  wmf_memcpy(p->stream, 4, SIZE_LONG);
  wmf_memcpy(p->stream, WMF_SETBKMODE, SIZE_SHORT);
  wmf_memcpy(p->stream, 1, SIZE_SHORT);
  if (p->maxrecord < 4)
    {
      p->maxrecord = 4;
    }
}

static void wmf_createpenindirect(int linetype, int size, int r, int g, int b)
{
  wmf_memcpy(p->stream, 8, SIZE_LONG);
  wmf_memcpy(p->stream, WMF_CREATEPENINDIRECT, SIZE_SHORT);
  wmf_memcpy(p->stream, linetype, SIZE_SHORT);
  wmf_memcpy(p->stream, size, SIZE_SHORT);
  wmf_memcpy(p->stream, 0, SIZE_SHORT);
  wmf_memcpy(p->stream, g * 256 + r, SIZE_SHORT);
  wmf_memcpy(p->stream, b, SIZE_SHORT);
  if (p->maxrecord < 8)
    {
      p->maxrecord = 8;
    }
}

static void wmf_selectobject(int value)
{
  wmf_memcpy(p->stream, 4, SIZE_LONG);
  wmf_memcpy(p->stream, WMF_SELECTOBJECT, SIZE_SHORT);
  wmf_memcpy(p->stream, value, SIZE_SHORT);
  if (p->maxrecord < 4)
    {
      p->maxrecord = 4;
    }
}

static void wmf_selectclipregion(void)
{
  wmf_memcpy(p->stream, 4, SIZE_LONG);
  wmf_memcpy(p->stream, WMF_SELECTCLIPREGION, SIZE_SHORT);
  wmf_memcpy(p->stream, 0, SIZE_SHORT);
  if (p->maxrecord < 4)
    {
      p->maxrecord = 4;
    }
}

static void wmf_createbrushindirect(int style, int r, int g, int b)
{
  wmf_memcpy(p->stream, 7, SIZE_LONG);
  wmf_memcpy(p->stream, WMF_CREATEBRUSHINDIRECT, SIZE_SHORT);
  wmf_memcpy(p->stream, style, SIZE_SHORT);
  wmf_memcpy(p->stream, g * 256 + r, SIZE_SHORT);
  wmf_memcpy(p->stream, b, SIZE_SHORT);
  wmf_memcpy(p->stream, 0, SIZE_SHORT);
  if (p->maxrecord < 7)
    {
      p->maxrecord = 7;
    }
}

static void wmf_dibcreatepatternbrush(int r, int g, int b, int nr)
{
  int parray[33];
  int height, i, j;

  gks_inq_pattern_array(nr, parray);
  height = (*parray == 32) ? 16 : (*parray == 4) ? 8 : *parray;
  for (j = *parray; j < height; j++)
    {
      parray[j + 1] = parray[j % *parray + 1];
    }

  wmf_memcpy(p->stream, 29 + 2 * height, SIZE_LONG);
  wmf_memcpy(p->stream, WMF_DIBCREATEPATTERNBRUSH, SIZE_SHORT);
  wmf_memcpy(p->stream, 5, SIZE_LONG);
  wmf_memcpy(p->stream, 40, SIZE_LONG);
  wmf_memcpy(p->stream, height, SIZE_LONG);
  wmf_memcpy(p->stream, height, SIZE_LONG);
  wmf_memcpy(p->stream, 1, SIZE_SHORT);
  wmf_memcpy(p->stream, 1, SIZE_SHORT);
  wmf_memcpy(p->stream, 0, SIZE_LONG);
  wmf_memcpy(p->stream, 32, SIZE_LONG);
  wmf_memcpy(p->stream, 0, SIZE_LONG);
  wmf_memcpy(p->stream, 0, SIZE_LONG);
  wmf_memcpy(p->stream, 0, SIZE_LONG);
  wmf_memcpy(p->stream, 0, SIZE_LONG);
  wmf_memcpy(p->stream, b, SIZE_BYTE);
  wmf_memcpy(p->stream, g, SIZE_BYTE);
  wmf_memcpy(p->stream, r, SIZE_BYTE);
  wmf_memcpy(p->stream, 0, SIZE_BYTE);
  wmf_memcpy(p->stream, 255, SIZE_BYTE);
  wmf_memcpy(p->stream, 255, SIZE_BYTE);
  wmf_memcpy(p->stream, 255, SIZE_BYTE);
  wmf_memcpy(p->stream, 0, SIZE_BYTE);
  for (i = 0; i < height; ++i)
    {
      wmf_memcpy(p->stream, parray[i + 1], SIZE_LONG);
    }
  if (p->maxrecord < 29 + 2 * height)
    {
      p->maxrecord = 29 + 2 * height;
    }
}

static void wmf_deleteobject(int value)
{
  wmf_memcpy(p->stream, 4, SIZE_LONG);
  wmf_memcpy(p->stream, WMF_DELETEOBJECT, SIZE_SHORT);
  wmf_memcpy(p->stream, value, SIZE_SHORT);
  if (p->maxrecord < 4)
    {
      p->maxrecord = 4;
    }
}

static void wmf_textalign(int value)
{
  wmf_memcpy(p->stream, 4, SIZE_LONG);
  wmf_memcpy(p->stream, WMF_TEXTALIGN, SIZE_SHORT);
  wmf_memcpy(p->stream, value, SIZE_SHORT);
  if (p->maxrecord < 4)
    {
      p->maxrecord = 4;
    }
}

static void wmf_textcolor(int cindex)
{
  wmf_memcpy(p->stream, 5, SIZE_LONG);
  wmf_memcpy(p->stream, WMF_TEXTCOLOR, SIZE_SHORT);
  wmf_memcpy(p->stream, p->green[cindex] * 256 + p->red[cindex], SIZE_SHORT);
  wmf_memcpy(p->stream, p->blue[cindex], SIZE_SHORT);
  if (p->maxrecord < 5)
    {
      p->maxrecord = 5;
    }
}

static void wmf_createfontindirect(int fontname, int italic, int bold, int height, double angle)
{
  int len, ch, i, nchars;

  nchars = strlen(fonts[fontname]) + 1;

  len = nchars / 2 + nchars % 2;
  wmf_memcpy(p->stream, len + 12, SIZE_LONG);
  wmf_memcpy(p->stream, WMF_CREATEFONTINDIRECT, SIZE_SHORT);
  wmf_memcpy(p->stream, height, SIZE_SHORT);
  wmf_memcpy(p->stream, 0, SIZE_SHORT);
  wmf_memcpy(p->stream, (int)(angle * 10), SIZE_SHORT);
  wmf_memcpy(p->stream, (int)(angle * 10), SIZE_SHORT);
  wmf_memcpy(p->stream, bold ? 700 : 400, SIZE_SHORT);
  wmf_memcpy(p->stream, italic, SIZE_SHORT);
  wmf_memcpy(p->stream, 0, SIZE_SHORT);
  wmf_memcpy(p->stream, 0, SIZE_SHORT);
  wmf_memcpy(p->stream, 0, SIZE_SHORT);
  for (i = 0; i < nchars; ++i)
    {
      ch = fonts[fontname][i];
      if (ch < 0) ch += 256;
      wmf_memcpy(p->stream, ch, SIZE_BYTE);
    }
  if (nchars % 2)
    {
      wmf_memcpy(p->stream, 0, SIZE_BYTE);
    }

  if (p->maxrecord < len + 12)
    {
      p->maxrecord = len + 12;
    }
}

static void wmf_intersectrectclip(int xmin, int ymin, int xmax, int ymax)
{
  wmf_memcpy(p->stream, 7, SIZE_LONG);
  wmf_memcpy(p->stream, WMF_INTERSECTRECTCLIP, SIZE_SHORT);
  wmf_memcpy(p->stream, ymin, SIZE_SHORT);
  wmf_memcpy(p->stream, xmin, SIZE_SHORT);
  wmf_memcpy(p->stream, ymax, SIZE_SHORT);
  wmf_memcpy(p->stream, xmax, SIZE_SHORT);
  if (p->maxrecord < 7)
    {
      p->maxrecord = 7;
    }
}

static void wmf_exttextout(int xstart, int ystart, char *chars, int nchars)
{
  int ch, size, i;

  size = nchars / 2 + nchars % 2;
  wmf_memcpy(p->stream, size + 7, SIZE_LONG);
  wmf_memcpy(p->stream, WMF_EXTTEXTOUT, SIZE_SHORT);
  wmf_memcpy(p->stream, ystart, SIZE_SHORT);
  wmf_memcpy(p->stream, xstart, SIZE_SHORT);
  wmf_memcpy(p->stream, nchars, SIZE_LONG);

  for (i = 0; i < nchars; ++i)
    {
      ch = chars[i];
      if (ch < 0) ch += 256;
      wmf_memcpy(p->stream, ch, SIZE_BYTE);
    }
  if (nchars % 2)
    {
      wmf_memcpy(p->stream, 0, SIZE_BYTE);
    }
  if (p->maxrecord < size + 7)
    {
      p->maxrecord = size + 7;
    }
}

static void wmf_trailer(void)
{
  unsigned char s[4];
  int src;

  wmf_memcpy(p->stream, WMF_EOF, SIZE_SHORT);
  wmf_memcpy(p->stream, 0, SIZE_LONG);

  src = p->stream->length / 2;
  s[0] = src & 0xff;
  s[1] = (src >> 8) & 0xff;
  s[2] = (src >> 16) & 0xff;
  s[3] = (src >> 24) & 0xff;
  memmove(p->stream->buffer + 28, s, SIZE_LONG);

  src = p->maxrecord;
  s[0] = src & 0xff;
  s[1] = (src >> 8) & 0xff;
  s[2] = (src >> 16) & 0xff;
  s[3] = (src >> 24) & 0xff;
  memmove(p->stream->buffer + 34, s, SIZE_LONG);

  p->maxrecord = 0;
}

static void wmf_header(void)
{
  short inch = 75;
  short cksum = 0x9ac6 ^ 0xcdd7 ^ p->width ^ p->height ^ inch;

  wmf_memcpy(p->stream, (int)0x9ac6cdd7, SIZE_LONG); /* magic number */
  wmf_memcpy(p->stream, 0, SIZE_SHORT);              /* (reserved) */
  wmf_memcpy(p->stream, 0, SIZE_SHORT);              /* left */
  wmf_memcpy(p->stream, 0, SIZE_SHORT);              /* top */
  wmf_memcpy(p->stream, p->width, SIZE_SHORT);       /* right */
  wmf_memcpy(p->stream, p->height, SIZE_SHORT);      /* bottom */
  wmf_memcpy(p->stream, inch, SIZE_SHORT);           /* inch */
  wmf_memcpy(p->stream, 0, SIZE_LONG);               /* reserved */
  wmf_memcpy(p->stream, cksum, SIZE_SHORT);          /* ckecksum */

  wmf_memcpy(p->stream, 1, SIZE_SHORT);     /* 0|1 disk|metafile */
  wmf_memcpy(p->stream, 9, SIZE_SHORT);     /* headersize */
  wmf_memcpy(p->stream, 0x300, SIZE_SHORT); /* version 0300 */
  wmf_memcpy(p->stream, 0, SIZE_LONG);      /* filesize */
  wmf_memcpy(p->stream, 4, SIZE_SHORT);     /* # of objects */
  wmf_memcpy(p->stream, 5, SIZE_LONG);      /* maxrecord */
  wmf_memcpy(p->stream, 0, SIZE_SHORT);     /* (always 0) */

  wmf_setmapmode();
  wmf_setwindoworg();
  wmf_setwindowext();
  wmf_setbkmode();
  wmf_createpenindirect(0, 0, 0, 0, 0);
  wmf_selectobject(0);
  wmf_createbrushindirect(0, 255, 255, 0);
  wmf_selectobject(1);
  wmf_createfontindirect(0, 0, 500, 10, 0);
  wmf_selectobject(2);
  wmf_dibcreatepatternbrush(0, 0, 0, 0);
  wmf_selectobject(3);
}

static void set_norm_xform(int tnr, double *wn, double *vp)
{
  a[tnr] = (vp[1] - vp[0]) / (wn[1] - wn[0]);
  b[tnr] = vp[0] - wn[0] * a[tnr];
  c[tnr] = (vp[3] - vp[2]) / (wn[3] - wn[2]);
  d[tnr] = vp[2] - wn[2] * c[tnr];

  NDC_to_DC(vp[0], vp[3], p->cxl[tnr], p->cyt[tnr]);
  NDC_to_DC(vp[1], vp[2], p->cxr[tnr], p->cyb[tnr]);
  p->cxr[tnr] += 1;
  p->cyb[tnr] += 1;
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
      p->red[color] = (int)(red * 255);
      p->green[color] = (int)(green * 255);
      p->blue[color] = (int)(blue * 255);
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

static void resize_window(void)
{
  p->width = nint((p->viewport[1] - p->viewport[0]) / MWIDTH * WIDTH);
  p->height = nint((p->viewport[3] - p->viewport[2]) / MHEIGHT * HEIGHT);
}

static void draw_marker(double xn, double yn, int mtype, double mscale, int mcolor)
{
  int r, x, y, i;
  double scale, xr, yr;
  int pc, op;

#include "marker.h"

  r = (int)(3 * mscale);
  scale = 0.01 * mscale / 3.0;

  xr = r;
  yr = 0;
  seg_xform_rel(&xr, &yr);
  r = nint(sqrt(xr * xr + yr * yr));

  NDC_to_DC(xn, yn, x, y);

  pc = 0;
  mtype = (r > 0) ? mtype + marker_off : marker_off + 1;

  wmf_selectobject(0);
  wmf_deleteobject(0);
  wmf_createpenindirect(0, 0, p->red[mcolor], p->green[mcolor], p->blue[mcolor]);
  wmf_selectobject(0);
  do
    {
      op = marker[mtype][pc];
      switch (op)
        {
        case 1: /* point */
          wmf_memcpy(p->stream, 2 * 2 + 4, SIZE_LONG);
          wmf_memcpy(p->stream, WMF_POLYLINE, SIZE_SHORT);
          wmf_memcpy(p->stream, 2, SIZE_SHORT);
          wmf_memcpy(p->stream, x, SIZE_SHORT);
          wmf_memcpy(p->stream, y, SIZE_SHORT);
          wmf_memcpy(p->stream, x, SIZE_SHORT);
          wmf_memcpy(p->stream, y, SIZE_SHORT);
          if (p->maxrecord < 8)
            {
              p->maxrecord = 8;
            }
          break;

        case 2: /* line */
          wmf_memcpy(p->stream, 2 * 2 + 4, SIZE_LONG);
          wmf_memcpy(p->stream, WMF_POLYLINE, SIZE_SHORT);
          wmf_memcpy(p->stream, 2, SIZE_SHORT);
          for (i = 0; i < 2; i++)
            {
              xr = scale * marker[mtype][pc + 2 * i + 1];
              yr = -scale * marker[mtype][pc + 2 * i + 2];
              seg_xform_rel(&xr, &yr);
              if (i == 0)
                {
                  wmf_memcpy(p->stream, (int)(x - xr), SIZE_SHORT);
                  wmf_memcpy(p->stream, (int)(y - yr), SIZE_SHORT);
                }
              else
                {
                  wmf_memcpy(p->stream, (int)(x - xr), SIZE_SHORT);
                  wmf_memcpy(p->stream, (int)(y + yr), SIZE_SHORT);
                }
            }
          if (p->maxrecord < 8)
            {
              p->maxrecord = 8;
            }
          pc += 4;
          break;

        case 3: /* polyline */
          wmf_memcpy(p->stream, 2 * marker[mtype][pc + 1] + 4, SIZE_LONG);
          wmf_memcpy(p->stream, WMF_POLYLINE, SIZE_SHORT);
          wmf_memcpy(p->stream, marker[mtype][pc + 1], SIZE_SHORT);
          for (i = 0; i < marker[mtype][pc + 1]; i++)
            {
              xr = scale * marker[mtype][pc + 2 + 2 * i];
              yr = -scale * marker[mtype][pc + 3 + 2 * i];
              seg_xform_rel(&xr, &yr);
              wmf_memcpy(p->stream, (int)(x - xr), SIZE_SHORT);
              wmf_memcpy(p->stream, (int)(y + yr), SIZE_SHORT);
            }
          if (p->maxrecord < 2 * marker[mtype][pc + 1] + 4)
            {
              p->maxrecord = 2 * marker[mtype][pc + 1] + 4;
            }
          pc += 1 + 2 * marker[mtype][pc + 1];
          break;

        case 4: /* filled polygon */
        case 5: /* hollow polygon */
          if (op == 5)
            {
              wmf_selectobject(1);
              wmf_deleteobject(1);
              wmf_createbrushindirect(0, p->red[0], p->green[0], p->blue[0]);
              wmf_selectobject(1);
            }
          else
            {
              wmf_selectobject(1);
              wmf_deleteobject(1);
              wmf_createbrushindirect(0, p->red[mcolor], p->green[mcolor], p->blue[mcolor]);
              wmf_selectobject(1);
            }
          wmf_memcpy(p->stream, 2 * marker[mtype][pc + 1] + 4, SIZE_LONG);
          wmf_memcpy(p->stream, WMF_POLYGON, SIZE_SHORT);
          wmf_memcpy(p->stream, marker[mtype][pc + 1], SIZE_SHORT);
          for (i = 0; i < marker[mtype][pc + 1]; i++)
            {
              xr = scale * marker[mtype][pc + 2 + 2 * i];
              yr = -scale * marker[mtype][pc + 3 + 2 * i];
              seg_xform_rel(&xr, &yr);
              wmf_memcpy(p->stream, (int)(x - xr), SIZE_SHORT);
              wmf_memcpy(p->stream, (int)(y + yr), SIZE_SHORT);
            }
          if (p->maxrecord < 2 * marker[mtype][pc + 1] + 4)
            {
              p->maxrecord = 2 * marker[mtype][pc + 1] + 4;
            }
          pc += 1 + 2 * marker[mtype][pc + 1];
          if (op == 5) p->color = mcolor;
          break;

        case 6: /* arc */
          wmf_selectobject(1);
          wmf_deleteobject(1);
          wmf_createbrushindirect(0, p->red[mcolor], p->green[mcolor], p->blue[mcolor]);
          wmf_selectobject(1);
          wmf_memcpy(p->stream, 7, SIZE_LONG);
          wmf_memcpy(p->stream, WMF_ELLIPSE, SIZE_SHORT);
          wmf_memcpy(p->stream, y - r, SIZE_SHORT);
          wmf_memcpy(p->stream, x - r, SIZE_SHORT);
          wmf_memcpy(p->stream, y + r, SIZE_SHORT);
          wmf_memcpy(p->stream, x + r, SIZE_SHORT);
          if (p->maxrecord < 7)
            {
              p->maxrecord = 7;
            }
          break;

        case 7: /* filled arc */
        case 8: /* hollow arc */
          if (op == 8)
            {
              wmf_selectobject(1);
              wmf_deleteobject(1);
              wmf_createbrushindirect(0, p->red[0], p->green[0], p->blue[0]);
              wmf_selectobject(1);
            }
          else
            {
              wmf_selectobject(1);
              wmf_deleteobject(1);
              wmf_createbrushindirect(0, p->red[mcolor], p->green[mcolor], p->blue[mcolor]);
              wmf_selectobject(1);
            }
          wmf_memcpy(p->stream, 7, SIZE_LONG);
          wmf_memcpy(p->stream, WMF_ELLIPSE, SIZE_SHORT);
          wmf_memcpy(p->stream, y - r, SIZE_SHORT);
          wmf_memcpy(p->stream, x - r, SIZE_SHORT);
          wmf_memcpy(p->stream, y + r, SIZE_SHORT);
          wmf_memcpy(p->stream, x + r, SIZE_SHORT);
          if (p->maxrecord < 7)
            {
              p->maxrecord = 7;
            }
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

  wmf_selectobject(0);
  wmf_deleteobject(0);
  wmf_createpenindirect(0, p->linewidth, p->red[p->color], p->green[p->color], p->blue[p->color]);
  wmf_selectobject(0);

  wmf_memcpy(p->stream, 2 * p->npoints + 4, SIZE_LONG);
  wmf_memcpy(p->stream, WMF_POLYLINE, SIZE_SHORT);
  wmf_memcpy(p->stream, p->npoints, SIZE_SHORT);
  for (i = 0; i < p->npoints; i++)
    {
      wmf_memcpy(p->stream, p->points[i].x, SIZE_SHORT);
      wmf_memcpy(p->stream, p->points[i].y, SIZE_SHORT);
    }
  if (p->maxrecord < 2 * p->npoints + 4)
    {
      p->maxrecord = 2 * p->npoints + 4;
    }
  p->npoints = 0;
}

static void move_to(double x, double y)
{
  if (p->npoints > 0) stroke();

  NDC_to_DC(x, y, p->points[p->npoints].x, p->points[p->npoints].y);
  p->npoints++;
}

static void line_to(double x, double y)
{
  NDC_to_DC(x, y, p->points[p->npoints].x, p->points[p->npoints].y);
  p->npoints++;
}

static void move(double x, double y)
{
  gks_move(x, y, move_to);
}

static void draw(double x, double y)
{
  gks_dash(x, y, move_to, line_to);
}

static void line_routine(int n, double *px, double *py, int linetype, int tnr)
{
  double x, y;
  int i, x0, y0, xi, yi, xim1, yim1, len;

  WC_to_NDC(px[0], py[0], tnr, x, y);
  seg_xform(&x, &y);
  NDC_to_DC(x, y, x0, y0);

  wmf_selectobject(0);
  wmf_deleteobject(0);
  wmf_createpenindirect(0, 0, p->red[p->color], p->green[p->color], p->blue[p->color]);
  wmf_selectobject(0);
  len = 1;
  xim1 = x0;
  yim1 = y0;
  for (i = 1; i < n; i++)
    {
      WC_to_NDC(px[i], py[i], tnr, x, y);
      seg_xform(&x, &y);
      NDC_to_DC(x, y, xi, yi);

      if (i == 1 || xi != xim1 || yi != yim1)
        {
          xim1 = xi;
          yim1 = yi;
          len += 1;
        }
    }
  if (linetype == 0)
    {
      len = len + 1;
    }

  wmf_memcpy(p->stream, 2 * len + 4, SIZE_LONG);
  wmf_memcpy(p->stream, WMF_POLYLINE, SIZE_SHORT);
  wmf_memcpy(p->stream, len, SIZE_SHORT);

  wmf_memcpy(p->stream, x0, SIZE_SHORT);
  wmf_memcpy(p->stream, y0, SIZE_SHORT);
  xim1 = x0;
  yim1 = y0;
  for (i = 1; i < n; i++)
    {
      WC_to_NDC(px[i], py[i], tnr, x, y);
      seg_xform(&x, &y);
      NDC_to_DC(x, y, xi, yi);

      if (i == 1 || xi != xim1 || yi != yim1)
        {
          wmf_memcpy(p->stream, xi, SIZE_SHORT);
          wmf_memcpy(p->stream, yi, SIZE_SHORT);
          xim1 = xi;
          yim1 = yi;
        }
    }
  if (linetype == 0)
    {
      wmf_memcpy(p->stream, x0, SIZE_SHORT);
      wmf_memcpy(p->stream, y0, SIZE_SHORT);
    }

  if (p->maxrecord < 2 * n + 4)
    {
      p->maxrecord = 2 * n + 4;
    }
}

static void fill_routine(int n, double *px, double *py, int tnr)
{
  int i;
  double x, y;
  int ix, iy;

  if (p->pattern)
    {
      wmf_selectobject(0);
      wmf_deleteobject(0);
      wmf_createpenindirect(0, 0, 255, 255, 255);
      wmf_selectobject(0);
      wmf_selectobject(3);

      wmf_deleteobject(3);
      wmf_dibcreatepatternbrush(p->red[p->color], p->green[p->color], p->blue[p->color], p->pattern);
      wmf_selectobject(3);
    }
  else
    {
      wmf_selectobject(0);
      wmf_deleteobject(0);
      wmf_createpenindirect(0, 0, p->red[p->color], p->green[p->color], p->blue[p->color]);
      wmf_selectobject(0);
      wmf_selectobject(1);
      wmf_deleteobject(1);
      wmf_createbrushindirect(0, p->red[p->color], p->green[p->color], p->blue[p->color]);
      wmf_selectobject(1);
    }
  wmf_memcpy(p->stream, 2 * n + 4, SIZE_LONG);
  wmf_memcpy(p->stream, WMF_POLYGON, SIZE_SHORT);
  wmf_memcpy(p->stream, n, SIZE_SHORT);
  for (i = 0; i < n; i++)
    {
      WC_to_NDC(px[i], py[i], tnr, x, y);
      seg_xform(&x, &y);
      NDC_to_DC(x, y, ix, iy);
      wmf_memcpy(p->stream, ix, SIZE_SHORT);
      wmf_memcpy(p->stream, iy, SIZE_SHORT);
    }
  if (p->maxrecord < 2 * n + 4)
    {
      p->maxrecord = 2 * n + 4;
    }
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
      p->points = (WMF_point *)realloc(p->points, n * sizeof(WMF_point));
      p->max_points = n;
    }

  ln_type = gkss->asf[0] ? gkss->ltype : gkss->lindex;
  ln_width = gkss->asf[1] ? gkss->lwidth : 1;
  ln_color = gkss->asf[2] ? gkss->plcoli : 1;

  p->linewidth = nint(ln_width);
  p->color = ln_color;

  gks_set_dev_xform(gkss, p->window, p->viewport);
  gks_emul_polyline(n, px, py, ln_type, gkss->cntnr, move, draw);

  if (p->npoints > 0) stroke();
}

static void text_routine(double x, double y, int nchars, char *chars)
{
  int xstart, ystart, width, height;
  double xrel, yrel, ax, ay;

  NDC_to_DC(x, y, xstart, ystart);

  width = 0;
  height = p->capheight;

  xrel = width * xfac[gkss->txal[0]];
  yrel = height * yfac[gkss->txal[1]];
  CharXform(xrel, yrel, ax, ay);
  xstart += (int)ax;
  ystart -= (int)ay;

  if (gkss->txal[0] == GKS_K_TEXT_HALIGN_CENTER)
    wmf_textalign(30);
  else if (gkss->txal[0] == GKS_K_TEXT_HALIGN_RIGHT)
    wmf_textalign(26);
  else
    wmf_textalign(24);

  wmf_exttextout(xstart, ystart, chars, nchars);
}

static void set_font(int font)
{
  double scale, ux, uy;
  int family, size;
  double width, height, capheight;
  int bold, italic;
  double ang;

  font = abs(font);
  if (font >= 101 && font <= 129)
    font -= 100;
  else if (font >= 1 && font <= 32)
    font = map[font - 1];
  else
    font = 9;

  WC_to_NDC_rel(gkss->chup[0], gkss->chup[1], gkss->cntnr, ux, uy);
  seg_xform_rel(&ux, &uy);

  p->alpha = -atan2(ux, uy);
  if (p->alpha < 0)
    {
      ang = (int)(p->alpha * 180 / M_PI - 0.5);
    }
  else
    {
      ang = (int)(p->alpha * 180 / M_PI + 0.5);
    }

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

  size = -nint(capheight / capheights[font - 1]);
  if (font > 13) font += 3;
  family = (font - 1) / 4;
  bold = (font % 4 == 1 || font % 4 == 2) ? 0 : 1;
  italic = (font % 4 == 2 || font % 4 == 0);

  wmf_selectobject(2);
  wmf_deleteobject(2);
  wmf_createfontindirect(family, italic, bold, size, ang);
  wmf_selectobject(2);
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
      wmf_textcolor(tx_color);
      set_font(tx_font);

      WC_to_NDC(px, py, gkss->cntnr, x, y);
      seg_xform(&x, &y);

      text_routine(x, y, nchars, chars);
    }
  else
    {
      gks_emul_text(px, py, nchars, chars, line_routine, fill_routine);
    }
}

static void cellarray(double xmin, double xmax, double ymin, double ymax, int dx, int dy, int dimx, int *colia)
{
  double x1, y1, x2, y2;
  int ix1, ix2, iy1, iy2;
  int x, y, width, height;
  int i, j, ix, iy, ind;
  int swapx, swapy, fill;

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

  fill = (4 - (width * 3) % 4) % 4;
  wmf_memcpy(p->stream, 34 + (3 * width * height + fill * height) / 2, 4);
  wmf_memcpy(p->stream, WMF_STRETCHDIB, SIZE_SHORT);
  wmf_memcpy(p->stream, 2, SIZE_SHORT);
  wmf_memcpy(p->stream, 204, SIZE_LONG);
  wmf_memcpy(p->stream, height, SIZE_SHORT);
  wmf_memcpy(p->stream, width, SIZE_SHORT);
  wmf_memcpy(p->stream, 0, SIZE_SHORT);
  wmf_memcpy(p->stream, 0, SIZE_SHORT);
  wmf_memcpy(p->stream, height, SIZE_SHORT);
  wmf_memcpy(p->stream, width, SIZE_SHORT);
  wmf_memcpy(p->stream, y, SIZE_SHORT);
  wmf_memcpy(p->stream, x, SIZE_SHORT);
  wmf_memcpy(p->stream, 40, SIZE_LONG);
  wmf_memcpy(p->stream, width, SIZE_LONG);
  wmf_memcpy(p->stream, height, SIZE_LONG);
  wmf_memcpy(p->stream, 1, SIZE_SHORT);
  wmf_memcpy(p->stream, 24, SIZE_SHORT);
  wmf_memcpy(p->stream, 0, SIZE_SHORT);
  wmf_memcpy(p->stream, 0, SIZE_SHORT);
  wmf_memcpy(p->stream, 3 * width * height + fill * height, SIZE_SHORT);
  wmf_memcpy(p->stream, 0, SIZE_SHORT);
  wmf_memcpy(p->stream, 0x0b12, SIZE_LONG);
  wmf_memcpy(p->stream, 0x0b12, SIZE_LONG);
  wmf_memcpy(p->stream, 0, SIZE_LONG);
  wmf_memcpy(p->stream, 0, SIZE_LONG);

  for (j = height - 1; j >= 0; j--)
    {
      iy = dy * j / height;
      if (swapy) iy = dy - 1 - iy;
      for (i = 0; i < width; i++)
        {
          ix = dx * i / width;
          if (swapx) ix = dx - 1 - ix;
          ind = colia[iy * dimx + ix];
          ind = FIX_COLORIND(ind);
          wmf_memcpy(p->stream, p->blue[ind], SIZE_BYTE);
          wmf_memcpy(p->stream, p->green[ind], SIZE_BYTE);
          wmf_memcpy(p->stream, p->red[ind], SIZE_BYTE);
        }
      for (i = 0; i < fill; i++)
        {
          wmf_memcpy(p->stream, 0, SIZE_BYTE);
        }
    }
  if (p->maxrecord < 34 + (3 * width * height + fill * height) / 2)
    {
      p->maxrecord = 34 + (3 * width * height + fill * height) / 2;
    }
}

static void set_clip_path(int tnr)
{
  int x, y, width, height;

  wmf_selectclipregion();
  if (gkss->clip_tnr != 0 || gkss->clip == GKS_K_CLIP)
    {
      if (gkss->clip_tnr != 0)
        {
          tnr = gkss->clip_tnr;
        }
      x = p->cxl[tnr];
      y = p->cyt[tnr];
      width = p->cxr[tnr] - p->cxl[tnr];
      height = p->cyb[tnr] - p->cyt[tnr];
    }
  else
    {
      x = p->cxl[0];
      y = p->cyt[0];
      width = p->cxr[0] - p->cxl[0];
      height = p->cyb[0] - p->cyt[0];
    }
  wmf_intersectrectclip(x, y, x + width, y + height);
}

static void write_page(void)
{
  int should_close = 0;
  char path[MAXPATHLEN];
  FILE *stream;

  p->page_counter++;

  if (p->conid == 0)
    {
      gks_filepath(path, p->path, "wmf", p->page_counter, 0);
      stream = fopen(path, "wb");
      should_close = 1;
    }
  else
    stream = fdopen(p->conid, "wb");

  if (stream != NULL)
    {
      fwrite(p->stream->buffer, p->stream->length, 1, stream);
      fflush(stream);
      if (should_close)
        {
          fclose(stream);
        }
      p->stream->length = 0;
    }
  else
    {
      gks_perror("can't open WMF file");
      perror("open");
    }
  p->stream->length = 0;
}

void gks_wmfplugin(int fctid, int dx, int dy, int dimx, int *ia, int lr1, double *r1, int lr2, double *r2, int lc,
                   char *chars, void **ptr)
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

      p->height = 500;
      p->width = 500;
      p->window[0] = p->window[2] = 0.0;
      p->window[1] = p->window[3] = 1.0;
      p->viewport[0] = p->viewport[2] = 0;
      p->viewport[1] = (double)p->width * MWIDTH / WIDTH;
      p->viewport[3] = (double)p->height * MHEIGHT / HEIGHT;

      p->stream = wmf_alloc_stream();
      p->max_points = MAX_POINTS;
      p->points = (WMF_point *)gks_malloc(p->max_points * sizeof(WMF_point));
      p->npoints = 0;
      p->empty = 1;
      p->page_counter = 0;
      p->offset = 0;
      p->maxrecord = 0;
      set_xform();
      init_norm_xform();
      init_colors();

      *ptr = p;
      wmf_header();
      break;

      /* close workstation */
    case 3:
      wmf_trailer();

      if (!p->empty) write_page();

      free(p->stream->buffer);
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
      p->empty = 1;
      p->stream->length = 0;
      wmf_header();
      break;

      /* update workstation */
    case 8:
      if (ia[1] & GKS_K_WRITE_PAGE_FLAG)
        {
          if (!p->empty)
            {
              wmf_trailer();
              p->empty = 1;

              write_page();

              wmf_header();
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
      if (p->state == GKS_K_WS_ACTIVE)
        {
          cellarray(r1[0], r1[1], r2[0], r2[1], dx, dy, dimx, ia);
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
      set_clip_path(gkss->cntnr);
      break;

    default:;
    }
}
