#ifdef __unix__
#define _POSIX_C_SOURCE 200112L
#endif
#if defined(_WIN32) && (!defined(__STDC_VERSION__) || __STDC_VERSION__ < 199901L) && defined(__STRICT_ANSI__)
extern float __cdecl sqrtf(float);
#endif

#include <assert.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "gr.h"
#include "gr3.h"
#include "gr3_internals.h"
#include "gks.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef max
#define max(a, b) ((a) > (b) ? (a) : (b))
#endif

#ifndef FLT_MAX
#define FLT_MAX 1.701411735e+38
#endif

#define arc(angle) (M_PI * (angle) / 180.0)

#define OPTION_X_LOG (1 << 0)
#define OPTION_Y_LOG (1 << 1)
#define OPTION_Z_LOG (1 << 2)
#define OPTION_FLIP_X (1 << 3)
#define OPTION_FLIP_Y (1 << 4)
#define OPTION_FLIP_Z (1 << 5)

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN 1
#ifdef near
#undef near
#endif
#ifdef far
#undef far
#endif
#endif

typedef struct
{
  double a1, a2, b, c1, c2, c3, d;
} gr_world_xform_t;

typedef enum
{
  OPTION_LINES,
  OPTION_MESH,
  OPTION_FILLED_MESH,
  OPTION_Z_SHADED_MESH,
  OPTION_COLORED_MESH,
  OPTION_CELL_ARRAY,
  OPTION_SHADED_MESH
} gr_surface_option_t;

typedef struct
{
  double a, b;
  int o_log, o_flip;
} trans_t;

int gr3_drawimage_gks_(float xmin, float xmax, float ymin, float ymax, int width, int height)
{
  double _xmin = (double)xmin, _xmax = (double)xmax;
  double _ymin = (double)ymin, _ymax = (double)ymax;
  char *pixels;
  int err;
  gr3_log_("gr3_drawimage_gks_();");
  pixels = (char *)malloc(sizeof(int) * width * height);
  if (!pixels)
    {
      RETURN_ERROR(GR3_ERROR_OUT_OF_MEM);
    }
  err = gr3_getimage(width, height, 1, pixels);
  if (err != GR3_ERROR_NONE)
    {
      free(pixels);
      return err;
    }
  gr_drawimage(_xmin, _xmax, _ymax, _ymin, width, height, (int *)pixels, 0);
  free(pixels);
  return GR3_ERROR_NONE;
}

static void gr3_matmul_(float *a, float *b)
{
  int i, j, k;
  float row[4];

  for (i = 0; i < 4; i++)
    {
      for (j = 0; j < 4; j++)
        {
          row[j] = a[i + j * 4];
        }
      for (j = 0; j < 4; j++)
        {
          a[i + j * 4] = 0.0;
          for (k = 0; k < 4; k++)
            {
              a[i + j * 4] += row[k] * b[k + j * 4];
            }
        }
    }
}

static void gr3_identity_(float *a)
{
  int i, j;

  for (i = 0; i < 4; i++)
    {
      for (j = 0; j < i; j++)
        {
          a[j + i * 4] = 0.0f;
        }
      a[i + i * 4] = 1.0;
      for (j = i + 1; j < 4; j++)
        {
          a[j + i * 4] = 0.0f;
        }
    }
}

static void gr3_crossprod_(float *c, float *a, float *b)
{
  c[0] = a[1] * b[2] - a[2] * b[1];
  c[1] = a[2] * b[0] - a[0] * b[2];
  c[2] = a[0] * b[1] - a[1] * b[0];
}

static float gr3_norm_(float *v)
{
  int i;
  float s;

  s = 0.0f;
  for (i = 0; i < 3; i++)
    {
      s += v[i] * v[i];
    }
  return sqrtf(s);
}

static void gr3_normalize_(float *v)
{
  double s;

  s = v[0] * v[0] + v[1] * v[1] + v[2] * v[2];

  if (s > 0.0)
    {
      s = sqrt(s);
      v[0] /= s;
      v[1] /= s;
      v[2] /= s;
    }
}

static gr_world_xform_t gr3_grworldxform_(int rotation, int tilt)
{
  gr_world_xform_t wx;
  double xmin, xmax, ymin, ymax, zmin, zmax;
  double r, t, a, c;

  xmin = -0.5;
  xmax = 0.5;
  ymin = -0.5;
  ymax = 0.5;
  zmin = -0.5;
  zmax = 0.5;

  r = arc(rotation);
  wx.a1 = cos(r);
  wx.a2 = sin(r);

  a = (xmax - xmin) / (wx.a1 + wx.a2);
  wx.b = xmin;

  wx.a1 = a * wx.a1 / (xmax - xmin);
  wx.a2 = a * wx.a2 / (ymax - ymin);
  wx.b = wx.b - wx.a1 * xmin - wx.a2 * ymin;

  t = arc(tilt);
  wx.c1 = (pow(cos(r), 2.) - 1.) * tan(t / 2.);
  wx.c2 = -(pow(sin(r), 2.) - 1.) * tan(t / 2.);
  wx.c3 = cos(t);

  c = (ymax - ymin) / (wx.c2 + wx.c3 - wx.c1);
  wx.d = ymin - c * wx.c1;

  wx.c1 = c * wx.c1 / (xmax - xmin);
  wx.c2 = c * wx.c2 / (ymax - ymin);
  wx.c3 = c * wx.c3 / (zmax - zmin);
  wx.d = wx.d - wx.c1 * xmin - wx.c2 * ymin - wx.c3 * zmin;

  return wx;
}

static void gr3_grtransformation_(float *a, int rotation, int tilt)
{
  gr_world_xform_t wx;
  float row0[3], row1[3], row2[3];
  int i;

  wx = gr3_grworldxform_(rotation, tilt);
  row0[0] = wx.a1;
  row0[1] = 0.0f;
  row0[2] = -wx.a2;
  row1[0] = wx.c1;
  row1[1] = wx.c3;
  row1[2] = -wx.c2;
  gr3_crossprod_(row2, row0, row1);

  gr3_identity_(a);
  for (i = 0; i < 3; i++)
    {
      a[0 + i * 4] = row0[i];
      a[1 + i * 4] = row1[i];
      a[2 + i * 4] = row2[i];
    }
}

static double gr3_log10_(double x)
{
  if (x > 0)
    {
      return log10(x);
    }
  else
    {
      return -FLT_MAX;
    }
}

/* create a transformation into NDC coordinates */
static void gr3_ndctrans_(double xmin, double xmax, trans_t *tx, int option_log, int option_flip)
{
  if (option_log)
    {
      tx->a = 1.0 / gr3_log10_((xmax / xmin));
      tx->b = -gr3_log10_(xmin) * tx->a;
    }
  else
    {
      tx->a = 1.0 / (xmax - xmin);
      tx->b = -xmin * tx->a;
    }
  tx->o_log = option_log;
  tx->o_flip = option_flip;
}

static float gr3_transform_(float x, trans_t tx)
{
  if (tx.o_log)
    {
      x = gr3_log10_(x);
    }
  x = tx.a * x + tx.b;
  if (tx.o_flip)
    {
      x = 1.0 - x;
    }

  return x;
}

