#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "gkscore.h"
#include "gks.h"

#ifndef INFINITY
#define INFINITY (1.0 / 0.0)
#endif
#ifndef M_PI
#define M_PI (3.141592653589793)
#endif
#ifndef round
#define round(x) ((x) < 0 ? ceil((x)-.5) : floor((x) + .5))
#endif

static double lanczos(double x, int a)
{
  if (x == 0.0)
    {
      return 1.0;
    }
  else if (-a < x && x < a)
    {
      return (a * sin(M_PI * x) * sin(M_PI * x / a)) / (x * x * M_PI * M_PI);
    }
  return 0.0;
}

static double integrate_box(double left, double right, int width)
{
  if (left > width / 2.0)
    {
      return 0.0;
    }
  if (right < -width / 2.0)
    {
      return 0.0;
    }
  if (left < -width / 2.0)
    {
      left = -width / 2.0;
    }
  if (right > width / 2.0)
    {
      right = width / 2.0;
    }
  return (right - left) / width;
}

static double calculate_lanczos_factor(double source_position, double target_position, int a)
{
  return lanczos(source_position - target_position, a);
}

static double calculate_linear_factor(double source_position, double target_position, int a)
{
  return integrate_box(source_position - target_position - 0.5, source_position - target_position + 0.5, a);
}

static double *calculate_resampling_factors(size_t source_size, size_t target_size, int a, int flip,
                                            double (*factor_func)(double, double, int))
{
  size_t i;
  size_t i_flipped;
  int j;
  double *factors;
  int num_steps;
  if (source_size > target_size)
    {
      num_steps = (int)ceil((double)source_size / target_size * a) * 2;
    }
  else
    {
      num_steps = a * 2;
    }
  factors = (double *)gks_malloc((int)sizeof(double) * (int)target_size * num_steps);
  for (i = 0; i < target_size; i++)
    {
      if (flip)
        {
          i_flipped = target_size - 1 - i;
        }
      else
        {
          i_flipped = i;
        }
      double sum = 0.0;
      double target_position;
      int source_index_offset;

      if (source_size > target_size)
        {
          target_position = i_flipped;
          source_index_offset = (int)ceil((double)i_flipped / (double)(target_size - 1) * (double)source_size - 0.5 -
                                          (double)source_size / target_size * a);
        }
      else
        {
          target_position = i_flipped / (double)(target_size - 1) * source_size - 0.5;
          source_index_offset = (int)floor(target_position - (a - 1));
        }
      for (j = 0; j < num_steps; j++)
        {
          int source_index = source_index_offset + j;
          if (source_index < 0)
            {
              continue;
            }
          if (source_index > source_size - 1)
            {
              break;
            }

          double source_position;
          if (source_size > target_size)
            {
              source_position = (source_index_offset + j + 0.5) / (double)source_size * (double)(target_size - 1);
            }
          else
            {
              source_position = source_index;
            }
          double factor = factor_func(source_position, target_position, a);
          sum += factor;
          factors[i * num_steps + j] = factor;
        }
      for (j = 0; j < num_steps; j++)
        {
          factors[i * num_steps + j] /= sum;
        }
    }
  return factors;
}

static void resample_horizontal_rgba(const unsigned char *source_image, double *target_image, size_t source_width,
                                     size_t source_height, size_t target_width, size_t stride, int a, int flip,
                                     double (*factor_func)(double, double, int))
{
  size_t ix;
  size_t ix_flipped;
  size_t iy;
  int i, j;
  int num_steps;
  double *factors;
  if (source_width > target_width)
    {
      num_steps = (int)ceil((double)source_width / target_width * a) * 2;
    }
  else
    {
      num_steps = a * 2;
    }
  factors = calculate_resampling_factors(source_width, target_width, a, flip, factor_func);

  for (iy = 0; iy < source_height; iy++)
    {
      for (ix = 0; ix < target_width; ix++)
        {
          int source_index_offset;

          if (flip)
            {
              ix_flipped = (int)target_width - 1 - ix;
            }
          else
            {
              ix_flipped = ix;
            }

          if (source_width > target_width)
            {
              source_index_offset = (int)ceil((double)ix_flipped / (double)(target_width - 1) * (double)source_width -
                                              0.5 - (double)source_width / (double)target_width * a);
            }
          else
            {
              source_index_offset =
                  (int)floor((double)ix_flipped / (double)(target_width - 1) * (double)source_width + 0.5 - a);
            }
          for (i = 0; i < num_steps; i++)
            {
              int source_index = source_index_offset + i;
              if (source_index < 0)
                {
                  continue;
                }
              if (source_index > source_width - 1)
                {
                  break;
                }
              double factor = factors[ix * num_steps + i];
              for (j = 0; j < 4; j++)
                {
                  target_image[(iy * target_width + ix) * 4 + j] +=
                      source_image[(iy * stride + source_index) * 4 + j] * factor;
                }
            }
        }
    }
  gks_free(factors);
}

