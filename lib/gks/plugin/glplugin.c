
#ifndef NO_GLFW

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <GLFW/glfw3.h>

#ifndef __APPLE__
#include <GL/glext.h>
#ifdef _WIN32
#include <windows.h>
PFNGLLOADTRANSPOSEMATRIXDPROC glLoadTransposeMatrixd;
#else
#include <GL/glx.h>
#endif
PFNGLBUFFERDATAPROC glBufferData;
PFNGLBINDBUFFERPROC glBindBuffer;
PFNGLGENBUFFERSPROC glGenBuffers;
#endif

#endif

#include "gks.h"
#include "gkscore.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
#include <windows.h>
#ifndef DLLEXPORT
#define DLLEXPORT __declspec(dllexport)
#endif
#endif

DLLEXPORT void gks_glplugin(int fctid, int dx, int dy, int dimx, int *i_arr, int len_f_arr_1, double *f_arr_1,
                            int len_f_arr_2, double *f_arr_2, int len_c_arr, char *c_arr, void **ptr);

#ifdef __cplusplus
}
#endif

#ifndef NO_GLFW

#define MAX_POINTS 2048
#define MAX_SELECTIONS 100
#define PATTERNS 120
#define HATCH_STYLE 108

#define RESOLVE(arg, type, nbytes) \
  arg = (type *)(s + sp);          \
  sp += nbytes

#ifndef M_PI
#define M_PI 3.14159265358979323846
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
  yd = (int)(p->c * (yn) + p->d);

#define DC_to_NDC(xd, yd, xn, yn) \
  xn = ((xd)-p->b) / p->a;        \
  yn = ((yd)-p->d) / p->c;

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

#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif

static gks_state_list_t *gkss;

static double a[MAX_TNR], b[MAX_TNR], c[MAX_TNR], d[MAX_TNR];

typedef struct ws_state_list_t
{
  int state;
  gks_display_list_t dl;
  GLFWwindow *win;
  int existing_context_used;
  double mwidth, mheight;
  int swidth, sheight;
  int width, height;
  double a, b, c, d;
  double window[4], viewport[4];
  float rgb[MAX_COLOR][3];
  float transparency;
  int rect[MAX_TNR][4];
} ws_state_list;

static ws_state_list *p;

#ifndef NO_FT
static int predef_prec[] = {0, 1, 2, 2, 2, 2};
#endif

static int predef_ints[] = {0, 1, 3, 3, 3};

static int predef_styli[] = {1, 1, 1, 2, 3};

static void resize_window(void)
{
  p->width = nint((p->viewport[1] - p->viewport[0]) / p->mwidth * p->swidth);
  p->height = nint((p->viewport[3] - p->viewport[2]) / p->mheight * p->sheight);
  glfwSetWindowSize(p->win, p->width, p->height);
  glViewport(0, 0, p->width, p->height);
}

static void set_norm_xform(int tnr, double *wn, double *vp)
{
  int xp1, yp1, xp2, yp2;

  a[tnr] = (vp[1] - vp[0]) / (wn[1] - wn[0]);
  b[tnr] = vp[0] - wn[0] * a[tnr];
  c[tnr] = (vp[3] - vp[2]) / (wn[3] - wn[2]);
  d[tnr] = vp[2] - wn[2] * c[tnr];

  NDC_to_DC(vp[0], vp[3], xp1, yp1);
  NDC_to_DC(vp[1], vp[2], xp2, yp2);

  p->rect[tnr][0] = min(xp1, xp2);
  p->rect[tnr][1] = min(yp1, yp2);
  p->rect[tnr][2] = abs(xp1 - xp2) + 1;
  p->rect[tnr][3] = abs(yp1 - yp2) + 1;
}

static void init_norm_xform(void)
{
  int tnr;

  for (tnr = 0; tnr < MAX_TNR; tnr++) set_norm_xform(tnr, gkss->window[tnr], gkss->viewport[tnr]);
}