/*!
 * Create a mesh of a surface plot similar to gr_surface.
 * Uses the current colormap. To apply changes of the colormap
 * a new mesh has to be created.
 * \param [out] mesh    the mesh handle
 * \param [in]  nx      number of points in x-direction
 * \param [in]  ny      number of points in y-direction
 * \param [in]  px      an array containing the x-coordinates
 * \param [in]  py      an array containing the y-coordinates
 * \param [in]  pz      an array of length nx * ny containing
 *                      the z-coordinates
 * \param [in]  option  option for the surface mesh; the GR3_SURFACE
 *                      constants can be combined with bitwise or.
 */
GR3API int gr3_createsurfacemesh(int *mesh, int nx, int ny, float *px, float *py, float *pz, int option)
{
  double xmin, xmax, ymin, ymax, zmin, zmax;
  int rotation, tilt;
  int i, j;
  int num_vertices;
  float *vertices, *normals, *colors;
  int num_indices;
  int *indices;
  int result;
  int scale;
  int cmap;
  int first_color, last_color;
  int projection_type;
  trans_t tx, ty, tz;
  int new_num_vertices;
  float *new_vertices, *new_normals, *new_colors;
  int new_idx, l, errind;
  double linewidth_y, linewidth_x;

  gr_inqprojectiontype(&projection_type);
  gr_inqcolormapinds(&first_color, &last_color);

  num_vertices = nx * ny;
  vertices = malloc(num_vertices * 3 * sizeof(float));
  if (!vertices)
    {
      RETURN_ERROR(GR3_ERROR_OUT_OF_MEM);
    }
  normals = malloc(num_vertices * 3 * sizeof(float));
  if (!normals)
    {
      free(vertices);
      RETURN_ERROR(GR3_ERROR_OUT_OF_MEM);
    }
  colors = malloc(num_vertices * 3 * sizeof(float));
  if (!colors)
    {
      free(vertices);
      free(normals);
      RETURN_ERROR(GR3_ERROR_OUT_OF_MEM);
    }
  num_indices = (nx - 1) * (ny - 1) * 6; /* 2 triangles per square */
  indices = malloc(num_indices * sizeof(int));
  if (!indices)
    {
      free(vertices);
      free(normals);
      free(colors);
      RETURN_ERROR(GR3_ERROR_OUT_OF_MEM);
    }

  if (option & GR3_SURFACE_GRTRANSFORM)
    {
      if (projection_type == GR_PROJECTION_PERSPECTIVE || projection_type == GR_PROJECTION_ORTHOGRAPHIC)
        {
          gr_inqwindow3d(&xmin, &xmax, &ymin, &ymax, &zmin, &zmax);
        }
      else
        {
          gr_inqwindow(&xmin, &xmax, &ymin, &ymax);
          gr_inqspace(&zmin, &zmax, &rotation, &tilt);
        }
      gr_inqscale(&scale);
    }
  else
    {
      xmin = px[0];
      xmax = px[nx - 1];
      ymin = py[0];
      ymax = py[ny - 1];
      zmin = pz[0];
      zmax = pz[0];
      for (i = 1; i < nx * ny; i++)
        {
          if (pz[i] < zmin) zmin = pz[i];
          if (pz[i] > zmax) zmax = pz[i];
        }
      scale = 0;
    }

  gr3_ndctrans_(xmin, xmax, &tx, scale & OPTION_X_LOG, scale & OPTION_FLIP_X);
  /* flip because y-axis is projected to the negative z-axis */
  gr3_ndctrans_(ymin, ymax, &ty, scale & OPTION_Y_LOG, !(scale & OPTION_FLIP_Y));
  gr3_ndctrans_(zmin, zmax, &tz, scale & OPTION_Z_LOG, scale & OPTION_FLIP_Z);

  for (j = 0; j < ny; j++)
    {
      for (i = 0; i < nx; i++)
        {
          int k = j * nx + i;
          float *v = vertices + 3 * k;
          float *n = normals + 3 * k;
          float *c = colors + 3 * k;
          float zvalue;

          if (projection_type == GR_PROJECTION_ORTHOGRAPHIC || projection_type == GR_PROJECTION_PERSPECTIVE)
            {
              v[0] = px[i];
              zvalue = pz[k];
              v[1] = py[j];
            }
          else
            {
              v[0] = gr3_transform_(px[i], tx);
              zvalue = gr3_transform_(pz[k], tz);
              v[2] = gr3_transform_(py[j], ty);
            }

          if (option & GR3_SURFACE_FLAT)
            {
              v[1] = 0.0f;
            }
          else
            {
              if (projection_type == GR_PROJECTION_PERSPECTIVE || projection_type == GR_PROJECTION_ORTHOGRAPHIC)
                {
                  v[2] = zvalue;
                }
              else
                {
                  v[1] = zvalue;
                }
            }

          if (projection_type == GR_PROJECTION_PERSPECTIVE || projection_type == GR_PROJECTION_ORTHOGRAPHIC)
            {
              zvalue = gr3_transform_(pz[k], tz);
            }

          if (option & GR3_SURFACE_FLAT || !(option & GR3_SURFACE_NORMALS))
            {
              n[0] = 0.0f;
              n[1] = 1.0f;
              n[2] = 0.0f;
            }

          if (option & (GR3_SURFACE_GRCOLOR | GR3_SURFACE_GRZSHADED))
            {
              int color, rgb;

              if (option & GR3_SURFACE_GRZSHADED)
                color = (int)pz[k] + first_color;
              else
                color = (int)(zvalue * (last_color - first_color) + first_color);
              if (color < first_color)
                color = first_color;
              else if (color > last_color)
                color = last_color;

              gr_inqcolor(color, &rgb);
              c[0] = (float)(rgb & 0xff) / 255;
              c[1] = (float)((rgb >> 8) & 0xff) / 255;
              c[2] = (float)((rgb >> 16) & 0xff) / 255;
            }
          else
            {
              c[0] = 1.0;
              c[1] = 1.0;
              c[2] = 1.0;
            }
        }
    }

  /* interpolate normals from the gradient */
  if (option & GR3_SURFACE_NORMALS && !(option & GR3_SURFACE_FLAT))
    {
      int dirx = 3, diry = 3 * nx;

      for (j = 0; j < ny; j++)
        {
          for (i = 0; i < nx; i++)
            {
              int k = j * nx + i;
              float *v = vertices + 3 * k;
              float *n = normals + 3 * k;
              float dx, dy;

              if (i == 0)
                {
                  dx = (v[dirx + 1] - v[1]) / (v[dirx + 0] - v[0]);
                }
              else if (i == nx - 1)
                {
                  dx = (v[1] - v[-dirx + 1]) / (v[0] - v[-dirx + 0]);
                }
              else
                {
                  dx = ((v[1] - v[-dirx + 1]) / (v[0] - v[-dirx + 0]) + (v[dirx + 1] - v[1]) / (v[dirx + 0] - v[0])) /
                       2.0;
                }

              if (j == 0)
                {
                  dy = (v[diry + 1] - v[1]) / (v[diry + 2] - v[2]);
                }
              else if (j == ny - 1)
                {
                  dy = (v[1] - v[-diry + 1]) / (v[2] - v[-diry + 2]);
                }
              else
                {
                  dy = ((v[1] - v[-diry + 1]) / (v[2] - v[-diry + 2]) + (v[diry + 1] - v[1]) / (v[diry + 2] - v[2])) /
                       2.0;
                }

              n[0] = -dx;
              n[1] = 1.0f;
              n[2] = -dy;
              gr3_normalize_(n);
            }
        }
    }
  new_num_vertices = num_indices;
  if (context_struct_.use_software_renderer && context_struct_.option <= OPTION_FILLED_MESH)
    {
      int quality = context_struct_.quality;
      int ssaa_factor = quality & ~1;
      if (ssaa_factor == 0) ssaa_factor = 1;
      new_vertices = malloc(new_num_vertices * 3 * sizeof(float));
      if (!new_vertices)
        {
          RETURN_ERROR(GR3_ERROR_OUT_OF_MEM);
        }
      new_normals = calloc(new_num_vertices * 3, sizeof(float));
      if (!new_normals)
        {
          free(new_vertices);
          RETURN_ERROR(GR3_ERROR_OUT_OF_MEM);
        }
      new_colors = malloc(new_num_vertices * 3 * sizeof(float));
      if (!new_colors)
        {
          free(new_vertices);
          free(new_normals);
          RETURN_ERROR(GR3_ERROR_OUT_OF_MEM);
        }
      gks_inq_pline_linewidth(&errind, &linewidth_y);
      linewidth_y *= 2 * ssaa_factor;
      if (errind != GKS_K_NO_ERROR)
        {
          RETURN_ERROR(errind);
        }
      linewidth_x = linewidth_y;
      if (context_struct_.option == OPTION_LINES)
        {
          linewidth_x = 0; /* set to zero to not be drawn */
        }
    }
  new_idx = 0;
  /* create triangles */
  for (j = 0; j < ny - 1; j++)
    {
      for (i = 0; i < nx - 1; i++)
        {
          /* Unroll the indexbuffer for the software-renderer, if the edges should be drawn (cf Options in gr3_surface).
           * The idea is to store a linewidth in the normals x value of every vertex.
           * The normals x-value of the first vertex determines the width of edge 0-1, the second vertex normal's x
           * coordinate to the edge 1-2 and the last one to 2-0.*/
          int k = j * nx + i;
          if (context_struct_.use_software_renderer && context_struct_.option <= OPTION_FILLED_MESH)
            {
              for (l = 0; l < 3; l++)
                {
                  new_vertices[new_idx + l] = vertices[k * 3 + l];
                  new_vertices[new_idx + 3 + l] = vertices[(k + 1) * 3 + l];
                  new_vertices[new_idx + 6 + l] = vertices[(k + nx) * 3 + l];
                  new_vertices[new_idx + 9 + l] = vertices[(k + nx) * 3 + l];
                  new_vertices[new_idx + 12 + l] = vertices[(k + 1) * 3 + l];
                  new_vertices[new_idx + 15 + l] = vertices[(k + nx + 1) * 3 + l];
                }
              new_normals[new_idx] = linewidth_y; /* vertical line */
              new_normals[new_idx + 3] = 0;
              new_normals[new_idx + 6] = linewidth_x; /* horizontal line */
              new_normals[new_idx + 9] = 0;
              new_normals[new_idx + 12] = linewidth_x; /* horizontal line */
              new_normals[new_idx + 15] = linewidth_y; /* vertical line */

              /* If the edges have to be drawn forming a square shape, every triangle must additionally have
               * information about the vertex that is missing to make the triangle a square, because
               * all the edges of the square have to be rasterized. Thus the coordinates are passed by
               * storing them in the normals, because those aren't needed. There are two cases depending on
               * which vertex is left out in the square to form a triangle, and to keep them apart
               * the linewidth is passed with a negative sign in one case.*/
              new_normals[new_idx + 1] = vertices[(k + nx + 1) * 3];
              new_normals[new_idx + 2] = vertices[(k + nx + 1) * 3 + 1];
              new_normals[new_idx + 4] = vertices[(k + nx + 1) * 3 + 2];
              new_normals[new_idx + 5] = linewidth_y;

              new_normals[new_idx + 10] = vertices[k * 3];
              new_normals[new_idx + 11] = vertices[k * 3 + 1];
              new_normals[new_idx + 13] = vertices[k * 3 + 2];
              new_normals[new_idx + 14] = -linewidth_y;
              if (j == 0) /*left border*/
                {
                  new_normals[new_idx] = linewidth_y;
                }
              if (i == 0)
                {
                  new_normals[new_idx + 6] = linewidth_y;
                }
              if (j == ny - 2) /*right border*/
                {
                  new_normals[new_idx + 15] = linewidth_y;
                }
              if (i == nx - 2)
                {
                  new_normals[new_idx + 12] = linewidth_y;
                }
              new_idx += 18;
            }
          else
            {
              int *idx = indices + 6 * (j * (nx - 1) + i);
              idx[0] = k;
              idx[1] = k + 1;
              idx[2] = k + nx;
              idx[3] = k + nx;
              idx[4] = k + 1;
              idx[5] = k + nx + 1;
            }
        }
    }
  if (context_struct_.use_software_renderer && context_struct_.option <= OPTION_FILLED_MESH)
    {
      result = gr3_createmesh_nocopy(mesh, new_num_vertices, new_vertices, new_normals, new_colors);
    }
  else
    {
      result = gr3_createindexedmesh_nocopy(mesh, num_vertices, vertices, normals, colors, num_indices, indices);
    }
  if (result != GR3_ERROR_NONE && result != GR3_ERROR_OPENGL_ERR)
    {
      free(indices);
      free(colors);
      free(normals);
      free(vertices);
      if (context_struct_.use_software_renderer && context_struct_.option <= OPTION_FILLED_MESH)
        {
          free(new_normals);
          free(new_vertices);
          free(new_colors);
        }
    }

  return result;
}