static void resample_vertical_rgba(const double *source_image, unsigned char *target_image, size_t source_width,
                                   size_t source_height, size_t target_height, size_t stride, int a, int flip,
                                   double (*factor_func)(double, double, int))
{
  size_t ix;
  size_t iy;
  size_t iy_flipped;
  int i, j;
  int num_steps;
  double *factors;
  if (source_height > target_height)
    {
      num_steps = (int)ceil((double)source_height / target_height * a) * 2;
    }
  else
    {
      num_steps = 2 * a;
    }
  factors = calculate_resampling_factors(source_height, target_height, a, flip, factor_func);

  for (ix = 0; ix < source_width; ix++)
    {
      for (iy = 0; iy < target_height; iy++)
        {
          int source_index_offset;

          if (flip)
            {
              iy_flipped = (int)target_height - 1 - iy;
            }
          else
            {
              iy_flipped = iy;
            }

          if (source_height > target_height)
            {
              source_index_offset = (int)ceil((double)iy_flipped / (double)(target_height - 1) * (double)source_height -
                                              0.5 - (double)source_height / target_height * a);
            }
          else
            {
              source_index_offset =
                  (int)floor((double)iy_flipped / (double)(target_height - 1) * (double)source_height + 0.5 - a);
            }
          double result[4] = {0};
          for (i = 0; i < num_steps; i++)
            {
              int source_index = source_index_offset + i;
              if (source_index < 0)
                {
                  continue;
                }
              if (source_index > source_height - 1)
                {
                  break;
                }
              double factor = factors[iy * num_steps + i];
              for (j = 0; j < 4; j++)
                {
                  result[j] += source_image[(source_index * stride + ix) * 4 + j] * factor;
                }
            }
          for (j = 0; j < 4; j++)
            {
              if (result[j] > 255)
                {
                  result[j] = 255;
                }
              else if (result[j] < 0)
                {
                  result[j] = 0;
                }
              target_image[(iy * source_width + ix) * 4 + j] = (unsigned char)round(result[j]);
            }
        }
    }
  gks_free(factors);
}


static void resample_rgba_nearest(const unsigned char *source_image, unsigned char *target_image, size_t source_width,
                                  size_t source_height, size_t target_width, size_t target_height, size_t stride,
                                  int flip_x, int flip_y)
{
  size_t ix, iy, ix_flipped, iy_flipped;
  for (iy = 0; iy < target_height; iy++)
    {
      iy_flipped = source_height * iy / target_height;
      if (flip_y)
        {
          iy_flipped = source_height - 1 - iy_flipped;
        }
      for (ix = 0; ix < target_width; ix++)
        {
          ix_flipped = source_width * ix / target_width;
          if (flip_x)
            {
              ix_flipped = source_width - 1 - ix_flipped;
            }

          target_image[(iy * target_width + ix) * 4 + 0] = source_image[(iy_flipped * stride + ix_flipped) * 4 + 0];
          target_image[(iy * target_width + ix) * 4 + 1] = source_image[(iy_flipped * stride + ix_flipped) * 4 + 1];
          target_image[(iy * target_width + ix) * 4 + 2] = source_image[(iy_flipped * stride + ix_flipped) * 4 + 2];
          target_image[(iy * target_width + ix) * 4 + 3] = source_image[(iy_flipped * stride + ix_flipped) * 4 + 3];
        }
    }
}