static void set_xform(void)
{
  p->a = (p->width - 1) / (p->window[1] - p->window[0]);
  p->b = -p->window[0] * p->a;
  p->c = (p->height - 1) / (p->window[2] - p->window[3]);
  p->d = p->height - 1 - p->window[2] * p->c;
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

static void set_clip_rect(int tnr)
{
  int *clrt;

  if (gkss->clip_tnr != 0)
    clrt = p->rect[gkss->clip_tnr];
  else if (gkss->clip == GKS_K_CLIP)
    clrt = p->rect[tnr];
  else
    clrt = p->rect[0];

  glScissor(clrt[0], clrt[1], clrt[2], clrt[3]);
  glEnable(GL_SCISSOR_TEST);
}

static void set_color_rep(int color, double red, double green, double blue)
{
  if (color >= 0 && color < MAX_COLOR)
    {
      p->rgb[color][0] = (float)red;
      p->rgb[color][1] = (float)green;
      p->rgb[color][2] = (float)blue;
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
  float rgba[4];

  memmove(rgba, p->rgb[index], 3 * sizeof(float));
  rgba[3] = p->transparency;

  glColor4fv(rgba);
}

static void gl_init(void)
{
#ifndef __APPLE__
#define _P (const GLubyte *)
#ifdef _WIN32
  glLoadTransposeMatrixd = (PFNGLLOADTRANSPOSEMATRIXDPROC)wglGetProcAddress(_P "glLoadTransposeMatrixd");
  glBufferData = (PFNGLBUFFERDATAPROC)wglGetProcAddress(_P "glBufferData");
  glBindBuffer = (PFNGLBINDBUFFERPROC)wglGetProcAddress(_P "glBindBuffer");
  glGenBuffers = (PFNGLGENBUFFERSPROC)wglGetProcAddress(_P "glGenBuffers");
#else
  glBufferData = (PFNGLBUFFERDATAPROC)glXGetProcAddress(_P "glBufferData");
  glBindBuffer = (PFNGLBINDBUFFERPROC)glXGetProcAddress(_P "glBindBuffer");
  glGenBuffers = (PFNGLGENBUFFERSPROC)glXGetProcAddress(_P "glGenBuffers");
#endif
#undef _P
#endif
}

static void error_callback(int error, const char *description)
{
  fprintf(stderr, "GKS GL: %s\n", description);
}

static void open_window(void)
{
  GLFWmonitor *monitor;
  int width, height;
  const GLFWvidmode *vidmode;

  glfwSetErrorCallback(error_callback);
  glfwInit();

  p->win = glfwGetCurrentContext();
  if (p->win)
    {
      p->existing_context_used = 1;
    }
  else
    {
      p->existing_context_used = 0;
      glfwDefaultWindowHints();
      glfwWindowHint(GLFW_RED_BITS, 8);
      glfwWindowHint(GLFW_GREEN_BITS, 8);
      glfwWindowHint(GLFW_BLUE_BITS, 8);
      glfwWindowHint(GLFW_ALPHA_BITS, 0);
      glfwWindowHint(GLFW_DEPTH_BITS, 0);
      glfwWindowHint(GLFW_STENCIL_BITS, 0);
      p->win = glfwCreateWindow(p->width, p->height, "GKS GL", NULL, NULL);
      glfwMakeContextCurrent(p->win);
    }
  monitor = glfwGetPrimaryMonitor();
  glfwGetMonitorPhysicalSize(monitor, &width, &height);
  p->mwidth = 0.001 * width;
  p->mheight = 0.001 * height;

  vidmode = glfwGetVideoMode(monitor);
  p->swidth = vidmode->width;
  p->sheight = vidmode->height;

  gl_init();
  glClearColor(1, 1, 1, 1);
  glClear(GL_COLOR_BUFFER_BIT);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  set_color(1);
}

static void close_window(void)
{
  if (!p->existing_context_used)
    {
      glfwDestroyWindow(p->win);
      glfwTerminate();
    }
}

static void update(void)
{
  if (!glfwWindowShouldClose(p->win))
    {
      glfwSwapBuffers(p->win);
      glfwPollEvents();
    }
  else
    {
      close_window();
      exit(0);
    }
}

static void line_routine(int num_points, double *x, double *y, int linetype, int tnr)
{
  int i;
  double xn, yn, xd, yd;
  const double modelview_matrix[16] = {2.0 / p->width, 0, 0, -1, 0, -2.0 / p->height, 0, 1, 0, 0, 1, 0, 0, 0, 0, 1};

  glMatrixMode(GL_MODELVIEW);
  glLoadTransposeMatrixd(modelview_matrix);
  glBegin(GL_LINE_STRIP);
  for (i = 0; i < num_points; ++i)
    {
      WC_to_NDC(x[i], y[i], gkss->cntnr, xn, yn);
      seg_xform(&xn, &yn);
      NDC_to_DC(xn, yn, xd, yd);
      glVertex2d(xd, yd);
    }
  glEnd();
  glLoadIdentity();
}

static void polyline(int num_points, double *x, double *y)
{
  static GLushort pattern[13] = {0x0111, 0x0041, 0x5555, 0x7F7F, 0x3CFF, 0x0FFF, 0x249F,
                                 0x111F, 0xFFFF, 0xFFFF, 0x03FF, 0x1111, 0x087F};
  static GLint factor[13] = {1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1};
  int ln_type, ln_color;
  double ln_width;

  ln_type = gkss->asf[0] ? gkss->ltype : gkss->lindex;
  ln_width = gkss->asf[1] ? gkss->lwidth : 1;
  ln_color = gkss->asf[2] ? gkss->plcoli : 1;

  ln_width = max(1, nint(ln_width));

  glLineWidth(ln_width);
  glLineStipple(nint(ln_width * factor[ln_type + 8]), pattern[ln_type + 8]);
  glEnable(GL_LINE_STIPPLE);
  set_color(ln_color);

  line_routine(num_points, x, y, ln_type, gkss->cntnr);

  set_color(1);
  glDisable(GL_LINE_STIPPLE);
  glLineWidth(1.0);
}

static void draw_marker(double xn, double yn, int mtype, double mscale, int mcolor)
{

#include "marker.h"

  static int is_concav[37] = {0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 1, 0,
                              0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

  const double modelview_matrix[16] = {2.0 / p->width, 0, 0, -1, 0, -2.0 / p->height, 0, 1, 0, 0, 1, 0, 0, 0, 0, 1};

  int r, i, num_segments;
  int pc, op;
  double scale, x, y, xr, yr, c, s, tmp;

  r = (int)(3 * mscale);
  scale = 0.01 * mscale / 3.0;

  xr = r;
  yr = 0;
  seg_xform_rel(&xr, &yr);
  r = nint(sqrt(xr * xr + yr * yr));

  NDC_to_DC(xn, yn, x, y);

  pc = 0;
  mtype = (2 * r > 1) ? mtype + marker_off : marker_off + 1;

  glMatrixMode(GL_MODELVIEW);
  glLoadTransposeMatrixd(modelview_matrix);
  set_color(mcolor);

  do
    {
      op = marker[mtype][pc];
      switch (op)
        {
        case 1: /* point */
          glBegin(GL_POINTS);
          glVertex2d(x, y);
          glEnd();
          break;

        case 2: /* line */
          glBegin(GL_LINES);
          for (i = 0; i < 2; i++)
            {
              xr = scale * marker[mtype][pc + 2 * i + 1];
              yr = -scale * marker[mtype][pc + 2 * i + 2];
              seg_xform_rel(&xr, &yr);
              glVertex2d(x - xr, y + yr);
            }
          glEnd();
          pc += 4;
          break;

        case 3: /* polygon */
        case 4: /* filled polygon */
        case 5: /* hollow polygon */
          if (op == 4)
            {
              glBegin(GL_TRIANGLE_FAN);
            }
          else if (op == 5)
            {
              set_color(0);
              glBegin(GL_TRIANGLE_FAN);
            }
          else
            {
              glBegin(GL_LINE_LOOP);
            }
          if (op != 3 && is_concav[mtype]) glVertex2d(x, y);
          for (i = 0; i < marker[mtype][pc + 1]; i++)
            {
              xr = scale * marker[mtype][pc + 2 + 2 * i];
              yr = -scale * marker[mtype][pc + 3 + 2 * i];
              seg_xform_rel(&xr, &yr);
              glVertex2d(x - xr, y + yr);
            }
          glEnd();
          if (op == 5) set_color(mcolor);
          pc += 1 + 2 * marker[mtype][pc + 1];
          break;

        case 6: /* arc */
        case 7: /* filled arc */
        case 8: /* hollow arc */
          {
            num_segments = 4 * r;
            c = cos(2 * M_PI / (num_segments - 1));
            s = sin(2 * M_PI / (num_segments - 1));
            xr = r;
            yr = 0;
            if (op == 7)
              {
                glBegin(GL_TRIANGLE_FAN);
              }
            else if (op == 8)
              {
                set_color(0);
                glBegin(GL_TRIANGLE_FAN);
              }
            else
              {
                glBegin(GL_LINE_LOOP);
              }
            for (i = 0; i < num_segments; i++)
              {
                glVertex2d(x + xr, y + yr);
                tmp = xr;
                xr = c * xr - s * yr;
                yr = s * tmp + c * yr;
              }
            glEnd();
            if (op == 8) set_color(mcolor);
          }
          break;
        }
      pc++;
    }
  while (op != 0);

  glLoadIdentity();
}

static void polymarker(int n, double *px, double *py)
{
  int mk_type, mk_color;
  double mk_size, *clrt;
  double x, y;
  int i;

  mk_type = gkss->asf[3] ? gkss->mtype : gkss->mindex;
  mk_size = gkss->asf[4] ? gkss->mszsc : 1;
  mk_color = gkss->asf[5] ? gkss->pmcoli : 1;

  glLineWidth(1);

  clrt = gkss->viewport[gkss->cntnr];

  for (i = 0; i < n; i++)
    {
      WC_to_NDC(px[i], py[i], gkss->cntnr, x, y);
      seg_xform(&x, &y);
      if (gkss->clip != GKS_K_CLIP || (x >= clrt[0] && x <= clrt[1] && y >= clrt[2] && y <= clrt[3]))
        {
          draw_marker(x, y, mk_type, mk_size, mk_color);
        }
    }
  glLineWidth(1.0);
}

static void fill_routine(int n, double *px, double *py, int tnr)
{
  int fl_inter, fl_style, i, j;
  GLfloat vertices[2 * n];
  GLuint texture = 0;
  int parray[33];
  GLubyte bitmap[8][8];
  static GLuint buffer = 0;
  GLboolean draw_pattern = 0;
  double x, y;

  const double modelview_matrix[16] = {2.0 / p->width, 0, 0, -1, 0, -2.0 / p->height, 0, 1, 0, 0, 1, 0, 0, 0, 0, 1};
  const double texcoord_matrix[16] = {1. / 8., 0, 0, 0, 0, 1. / 8., 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};

  for (i = 0; i < n; i++)
    {
      WC_to_NDC(px[i], py[i], gkss->cntnr, x, y);
      seg_xform(&x, &y);
      NDC_to_DC(x, y, vertices[2 * i], vertices[2 * i + 1]);
    }

  fl_inter = gkss->asf[10] ? gkss->ints : predef_ints[gkss->findex - 1];

  glLineWidth(1);

  draw_pattern = (fl_inter == GKS_K_INTSTYLE_PATTERN || fl_inter == GKS_K_INTSTYLE_HATCH);

  if (draw_pattern)
    {
      fl_style = gkss->asf[11] ? gkss->styli : predef_styli[gkss->findex - 1];
      if (fl_inter == GKS_K_INTSTYLE_HATCH) fl_style += HATCH_STYLE;
      if (fl_style >= PATTERNS) fl_style = 1;
      gks_inq_pattern_array(fl_style, parray);

      for (i = 0; i < 8; i++)
        {
          for (j = 0; j < 8; j++)
            {
              bitmap[(j + 7) % 8][(i + 7) % 8] = (parray[(j % parray[0]) + 1] >> i) & 0x01 ? 0 : 255;
            }
        }

      glGenTextures(1, &texture);
      glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
      glBindTexture(GL_TEXTURE_2D, texture);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, 8, 8, 0, GL_ALPHA, GL_UNSIGNED_BYTE, bitmap);
      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
      glEnable(GL_TEXTURE_2D);
    }

  glMatrixMode(GL_MODELVIEW);
  glLoadTransposeMatrixd(modelview_matrix);
  if (draw_pattern)
    {
      glMatrixMode(GL_TEXTURE);
      glLoadTransposeMatrixd(texcoord_matrix);
      glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    }
  glEnableClientState(GL_VERTEX_ARRAY);
  if (!buffer)
    {
      glGenBuffers(1, &buffer);
      glBindBuffer(GL_ARRAY_BUFFER, buffer);
    }
  glBindBuffer(GL_ARRAY_BUFFER, buffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
  glVertexPointer(2, GL_FLOAT, 0, 0);
  if (draw_pattern)
    {
      glTexCoordPointer(2, GL_FLOAT, 0, 0);
      glDrawArrays(GL_POLYGON, 0, n);
      glDisable(GL_TEXTURE_2D);
      glDeleteTextures(1, &texture);
    }
  else if (fl_inter == GKS_K_INTSTYLE_HOLLOW)
    {
      glDrawArrays(GL_LINE_LOOP, 0, n);
    }
  else if (fl_inter == GKS_K_INTSTYLE_SOLID)
    {
      glDrawArrays(GL_POLYGON, 0, n);
    }
  glLoadIdentity();
}

static void fillarea(int n, double *px, double *py)
{
  int fl_color;

  fl_color = gkss->asf[12] ? gkss->facoli : 1;
  set_color(fl_color);

  fill_routine(n, px, py, gkss->cntnr);

  set_color(1);
}

static void cellarray(double xmin, double xmax, double ymin, double ymax, int dx, int dy, int dimx, int *colia,
                      int true_color)
{
  double x1, y1, x2, y2;
  int x, y, width, height, ix, iy;
  static GLuint buffer = 0;
  GLuint texture = 0;
  int i, j, k, index, rgb;
  GLfloat bitmap[dx][dy][4];

  WC_to_NDC(xmin, ymax, gkss->cntnr, x1, y1);
  seg_xform(&x1, &y1);
  NDC_to_DC(x1, y1, x1, y1);

  WC_to_NDC(xmax, ymin, gkss->cntnr, x2, y2);
  seg_xform(&x2, &y2);
  NDC_to_DC(x2, y2, x2, y2);

  width = (int)fabs(x2 - x1);
  height = (int)fabs(y2 - y1);
  if (width == 0 || height == 0) return;
  x = (int)min(x1, x2);
  y = (int)min(y1, y2);

  const double modelview_matrix[16] = {2.0 * width / p->width,
                                       0,
                                       0,
                                       2.0 * x / p->width - 1,
                                       0,
                                       -2.0 * height / p->height,
                                       0,
                                       -2.0 * y / p->height + 1,
                                       0,
                                       0,
                                       1,
                                       0,
                                       0,
                                       0,
                                       0,
                                       1};

  for (i = 0; i < dx; i++)
    {
      ix = (x1 > x2) ? dx - i - 1 : i;
      for (j = 0; j < dy; j++)
        {
          iy = (y1 < y2) ? dy - j - 1 : j;
          if (true_color)
            {
              rgb = colia[ix * dy + iy];
              bitmap[i][j][0] = (rgb & 0xff) / 255.;
              bitmap[i][j][1] = ((rgb & 0xff00) >> 8) / 255.;
              bitmap[i][j][2] = ((rgb & 0xff0000) >> 16) / 255.;
              bitmap[i][j][3] = ((rgb & 0xff000000) >> 24) / 255.;
            }
          else
            {
              index = colia[ix * dy + iy];
              index = FIX_COLORIND(index);
              for (k = 0; k < 3; k++)
                {
                  bitmap[i][j][k] = p->rgb[index][k];
                }
              bitmap[i][j][3] = p->transparency;
            }
        }
    }
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, dx, dy, 0, GL_RGBA, GL_FLOAT, bitmap);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  glEnable(GL_TEXTURE_2D);
  glMatrixMode(GL_MODELVIEW);
  glLoadTransposeMatrixd(modelview_matrix);
  glEnableClientState(GL_VERTEX_ARRAY);
  glEnableClientState(GL_TEXTURE_COORD_ARRAY);

  if (!buffer)
    {
      GLfloat vertices[8] = {0, 0, 1, 0, 0, 1, 1, 1};
      glGenBuffers(1, &buffer);
      glBindBuffer(GL_ARRAY_BUFFER, buffer);
      glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    }
  glBindBuffer(GL_ARRAY_BUFFER, buffer);
  glVertexPointer(2, GL_FLOAT, 0, 0);
  glBindBuffer(GL_ARRAY_BUFFER, buffer);
  glTexCoordPointer(2, GL_FLOAT, 0, 0);
  set_color(0);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glDisable(GL_TEXTURE_2D);
  glDeleteTextures(1, &texture);
}

#ifndef NO_FT

static void gl_drawimage(int x, int y, int w, int h, unsigned char *bitmap)
{
  static GLuint buffers[2] = {0, 0};
  static GLuint texture = 0;
  int tx_color;

  const double modelview_matrix[16] = {2.0 * w / p->width,
                                       0,
                                       0,
                                       2.0 * x / p->width - 1,
                                       0,
                                       2.0 * h / p->height,
                                       0,
                                       2.0 * y / p->height - 1,
                                       0,
                                       0,
                                       1,
                                       0,
                                       0,
                                       0,
                                       0,
                                       1};

  tx_color = gkss->asf[9] ? gkss->txcoli : 1;

  if (!texture)
    {
      glGenTextures(1, &texture);
    }
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, w, h, 0, GL_ALPHA, GL_UNSIGNED_BYTE, bitmap);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glBindTexture(GL_TEXTURE_2D, texture);

  glEnable(GL_TEXTURE_2D);
  set_color(tx_color);

  glMatrixMode(GL_MODELVIEW);
  glLoadTransposeMatrixd(modelview_matrix);
  glEnableClientState(GL_VERTEX_ARRAY);
  glEnableClientState(GL_TEXTURE_COORD_ARRAY);

  if (!buffers[0])
    {
      GLfloat vertices[] = {0, 0, 1, 0, 0, 1, 1, 1};
      GLint text_box[] = {0, 1, 1, 1, 0, 0, 1, 0};
      glGenBuffers(2, buffers);
      glBindBuffer(GL_ARRAY_BUFFER, buffers[0]);
      glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
      glBindBuffer(GL_ARRAY_BUFFER, buffers[1]);
      glBufferData(GL_ARRAY_BUFFER, sizeof(text_box), text_box, GL_STATIC_DRAW);
    }
  glBindBuffer(GL_ARRAY_BUFFER, buffers[0]);
  glVertexPointer(2, GL_FLOAT, 0, 0);
  glBindBuffer(GL_ARRAY_BUFFER, buffers[1]);
  glTexCoordPointer(2, GL_INT, 0, 0);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

  glLoadIdentity();
  glDisable(GL_TEXTURE_2D);
  glDeleteTextures(1, &texture);
}

#endif

static void text(double x_pos, double y_pos, int nchars, char *text)
{
  int tx_color;
#ifndef NO_FT
  unsigned char *bitmap;
  int x, y, w, h;
  int tx_prec = gkss->asf[6] ? gkss->txprec : predef_prec[gkss->tindex - 1];

  if (tx_prec == GKS_K_TEXT_PRECISION_STROKE)
    {
#endif
      tx_color = gkss->asf[9] ? gkss->txcoli : 1;
      if (tx_color <= 0 || tx_color >= MAX_COLOR) tx_color = 1;

      set_color(tx_color);
      gks_emul_text(x_pos, y_pos, nchars, text, line_routine, fill_routine);
      set_color(1);

#ifndef NO_FT
    }
  else
    {
      NDC_to_DC(x_pos, y_pos, x, y);
      w = p->width;
      y = p->height - y; /* in FT y-axis is in up direction */
      bitmap = gks_ft_get_bitmap(&x, &y, &w, &h, gkss, text, nchars);
      if (bitmap != NULL)
        {
          gl_drawimage(x, y, w, h, bitmap);
          free(bitmap);
        }
    }
#endif
}

static void interp(char *str)
{
  char *s;
  gks_state_list_t *sl = NULL, saved_gkss;
  int sp = 0, *len, *f;
  int *i_arr = NULL, *dx = NULL, *dy = NULL, *dimx = NULL, *len_c_arr = NULL;
  int *n = NULL, *primid = NULL, *ldr = NULL;
  double *f_arr_1 = NULL, *f_arr_2 = NULL;
  char *c_arr = NULL;
  int i, true_color = 0;

  s = str;

  RESOLVE(len, int, sizeof(int));
  while (*len)
    {
      RESOLVE(f, int, sizeof(int));

      switch (*f)
        {
        case 2: /* open workstation */
          RESOLVE(sl, gks_state_list_t, sizeof(gks_state_list_t));
          break;

        case 12: /* polyline */
        case 13: /* polymarker */
        case 15: /* fill area */
          RESOLVE(i_arr, int, sizeof(int));
          RESOLVE(f_arr_1, double, i_arr[0] * sizeof(double));
          RESOLVE(f_arr_2, double, i_arr[0] * sizeof(double));
          break;

        case 14: /* text */
          RESOLVE(f_arr_1, double, sizeof(double));
          RESOLVE(f_arr_2, double, sizeof(double));
          RESOLVE(len_c_arr, int, sizeof(int));
          RESOLVE(c_arr, char, 132);
          /* dummy assignment to avoid warning 'set but not used' */
          *len_c_arr = *len_c_arr;
          break;

        case 16:  /* cell array */
        case 201: /* draw image */
          RESOLVE(f_arr_1, double, 2 * sizeof(double));
          RESOLVE(f_arr_2, double, 2 * sizeof(double));
          RESOLVE(dx, int, sizeof(int));
          RESOLVE(dy, int, sizeof(int));
          RESOLVE(dimx, int, sizeof(int));
          RESOLVE(i_arr, int, *dimx **dy * sizeof(int));
          break;

        case 17: /* GDP */
          RESOLVE(n, int, sizeof(int));
          RESOLVE(primid, int, sizeof(int));
          RESOLVE(ldr, int, sizeof(int));
          RESOLVE(i_arr, int, *ldr * sizeof(int));
          RESOLVE(f_arr_1, double, *n * sizeof(double));
          RESOLVE(f_arr_2, double, *n * sizeof(double));
          break;

        case 19:  /* set linetype */
        case 21:  /* set polyline color index */
        case 23:  /* set markertype */
        case 25:  /* set polymarker color index */
        case 30:  /* set text color index */
        case 33:  /* set text path */
        case 36:  /* set fillarea interior style */
        case 37:  /* set fillarea style index */
        case 38:  /* set fillarea color index */
        case 52:  /* select normalization transformation */
        case 53:  /* set clipping indicator */
        case 108: /* set resample method */
          RESOLVE(i_arr, int, sizeof(int));
          break;

        case 27: /* set text font and precision */
        case 34: /* set text alignment */
          RESOLVE(i_arr, int, 2 * sizeof(int));
          break;

        case 20:  /* set linewidth scale factor */
        case 24:  /* set marker size scale factor */
        case 28:  /* set character expansion factor */
        case 29:  /* set character spacing */
        case 31:  /* set character height */
        case 200: /* set text slant */
        case 203: /* set transparency */
          RESOLVE(f_arr_1, double, sizeof(double));
          break;

        case 32: /* set character up vector */
          RESOLVE(f_arr_1, double, sizeof(double));
          RESOLVE(f_arr_2, double, sizeof(double));
          break;

        case 41: /* set aspect source flags */
          RESOLVE(i_arr, int, 13 * sizeof(int));
          break;

        case 48: /* set color representation */
          RESOLVE(i_arr, int, sizeof(int));
          RESOLVE(f_arr_1, double, 3 * sizeof(double));
          break;

        case 49: /* set window */
        case 50: /* set viewport */
        case 54: /* set workstation window */
        case 55: /* set workstation viewport */
          RESOLVE(i_arr, int, sizeof(int));
          RESOLVE(f_arr_1, double, 2 * sizeof(double));
          RESOLVE(f_arr_2, double, 2 * sizeof(double));
          break;

        case 202: /* set shadow */
          RESOLVE(f_arr_1, double, 3 * sizeof(double));
          break;

        case 204: /* set coord xform */
          RESOLVE(f_arr_1, double, 6 * sizeof(double));
          break;

        default:
          gks_perror("display list corrupted (len=%d, fctid=%d)", *len, *f);
          exit(1);
        }

      switch (*f)
        {
        case 2:
          memmove(&saved_gkss, gkss, sizeof(gks_state_list_t));
          memmove(gkss, sl, sizeof(gks_state_list_t));

          p->window[0] = p->window[2] = 0.0;
          p->window[1] = p->window[3] = 1.0;
          p->viewport[0] = p->viewport[2] = 0;
          p->viewport[1] = (double)p->width * p->mwidth / p->swidth;
          p->viewport[3] = (double)p->height * p->mheight / p->sheight;

          p->transparency = 1.0;

          set_xform();
          init_norm_xform();
          init_colors();

          gks_init_core(gkss);
          break;

        case 12:
          polyline(i_arr[0], f_arr_1, f_arr_2);
          break;

        case 13:
          polymarker(i_arr[0], f_arr_1, f_arr_2);
          break;

        case 14:
          text(f_arr_1[0], f_arr_2[0], strlen(c_arr), c_arr);
          break;

        case 15:
          fillarea(i_arr[0], f_arr_1, f_arr_2);
          break;

        case 16:
        case 201:
          true_color = *f == DRAW_IMAGE;
          cellarray(f_arr_1[0], f_arr_1[1], f_arr_2[0], f_arr_2[1], *dx, *dy, *dimx, i_arr, true_color);
          break;

        case 17:
          gks_perror("GDP primitive not supported for OpenGL");
          break;

        case 19:
          gkss->ltype = i_arr[0];
          break;

        case 20:
          gkss->lwidth = f_arr_1[0];
          break;

        case 21:
          gkss->plcoli = i_arr[0];
          break;

        case 23:
          gkss->mtype = i_arr[0];
          break;

        case 24:
          gkss->mszsc = f_arr_1[0];
          break;

        case 25:
          gkss->pmcoli = i_arr[0];
          break;

        case 27:
          gkss->txfont = i_arr[0];
          gkss->txprec = i_arr[1];
          break;

        case 28:
          gkss->chxp = f_arr_1[0];
          break;

        case 29:
          gkss->chsp = f_arr_1[0];
          break;

        case 30:
          gkss->txcoli = i_arr[0];
          break;

        case 31:
          gkss->chh = f_arr_1[0];
          break;

        case 32:
          gkss->chup[0] = f_arr_1[0];
          gkss->chup[1] = f_arr_2[0];
          break;

        case 33:
          gkss->txp = i_arr[0];
          break;

        case 34:
          gkss->txal[0] = i_arr[0];
          gkss->txal[1] = i_arr[1];
          break;

        case 36:
          gkss->ints = i_arr[0];
          break;

        case 37:
          gkss->styli = i_arr[0];
          break;

        case 38:
          gkss->facoli = i_arr[0];
          break;

        case 41:
          for (i = 0; i < 13; i++) gkss->asf[i] = i_arr[i];
          break;

        case 48:
          set_color_rep(i_arr[0], f_arr_1[0], f_arr_1[1], f_arr_1[2]);
          break;

        case 49:
          gkss->window[*i_arr][0] = f_arr_1[0];
          gkss->window[*i_arr][1] = f_arr_1[1];
          gkss->window[*i_arr][2] = f_arr_2[0];
          gkss->window[*i_arr][3] = f_arr_2[1];
          set_xform();
          set_norm_xform(*i_arr, gkss->window[*i_arr], gkss->viewport[*i_arr]);
          break;

        case 50:
          gkss->viewport[*i_arr][0] = f_arr_1[0];
          gkss->viewport[*i_arr][1] = f_arr_1[1];
          gkss->viewport[*i_arr][2] = f_arr_2[0];
          gkss->viewport[*i_arr][3] = f_arr_2[1];
          set_xform();
          set_norm_xform(*i_arr, gkss->window[*i_arr], gkss->viewport[*i_arr]);
          if (*i_arr == gkss->cntnr)
            {
              set_clip_rect(*i_arr);
            }
          break;

        case 52:
          gkss->cntnr = i_arr[0];
          set_clip_rect(gkss->cntnr);
          break;

        case 53:
          gkss->clip = i_arr[0];
          set_clip_rect(gkss->cntnr);
          break;

        case 54:
          p->window[0] = f_arr_1[0];
          p->window[1] = f_arr_1[1];
          p->window[2] = f_arr_2[0];
          p->window[3] = f_arr_2[1];
          set_xform();
          break;

        case 55:
          p->viewport[0] = f_arr_1[0];
          p->viewport[1] = f_arr_1[1];
          p->viewport[2] = f_arr_2[0];
          p->viewport[3] = f_arr_2[1];

          resize_window();
          set_xform();
          break;

        case 108:
          gkss->resample_method = i_arr[0];
          break;

        case 200:
          gkss->txslant = f_arr_1[0];
          break;

        case 203:
          p->transparency = f_arr_1[0];
          break;
        }

      RESOLVE(len, int, sizeof(int));
    }
  memmove(gkss, &saved_gkss, sizeof(gks_state_list_t));
}

void gks_glplugin(int fctid, int dx, int dy, int dimx, int *i_arr, int len_f_arr_1, double *f_arr_1, int len_f_arr_2,
                  double *f_arr_2, int len_c_arr, char *c_arr, void **ptr)
{
  p = (ws_state_list *)*ptr;

  switch (fctid)
    {
    case 2:
      gkss = (gks_state_list_t *)*ptr;

      p = (ws_state_list *)gks_malloc(sizeof(ws_state_list));

      p->height = 500;
      p->width = 500;
      open_window();

      f_arr_1[0] = p->mwidth;
      f_arr_2[0] = p->mheight;
      i_arr[0] = p->swidth;
      i_arr[1] = p->sheight;

      *ptr = p;
      break;

    case 3:
      close_window();
      gks_free(p);
      p = NULL;
      break;

    case 6:
      /* set display list length to zero */
      memset(p->dl.buffer, 0, sizeof(int));
      p->dl.nbytes = 0;
      glClear(GL_COLOR_BUFFER_BIT);
      break;

    case 8:
      if (i_arr[1] & GKS_K_PERFORM_FLAG)
        {
          interp(p->dl.buffer);
          update();
        }
      break;
    }

  if (p != NULL)
    gks_dl_write_item(&p->dl, fctid, dx, dy, dimx, i_arr, len_f_arr_1, f_arr_1, len_f_arr_2, f_arr_2, len_c_arr, c_arr,
                      gkss);
}

#else

void gks_glplugin(int fctid, int dx, int dy, int dimx, int *i_arr, int len_f_arr_1, double *f_arr_1, int len_f_arr_2,
                  double *f_arr_2, int len_c_arr, char *c_arr, void **ptr)
{
  if (fctid == 2)
    {
      gks_perror("GLFW support not compiled in");
      i_arr[0] = 0;
    }
}

#endif