/*!
 * Draw a mesh with the projection of gr. It uses the current
 * projection parameters (rotation, tilt) of gr.
 * This function alters the projection type, the projection parameters,
 * the viewmatrix and the light direction. If necessary, the user has to
 * save them before the call to this function and restore them after
 * the call to gr3_drawimage.
 * \param [in] mesh       the mesh to be drawn
 * \param [in] n          the number of meshes to be drawn
 * \param [in] positions  the positions where the meshes should be drawn
 * \param [in] directions the forward directions the meshes should be
 *                        facing at
 * \param [in] ups        the up directions
 * \param [in] colors     the colors the meshes should be drawn in,
 *                        it will be multiplied with each vertex color
 * \param [in] scales     the scaling factors
 */
GR3API void gr3_drawmesh_grlike(int mesh, int n, const float *positions, const float *directions, const float *ups,
                                const float *colors, const float *scales)
{
  double zmin, zmax;
  int rotation, tilt;
  float grmatrix[16], grviewmatrix[16];
  float grscales[4];
  float *modelscales, *modelpos;
  int i, j;
  int projection_type;
  gr_inqprojectiontype(&projection_type);

  if (projection_type == GR_PROJECTION_DEFAULT)
    {
      gr3_setprojectiontype(GR3_PROJECTION_PARALLEL);
      if (gr3_geterror(0, NULL, NULL)) return;
      gr3_setcameraprojectionparameters(90.0f, 1.0f, 200.0f);
      if (gr3_geterror(0, NULL, NULL)) return;
      gr3_setlightdirection(0.0f, 1.0f, 0.0f);
    }
  else if (projection_type == GR_PROJECTION_PERSPECTIVE || projection_type == GR_PROJECTION_ORTHOGRAPHIC)
    {
      if (projection_type == GR_PROJECTION_PERSPECTIVE)
        {
          double znear, zfar, fov;
          gr3_setprojectiontype(GR3_PROJECTION_PERSPECTIVE);
          if (gr3_geterror(0, NULL, NULL)) return;
          gr_inqperspectiveprojection(&znear, &zfar, &fov);
          gr3_setcameraprojectionparameters((GLfloat)fov, (GLfloat)znear, (GLfloat)zfar);
        }
      else
        {
          double left, right, bottom, top, znear, zfar;
          gr3_setprojectiontype(GR3_PROJECTION_ORTHOGRAPHIC);
          if (gr3_geterror(0, NULL, NULL)) return;
          gr_inqorthographicprojection(&left, &right, &bottom, &top, &znear, &zfar);
          gr3_setorthographicprojection((GLfloat)left, (GLfloat)right, (GLfloat)bottom, (GLfloat)top, (GLfloat)znear,
                                        (GLfloat)zfar);
        }
      if (gr3_geterror(0, NULL, NULL)) return;
      gr3_setlightdirection(ups[0], ups[1], ups[2]);
    }
  if (gr3_geterror(0, NULL, NULL)) return;

  grscales[3] = 1.0f;

  /* translate */
  if (projection_type == GR_PROJECTION_DEFAULT)
    {
      gr_inqspace(&zmin, &zmax, &rotation, &tilt);
      gr3_grtransformation_(grmatrix, rotation, tilt);

      /* extract non-uniform scaling */
      for (i = 0; i < 3; i++)
        {
          grscales[i] = gr3_norm_(&grmatrix[i * 4]);
          for (j = 0; j < 4; j++)
            {
              grmatrix[j + i * 4] /= grscales[i];
            }
        }

      gr3_identity_(grviewmatrix);
      grviewmatrix[2 + 3 * 4] = -4;
      gr3_matmul_(grviewmatrix, grmatrix);
      gr3_setviewmatrix(grviewmatrix);
    }
  else if (projection_type == GR_PROJECTION_PERSPECTIVE || projection_type == GR_PROJECTION_ORTHOGRAPHIC)
    {
      memset(grviewmatrix, 0, 16 * sizeof(GLfloat));
      double camera_pos[3], up[3], focus_point[3];
      gr_inqtransformationparameters(&camera_pos[0], &camera_pos[1], &camera_pos[2], &up[0], &up[1], &up[2],
                                     &focus_point[0], &focus_point[1], &focus_point[2]);
      gr3_cameralookat((GLfloat)camera_pos[0], (GLfloat)camera_pos[1], (GLfloat)camera_pos[2], (GLfloat)focus_point[0],
                       (GLfloat)focus_point[1], (GLfloat)focus_point[2], (GLfloat)up[0], (GLfloat)up[1],
                       (GLfloat)up[2]);
    }

  /* apply the scaling where it does not affect the normals */
  modelpos = malloc(n * 3 * sizeof(float));
  modelscales = malloc(n * 3 * sizeof(float));
  for (i = 0; i < n; i++)
    {
      for (j = 0; j < 3; j++)
        {
          if (projection_type == GR_PROJECTION_ORTHOGRAPHIC || projection_type == GR_PROJECTION_PERSPECTIVE)
            {
              modelscales[i * 3 + j] = scales[i * 3 + j];
              modelpos[i * 3 + j] = positions[i * 3 + j];
            }
          else
            {
              modelscales[i * 3 + j] = scales[i * 3 + j] * grscales[j];
              modelpos[i * 3 + j] = positions[i * 3 + j] * grscales[j];
            }
        }
    }
  gr3_drawmesh(mesh, n, modelpos, directions, ups, colors, modelscales);
  free(modelpos);
  free(modelscales);
}