static void resample_horizontal_rgba_nearest(const unsigned char *source_image, double *target_image,
                                             size_t source_width, size_t source_height, size_t target_width,
                                             size_t stride, int flip)
{
  size_t ix, iy, ix_flipped, j;
  for (iy = 0; iy < source_height; iy++)
    {
      for (ix = 0; ix < target_width; ix++)
        {
          ix_flipped = source_width * ix / target_width;
          if (flip)
            {
              ix_flipped = source_width - 1 - ix_flipped;
            }

          for (j = 0; j < 4; j++)
            {
              target_image[(iy * target_width + ix) * 4 + j] = source_image[(iy * stride + ix_flipped) * 4 + j];
            }
        }
    }
}


static void resample_vertical_rgba_nearest(const double *source_image, unsigned char *target_image, size_t source_width,
                                           size_t source_height, size_t target_height, size_t stride, int flip)
{
  size_t ix, iy, iy_flipped, j;
  for (ix = 0; ix < source_width; ix++)
    {
      for (iy = 0; iy < target_height; iy++)
        {
          iy_flipped = source_height * iy / target_height;
          if (flip)
            {
              iy_flipped = source_height - 1 - iy_flipped;
            }


          for (j = 0; j < 4; j++)
            {
              double value = round(source_image[(iy_flipped * stride + ix) * 4 + j]);
              if (value > 255)
                {
                  value = 255;
                }
              if (value < 0)
                {
                  value = 0;
                }
              target_image[(iy * source_width + ix) * 4 + j] = (unsigned char)value;
            }
        }
    }
}

static unsigned int get_default_resampling_method()
{
  unsigned int resample_method = GKS_K_RESAMPLE_NEAREST;
  char *env;
  env = (char *)gks_getenv("GKS_DEFAULT_RESAMPLE_METHOD");
  if (env)
    {
      int i;
      env = gks_strdup(env);
      for (i = 0; env[i]; i++)
        {
          if (0 <= env[i] && env[i] <= 127)
            {
              env[i] = (char)tolower(env[i]);
            }
        }
      if (!strcmp(env, "nearest"))
        {
          resample_method = GKS_K_RESAMPLE_NEAREST;
        }
      else if (!strcmp(env, "linear"))
        {
          resample_method = GKS_K_RESAMPLE_LINEAR;
        }
      else if (!strcmp(env, "lanczos"))
        {
          resample_method = GKS_K_RESAMPLE_LANCZOS;
        }
      else
        {
          gks_perror("Unknown resample method: %s", env);
        }
      gks_free(env);
    }
  return resample_method;
}

