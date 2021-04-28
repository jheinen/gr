#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "gr.h"
#include "gr3.h"

int main(void)
{
  int mesh = 0;
  int nx = 93;
  int ny = 64;
  int nz = 64;
  unsigned int stride_x = 1;
  unsigned int stride_y = nx;
  unsigned int stride_z = nx * ny;
  double step_x = 2.0 / (nx - 1);
  double step_y = 2.0 * ny / nx / (ny - 1) * 1.5;
  double step_z = 2.0 * nz / nx / (nz - 1) * 1.5;
  double offset_x = -step_x * (nx - 1) / 2;
  double offset_y = -step_y * (ny - 1) / 2;
  double offset_z = -step_z * (nz - 1) / 2;
  float position[3] = {0, 0, 0};
  float direction[3] = {0, 0, 1};
  float up[3] = {0, 1, 0};
  float color[3] = {1, 1, 1};
  float scales[3] = {1, 1, 1};
  GR3_MC_DTYPE *data = malloc(nx * ny * nz * sizeof(GR3_MC_DTYPE));
  FILE *fp = fopen("mri.raw", "rb");
  assert(fp);
  assert(data);
  fread(data, 2, nx * ny * nz, fp);
  fclose(fp);
  {
    int ix, iy, iz;
    float input_max = 2000;
    GR3_MC_DTYPE dtype_max = (1UL << (8 * sizeof(GR3_MC_DTYPE))) - 1;
    for (iz = 0; iz < nz; iz++)
      {
        for (iy = 0; iy < ny; iy++)
          {
            for (ix = 0; ix < nx; ix++)
              {
                if (data[nx * nz * iz + nx * iy + ix] > input_max)
                  {
                    data[nx * nz * iz + nx * iy + ix] = input_max;
                  }
                data[nx * ny * iz + nx * iy + ix] =
                    (GR3_MC_DTYPE)(data[nx * nz * iz + nx * iy + ix] / input_max * dtype_max);
              }
          }
      }
  }
  gr_setviewport(0, 1, 0, 1);
  gr_setcolormap(1);
  gr3_init(NULL);

  gr3_createisosurfacemesh(&mesh, data, 40000, nx, ny, nz, stride_x, stride_y, stride_z, step_x, step_y, step_z,
                           offset_x, offset_y, offset_z);
  gr3_cameralookat(-3, 3, -4, 0, 0, 0, -1, 0, 0);
  {
    int i;
    for (i = 0; i < 200; i++)
      {
        gr3_clear();
        gr3_drawmesh(mesh, 1, position, direction, up, color, scales);
        gr3_drawxslicemesh(data, i * nx / 199, nx, ny, nz, stride_x, stride_y, stride_z, step_x, step_y, step_z,
                           offset_x, offset_y, offset_z);
        gr3_drawyslicemesh(data, i * ny / 199, nx, ny, nz, stride_x, stride_y, stride_z, step_x, step_y, step_z,
                           offset_x, offset_y, offset_z);
        gr3_drawzslicemesh(data, i * nz / 199, nx, ny, nz, stride_x, stride_y, stride_z, step_x, step_y, step_z,
                           offset_x, offset_y, offset_z);
        gr_clearws();
        gr3_drawimage(0, 1, 0, 1, 500, 500, GR3_DRAWABLE_GKS);
        gr_updatews();
      }
  }
  gr_setcolormap(19);
  {
    int i;
    for (i = 299; i >= 0; i--)
      {
        gr3_clear();
        gr3_drawmesh(mesh, 1, position, direction, up, color, scales);
        gr3_drawxslicemesh(data, i * nx / 299, nx, ny, nz, stride_x, stride_y, stride_z, step_x, step_y, step_z,
                           offset_x, offset_y, offset_z);
        gr_clearws();
        gr3_drawimage(0, 1, 0, 1, 500, 500, GR3_DRAWABLE_GKS);
        gr_updatews();
      }
    for (i = 0; i < 300; i++)
      {
        gr3_clear();
        gr3_drawmesh(mesh, 1, position, direction, up, color, scales);
        gr3_drawyslicemesh(data, i * ny / 299, nx, ny, nz, stride_x, stride_y, stride_z, step_x, step_y, step_z,
                           offset_x, offset_y, offset_z);
        gr_clearws();
        gr3_drawimage(0, 1, 0, 1, 500, 500, GR3_DRAWABLE_GKS);
        gr_updatews();
      }
    for (i = 299; i >= 0; i--)
      {
        gr3_clear();
        gr3_drawmesh(mesh, 1, position, direction, up, color, scales);
        gr3_drawzslicemesh(data, i * nz / 299, nx, ny, nz, stride_x, stride_y, stride_z, step_x, step_y, step_z,
                           offset_x, offset_y, offset_z);
        gr_clearws();
        gr3_drawimage(0, 1, 0, 1, 500, 500, GR3_DRAWABLE_GKS);
        gr_updatews();
      }
  }
  gr3_deletemesh(mesh);
  free(data);
  gr3_terminate();
  return 0;
}