/*!
 * Convenience function for drawing a surfacemesh
 * \param [in] mesh the mesh to be drawn
 */
GR3API void gr3_drawsurface(int mesh)
{
  int projection_type;
  gr_inqprojectiontype(&projection_type);
  float directions[3] = {0.0f, 0.0f, 1.0f};
  float ups[3] = {0.0f, 1.0f, 0.0f};
  float positions[3] = {-1.0f, -1.0f, -1.0f};
  float colors[3] = {1.0f, 1.0f, 1.0f};
  float scales[3] = {2.0f, 2.0f, 2.0f};

  if (projection_type == GR_PROJECTION_ORTHOGRAPHIC || projection_type == GR_PROJECTION_PERSPECTIVE)
    {
      double x_axis_scale, y_axis_scale, z_axis_scale;
      gr_inqscalefactors3d(&x_axis_scale, &y_axis_scale, &z_axis_scale);

      scales[0] = (float)x_axis_scale;
      scales[1] = (float)y_axis_scale;
      scales[2] = (float)z_axis_scale;
      positions[0] = 0.0f;
      positions[1] = 0.0f;
      positions[2] = 0.0f;
    }

  gr3_setbackgroundcolor(1.0f, 1.0f, 1.0f, 0.0f);
  if (gr3_geterror(0, NULL, NULL)) return;
  gr3_clear();
  if (gr3_geterror(0, NULL, NULL)) return;
  gr3_drawmesh_grlike(mesh, 1, positions, directions, ups, colors, scales);
  if (gr3_geterror(0, NULL, NULL)) return;
}

/*!
 * Create a surface plot with gr3 and draw it with gks as cellarray
 * \param [in]  nx      number of points in x-direction
 * \param [in]  ny      number of points in y-direction
 * \param [in]  px      an array containing the x-coordinates
 * \param [in]  py      an array containing the y-coordinates
 * \param [in]  pz      an array of length nx * ny containing
 *                      the z-coordinates
 * \param [in]  option  see the option parameter of gr_surface
 */