/*!
 * Resample an RGBA image using one of several resampling methods.
 *
 * \param[in] source_image RGBA-data of the original image
 * \param[out] target_image RGBA-data for resampled values
 * \param[in] source_width number of columns in source_image
 * \param[in] source_height number of rows in source_image
 * \param[in] target_width number of columns in target_image
 * \param[in] target_height number of rows in target_image
 * \param[in] stride stride of source_image
 * \param[in] flip_x whether the image should also be flipped horizontally
 * \param[in] flip_y whether the image should also be flipped vertically
 * \param[in] resample_method the resample method
 *
 * The available options are:
 *
 * \verbatim embed:rst:leading-asterisk
 *
 * +------------------------+------------+--------------------+
 * |GKS_K_RESAMPLE_DEFAULT  | 0x00000000 |default             |
 * +------------------------+------------+--------------------+
 * |GKS_K_RESAMPLE_NEAREST  | 0x01010101 |nearest neighbour   |
 * +------------------------+------------+--------------------+
 * |GKS_K_RESAMPLE_LINEAR   | 0x02020202 |linear              |
 * +------------------------+------------+--------------------+
 * |GKS_K_RESAMPLE_LANCZOS  | 0x03030303 |Lanczos             |
 * +------------------------+------------+--------------------+
 *
 * \endverbatim
 *
 * Alternatively, combinations of these methods can be selected for horizontal or vertical upsampling or downsampling:
 *
 * \verbatim embed:rst:leading-asterisk
 *
 * +-------------------------------------+------------+----------------------------------------------+
 * | GKS_K_UPSAMPLE_VERTICAL_DEFAULT     | 0x00000000 | default for vertical upsampling              |
 * +-------------------------------------+------------+----------------------------------------------+
 * | GKS_K_UPSAMPLE_HORIZONTAL_DEFAULT   | 0x00000000 | default for horizontal upsampling            |
 * +-------------------------------------+------------+----------------------------------------------+
 * | GKS_K_DOWNSAMPLE_VERTICAL_DEFAULT   | 0x00000000 | default for vertical downsampling            |
 * +-------------------------------------+------------+----------------------------------------------+
 * | GKS_K_DOWNSAMPLE_HORIZONTAL_DEFAULT | 0x00000000 | default for horizontal downsampling          |
 * +-------------------------------------+------------+----------------------------------------------+
 * | GKS_K_UPSAMPLE_VERTICAL_NEAREST     | 0x00000001 | nearest neighbor for vertical upsampling     |
 * +-------------------------------------+------------+----------------------------------------------+
 * | GKS_K_UPSAMPLE_HORIZONTAL_NEAREST   | 0x00000100 | nearest neighbor for horizontal upsampling   |
 * +-------------------------------------+------------+----------------------------------------------+
 * | GKS_K_DOWNSAMPLE_VERTICAL_NEAREST   | 0x00010000 | nearest neighbor for vertical downsampling   |
 * +-------------------------------------+------------+----------------------------------------------+
 * | GKS_K_DOWNSAMPLE_HORIZONTAL_NEAREST | 0x01000000 | nearest neighbor for horizontal downsampling |
 * +-------------------------------------+------------+----------------------------------------------+
 * | GKS_K_UPSAMPLE_VERTICAL_LINEAR      | 0x00000002 | linear for vertical upsampling               |
 * +-------------------------------------+------------+----------------------------------------------+
 * | GKS_K_UPSAMPLE_HORIZONTAL_LINEAR    | 0x00000200 | linear for horizontal upsampling             |
 * +-------------------------------------+------------+----------------------------------------------+
 * | GKS_K_DOWNSAMPLE_VERTICAL_LINEAR    | 0x00020000 | linear for vertical downsampling             |
 * +-------------------------------------+------------+----------------------------------------------+
 * | GKS_K_DOWNSAMPLE_HORIZONTAL_LINEAR  | 0x02000000 | linear for horizontal downsampling           |
 * +-------------------------------------+------------+----------------------------------------------+
 * | GKS_K_UPSAMPLE_VERTICAL_LANCZOS     | 0x00000003 | lanczos for vertical upsampling              |
 * +-------------------------------------+------------+----------------------------------------------+
 * | GKS_K_UPSAMPLE_HORIZONTAL_LANCZOS   | 0x00000300 | lanczos for horizontal upsampling            |
 * +-------------------------------------+------------+----------------------------------------------+
 * | GKS_K_DOWNSAMPLE_VERTICAL_LANCZOS   | 0x00030000 | lanczos for vertical downsampling            |
 * +-------------------------------------+------------+----------------------------------------------+
 * | GKS_K_DOWNSAMPLE_HORIZONTAL_LANCZOS | 0x03000000 | lanczos for horizontal downsampling          |
 * +-------------------------------------+------------+----------------------------------------------+
 *
 * \endverbatim
 */
void gks_resample(const unsigned char *source_image, unsigned char *target_image, size_t source_width,
                  size_t source_height, size_t target_width, size_t target_height, size_t stride, int flip_x,
                  int flip_y, unsigned int resample_method)
{
  double *temp_image;
  const unsigned int resampling_methods[] = {GKS_K_RESAMPLE_DEFAULT, GKS_K_RESAMPLE_NEAREST, GKS_K_RESAMPLE_LINEAR,
                                             GKS_K_RESAMPLE_LANCZOS};
  unsigned int horizontal_resampling_method;
  unsigned int vertical_resampling_method;

  unsigned int vertical_upsampling_method = (resample_method >> 0u) & 0xffu;
  unsigned int horizontal_upsampling_method = (resample_method >> 8u) & 0xffu;
  unsigned int vertical_downsampling_method = (resample_method >> 16u) & 0xffu;
  unsigned int horizontal_downsampling_method = (resample_method >> 24u) & 0xffu;

  if (vertical_upsampling_method > 3)
    {
      gks_perror("Invalid vertical upsampling method.");
      vertical_upsampling_method = 0;
    }
  if (horizontal_upsampling_method > 3)
    {
      gks_perror("Invalid horizontal upsampling method.");
      horizontal_upsampling_method = 0;
    }
  if (vertical_downsampling_method > 3)
    {
      gks_perror("Invalid vertical downsampling method.");
      vertical_downsampling_method = 0;
    }
  if (horizontal_downsampling_method > 3)
    {
      gks_perror("Invalid horizontal downsampling method.");
      horizontal_downsampling_method = 0;
    }

  if (source_width > target_width)
    {
      horizontal_resampling_method = resampling_methods[horizontal_downsampling_method];
    }
  else if (source_width < target_width)
    {
      horizontal_resampling_method = resampling_methods[horizontal_upsampling_method];
    }
  else
    {
      /* No horizontal resampling needed */
      horizontal_resampling_method = GKS_K_RESAMPLE_NEAREST;
    }

  if (source_height > target_height)
    {
      vertical_resampling_method = resampling_methods[vertical_downsampling_method];
    }
  else if (source_height < target_height)
    {
      vertical_resampling_method = resampling_methods[vertical_upsampling_method];
    }
  else
    {
      /* No vertical resampling needed */
      vertical_resampling_method = GKS_K_RESAMPLE_NEAREST;
    }

  if (horizontal_resampling_method == GKS_K_RESAMPLE_DEFAULT)
    {
      horizontal_resampling_method = get_default_resampling_method();
    }
  if (vertical_resampling_method == GKS_K_RESAMPLE_DEFAULT)
    {
      vertical_resampling_method = get_default_resampling_method();
    }

  if (horizontal_resampling_method == GKS_K_RESAMPLE_NEAREST && vertical_resampling_method == GKS_K_RESAMPLE_NEAREST)
    {
      /* Only nearest-neighbor resampling, so no intermediate image is required. */
      resample_rgba_nearest(source_image, target_image, source_width, source_height, target_width, target_height,
                            stride, flip_x, flip_y);
      return;
    }

  temp_image = (double *)gks_malloc((int)sizeof(double) * 4 * (int)target_width * (int)source_height);

  switch (horizontal_resampling_method)
    {
    case GKS_K_RESAMPLE_NEAREST:
      resample_horizontal_rgba_nearest(source_image, temp_image, source_width, source_height, target_width, stride,
                                       flip_x);
      break;
    case GKS_K_RESAMPLE_LINEAR:
      resample_horizontal_rgba(source_image, temp_image, source_width, source_height, target_width, stride, 1, flip_x,
                               calculate_linear_factor);
      break;
    case GKS_K_RESAMPLE_LANCZOS:
      resample_horizontal_rgba(source_image, temp_image, source_width, source_height, target_width, stride, 3, flip_x,
                               calculate_lanczos_factor);
      break;
    default:
      gks_perror("Invalid horizontal resampling method.");
      break;
    }

  switch (vertical_resampling_method)
    {
    case GKS_K_RESAMPLE_NEAREST:
      resample_vertical_rgba_nearest(temp_image, target_image, target_width, source_height, target_height, target_width,
                                     flip_y);
      break;
    case GKS_K_RESAMPLE_LINEAR:
      resample_vertical_rgba(temp_image, target_image, target_width, source_height, target_height, target_width, 1,
                             flip_y, calculate_linear_factor);
      break;
    case GKS_K_RESAMPLE_LANCZOS:
      resample_vertical_rgba(temp_image, target_image, target_width, source_height, target_height, target_width, 3,
                             flip_y, calculate_lanczos_factor);
      break;
    default:
      gks_perror("Invalid vertical resampling method.");
      break;
    }

  gks_free(temp_image);
}