GR3API void gr3_surface(int nx, int ny, float *px, float *py, float *pz, int option)
{
  if (option == OPTION_Z_SHADED_MESH || option == OPTION_COLORED_MESH ||
      (context_struct_.use_software_renderer && option <= OPTION_FILLED_MESH))
    {
      int mesh;
      double xmin, xmax, ymin, ymax;
      int scale;
      int surfaceoption;
      context_struct_.option = option;
      surfaceoption = GR3_SURFACE_GRTRANSFORM;
      if (option == OPTION_Z_SHADED_MESH)
        {
          surfaceoption |= GR3_SURFACE_GRZSHADED;
        }
      else
        {
          surfaceoption |= GR3_SURFACE_GRCOLOR;
        }
      gr3_createsurfacemesh(&mesh, nx, ny, px, py, pz, surfaceoption);
      if (gr3_geterror(0, NULL, NULL)) return;
      gr3_drawsurface(mesh);
      if (gr3_geterror(0, NULL, NULL)) return;
      gr3_deletemesh(mesh);
      if (gr3_geterror(0, NULL, NULL)) return;
      gr_inqwindow(&xmin, &xmax, &ymin, &ymax);
      gr_inqscale(&scale);

      if (scale & OPTION_FLIP_X)
        {
          double tmp = xmin;
          xmin = xmax;
          xmax = tmp;
        }
      if (scale & OPTION_FLIP_Y)
        {
          double tmp = ymin;
          ymin = ymax;
          ymax = tmp;
        }
      /* TODO: inquire the required resolution */
      int base_resolution = 1000;
      double vpxmin, vpxmax, vpymin, vpymax;
      gr_inqviewport(&vpxmin, &vpxmax, &vpymin, &vpymax);
      double aspect = fabs((vpxmax - vpxmin) / (vpymax - vpymin));
      int projection_type;
      gr_inqprojectiontype(&projection_type);
      if (projection_type == GR_PROJECTION_DEFAULT)
        {
          /* projection type does not consider the viewport aspect ratio */
          aspect = 1;
        }
      if (aspect > 1)
        {
          gr3_drawimage((float)xmin, (float)xmax, (float)ymin, (float)ymax, (int)(base_resolution * aspect),
                        base_resolution, GR3_DRAWABLE_GKS);
        }
      else
        {
          double fovy = context_struct_.vertical_field_of_view;
          context_struct_.vertical_field_of_view =
              (float)(atan(tan(context_struct_.vertical_field_of_view / 360 * M_PI) / aspect) / M_PI * 360);
          gr3_drawimage((float)xmin, (float)xmax, (float)ymin, (float)ymax, base_resolution,
                        (int)(base_resolution / aspect), GR3_DRAWABLE_GKS);
          context_struct_.vertical_field_of_view = (float)fovy;
        }
      if (gr3_geterror(0, NULL, NULL)) return;
    }
  else
    {
      double *dpx, *dpy, *dpz;
      int i;

      dpx = malloc(nx * sizeof(double));
      dpy = malloc(ny * sizeof(double));
      dpz = malloc(nx * ny * sizeof(double));
      if (dpx != NULL && dpy != NULL && dpz != NULL)
        {
          for (i = 0; i < nx; i++)
            {
              dpx[i] = (double)px[i];
            }
          for (i = 0; i < ny; i++)
            {
              dpy[i] = (double)py[i];
            }
          for (i = 0; i < nx * ny; i++)
            {
              dpz[i] = (double)pz[i];
            }
          gr_surface(nx, ny, dpx, dpy, dpz, option);
        }
      free(dpz);
      free(dpy);
      free(dpx);
    }
}


/*!
 * Draw the given triangles using the current GR colormap
 * \param [in]  n          number of triangles
 * \param [in]  positions  pointer to an array of 3*3*n float values
 */
GR3API void gr3_drawtrianglesurface(int n, const float *positions)
{
  int i;
  int j;
  int mesh;
  int scale;
  double z_min;
  double z_max;
  struct
  {
    double x_min;
    double x_max;
    double y_min;
    double y_max;
  } window;
  float *normals;
  float *colors;
  if (n < 1)
    {
      return;
    }
  z_min = positions[2];
  z_max = positions[2];
  for (i = 0; i < n; i++)
    {
      for (j = 0; j < 3; j++)
        {
          if (z_min > positions[(i * 3 + j) * 3 + 2])
            {
              z_min = positions[(i * 3 + j) * 3 + 2];
            }
          if (z_max < positions[(i * 3 + j) * 3 + 2])
            {
              z_max = positions[(i * 3 + j) * 3 + 2];
            }
        }
    }
  if (z_min == z_max)
    {
      /* if all z are equal, use the central color of the colormap */
      z_max += 0.5;
      z_min -= 0.5;
    }

  normals = (float *)malloc(n * 3 * 3 * sizeof(float));
  colors = (float *)malloc(n * 3 * 3 * sizeof(float));
  assert(positions);
  assert(normals);
  assert(colors);
  for (i = 0; i < n; i++)
    {
      for (j = 0; j < 3; j++)
        {
          int color;
          int rgb;
          /* light direction in gr3_drawmesh_grlike() is fixed to (0, 1, 0) */
          normals[(i * 3 + j) * 3 + 0] = 0;
          normals[(i * 3 + j) * 3 + 1] = 1;
          normals[(i * 3 + j) * 3 + 2] = 0;
          color = 1000 + 255 * (positions[(i * 3 + j) * 3 + 2] - z_min) / (z_max - z_min);
          gr_inqcolor(color, &rgb);
          colors[(i * 3 + j) * 3 + 0] = (float)(rgb & 0xff) / 255;
          colors[(i * 3 + j) * 3 + 1] = (float)((rgb >> 8) & 0xff) / 255;
          colors[(i * 3 + j) * 3 + 2] = (float)((rgb >> 16) & 0xff) / 255;
        }
    }
  mesh = 0;
  gr3_createmesh(&mesh, 3 * n, positions, normals, colors);
  free(normals);
  free(colors);

  if (gr3_geterror(0, NULL, NULL)) return;
  gr3_drawsurface(mesh);
  if (gr3_geterror(0, NULL, NULL)) return;
  gr3_deletemesh(mesh);
  if (gr3_geterror(0, NULL, NULL)) return;
  gr_inqwindow(&window.x_min, &window.x_max, &window.y_min, &window.y_max);
  scale = 0;
  gr_inqscale(&scale);
  if (scale & OPTION_FLIP_X)
    {
      double tmp = window.x_min;
      window.x_min = window.x_max;
      window.x_max = tmp;
    }
  if (scale & OPTION_FLIP_Y)
    {
      double tmp = window.y_min;
      window.y_min = window.y_max;
      window.y_max = tmp;
    }

  /* TODO: inquire the required resolution */
  gr3_drawimage((float)window.x_min, (float)window.x_max, (float)window.y_min, (float)window.y_max, 500, 500,
                GR3_DRAWABLE_GKS);
  if (gr3_geterror(0, NULL, NULL)) return;
}

/*!
 * Draw volume data using the given algorithm and apply the current GR colormap.
 *
 * \param [in]     nx         number of points in x-direction
 * \param [in]     ny         number of points in y-direction
 * \param [in]     nz         number of points in z-direction
 * \param [in]     data       an array of shape nx * ny * nz containing the intensities for each point
 * \param [in]     algorithm  the algorithm to reduce the volume data
 * \param [in,out] dmin_ptr   The variable this parameter points at will be used as minimum data value when applying the
 *                            colormap. If it is negative, the variable will be set to the actual occuring minimum and
 *                            that value will be used instead. If dmin_ptr is NULL, it will be ignored.
 * \param [in,out] dmax_ptr   The variable this parameter points at will be used as maximum data value when applying the
 *                            colormap. If it is negative, the variable will be set to the actual occuring maximum and
 *                            that value will be used instead. If dmax_ptr is NULL, it will be ignored.
 *
 * \verbatim embed:rst:leading-asterisk
 *
 * Available algorithms are:
 *
 * +---------------------+---+-----------------------------+
 * |GR_VOLUME_EMISSION   |  0|emission model               |
 * +---------------------+---+-----------------------------+
 * |GR_VOLUME_ABSORPTION |  1|absorption model             |
 * +---------------------+---+-----------------------------+
 * |GR_VOLUME_MIP        |  2|maximum intensity projection |
 * +---------------------+---+-----------------------------+
 *
 * \endverbatim
 */
GR3API void gr_volume(int nx, int ny, int nz, double *data, int algorithm, double *dmin_ptr, double *dmax_ptr)
{
#if defined(GR3_CAN_USE_VBO) && (defined(GL_ARB_framebuffer_object) || defined(GL_EXT_framebuffer_object))
  double xmin, ymin, xmax, ymax, zmin, zmax;
  int rotation, tilt, scale;
  int projection_type;
  double min, max;
  int i;
  int *color_data, *colormap;
  int first_color, last_color;
  GLfloat fovy, zNear, zFar, aspect, tfov2;
  GLfloat *pixel_data, *fdata;
  GLsizei vertex_shader_source_lines, fragment_shader_source_lines;
  GLfloat grmatrix[16], grviewmatrix[16], projection_matrix[16];
  GLint width, height;
  GLfloat camera_direction[3];
  GLint nmax, success;
  GLuint vertex_shader, fragment_shader, program;
  GLuint vbo, texture, framebuffer, framebuffer_texture;
  GLint previous_viewport[4];
  GLint previous_framebuffer_binding;
  GLboolean previous_cull_face_state;
  GLint previous_cull_face_mode;
  GLfloat previous_clear_color[4];

  const float vertices[] = {
      -1, -1, -1, -1, -1, +1, -1, +1, -1, -1, +1, -1, -1, -1, +1, -1, +1, +1, /* yz-plane, negative x */
      +1, -1, -1, +1, +1, -1, +1, -1, +1, +1, -1, +1, +1, +1, -1, +1, +1, +1, /* yz-plane, positive x */
      -1, -1, -1, +1, -1, -1, -1, -1, +1, -1, -1, +1, +1, -1, -1, +1, -1, +1, /* xz-plane, negative y */
      -1, +1, -1, -1, +1, +1, +1, +1, -1, +1, +1, -1, -1, +1, +1, +1, +1, +1, /* xz-plane, positive y*/
      -1, -1, -1, -1, +1, -1, +1, -1, -1, +1, -1, -1, -1, +1, -1, +1, +1, -1, /* xy-plane, negatize z*/
      -1, -1, +1, +1, -1, +1, -1, +1, +1, -1, +1, +1, +1, -1, +1, +1, +1, +1  /* xz-plane, positive z*/
  };

  const char *vertex_shader_source[] = {
      "#version 120\n",
      "\n",
      "attribute vec3 position;\n",
      "attribute vec3 normal;\n",
      "varying vec3 vf_tex_coord;\n",
      "varying vec3 vf_camera_direction;\n",
      "uniform vec3 camera_direction;\n",
      "uniform vec3 camera_position;\n",
      "uniform mat4 model;\n",
      "uniform mat4 view;\n",
      "uniform mat4 projection;\n",
      "\n",
      "void main() {\n",
      "    vf_camera_direction = float(abs(projection[2][3]) < 0.5) * (transpose(view)*vec4(camera_direction, 0)).xyz ",
      "    + float(abs(projection[2][3]) > 0.5) * (position - camera_position);\n",
      "    vf_tex_coord = position*0.5+vec3(0.5);\n",
      "    gl_Position = projection*view*vec4(position, 1.0);\n",
      "}\n",
  };

  const char *fragment_shader_source[] = {
      "#version 120\n",
      "\n",
      "varying vec3 vf_tex_coord;\n",
      "varying vec3 vf_camera_position;\n",
      "varying vec3 vf_camera_direction;\n",
      "\n",
      "uniform int n;\n",
      "uniform sampler3D tex;\n",
      "\n",
      "float transfer_function(float step_length, float tex_val, float current_value);\n",
      "float initial_value();\n",
      "\n",
      "void main() {\n",
      "    vec3 camera_dir = normalize(vf_camera_direction);\n",
      "\n",
      "    float result = initial_value();\n",
      "    int n_samples = int(max(1000, sqrt(3)*n));\n",
      "    float step_length = sqrt(3.0) / n_samples;\n",
      "    vec3 tex_coord = vf_tex_coord;\n",
      "    for (int i = 0; i <= n_samples; i++) {\n",
      "        float tex_val = max(0, texture3D(tex, tex_coord).r);\n",
      "        tex_coord += camera_dir * step_length;\n",
      "        result = transfer_function(step_length, tex_val, result);\n",
      "        if (any(greaterThan(tex_coord, vec3(1.0)))) break;\n",
      "        if (any(lessThan(tex_coord, vec3(0.0)))) break;\n",
      "    }\n",
      "    gl_FragColor.r = 1+result;\n",
      "}\n",
      NULL};

  const char *transfer_functions[] = {
      /* emission */
      "float initial_value() {\n"
      "   return 0.0;\n"
      "}\n"
      "float transfer_function(float step_length, float tex_value, float current_value) {\n"
      "   return current_value + step_length * tex_value;\n"
      "}\n",
      /* absorption */
      "float initial_value() {\n"
      "   return 1.0;\n"
      "}\n"
      "float transfer_function(float step_length, float tex_value, float current_value) {\n"
      "   return current_value * exp(-step_length * tex_value);\n"
      "}\n",
      /* maximum intensity projection */
      "float initial_value() {\n"
      "   return 0.0;\n"
      "}\n"
      "float transfer_function(float step_length, float tex_value, float current_value) {\n"
      "   return max(current_value, tex_value);\n"
      "}\n"};

  if (nx <= 0 || ny <= 0 || nz <= 0)
    {
      fprintf(stderr, "Invalid dimensions in gr_volume.\n");
      return;
    }

  if (algorithm < 0 || algorithm > 2)
    {
      fprintf(stderr, "Invalid algorithm for gr_volume.\n");
      return;
    }

  gr3_getrenderpathstring(); /* Initializes GR3 if it is not initialized yet */

  if (context_struct_.use_software_renderer)
    {
      double min_val[3] = {-1, -1, -1};
      double max_val[3] = {1, 1, 1};
      gr_cpubasedvolume(nx, ny, nz, data, algorithm, dmin_ptr, dmax_ptr, min_val, max_val);
      return;
    }
  else
    {
      int border, max_threads;
      gr_inqvolumeflags(&border, &max_threads, &height, &width);
      gr_inqprojectiontype(&projection_type);
      if (projection_type == GR_PROJECTION_DEFAULT)
        {
          aspect = 1.0;
        }
      else
        {
          double vpxmin, vpxmax, vpymin, vpymax;
          gr_inqviewport(&vpxmin, &vpxmax, &vpymin, &vpymax);
          aspect = fabs((vpxmax - vpxmin) / (vpymax - vpymin));
          if (aspect > 1)
            {
              width = (int)(width * aspect);
            }
          else
            {
              height = (int)(height / aspect);
            }
        }
    }

  pixel_data = malloc(width * height * sizeof(float));
  assert(pixel_data);
  fdata = malloc(nx * ny * nz * sizeof(float));
  assert(fdata);
  gr_inqcolormapinds(&first_color, &last_color);
  colormap = malloc((last_color - first_color + 1) * sizeof(int));
  assert(colormap);
  color_data = malloc(width * height * sizeof(int));
  assert(color_data);

  for (i = 0; i < nx * ny * nz; i++)
    {
      fdata[i] = (float)data[i];
    }

  /* Add transfer function implementation to fragment shader source */
  vertex_shader_source_lines = sizeof(vertex_shader_source) / sizeof(vertex_shader_source[0]);
  fragment_shader_source_lines = sizeof(fragment_shader_source) / sizeof(fragment_shader_source[0]);
  fragment_shader_source[fragment_shader_source_lines - 1] = transfer_functions[algorithm];

  /* Create and compile shader, link shader program */
  vertex_shader = glCreateShader(GL_VERTEX_SHADER);
  fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(vertex_shader, vertex_shader_source_lines, vertex_shader_source, NULL);
  glCompileShader(vertex_shader);
  glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
  if (!success)
    {
      fprintf(stderr, "Failed to compile vertex shader in gr_volume.\n");
      return;
    }
  glShaderSource(fragment_shader, fragment_shader_source_lines, fragment_shader_source, NULL);
  glCompileShader(fragment_shader);
  glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);
  if (!success)
    {
      fprintf(stderr, "Failed to compile fragment shader in gr_volume.\n");
      return;
    }
  program = glCreateProgram();
  glAttachShader(program, vertex_shader);
  glAttachShader(program, fragment_shader);
  glLinkProgram(program);
  glGetProgramiv(program, GL_LINK_STATUS, &success);
  if (!success)
    {
      fprintf(stderr, "Failed to link shader program in gr_volume.\n");
      return;
    }
  glDeleteShader(vertex_shader);
  glDeleteShader(fragment_shader);
  glUseProgram(program);

  memset(projection_matrix, 0, 16 * sizeof(GLfloat));
  if (projection_type == GR_PROJECTION_DEFAULT)
    {
      /* Set projection parameter like in `gr3_drawmesh_grlike` and create (orthographic) projection matrix */
      fovy = 90.0f;
      zNear = 1.0f;
      zFar = 200.0f;
      tfov2 = (GLfloat)tan(fovy * M_PI / 360.0);
      GLfloat right = zNear * aspect * tfov2;
      GLfloat top = zNear * tfov2;

      projection_matrix[0 + 0 * 4] = (GLfloat)1.0 / right; /* left = -right */
      projection_matrix[0 + 3 * 4] = 0.0;
      projection_matrix[1 + 1 * 4] = (GLfloat)1.0 / top; /* bottom = -top */
      projection_matrix[1 + 3 * 4] = 0.0;
      projection_matrix[2 + 2 * 4] = (GLfloat)-2.0 / (zFar - zNear);
      projection_matrix[2 + 3 * 4] = -(zFar + zNear) / (zFar - zNear);
      projection_matrix[3 + 3 * 4] = 1.0;
    }
  else if (projection_type == GR_PROJECTION_ORTHOGRAPHIC)
    {
      double near, far, left, right, bottom, top;
      gr_inqorthographicprojection(&left, &right, &bottom, &top, &near, &far);
      if (aspect > 1)
        {
          right *= aspect;
          left *= aspect;
        }
      else
        {
          top /= aspect;
          bottom /= aspect;
        }

      projection_matrix[0 + 0 * 4] = (GLfloat)(2. / (right - left));
      projection_matrix[0 + 3 * 4] = (GLfloat)(-(left + right) / (right - left));
      projection_matrix[1 + 1 * 4] = (GLfloat)(2. / (top - bottom));
      projection_matrix[1 + 3 * 4] = (GLfloat)(-(bottom + top) / (top - bottom));
      projection_matrix[2 + 2 * 4] = (GLfloat)(-2. / (far - near));
      projection_matrix[2 + 3 * 4] = (GLfloat)(-(far + near) / (far - near));
      projection_matrix[3 + 3 * 4] = 1;
    }
  else if (projection_type == GR_PROJECTION_PERSPECTIVE)
    {
      double near, far, fov;
      gr_inqperspectiveprojection(&near, &far, &fov);

      if (aspect >= 1)
        {
          projection_matrix[0 + 0 * 4] = (GLfloat)(cos(fov * M_PI / 360) / sin(fov * M_PI / 360) / aspect);
          projection_matrix[1 + 1 * 4] = (GLfloat)(cos(fov * M_PI / 360) / sin(fov * M_PI / 360));
        }
      else
        {
          projection_matrix[0 + 0 * 4] = (GLfloat)(cos(fov * M_PI / 360) / sin(fov * M_PI / 360));
          projection_matrix[1 + 1 * 4] = (GLfloat)(cos(fov * M_PI / 360) / sin(fov * M_PI / 360) * aspect);
        }
      projection_matrix[2 + 2 * 4] = (GLfloat)((far + near) / (near - far));
      projection_matrix[2 + 3 * 4] = (GLfloat)(2 * far * near / (near - far));
      projection_matrix[3 + 2 * 4] = -1;
    }

  /* Create view matrix */
  double camera_pos[3];
  if (projection_type == GR_PROJECTION_DEFAULT)
    {
      gr_inqspace(&zmin, &zmax, &rotation, &tilt);
      gr3_grtransformation_(grmatrix, rotation, tilt);
      gr3_identity_(grviewmatrix);
      grviewmatrix[2 + 3 * 4] = -4;
      gr3_matmul_(grviewmatrix, grmatrix);
    }
  else if (projection_type == GR_PROJECTION_PERSPECTIVE || projection_type == GR_PROJECTION_ORTHOGRAPHIC)
    {
      memset(grviewmatrix, 0, 16 * sizeof(GLfloat));
      double up[3];
      double focus_point[3];

      gr_inqtransformationparameters(&camera_pos[0], &camera_pos[1], &camera_pos[2], &up[0], &up[1], &up[2],
                                     &focus_point[0], &focus_point[1], &focus_point[2]);

      /* direction between camera and focus point */
      double F[3] = {focus_point[0] - camera_pos[0], focus_point[1] - camera_pos[1], focus_point[2] - camera_pos[2]};
      double norm_func = sqrt(F[0] * F[0] + F[1] * F[1] + F[2] * F[2]);
      double f[3] = {F[0] / norm_func, F[1] / norm_func, F[2] / norm_func};
      double s_deri[3];
      for (i = 0; i < 3; i++) /* f cross up */
        {
          s_deri[i] = f[(i + 1) % 3] * up[(i + 2) % 3] - up[(i + 1) % 3] * f[(i + 2) % 3];
        }
      double s_norm = sqrt(s_deri[0] * s_deri[0] + s_deri[1] * s_deri[1] + s_deri[2] * s_deri[2]);
      double s[3] = {s_deri[0] / s_norm, s_deri[1] / s_norm, s_deri[2] / s_norm};

      /* transformation matrix */
      grviewmatrix[0 + 0 * 4] = (GLfloat)s[0];
      grviewmatrix[0 + 1 * 4] = (GLfloat)s[1];
      grviewmatrix[0 + 2 * 4] = (GLfloat)s[2];
      grviewmatrix[0 + 3 * 4] = (GLfloat)(-camera_pos[0] * s[0] - camera_pos[1] * s[1] - camera_pos[2] * s[2]);
      grviewmatrix[1 + 0 * 4] = (GLfloat)up[0];
      grviewmatrix[1 + 1 * 4] = (GLfloat)up[1];
      grviewmatrix[1 + 2 * 4] = (GLfloat)up[2];
      grviewmatrix[1 + 3 * 4] = (GLfloat)(-camera_pos[0] * up[0] - camera_pos[1] * up[1] - camera_pos[2] * up[2]);
      grviewmatrix[2 + 0 * 4] = (GLfloat)-f[0];
      grviewmatrix[2 + 1 * 4] = (GLfloat)-f[1];
      grviewmatrix[2 + 2 * 4] = (GLfloat)-f[2];
      grviewmatrix[2 + 3 * 4] = (GLfloat)(camera_pos[0] * f[0] + camera_pos[1] * f[1] + camera_pos[2] * f[2]);
      grviewmatrix[3 + 3 * 4] = 1;

      if (projection_type == GR_PROJECTION_PERSPECTIVE)
        {
          camera_direction[0] = focus_point[1] - camera_pos[1];
          camera_direction[1] = focus_point[1] - camera_pos[1];
          camera_direction[2] = focus_point[2] - camera_pos[2];
        }
    }

  /* Buffer Vertices */
  glGenBuffers(1, &vbo);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  /* Buffer input data in a 3D float texture */
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_3D, texture);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
  glTexImage3D(GL_TEXTURE_3D, 0, GL_R32F, nx, ny, nz, 0, GL_RED, GL_FLOAT, fdata);
  free(fdata);

  /* Create framebuffer object and bind 2D float texture as COLOR_ATTACHMENT0 to it */
  glGenTextures(1, &framebuffer_texture);
  glBindTexture(GL_TEXTURE_2D, framebuffer_texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, width, height, 0, GL_RED, GL_FLOAT, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glGetIntegerv(GL_FRAMEBUFFER_BINDING, &previous_framebuffer_binding);
#ifdef GL_ARB_framebuffer_object
  glGenFramebuffers(1, &framebuffer);
  glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, framebuffer_texture, 0);
#else
  glGenFramebuffersEXT(1, &framebuffer);
  glBindFramebufferEXT(GL_FRAMEBUFFER, framebuffer);
  glFramebufferTexture2DEXT(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, framebuffer_texture, 0);
#endif

  glGetIntegerv(GL_VIEWPORT, previous_viewport);
  glViewport(0, 0, width, height);

  glGetBooleanv(GL_CULL_FACE, &previous_cull_face_state);
  glGetIntegerv(GL_CULL_FACE_MODE, &previous_cull_face_mode);
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);
  if (nx > ny && nx > nz)
    {
      nmax = nx;
    }
  else if (ny > nz)
    {
      nmax = ny;
    }
  else
    {
      nmax = nz;
    }

  glActiveTexture(GL_TEXTURE0);
  glGetFloatv(GL_COLOR_CLEAR_VALUE, previous_clear_color);
  glClearColor(0, 0, 0, 1);
  glClear(GL_COLOR_BUFFER_BIT);

  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)NULL);
  glEnableVertexAttribArray(0);

  if (projection_type != GR_PROJECTION_PERSPECTIVE)
    {
      camera_direction[0] = context_struct_.center_x - context_struct_.camera_x;
      camera_direction[1] = context_struct_.center_y - context_struct_.camera_y;
      camera_direction[2] = context_struct_.center_z - context_struct_.camera_z;
    }

  glUniformMatrix4fv(glGetUniformLocation(program, "view"), 1, GL_FALSE, grviewmatrix);
  glUniformMatrix4fv(glGetUniformLocation(program, "projection"), 1, GL_FALSE, projection_matrix);
  glUniform3f(glGetUniformLocation(program, "camera_direction"), camera_direction[0], camera_direction[1],
              camera_direction[2]);
  glUniform3f(glGetUniformLocation(program, "camera_position"), camera_pos[0], camera_pos[1], camera_pos[2]);
  glUniform1i(glGetUniformLocation(program, "n"), nmax);

  glDrawArrays(GL_TRIANGLES, 0, sizeof(vertices) / sizeof(vertices[0]));
  for (i = 0; i < height; i++)
    {
      glPixelStorei(GL_PACK_ROW_LENGTH, width);
      glReadPixels(0, i, width, 1, GL_RED, GL_FLOAT, pixel_data + i * width);
    }

  if (dmin_ptr && *dmin_ptr >= 0)
    {
      min = *dmin_ptr;
    }
  else
    {
      min = 1.0;
      for (i = 0; i < width * height; i++)
        {
          if (pixel_data[i] < min && pixel_data[i] >= 1)
            {
              min = pixel_data[i];
            }
        }
      min -= 1;
      if (dmin_ptr)
        {
          *dmin_ptr = min;
        }
    }

  if (dmax_ptr && *dmax_ptr >= 0)
    {
      max = *dmax_ptr;
    }
  else
    {
      max = 1.0;
      for (i = 0; i < width * height; i++)
        {
          if (pixel_data[i] > max)
            {
              max = pixel_data[i];
            }
        }
      max -= 1;
      if (dmax_ptr)
        {
          *dmax_ptr = max;
        }
    }

  for (i = first_color; i <= last_color; i++)
    {
      gr_inqcolor(i, colormap + i - first_color);
    }

  for (i = 0; i < width * height; i++)
    {
      if (pixel_data[i] < 1)
        {
          color_data[i] = 0;
        }
      else
        {
          int val = (int)((last_color - first_color) * ((pixel_data[i] - 1) - min) / (max - min));
          if (val < 0)
            {
              val = 0;
            }
          else if (val > last_color - first_color)
            {
              val = last_color - first_color;
            }
          color_data[i] = (255u << 24) + colormap[val];
        }
    }
  free(pixel_data);
  free(colormap);

  gr_inqwindow(&xmin, &xmax, &ymin, &ymax);
  gr_inqscale(&scale);
  if (scale & OPTION_FLIP_X)
    {
      double tmp = xmin;
      xmin = xmax;
      xmax = tmp;
    }
  if (scale & OPTION_FLIP_Y)
    {
      double tmp = ymin;
      ymin = ymax;
      ymax = tmp;
    }

  gr_drawimage(xmin, xmax, ymax, ymin, width, height, color_data, 0);

  free(color_data);

  /* Cleanup and restore previous GL state */
  glViewport(previous_viewport[0], previous_viewport[1], previous_viewport[2], previous_viewport[3]);
  if (!previous_cull_face_state)
    {
      glDisable(GL_CULL_FACE);
    }
  glCullFace(previous_cull_face_mode);
  glClearColor(previous_clear_color[0], previous_clear_color[1], previous_clear_color[2], previous_clear_color[3]);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindTexture(GL_TEXTURE_3D, 0);
  glBindTexture(GL_TEXTURE_2D, 0);

#ifdef GL_ARB_framebuffer_object
  glBindFramebuffer(GL_FRAMEBUFFER, previous_framebuffer_binding);
  glDeleteFramebuffers(1, &framebuffer);
#else
  glBindFramebufferEXT(GL_FRAMEBUFFER, previous_framebuffer_binding);
  glDeleteFramebuffersEXT(1, &framebuffer);
#endif
  glDeleteBuffers(1, &vbo);
  glDeleteTextures(1, &framebuffer_texture);
  glDeleteTextures(1, &texture);
  glDeleteProgram(program);
#else
  fprintf(stderr, "gr_volume support not compiled in.\n");
#endif
}
