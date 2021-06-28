/*!\file gr3_sr.c
 *
 * Bugs:
 * -
 */

#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include "gks.h"

#ifdef _MSC_VER
#define NO_THREADS 1
#endif
#ifndef NO_THREADS
#include <pthread.h>
#endif
#include "gr3_internals.h"
#include "gr3_sr.h"
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>
#else
#include <unistd.h>
#endif
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MINTHREE(a, b, c) MIN(MIN(a, b), c)

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MAXTHREE(a, b, c) MAX(MAX(a, b), c)

/* the following macro enables BACKFACE_CULLING */
/*#define BACKFACE_CULLING*/

static int queue_destroy(queue *queue);
static queue *queue_new(void);
static void *queue_dequeue(queue *queue);
static int queue_enqueue(queue *queue, void *data);
static void create_queues_and_pixmaps(int width, int height);
static void merge_pixmaps(int width, int starty, int endy);
static void initialise_consumer(queue *queues[MAX_NUM_THREADS], int height, int width);

static matrix get_projection(int width, int height, float fovy, float zNear, float zFar, int projection_type);
static matrix matrix_perspective_proj(float left, float right, float bottom, float top, float zNear, float zFar);
static matrix matrix_ortho_proj(float left, float right, float bottom, float top, float nearVal, float farVal);
static matrix matrix_viewport_trafo(int width, int height);
static matrix mat_mul_4x4(matrix *a, matrix *b);
static matrix3x3 mat_mul_3x3(matrix3x3 *a, matrix3x3 *b);
static void mat_vec_mul_4x1(matrix *a, vertex_fp *b);
static void mat_vec_mul_3x1(matrix3x3 *a, vector *b);
static void divide_by_w(vertex_fp *v_fp);
static void cross_product(vector *a, vector *b, vector *res);
vector VECTOR3x1_INIT_NUL = {0, 0, 0};
matrix MAT4x4_INIT_NUL = {{0}};
matrix3x3 MAT3x3_INIT_NUL = {{0}};

static void mult_color(color_float *c, float fac, float alpha_fac);
static color color_float_to_color(color_float c);
static color_float linearcombination_color(color_float c1, color_float c2, color_float c3, float fac1, float fac2,
                                           float fac3);

static void mult_vector(vector *v, float fac);
static void normalize_vector(vector *v);
static float dot_vector(vector *v1, vector *v2);
static vector linearcombination(vector *v1, vector *v2, vector *v3, float fac1, float fac2, float fac3);
static float triangle_surface_2d(float dif_a_b_x, float dif_a_b_y, float cy, float cx, float ay, float ax);

static args *malloc_arg(int thread_idx, int mesh, matrix model_view_perspective, matrix viewport,
                        matrix3x3 model_view_3x3, vector light_dir, const float *colors, const float *scales, int width,
                        int height, int id, int idxstart, int idxend, vertex_fp *vertices_fp);
static void *draw_triangle_indexbuffer(void *v_arguments);
static void draw_triangle(unsigned char *pixels, float *dep_buf, int width, int height, vertex_fp *v_fp[3],
                          const float *colors, vector light_dir);
static void draw_triangle_with_edges(unsigned char *pixels, float *dep_buf, int width, int height, vertex_fp *v_fp[3],
                                     color line_color, color fill_color);
static void fill_triangle(unsigned char *pixels, float *dep_buf, int width, int height, const float *colors,
                          vector light_dir, vertex_fp **v_fp_sorted, vertex_fp **v_fp, float A12, float A20, float A01,
                          float B12, float B20, float B01);
static void draw_line(unsigned char *pixels, float *dep_buf, int width, const float *colors, vector light_dir,
                      int startx, int y, int endx, vertex_fp *v_fp[3], float A12, float A20, float A01, float w0,
                      float w1, float w2, float sum_inv);
static void color_pixel(unsigned char *pixels, float *depth_buffer, float depth, int width, int x, int y, color *col);
static color calc_colors(color_float col_one, color_float col_two, color_float col_three, float fac_one, float fac_two,
                         float fac_three, vertex_fp *v_fp[3], const float *colors, vector light_dir);

static int gr3_draw_softwarerendered(queue *queues[MAX_NUM_THREADS], int width, int height);
static void gr3_dodrawmesh_softwarerendered(queue *queues[MAX_NUM_THREADS], int width, int height,
                                            struct _GR3_DrawList_t_ *draw, int id);
static int draw_mesh_softwarerendered(queue *queues[MAX_NUM_THREADS], int mesh, float *model, float *view,
                                      const float *colors_facs, const float *scales, int width, int height, int id,
                                      struct _GR3_DrawList_t_ *draw, int draw_id);
static void downsample(unsigned char *pixels_high, unsigned char *pixels_low, int width, int height, int ssaa_factor);

/* The following variables are essential for the communication of the threads. The Mesh is divided into parts
 * and every thread draws one of them. If a thread is done with its part of a mesh, it increments the value of
 * threads_done. The first lock is for the queue. Every Thread has its own queue, where the mesh parts
 * to be drawn by this thread are enqueued by the main thread. The second mutex lock is for the main
 * thread to wait for finishing the execution of the other threads drawing their mesh parts, so that the merging
 * process can begin. The first condition variable waits for all the threads to finish so that the merging of
 * the different pixmaps the threads have drawn into can begin, the second one waits for the merging process
 * to finish so that the main thread can finally return the pixmap containing the final image.*/
#ifndef NO_THREADS
static volatile int threads_done = 0;
static pthread_mutex_t lock;
static pthread_mutex_t lock_main;
static pthread_cond_t wait_for_merge;
static pthread_cond_t wait_after_merge;
#endif


static void cross_product(vector *a, vector *b, vector *res)
{
  res->x = a->y * b->z - a->z * b->y;
  res->y = a->z * b->x - a->x * b->z;
  res->z = a->x * b->y - a->y * b->x;
}

/* Every thread has its own queue containing equal sized parts of every mesh that has to be drawn. The main
 * threads divides the meshes and enqueues jobs for the worker threads meaning they have to draw a part of
 * a mesh into their own pixmap. After all meshes are drawn, the pixmaps are merged.*/
static int queue_destroy(queue *queue)
{
  if (queue == NULL)
    {
      return ERR_INVAL;
    }
  while (queue->front != NULL)
    {
      struct queue_node_s *node = queue->front;
      queue->front = node->next;
      free(node);
    }
  free(queue);
  return SUCCESS;
}

static queue *queue_new(void)
{
  queue *queue = malloc(sizeof(*queue));
  if (queue == NULL)
    {
      return NULL;
    }
#ifndef NO_THREADS
  pthread_mutex_init(&queue->lock, NULL);
  pthread_cond_init(&queue->cond, NULL);
#endif

  queue->front = queue->back = NULL;
  return queue;
}

static void *queue_dequeue(queue *queue)
{
  struct queue_node_s *node;
  void *argument;
#ifndef NO_THREADS
  pthread_mutex_lock(&queue->lock);
  if (queue == NULL || queue->front == NULL)
    {
      pthread_cond_wait(&queue->cond, &queue->lock);
    }
#endif
  node = queue->front;
  if (node == NULL)
    {
      return NULL;
    }
  argument = node->data;
  queue->front = node->next;
  if (queue->front == NULL)
    {
      queue->back = NULL;
    }
  free(node);
#ifndef NO_THREADS
  pthread_mutex_unlock(&queue->lock);
#endif
  return argument;
}

static int queue_enqueue(queue *queue, void *data)
{
  struct queue_node_s *node;
#ifndef NO_THREADS
  pthread_mutex_lock(&queue->lock);
#endif
  if (queue == NULL)
    {
      abort();
      return ERR_INVAL;
    }
  node = malloc(sizeof(*node));
  if (node == NULL)
    {
      abort();
      return ERR_NOMEM;
    }
  node->data = data;
  node->next = NULL;
  if (queue->back == NULL)
    {
      queue->front = queue->back = node;
    }
  else
    {
      queue->back->next = node;
      queue->back = node;
    }
#ifndef NO_THREADS
  pthread_mutex_unlock(&queue->lock);
  pthread_cond_signal(&queue->cond);
#endif
  return SUCCESS;
}

/*!
 * This method creates a pixmap for every thread and a queue where its jobs can be enqueued.
 * \param [in] width width of the pixmap
 * \param [in] height height of the pixmap
 */
static void create_queues_and_pixmaps(int width, int height)
{
  int i;
  for (i = 0; i < context_struct_.num_threads; i++)
    {
      context_struct_.queues[i] = queue_new();
      if (i != 0)
        {
          context_struct_.pixmaps[i] = (unsigned char *)calloc(width * height * 4, sizeof(unsigned char));
        }
      context_struct_.depth_buffers[i] = (float *)malloc(width * height * sizeof(float));
    }
  context_struct_.last_height = height;
  context_struct_.last_width = width;
#ifndef NO_THREADS
  /* If threading support is enabled, the threads must wait
   * for work before work packages were put into the queue */
  initialise_consumer(context_struct_.queues, height, width);
#endif
}

/*!
 * This method merges a certain section defined by the parameter of all the pixmaps. Because the sections are disjoint,
 * this method can be called by multiple threads caring about their own part without causing any race condition.
 * \param [in] width width of the pixmap
 * \param [in] starty height for the merging process to start
 * \param [in] endy height for the merging process to end
 */
static void merge_pixmaps(int width, int starty, int endy)
{
  int iy, ix, minind, i;
  for (iy = starty; iy < endy; iy++)
    {
      for (ix = 0; ix < width; ix++)
        {
          minind = 0;
          for (i = 1; i < context_struct_.num_threads; i++)
            {
              if (context_struct_.depth_buffers[i][iy * width + ix] <
                  context_struct_.depth_buffers[minind][iy * width + ix])
                {
                  minind = i;
                }
            }
          context_struct_.pixmaps[0][iy * width * 4 + ix * 4 + 0] =
              context_struct_.pixmaps[minind][iy * width * 4 + ix * 4 + 0];
          context_struct_.pixmaps[0][iy * width * 4 + ix * 4 + 1] =
              context_struct_.pixmaps[minind][iy * width * 4 + ix * 4 + 1];
          context_struct_.pixmaps[0][iy * width * 4 + ix * 4 + 2] =
              context_struct_.pixmaps[minind][iy * width * 4 + ix * 4 + 2];
          context_struct_.pixmaps[0][iy * width * 4 + ix * 4 + 3] =
              context_struct_.pixmaps[minind][iy * width * 4 + ix * 4 + 3];
        }
    }
}

/*!
 * This method is called simultaneously by many threads meaning this is the worker method. Every thread
 * dequeues all the mesh parts of its queue and draws them in its own pixmap. After this is done, the thread waits
 * for all the other threads to finish so that the merging process can start. Every threads merges an equal sized
 * part of the pixmap which is defined in the struct given as parameter. After all threads have merged the
 * rendering process is done.
 * \param [in] queue_and_merge_area defines the queue for a mesh to take the mesh parts out and
 * the area of the pixmaps to be merged by the thread
 */
static void *draw_and_merge(void *queue_and_merge_area)
{
  void *argument;
  struct queue_merge_area queue_and_merge_area_s = *((struct queue_merge_area *)queue_and_merge_area);
  while ((argument = queue_dequeue(queue_and_merge_area_s.queue)))
    {
      draw_triangle_indexbuffer(argument);
      if (((args *)argument)->id == 1)
        {
#ifndef NO_THREADS
          pthread_mutex_lock(&lock);
          threads_done += 1;
          if (threads_done == context_struct_.num_threads)
            {
              pthread_cond_broadcast(&wait_for_merge);
            }
          else
            {
              pthread_cond_wait(&wait_for_merge, &lock);
            }
          pthread_mutex_unlock(&lock);
#endif

          merge_pixmaps(queue_and_merge_area_s.width, queue_and_merge_area_s.starty, queue_and_merge_area_s.endy);
#ifndef NO_THREADS
          pthread_mutex_lock(&lock);
          threads_done += 1;
          pthread_mutex_unlock(&lock);
          if (threads_done == 2 * context_struct_.num_threads)
            {
              pthread_cond_signal(&wait_after_merge);
            }
#endif
        }
      else if (((args *)argument)->id == 2)
        {
          free(argument);
          break;
        }
      free(argument);
    }
  free(queue_and_merge_area);
  return NULL;
}

/*!
 * This method initialised the consumer threads. Every thread is created with its own queue and a certain
 * area of the pixmaps which the thread should merge
 * \param [in] queues queues of the threads
 * \param [in] height of the pixmaps being split equally on the threads
 * \param [in] width of the pixmaps
 */
static void initialise_consumer(queue *queues[MAX_NUM_THREADS], int height, int width)
{
  int i;
  int height_start_end[MAX_NUM_THREADS + 1];
  int part_per_thread = height / context_struct_.num_threads;
  int rest = height % context_struct_.num_threads;
  int rest_distributed = 0;
  height_start_end[0] = 0;
  height_start_end[context_struct_.num_threads] = height;
  for (i = 1; i < context_struct_.num_threads; i++)
    {
      height_start_end[i] = i * part_per_thread + rest_distributed;
      if (rest >= 1)
        {
          height_start_end[i] += 1;
          rest -= 1;
          rest_distributed += 1;
        }
    }

  for (i = 0; i < context_struct_.num_threads; i++)
    {
      struct queue_merge_area *queue_and_merge_area = malloc(sizeof(struct queue_merge_area));
      queue_and_merge_area->starty = height_start_end[i];
      queue_and_merge_area->endy = height_start_end[i + 1];
      queue_and_merge_area->queue = queues[i];
      queue_and_merge_area->width = width;
#ifndef NO_THREADS
      pthread_create(&context_struct_.threads[i], NULL, draw_and_merge, (void *)queue_and_merge_area);
#else
      draw_and_merge(queue_and_merge_area);
#endif
    }
}

/*!
 * This method creates a matrix used for the projection depending on which projection type is specified.
 * \param [in] width of the pixmap
 * \param [in] height of the pixmap
 * \param [in] fovy field of view in y direction
 * \param [in] zNear z coordinate of the near clipping plane
 * \param [in] zFar z coordinate of the far clipping plane
 * \param [in] projection_type can be GR3_PROJECTION_PARALLEL, GR3_PROJECTION_PERSPECTIVE or GR3_PROJECTION_ORTHOGRAPHIC
 *
 */
static matrix get_projection(int width, int height, float fovy, float zNear, float zFar, int projection_type)
{
  float aspect = (float)width / height;
  if (context_struct_.projection_type == GR3_PROJECTION_PARALLEL && context_struct_.aspect_override > 0)
    {
      aspect = context_struct_.aspect_override;
    }
  float tfov2 = tan(fovy * PI / 360.0);
  float right = zNear * aspect * tfov2;
  float top = zNear * tfov2;
  float left = -right;
  float bottom = -top;
  matrix perspective;
  if (projection_type == GR3_PROJECTION_PARALLEL)
    {
      perspective = matrix_ortho_proj(left, right, bottom, top, zNear, zFar);
    }
  else if (projection_type == GR3_PROJECTION_ORTHOGRAPHIC)
    {
      left = context_struct_.left;
      right = context_struct_.right;
      bottom = context_struct_.bottom;
      top = context_struct_.top;
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
      perspective = matrix_ortho_proj(left, right, bottom, top, zNear, zFar);
    }
  else
    {
      perspective = matrix_perspective_proj(left, right, bottom, top, zNear, zFar);
    }
  return perspective;
}

/*!
 * This method returns a perspective projection matrix without dividing by w.
 *   Source: http://www.opengl.org/sdk/docs/man2/xhtml/glFrustum.xml
 */
static matrix matrix_perspective_proj(float left, float right, float bottom, float top, float zNear, float zFar)
{
  matrix perspective = {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}};
  perspective.mat[0] = 2.0 * zNear / (right - left);
  perspective.mat[2] = (right + left) / (right - left);
  perspective.mat[5] = 2.0 * zNear / (top - bottom);
  perspective.mat[6] = (top + bottom) / (top - bottom);
  perspective.mat[10] = -(zFar + zNear) / (zFar - zNear);
  perspective.mat[11] = -2.0 * zFar * zNear / (zFar - zNear);
  perspective.mat[14] = -1.0;
  return perspective;
}

/*!
 * This method returns an orthograpic projection matrix without dividing by w.
 *  Source: http://www.opengl.org/sdk/docs/man2/xhtml/glOrtho.xml
 */
static matrix matrix_ortho_proj(float left, float right, float bottom, float top, float nearVal, float farVal)
{
  float dif_right_left = right - left;
  float sum_right_left = right + left;
  float dif_top_bot = top - bottom;
  float sum_top_bot = top + bottom;
  float dif_far_near = farVal - nearVal;
  float sum_far_near = farVal + nearVal;
  float zero = 2.0f / dif_right_left;
  float three = -sum_right_left / dif_right_left;
  float five = 2.0f / dif_top_bot;
  float seven = -(sum_top_bot) / dif_top_bot;
  float ten = -2.0f / dif_far_near;
  float eleven = -sum_far_near / dif_far_near;
  matrix res = {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}};
  res.mat[0] = zero;
  res.mat[3] = three;
  res.mat[5] = five;
  res.mat[7] = seven;
  res.mat[10] = ten;
  res.mat[11] = eleven;
  res.mat[15] = 1;
  return res;
}

/*!
 * This method returns a viewport matrix mapping coordinates on pixel coordinates.
 */
static matrix matrix_viewport_trafo(int width, int height)
{
  matrix res = {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}};
  float a = (width - 1) / 2.0;
  float b = (height - 1) / 2.0;
  res.mat[0] = width / 2.0;
  res.mat[3] = a;
  res.mat[5] = height / 2.0;
  res.mat[7] = b;
  res.mat[10] = 1 / 2.0;
  res.mat[11] = 1 / 2.0;
  res.mat[15] = 1;
  return res;
}

/*!
 * This method multiplies two matrices sized 4x4.
 */
static matrix mat_mul_4x4(matrix *a, matrix *b)
{
  matrix res = {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}};
  float sum = 0.0;
  int c;
  int d;
  int k;
  for (c = 0; c < 4; c++)
    {
      for (d = 0; d < 4; d++)
        {
          for (k = 0; k < 4; k++)
            {
              sum = sum + a->mat[c * 4 + k] * b->mat[k * 4 + d];
            }
          res.mat[c * 4 + d] = sum;
          sum = 0;
        }
    }
  return res;
}

/*!
 * This method multiplies two matrices sized 3x3.
 */
static matrix3x3 mat_mul_3x3(matrix3x3 *a, matrix3x3 *b)
{
  matrix3x3 res = {{0, 0, 0, 0, 0, 0, 0, 0, 0}};
  float sum = 0.0;
  int c;
  int d;
  int k;
  for (c = 0; c < 3; c++)
    {
      for (d = 0; d < 3; d++)
        {
          for (k = 0; k < 3; k++)
            {
              sum = sum + a->mat[c * 3 + k] * b->mat[k * 3 + d];
            }
          res.mat[c * 3 + d] = sum;
          sum = 0;
        }
    }
  return res;
}

/*!
 * This method multiplies a 4x4 mat with a 1x4 vector.
 */
static void mat_vec_mul_4x1(matrix *a, vertex_fp *b)
{
  float bx = b->x;
  float by = b->y;
  float bz = b->z;
  float bw = b->w;
  b->x = a->mat[0] * bx + a->mat[1] * by + a->mat[2] * bz + a->mat[3] * bw;
  b->y = a->mat[4] * bx + a->mat[5] * by + a->mat[6] * bz + a->mat[7] * bw;
  b->z = a->mat[8] * bx + a->mat[9] * by + a->mat[10] * bz + a->mat[11] * bw;
  b->w = a->mat[12] * bx + a->mat[13] * by + a->mat[14] * bz + a->mat[15] * bw;
}

/*!
 * This method multiplies a 3x3 mat with a 1x3 vector.
 */
static void mat_vec_mul_3x1(matrix3x3 *a, vector *b)
{
  float bx = b->x;
  float by = b->y;
  float bz = b->z;
  b->x = a->mat[0] * bx + a->mat[1] * by + a->mat[2] * bz;
  b->y = a->mat[3] * bx + a->mat[4] * by + a->mat[5] * bz;
  b->z = a->mat[6] * bx + a->mat[7] * by + a->mat[8] * bz;
}

/*!
 * This method divides every component of a vertex by its w component
 * \param [in] v_fp the vertex from which the points should be divided by w
 */
static void divide_by_w(vertex_fp *v_fp)
{
  v_fp->x /= v_fp->w;
  v_fp->y /= v_fp->w;
  v_fp->z /= v_fp->w;
  v_fp->w = 1.0;
}

/*!
 * This method multiplies a color with a constant scalar value *
 * \param [in] c color
 * \param [in] fac factor to multiply
 * \return c*fac where c is a color vector*/
static void mult_color(color_float *c, float fac, float alpha_fac)
{
  c->r *= fac;
  c->g *= fac;
  c->b *= fac;
  c->a *= alpha_fac;
}

/*!
 * This method converts a color_int struct to a color struct.
 *
 * \param [in] c color with int values
 * \return a color with unsigned char values*/
static color color_float_to_color(color_float c)
{
  color new_color;
  new_color.r = (c.r > 1.0 ? 255 : (unsigned char)floor(c.r * 255 + 0.5));
  new_color.g = (c.g > 1.0 ? 255 : (unsigned char)floor(c.g * 255 + 0.5));
  new_color.b = (c.b > 1.0 ? 255 : (unsigned char)floor(c.b * 255 + 0.5));
  new_color.a = (c.a > 1.0 ? 255 : (unsigned char)floor(c.a * 255 + 0.5));
  return new_color;
}

/*!
 * This method multiplies a vector with a constant scalar value
 *
 * \param [in] v vector
 * \param [in] fac factor to multiply
 * \return vector*fac where v is a vector*/
static color_float linearcombination_color(color_float c1, color_float c2, color_float c3, float fac1, float fac2,
                                           float fac3)
{
  color_float c;
  c.r = c1.r * fac1 + c2.r * fac2 + c3.r * fac3;
  c.g = c1.g * fac1 + c2.g * fac2 + c3.g * fac3;
  c.b = c1.b * fac1 + c2.b * fac2 + c3.b * fac3;
  c.a = c1.a + c2.a + c3.a;
  return c;
}

/*!
 * This method multiplies a vector with a constant scalar value
 *
 * \param [in] v vector
 * \param [in] fac factor to multiply
 * \return vector*fac where v is a vector*/
static void mult_vector(vector *v, float fac)
{
  v->x *= fac;
  v->y *= fac;
  v->z *= fac;
}

/*!
 * This method normalizes a vector (3-dimensional) so that after this operation its length is one.
 *
 * \param [in] v vector to be normalized
 * \return v with length 1*/
static void normalize_vector(vector *v)
{
  float norm = sqrt(dot_vector(v, v));
  mult_vector(v, 1 / norm);
}

/*!
 * This method performs the dot product of two vectors (3-dimensional).
 *
 * \param [in] v1 first vector
 * \param [in] v2 second vector
 * \return the scalar value of v1*v2 */
static float dot_vector(vector *v1, vector *v2)
{
  return v1->x * v2->x + v1->y * v2->y + v1->z * v2->z;
}

/*!
 * This method multiplies a vector with a constant scalar value
 *
 * \param [in] v vector
 * \param [in] fac factor to multiply
 * \return vector*fac where v is a vector*/
static vector linearcombination(vector *v1, vector *v2, vector *v3, float fac1, float fac2, float fac3)
{

  vector v;
  v.x = v1->x * fac1 + v2->x * fac2 + v3->x * fac3;
  v.y = v1->y * fac1 + v2->y * fac2 + v3->y * fac3;
  v.z = v1->z * fac1 + v2->z * fac2 + v3->z * fac3;
  return v;
}

/*!
 * This method calculates the surface of a triangle. (cf. edge function
 * https://www.scratchapixel.com/lessons/3d-basic-rendering/rasterization-practical-implementation/rasterization-stage )
 * \param [in] dif_a_b_x difference of two vertices' x components
 * \param [in] dif_a_b_y difference of two vertices' y components
 * \param [in] a one vertex of the triangle
 * \param [in] b another vertex of the triangle
 * \return the surface of the triangle
 */
static float triangle_surface_2d(float dif_a_b_x, float dif_a_b_y, float ay, float ax, float cy, float cx)
{
  return dif_a_b_x * (cy - ay) - dif_a_b_y * (cx - ax);
}

/*!
 * This initialises the software-renderer. It determines the number of available cores on a system, so that the
 * number of threads drawing the image is equal to the number of cores minus one. This is the most efficient
 * because the main thread needs its own core. The software-renderer is initialised and the method get_pixmap
 * can be invoked.
 */
GR3API int gr3_initSR_()
{
#ifdef _WIN32
#ifndef _SC_NPROCESSORS_ONLN
  SYSTEM_INFO info;
  GetSystemInfo(&info);
#define sysconf(a) info.dwNumberOfProcessors
#define _SC_NPROCESSORS_ONLN
#endif
#endif
  gr3_log_("gr3_initSR_();");
  context_struct_.use_software_renderer = 1;
#ifndef NO_THREADS
  if (context_struct_.init_struct.num_threads == 0)
    {
      gr3_log_("Number of Threads equals number of cores minus one");
      context_struct_.num_threads = ((int)sysconf(_SC_NPROCESSORS_ONLN) - 1) < MAX_NUM_THREADS
                                        ? (int)sysconf(_SC_NPROCESSORS_ONLN) - 1
                                        : MAX_NUM_THREADS;
    }
  else
    {
      if (context_struct_.init_struct.num_threads > MAX_NUM_THREADS)
        {
          gr3_log_("Built-In maximum number of threads exceeded!");
          context_struct_.num_threads = MAX_NUM_THREADS;
        }
      else
        {
          context_struct_.num_threads = context_struct_.init_struct.num_threads;
        }
    }

  if (context_struct_.num_threads <= 0)
    {
      context_struct_.num_threads = 1;
    }
#else
  context_struct_.num_threads = 1;
#endif
  gr3_appendtorenderpathstring_("software");
  return GR3_ERROR_NONE;
}

/*!
 * This method creates an args object which contains all needed information to draw a part of the mesh.
 * The mesh is specified and the idxstart and idxend determines positions in the index buffer where
 * the drawing process should start and end. The transformation matrices are included so that every thread
 * transforms its own coordinates leading to a higher proportion of parallelisation.
 */
static args *malloc_arg(int thread_idx, int mesh, matrix model_view_perspective, matrix viewport,
                        matrix3x3 model_view_3x3, vector light_dir, const float *colors, const float *scales, int width,
                        int height, int id, int idxstart, int idxend, vertex_fp *vertices_fp)
{
  args *arg = malloc(sizeof(args));
  arg->thread_idx = thread_idx;
  arg->mesh = mesh;
  arg->model_view_perspective = model_view_perspective;
  arg->viewport = viewport;
  arg->model_view_3x3 = model_view_3x3;
  arg->light_dir = light_dir;
  arg->colors = colors;
  arg->scales = scales;
  arg->width = width;
  arg->height = height;
  arg->id = id;
  arg->idxstart = idxstart;
  arg->idxend = idxend;
  arg->vertices_fp = vertices_fp;
  return arg;
}


/*!
 * This method draws a certain part of triangles defined by an indexbuffer. It starts with the
 * idxstart  and ends with idxend defined in v_arguments. Every thread only draws a part of a mesh
 * so that every thread gets to draw a disjoint part of the mesh.
 */
static void *draw_triangle_indexbuffer(void *v_arguments)
{
  /* idxstart and idxend refers to the first and last triangle to be drawn; if indexbuffer is defined, that means
   * idxstart and idxend *3, otherwise they are in the normal vertices array with 9 elements per triangle */
  int i, j;
  args *arg = (args *)v_arguments;
  float *colors = context_struct_.mesh_list_[arg->mesh].data.colors;
  float *normals = context_struct_.mesh_list_[arg->mesh].data.normals;
  float *vertices = context_struct_.mesh_list_[arg->mesh].data.vertices;
  int num_indices = context_struct_.mesh_list_[arg->mesh].data.number_of_indices;
  int *indices = context_struct_.mesh_list_[arg->mesh].data.indices;
  if (num_indices != 0)
    {
      vertex_fp *vertices_fp = arg->vertices_fp;
      for (i = arg->idxstart * 3; i < arg->idxend * 3; i += 3)
        {
          vertex_fp *vertex_fpp[3];
          vertex_fpp[0] = &vertices_fp[indices[i]];
          vertex_fpp[1] = &vertices_fp[indices[i + 1]];
          vertex_fpp[2] = &vertices_fp[indices[i + 2]];
          draw_triangle(context_struct_.pixmaps[arg->thread_idx], context_struct_.depth_buffers[arg->thread_idx],
                        arg->width, arg->height, vertex_fpp, arg->colors, arg->light_dir);
        }
    }
  else
    {
      float div_0 = 0;
      float div_1 = 0;
      float div_2 = 0;
      color fill_color;
      color line_color;
      color_float dummy_color = {0, 0, 0, 0};
      vector dummy_vector = {0, 0, 0};
      if (arg->scales != NULL) /* there is a mesh passed to this function with the intention to finish the rendering
                                * process and it has all values set to 0/NULL */
        {
          div_0 = arg->scales[0];
          div_1 = arg->scales[1];
          div_2 = arg->scales[2];
          if (context_struct_.option >= 0 && context_struct_.option <= 2)
            {
              /* If a mesh representation with the lines is demanded, the fill color and the linecolor have
               * to be determined */
              int color, errind;
              double r, g, b;
              div_0 = 1;
              div_1 = 1;
              div_2 = 1;
              gks_inq_pline_color_index(&errind, &color);
              gks_inq_color_rep(1, color, GKS_K_VALUE_SET, &errind, &r, &g, &b);
              color_float line_color_f = {r, g, b, 1.0};
              line_color = color_float_to_color(line_color_f);
              if (context_struct_.option < 2)
                {
                  fill_color.r = (unsigned char)(context_struct_.background_color[0] * 255);
                  fill_color.g = (unsigned char)(context_struct_.background_color[1] * 255);
                  fill_color.b = (unsigned char)(context_struct_.background_color[2] * 255);
                  fill_color.a = (unsigned char)(context_struct_.background_color[3] * 255);
                }
              else
                {
                  gks_inq_fill_color_index(&errind, &color);
                  gks_inq_color_rep(1, color, GKS_K_VALUE_SET, &errind, &r, &g, &b);
                  color_float fill_color_f = {r, g, b, 1.0};
                  fill_color = color_float_to_color(fill_color_f);
                }
            }
        }
      vertex_fp vertices_fp[3];
      vertex_fp *vertex_fpp[3];
      for (i = arg->idxstart; i < arg->idxend; i++)
        {
          int index = 0;
          for (j = 0; j < 3; j++)
            {
              index = 9 * i + 3 * j;
              vertices_fp[j].c.r = colors[index];
              vertices_fp[j].c.g = colors[index + 1];
              vertices_fp[j].c.b = colors[index + 2];
              vertices_fp[j].c.a = 1.0f;
              vertices_fp[j].normal.x = normals[index] / div_0;
              vertices_fp[j].normal.y = normals[index + 1] / div_1;
              vertices_fp[j].normal.z = normals[index + 2] / div_2;
              mat_vec_mul_3x1(&arg->model_view_3x3, &vertices_fp[j].normal);
              vertices_fp[j].x = vertices[index];
              vertices_fp[j].y = vertices[index + 1];
              vertices_fp[j].z = vertices[index + 2];
              vertices_fp[j].w = 1.0;
              vertices_fp[j].w_div = 1.0;
              mat_vec_mul_4x1(&arg->model_view_perspective, &vertices_fp[j]);
              divide_by_w(&vertices_fp[j]);
              mat_vec_mul_4x1(&arg->viewport, &vertices_fp[j]);
            }
          vertex_fpp[0] = &vertices_fp[0];
          vertex_fpp[1] = &vertices_fp[1];
          vertex_fpp[2] = &vertices_fp[2];

          if (context_struct_.option > 2)
            {
              draw_triangle(context_struct_.pixmaps[arg->thread_idx], context_struct_.depth_buffers[arg->thread_idx],
                            arg->width, arg->height, vertex_fpp, arg->colors, arg->light_dir);
            }
          else
            { /* the mesh should be drawn represented by lines */
              if (arg->scales != NULL)
                {
                  /* If one of those values is specified, that means that an extra
                   * vertex is given to make the triangle a square shape, as square
                   * shapes are the ones that should be drawn. If an additional vertex
                   * is given, it must be transformed. */
                  if (vertices_fp[1].normal.z > 0 || vertices_fp[1].normal.z < 0)
                    {
                      vertex_fp tmp = {vertices_fp[0].normal.y,
                                       vertices_fp[0].normal.z,
                                       vertices_fp[1].normal.y,
                                       1.0,
                                       1.0,
                                       dummy_color,
                                       dummy_vector};
                      mat_vec_mul_4x1(&arg->model_view_perspective, &tmp);
                      divide_by_w(&tmp);
                      mat_vec_mul_4x1(&arg->viewport, &tmp);
                      vertices_fp[0].normal.y = tmp.x;
                      vertices_fp[0].normal.z = tmp.y;
                      vertices_fp[1].normal.y = tmp.z;
                    }
                }
              draw_triangle_with_edges(context_struct_.pixmaps[arg->thread_idx],
                                       context_struct_.depth_buffers[arg->thread_idx], arg->width, arg->height,
                                       vertex_fpp, line_color, fill_color);
            }
        }
    }
  return NULL;
}

/*!
 * If there is an option between zero and 2 specified in the gr3_surface method, a mesh with the edges should be drawn
 * (cf. gr_surface_option_t in gr3_gr.c). In this case there is a value stored in the normals to refer to the
 * thickness of the lines in a triangle (normal.x of v_fp[0] refers to edge 0-1, normal.x of v_fp[1] refers to edge 1-2,
 * and normal.x of v_fp[2] refers to edge 2-0). In the rendering process, the distance bewteen the pixel and the
 * relevant edge is defined by checking if there is a perpendicular line between these two and otherwise calculating the
 * distance to the closer vertex of the two vertices forming the edge. If this distance is smaller than the given
 * linewidth of this edge, the pixel is colored, otherwise it isnt.
 * Because squares should be drawn, there are coordinates of the vertex making the triangle a square stored in the
 * normal. (v_fp[0].normal.y, v_fp[0].normal.z, v_fp[1].normal.y). One missing edge forming a square and being relevant
 * in the represenation is taken into account, calculating the distances, as well.
 * \param [in] color line_color Color of the line to be drawn
 * \param [in] color fill_color Color to fill the triangles with
 */
static void draw_triangle_with_edges(unsigned char *pixels, float *dep_buf, int width, int height, vertex_fp *v_fp[3],
                                     color line_color, color fill_color)
{
  int x, y;
  int x_min = ceil(MINTHREE(v_fp[0]->x, v_fp[1]->x, v_fp[2]->x));
  int y_min = ceil(MINTHREE(v_fp[0]->y, v_fp[1]->y, v_fp[2]->y));
  int x_max = floor(MAXTHREE(v_fp[0]->x, v_fp[1]->x, v_fp[2]->x));
  int y_max = floor(MAXTHREE(v_fp[0]->y, v_fp[1]->y, v_fp[2]->y));
  int off = ceil(MAXTHREE(v_fp[0]->normal.x, v_fp[1]->normal.x, v_fp[2]->normal.x));
  int x_lim_lower = x_min - off < 0 ? 0 : x_min - off;
  int x_lim_upper = x_max + off > width - 1 ? width - 1 : x_max + off;
  int y_lim_lower = y_min - off < 0 ? 0 : y_min - off;
  int y_lim_upper = y_max + off > height - 1 ? height - 1 : y_max + off;
  for (x = x_lim_lower; x <= x_lim_upper; x++)
    {
      for (y = y_lim_lower; y <= y_lim_upper; y++)
        {
          /* calculate depth with interpolation/extrapolation by calculating the area of the sub triangles */
          float area_0, area_1, area_2, depth;
          float len_v_0_1 = sqrt((v_fp[0]->x - v_fp[1]->x) * (v_fp[0]->x - v_fp[1]->x) +
                                 (v_fp[0]->y - v_fp[1]->y) * (v_fp[0]->y - v_fp[1]->y));
          float len_v_1_2 = sqrt((v_fp[1]->x - v_fp[2]->x) * (v_fp[1]->x - v_fp[2]->x) +
                                 (v_fp[1]->y - v_fp[2]->y) * (v_fp[1]->y - v_fp[2]->y));
          float len_v_2_0 = sqrt((v_fp[2]->x - v_fp[0]->x) * (v_fp[2]->x - v_fp[0]->x) +
                                 (v_fp[2]->y - v_fp[0]->y) * (v_fp[2]->y - v_fp[0]->y));
          float len_p_0 = sqrt((x - v_fp[0]->x) * (x - v_fp[0]->x) + (y - v_fp[0]->y) * (y - v_fp[0]->y));
          float len_p_1 = sqrt((x - v_fp[1]->x) * (x - v_fp[1]->x) + (y - v_fp[1]->y) * (y - v_fp[1]->y));
          float len_p_2 = sqrt((x - v_fp[2]->x) * (x - v_fp[2]->x) + (y - v_fp[2]->y) * (y - v_fp[2]->y));
          float s_0 = (len_p_0 + len_p_1 + len_v_0_1) / 2;
          float tmp_0 = s_0 * (s_0 - len_p_0) * (s_0 - len_p_1) * (s_0 - len_v_0_1);

          float s_1 = (len_p_1 + len_p_2 + len_v_1_2) / 2;
          float tmp_1 = s_1 * (s_1 - len_p_1) * (s_1 - len_p_2) * (s_1 - len_v_1_2);
          float s_2 = (len_p_2 + len_p_0 + len_v_2_0) / 2;
          float tmp_2 = s_2 * (s_2 - len_p_2) * (s_2 - len_p_0) * (s_2 - len_v_2_0);
          if (tmp_0 < 0)
            {
              tmp_0 = 0;
            }
          area_0 = sqrt(tmp_0);

          if (tmp_1 < 0)
            {
              tmp_1 = 0;
            }
          area_1 = sqrt(tmp_1);

          if (tmp_2 < 0)
            {
              tmp_2 = 0;
            }
          area_2 = sqrt(tmp_2);
          depth = (area_0 * v_fp[1]->z + area_1 * v_fp[2]->z + area_2 * v_fp[0]->z) * (1 / (area_0 + area_1 + area_2));
          if (depth < dep_buf[y * width + x])
            {
              /* initialise distances with values that are definitly bigger than the linewidth so the default
               * is, that they are not colored in line color */
              double d1 = off + 1, d2 = off + 1, d3 = off + 1, d4 = off + 1, d5 = off + 1;
              if (v_fp[0]->normal.x > 0)
                {
                  vector diff_vec_1_inv = {x - v_fp[0]->x, y - v_fp[0]->y, 0};
                  vector diff_vec_1 = {-diff_vec_1_inv.x, -diff_vec_1_inv.y, 0};
                  vector edge_1_inv = {v_fp[1]->x - v_fp[0]->x, v_fp[1]->y - v_fp[0]->y, 0};
                  vector diff_vec_2 = {v_fp[1]->x - x, v_fp[1]->y - y, 0};
                  vector diff_vec_2_inv = {-diff_vec_2.x, -diff_vec_2.y, 0};
                  vector edge_1 = {-edge_1_inv.x, -edge_1_inv.y, 0};
                  float angle_01_1 = dot_vector(&diff_vec_1_inv, &edge_1_inv);
                  float angle_01_2 = dot_vector(&diff_vec_2_inv, &edge_1);
                  /* The two first cases mean, that the nearest point of the relevant edge
                   * to the pixel, which is questioned to be colored in linecolor, is one
                   * of the vertices of the relevant edge, so there is no perpedicular line between
                   * edge and pixel. */
                  if (angle_01_1 < 0)
                    {
                      d1 = sqrt(dot_vector(&diff_vec_1, &diff_vec_1));
                    }
                  else if (angle_01_2 < 0)
                    {
                      d1 = sqrt(dot_vector(&diff_vec_2, &diff_vec_2));
                    }
                  /* there is a perpendicular line between edge and pixel, thus the distance
                   * can be calculated with the cross product */
                  else
                    {
                      vector vec1;
                      cross_product(&diff_vec_1, &edge_1, &vec1);
                      d1 = sqrt(dot_vector(&vec1, &vec1)) / sqrt(dot_vector(&edge_1, &edge_1));
                    }
                }
              if (v_fp[1]->normal.x > 0)
                {
                  vector vec2;
                  vector diff_vec_2 = {v_fp[1]->x - x, v_fp[1]->y - y, 0};
                  vector diff_vec_2_inv = {-diff_vec_2.x, -diff_vec_2.y, 0};
                  vector diff_vec_3 = {v_fp[2]->x - x, v_fp[2]->y - y, 0};
                  vector diff_vec_3_inv = {-diff_vec_3.x, -diff_vec_3.y, 0};
                  vector edge_2 = {v_fp[1]->x - v_fp[2]->x, v_fp[1]->y - v_fp[2]->y, 0};
                  vector edge_2_inv = {-edge_2.x, -edge_2.y, 0};
                  float angle_12_1 = dot_vector(&diff_vec_2_inv, &edge_2_inv);
                  float angle_12_2 = dot_vector(&diff_vec_3_inv, &edge_2);
                  if (angle_12_1 < 0)
                    {
                      d2 = sqrt(dot_vector(&diff_vec_2, &diff_vec_2));
                    }
                  else if (angle_12_2 < 0)
                    {
                      d2 = sqrt(dot_vector(&diff_vec_3, &diff_vec_3));
                    }
                  else
                    {
                      cross_product(&diff_vec_2, &edge_2, &vec2);
                      d2 = sqrt(dot_vector(&vec2, &vec2)) / sqrt(dot_vector(&edge_2, &edge_2));
                    }
                }
              if (v_fp[2]->normal.x > 0)
                {
                  vector vec3;
                  vector diff_vec_1_inv = {x - v_fp[0]->x, y - v_fp[0]->y, 0};
                  vector diff_vec_1 = {-diff_vec_1_inv.x, -diff_vec_1_inv.y, 0};
                  vector diff_vec_3 = {v_fp[2]->x - x, v_fp[2]->y - y, 0};
                  vector diff_vec_3_inv = {-diff_vec_3.x, -diff_vec_3.y, 0};
                  vector edge_3 = {v_fp[2]->x - v_fp[0]->x, v_fp[2]->y - v_fp[0]->y, 0};
                  vector edge_3_inv = {-edge_3.x, -edge_3.y, 0};
                  float angle_20_1 = dot_vector(&diff_vec_3_inv, &edge_3_inv);
                  float angle_20_2 = dot_vector(&diff_vec_1_inv, &edge_3);
                  if (angle_20_1 < 0)
                    {
                      d3 = sqrt(dot_vector(&diff_vec_3, &diff_vec_3));
                    }
                  else if (angle_20_2 < 0)
                    {
                      d3 = sqrt(dot_vector(&diff_vec_1, &diff_vec_1));
                    }
                  else
                    {
                      cross_product(&diff_vec_3, &edge_3, &vec3);
                      d3 = sqrt(dot_vector(&vec3, &vec3)) / sqrt(dot_vector(&edge_3, &edge_3));
                    }
                }
              if (v_fp[1]->normal.z > 0.0)
                {
                  vector diff_vec_3 = {v_fp[2]->x - x, v_fp[2]->y - y, 0};
                  vector diff_vec_3_inv = {-diff_vec_3.x, -diff_vec_3.y, 0};
                  vector diff_vec_4 = {v_fp[0]->normal.y - x, v_fp[0]->normal.z - y, 0};
                  vector diff_vec_4_inv = {-diff_vec_4.x, -diff_vec_4.y, 0};
                  vector vec4;
                  vector edge_4 = {v_fp[2]->x - v_fp[0]->normal.y, v_fp[2]->y - v_fp[0]->normal.z, 0};
                  vector edge_4_inv = {-edge_4.x, -edge_4.y, 0};
                  float angle_23_1 = dot_vector(&diff_vec_4_inv, &edge_4);
                  float angle_23_2 = dot_vector(&diff_vec_3_inv, &edge_4_inv);
                  if (angle_23_1 < 0)
                    {
                      d4 = sqrt(dot_vector(&diff_vec_4, &diff_vec_4));
                    }
                  else if (angle_23_2 < 0)
                    {
                      d4 = sqrt(dot_vector(&diff_vec_3, &diff_vec_3));
                    }
                  else
                    {
                      cross_product(&diff_vec_3, &edge_4, &vec4);
                      d4 = sqrt(dot_vector(&vec4, &vec4)) / sqrt(dot_vector(&edge_4, &edge_4));
                    }
                }
              if (v_fp[1]->normal.z < 0.0)
                {
                  vector vec4;
                  vector diff_vec_2 = {v_fp[1]->x - x, v_fp[1]->y - y, 0};
                  vector diff_vec_2_inv = {-diff_vec_2.x, -diff_vec_2.y, 0};
                  vector diff_vec_4 = {v_fp[0]->normal.y - x, v_fp[0]->normal.z - y, 0};
                  vector diff_vec_4_inv = {-diff_vec_4.x, -diff_vec_4.y, 0};
                  vector edge_4 = {v_fp[1]->x - v_fp[0]->normal.y, v_fp[1]->y - v_fp[0]->normal.z, 0};
                  vector edge_4_inv = {-edge_4.x, -edge_4.y, 0};
                  float angle_13_1 = dot_vector(&diff_vec_4_inv, &edge_4);
                  float angle_13_2 = dot_vector(&diff_vec_2_inv, &edge_4_inv);
                  if (angle_13_1 < 0)
                    {
                      d5 = sqrt(dot_vector(&diff_vec_4, &diff_vec_4));
                    }
                  else if (angle_13_2 < 0)
                    {
                      d5 = sqrt(dot_vector(&diff_vec_2, &diff_vec_2));
                    }
                  else
                    {
                      cross_product(&diff_vec_2, &edge_4, &vec4);
                      d5 = sqrt(dot_vector(&vec4, &vec4)) / sqrt(dot_vector(&edge_4, &edge_4));
                    }
                }
              /* if the pixel has a distance to a line which is smaller than the linewidth, it should be colored */
              if (d1 < v_fp[0]->normal.x || d2 < v_fp[1]->normal.x || d3 < v_fp[2]->normal.x ||
                  d4 < v_fp[1]->normal.z || d5 < -v_fp[1]->normal.z)
                {
                  color_pixel(pixels, dep_buf, depth, width, x, y, &line_color);
                }
              else
                {
                  vector edge_1_inv = {v_fp[1]->x - v_fp[0]->x, v_fp[1]->y - v_fp[0]->y, 0};
                  vector diff_vec_1_inv = {x - v_fp[0]->x, y - v_fp[0]->y, 0};
                  vector edge_3 = {v_fp[2]->x - v_fp[0]->x, v_fp[2]->y - v_fp[0]->y, 0};
                  float d00 = dot_vector(&edge_1_inv, &edge_1_inv);
                  float d01 = dot_vector(&edge_1_inv, &edge_3);
                  float d11 = dot_vector(&edge_3, &edge_3);
                  float d20 = dot_vector(&diff_vec_1_inv, &edge_1_inv);
                  float d21 = dot_vector(&diff_vec_1_inv, &edge_3);
                  float denom = d00 * d11 - d01 * d01;
                  float w0 = (d11 * d20 - d01 * d21) / denom;
                  float w1 = (d00 * d21 - d01 * d20) / denom;
                  float w2 = 1.0f - w0 - w1;
                  if ((w0 > 0 && w1 > 0 && w2 > 0))
                    {
                      /* the pixel does not belong to a line, but lies inside a triangle => fill color */
                      color_pixel(pixels, dep_buf, depth, width, x, y, &fill_color);
                    }
                }
            }
        }
    }
}

/*!
 * This method sorts the three vertices by y-coordinate ascending so v1 is the topmost vertex.
 * After that it sets ups values to calculate barycentrical coordinates for interpolation of normals
 * and colors.
 */
static void draw_triangle(unsigned char *pixels, float *dep_buf, int width, int height, vertex_fp *v_fp[3],
                          const float *colors, vector light_dir)
{
  vertex_fp *v_fp_sorted_y[3];
  float A12, A20, A01, B12, B20, B01;
  int ind[3] = {0, 0, 0};
  if (v_fp[0]->y > v_fp[1]->y)
    {
      ind[0]++;
    }
  else
    {
      ind[1]++;
    }
  if (v_fp[0]->y > v_fp[2]->y)
    {
      ind[0]++;
    }
  else
    {
      ind[2]++;
    }
  if (v_fp[1]->y > v_fp[2]->y)
    {
      ind[1]++;
    }
  else
    {
      ind[2]++;
    }
  v_fp_sorted_y[ind[0]] = v_fp[0];
  v_fp_sorted_y[ind[1]] = v_fp[1];
  v_fp_sorted_y[ind[2]] = v_fp[2];

  A12 = v_fp[1]->y - v_fp[2]->y;
  A20 = v_fp[2]->y - v_fp[0]->y;
  A01 = v_fp[0]->y - v_fp[1]->y;
  B12 = v_fp[2]->x - v_fp[1]->x;
  B20 = v_fp[0]->x - v_fp[2]->x;
  B01 = v_fp[1]->x - v_fp[0]->x;
  fill_triangle(pixels, dep_buf, width, height, colors, light_dir, v_fp_sorted_y, v_fp, A12, A20, A01, B12, B20, B01);
}

/*!
 * This method really rasterizes a triangle in the pixmap. The triangles vertices are stored in v_fp.
 * The rasterisation algorithm works with slopes. The parameter light_dir defines the direction of the light.
 */
static void fill_triangle(unsigned char *pixels, float *dep_buf, int width, int height, const float *colors,
                          vector light_dir, vertex_fp **v_fp_sorted, vertex_fp **v_fp, float A12, float A20, float A01,
                          float B12, float B20, float B01)
{
  float invslope_short_1 = (v_fp_sorted[1]->x - v_fp_sorted[0]->x) / (v_fp_sorted[1]->y - v_fp_sorted[0]->y);
  float invslope_short_2 = (v_fp_sorted[2]->x - v_fp_sorted[1]->x) / (v_fp_sorted[2]->y - v_fp_sorted[1]->y);
  float invslope_long = (v_fp_sorted[2]->x - v_fp_sorted[0]->x) / (v_fp_sorted[2]->y - v_fp_sorted[0]->y);
  int scanlineY = ceil(v_fp_sorted[0]->y) > 0 ? ceil(v_fp_sorted[0]->y) : 0;

  int starty = scanlineY;
  int left_pointing =
      (v_fp_sorted[2]->x - ((v_fp_sorted[2]->y - v_fp_sorted[1]->y) * invslope_long)) > v_fp_sorted[1]->x;
  float curx1;
  float curx2 = v_fp_sorted[0]->x + (scanlineY - v_fp_sorted[0]->y) * invslope_long;
  int curx, dif, first_x = 0;
  float w0 = 0, w1 = 0, w2 = 0, sum_inv = 0;
  int lim = (int)v_fp_sorted[2]->y > height - 1 ? height - 1 : (int)v_fp_sorted[2]->y;
  for (scanlineY = starty; scanlineY <= lim; scanlineY++)
    {
      if (scanlineY < (int)(v_fp_sorted[1]->y))
        {
          curx1 = v_fp_sorted[0]->x + invslope_short_1 * (scanlineY - v_fp_sorted[0]->y);
        }
      else if (scanlineY == (int)(v_fp_sorted[1]->y))
        {
          if (scanlineY > (v_fp_sorted[1]->y))
            {
              curx1 = v_fp_sorted[1]->x + invslope_short_2 * (scanlineY - v_fp_sorted[1]->y);
            }
          else
            {
              curx1 = v_fp_sorted[0]->x + invslope_short_1 * (scanlineY - v_fp_sorted[0]->y);
            }
        }
      else
        {
          curx1 = v_fp_sorted[1]->x + invslope_short_2 * (scanlineY - v_fp_sorted[1]->y);
        }
      if (scanlineY == starty)
        {
          if (left_pointing)
            {
              first_x = (int)curx1 + 1;
              w0 = triangle_surface_2d(B12, -A12, v_fp[1]->y, v_fp[1]->x, scanlineY, first_x);
              w1 = triangle_surface_2d(B20, -A20, v_fp[2]->y, v_fp[2]->x, scanlineY, first_x);
              w2 = triangle_surface_2d(B01, -A01, v_fp[0]->y, v_fp[0]->x, scanlineY, first_x);
              sum_inv = 1 / (w0 + w1 + w2);
            }
          else
            {
              first_x = (int)curx2 + 1;
              w0 = triangle_surface_2d(B12, -A12, v_fp[1]->y, v_fp[1]->x, scanlineY, first_x);
              w1 = triangle_surface_2d(B20, -A20, v_fp[2]->y, v_fp[2]->x, scanlineY, first_x);
              w2 = triangle_surface_2d(B01, -A01, v_fp[0]->y, v_fp[0]->x, scanlineY, first_x);
              sum_inv = 1 / (w0 + w1 + w2);
            }
        }
      if (left_pointing)
        {
          curx = (int)curx1 + 1;
          dif = curx - first_x;
          w0 += dif * A12;
          w1 += dif * A20;
          w2 += dif * A01;
          draw_line(pixels, dep_buf, width, colors, light_dir, curx, (int)scanlineY, (int)curx2, v_fp, A12, A20, A01,
                    w0, w1, w2, sum_inv);
        }
      else
        {
          curx = (int)curx2 + 1;
          dif = curx - first_x;
          w0 += dif * A12;
          w1 += dif * A20;
          w2 += dif * A01;
          draw_line(pixels, dep_buf, width, colors, light_dir, curx, (int)scanlineY, (int)curx1, v_fp, A12, A20, A01,
                    w0, w1, w2, sum_inv);
        }
      first_x = curx;
      curx2 += invslope_long;
      w0 += B12;
      w1 += B20;
      w2 += B01;
    }
}

/*!
 * This method draws a horizontal line from startx to endx on height y meaning it colors the pixels in the
 * pixmap. The AIJ values are passed because they are needed for the calculation of barycentrical coordinates.
 * The barycentrical coordinates interpolate the colors and normals on the triangle.
 */
static void draw_line(unsigned char *pixels, float *dep_buf, int width, const float *colors, vector light_dir,
                      int startx, int y, int endx, vertex_fp *v_fp[3], float A12, float A20, float A01, float w0,
                      float w1, float w2, float sum_inv)
{
  color col;
  int x;
  float depth;
  if (startx < 0)
    {
      int dif = -startx;
      w0 += dif * A12;
      w1 += dif * A20;
      w2 += dif * A01;
      startx = 0;
    }
  for (x = startx; x <= endx && x < width; x += 1)
    {
#ifdef BACKFACE_CULLING
      if (w0 < 0 && w1 < 0 && w2 < 0)
        {
          return;
        }
#endif
      depth = (w0 * v_fp[0]->z + w1 * v_fp[1]->z + w2 * v_fp[2]->z) * sum_inv;
      if (depth < dep_buf[y * width + x])
        {
          col = calc_colors(v_fp[0]->c, v_fp[1]->c, v_fp[2]->c, w0, w1, w2, v_fp, colors, light_dir);
          color_pixel(pixels, dep_buf, depth, width, x, y, &col);
        }
      w0 += A12;
      w1 += A20;
      w2 += A01;
    }
}

/*!
 * This method colors one pixel (x, y) on the screen with the given color and deposit the depth in the depth_buffer.
 *
 * \param [in] pixels array of unsigned char values representing each color value for every pixel on the screen
 * \param [in] depth_buffer depth_buffer
 * \param [in] width number of pixels displayed in one row
 * \param [in] x x-coordinate of the pixel to be colored
 * \param [in] y y-coordinate of the pixel to be colored
 * \param [in] col color for the pixel
 */
static void color_pixel(unsigned char *pixels, float *depth_buffer, float depth, int width, int x, int y, color *col)
{
  pixels[y * width * 4 + x * 4 + 0] = col->r;
  pixels[y * width * 4 + x * 4 + 1] = col->g;
  pixels[y * width * 4 + x * 4 + 2] = col->b;
  pixels[y * width * 4 + x * 4 + 3] = col->a;
  depth_buffer[y * width + x] = depth;
}

/*!
 * This method calculates the color for a pixel, by interpolating the vertex
 * colors with perspective correction in help of the factors in param fac.
 * Furthermore the color is influenced by interpolating the
 * normals and calculating the resulting diffuse light.
 *
 * \param [in] col_one color of one vertex
 * \param [in] col_two color of the second vertex
 * \param [in] col_three color of the third vertex
 * \param [in] fac_one-fac_three the factors of the colors
 * \param [in] v_fp array of the vertices of the triangle
 * \param [in] colors array of 3 values to multiply the colors with when the rasterization/interpolation begins
 * \param [in] light_dir direction of the light to calculate shadows
 * \return a new color as a combination of the given ones*/
static color calc_colors(color_float col_one, color_float col_two, color_float col_three, float fac_one, float fac_two,
                         float fac_three, vertex_fp *v_fp[3], const float *colors, vector light_dir)
{
  /* correct barycentric coordinates
   * (https://github.com/ssloy/tinyrenderer/wiki/Technical-difficulties:-linear-interpolation-with-perspective-deformations)
   */

  float sum, diff_tmp, diff;
  color_float res;
  vector norm;
  color res_float;
  fac_one /= v_fp[0]->w_div;
  fac_two /= v_fp[1]->w_div;
  fac_three /= v_fp[2]->w_div;
  sum = fac_one + fac_two + fac_three;
  fac_one /= sum;
  fac_two /= sum;
  fac_three /= sum;
  /*interpolate color*/
  res = linearcombination_color(col_one, col_two, col_three, fac_one, fac_two, fac_three);
  /* interpolate normal */
  norm = linearcombination(&v_fp[0]->normal, &v_fp[1]->normal, &v_fp[2]->normal, fac_one, fac_two, fac_three);
  /* get light direction*/
  diff_tmp = light_dir.x * norm.x + light_dir.y * norm.y + light_dir.z * norm.z;
  diff = diff_tmp > 0.0 ? diff_tmp : 0.0;
  mult_color(&res, diff, 1);
  res.r *= colors[0];
  res.g *= colors[1];
  res.b *= colors[2];

  res_float = color_float_to_color(res);
  return res_float;
}

/*!
 * This is the method the software-renderer was build for. It is called in gr3_getpixmap_ (file gr3.c) and replaces the
 * invocation of OpenGL commands to create an image inside a given pixmap. Instead, the image arises from the
 * software-rendering process being implemented in this file.
 *
 * \param [in] pixmap given pixmap which has already been allocated to store the final image in
 * \param [in] width width of the final image
 * \param [in] height height of the final image
 * \param [in] ssaa_factor factor for the anti-aliasing
 * \return the final pixmap with the image */
GR3API void gr3_getpixmap_softwarerendered(char *pixmap, int width, int height, int ssaa_factor)
{
  int i, iy, ix;
  unsigned char b_r = (unsigned char)(context_struct_.background_color[0] * 255);
  unsigned char b_g = (unsigned char)(context_struct_.background_color[1] * 255);
  unsigned char b_b = (unsigned char)(context_struct_.background_color[2] * 255);
  unsigned char b_a = (unsigned char)(context_struct_.background_color[3] * 255);
  width *= ssaa_factor;
  height *= ssaa_factor;
#ifndef NO_THREADS
  pthread_mutex_init(&lock, NULL);
  pthread_mutex_init(&lock_main, NULL);
  pthread_cond_init(&wait_for_merge, NULL);
  pthread_cond_init(&wait_after_merge, NULL);
  threads_done = 0;
#endif
  if (width != context_struct_.last_width || height != context_struct_.last_height)
    {
      create_queues_and_pixmaps(width, height);
    }

  for (i = 0; i < width * height; i++)
    {
      context_struct_.depth_buffers[0][i] = 1.0f;
    }

  for (i = 1; i < context_struct_.num_threads; i++)
    {
      memset(context_struct_.depth_buffers[i], 127, width * height * 4);
    }
  if (ssaa_factor != 1)
    {
      context_struct_.pixmaps[0] = malloc(width * height * 4);
    }
  else
    {
      context_struct_.pixmaps[0] = (unsigned char *)pixmap;
    }
  for (iy = 0; iy < height; iy++)
    {
      for (ix = 0; ix < width; ix++)
        {
          context_struct_.pixmaps[0][iy * width * 4 + ix * 4 + 0] = b_r;
          context_struct_.pixmaps[0][iy * width * 4 + ix * 4 + 1] = b_g;
          context_struct_.pixmaps[0][iy * width * 4 + ix * 4 + 2] = b_b;
          context_struct_.pixmaps[0][iy * width * 4 + ix * 4 + 3] = b_a;
        }
    }
  context_struct_.software_renderer_pixmaps_initalised = 1;
  gr3_draw_softwarerendered(context_struct_.queues, width, height);
#ifdef NO_THREADS
  /* Run the code which consumes work packages from the queue
   * after the queue has been filled in `gr3_draw_softwarerendered`
   * if no threading support is available. */
  initialise_consumer(context_struct_.queues, height, width);
#endif

#ifndef NO_THREADS
  pthread_mutex_lock(&lock_main);
  if (threads_done < 2 * context_struct_.num_threads)
    {
      pthread_cond_wait(&wait_after_merge, &lock_main);
    }
  pthread_mutex_unlock(&lock_main);
#endif

  if (ssaa_factor != 1)
    {
      downsample(context_struct_.pixmaps[0], (unsigned char *)pixmap, width, height, ssaa_factor);
    }
}

/*!
 * This method iterates over the draw list containing and calls the method
 * gr3_dodrawmesh_softwarerendered. If there is no element left to draw, it puts an empty
 * element into the queue so that the worker threads drawing the mesh parts know that the
 * drawing process is down.
 *
 * \param [in] queues the array of the queues in which each element belongs to a thread
 * \param [in] width width of the final image
 * \param [in] height height of the final image
 * \return the final pixmap with the image */
static int gr3_draw_softwarerendered(queue *queues[MAX_NUM_THREADS], int width, int height)
{
  GR3_DrawList_t_ *draw;
  int id = 0, i = 0;
  draw = context_struct_.draw_list_;
  if (!draw)
    {
      for (i = 0; i < context_struct_.num_threads; i++)
        {
          args *arg = malloc_arg(i, 0, MAT4x4_INIT_NUL, MAT4x4_INIT_NUL, MAT3x3_INIT_NUL, VECTOR3x1_INIT_NUL, NULL,
                                 NULL, 0, 0, 1, 0, 0, NULL);
          queue_enqueue(context_struct_.queues[i], arg);
        }
    }
  while (draw)
    {
      if (draw->next == NULL)
        {
          id = 1;
        }
      if (draw->vertices_fp)
        {
          for (i = 0; i < draw->n; i++)
            {
              if (draw->vertices_fp[i])
                {
                  free(draw->vertices_fp[i]);
                }
            }
          free(draw->vertices_fp);
        }
      draw->vertices_fp = malloc(draw->n * sizeof(vertex_fp *));
      for (i = 0; i < draw->n; i++)
        {
          draw->vertices_fp[i] = NULL;
        }
      gr3_dodrawmesh_softwarerendered(queues, width, height, draw, id);
      draw = draw->next;
    }
  RETURN_ERROR(GR3_ERROR_NONE);
}

/*!
 * Equal to gr3_dodrawmesh_ in gr3.c with the difference of draw_mesh_softwarerendered being called. It iterates over
 * the meshes and passes it to a method which distributes the meshes on the threads.
 *
 * \param [in] queues the array of the queues in which each element belongs to a thread
 * \param [in] width width of the final image
 * \param [in] height height of the final image
 * \return the final pixmap with the image */
static void gr3_dodrawmesh_softwarerendered(queue *queues[MAX_NUM_THREADS], int width, int height,
                                            GR3_DrawList_t_ *draw, int id)
{
  int i, j;
  float *ups = draw->ups;
  float *directions = draw->directions;
  float *positions = draw->positions;
  int mesh = draw->mesh;
  int n = draw->n;
  float *scales = draw->scales;
  float *colors = draw->colors;

  float forward[3], up[3], left[3];
  float *model_matrix = calloc(16, sizeof(float));
  float *view = malloc(sizeof(float) * 16);
  float tmp;
  int pass_id = 0;
  for (i = 0; i < n; i++)
    {
      {
        /* Calculate an orthonormal base in IR^3, correcting the up vector
         * in case it is not perpendicular to the forward vector. This base
         * is used to create the model matrix as a base-transformation
         * matrix.
         */
        /* forward = normalize(&directions[i*3]); */
        tmp = 0;
        for (j = 0; j < 3; j++)
          {
            tmp += directions[i * 3 + j] * directions[i * 3 + j];
          }
        tmp = sqrt(tmp);
        for (j = 0; j < 3; j++)
          {
            forward[j] = directions[i * 3 + j] / tmp;
          } /* up = normalize(&ups[i*3]); */
        tmp = 0;
        for (j = 0; j < 3; j++)
          {
            tmp += ups[i * 3 + j] * ups[i * 3 + j];
          }
        tmp = sqrt(tmp);
        for (j = 0; j < 3; j++)
          {
            up[j] = ups[i * 3 + j] / tmp;
          }
        /* left = cross(forward,up); */
        for (j = 0; j < 3; j++)
          {
            left[j] = forward[(j + 1) % 3] * up[(j + 2) % 3] - up[(j + 1) % 3] * forward[(j + 2) % 3];
          }
        tmp = 0;
        for (j = 0; j < 3; j++)
          {
            tmp += left[j] * left[j];
          }
        tmp = sqrt(tmp);
        for (j = 0; j < 3; j++)
          {
            left[j] = left[j] / tmp;
          }
        /* up = cross(left,forward); */
        for (j = 0; j < 3; j++)
          {
            up[j] = left[(j + 1) % 3] * forward[(j + 2) % 3] - forward[(j + 1) % 3] * left[(j + 2) % 3];
          }
        for (j = 0; j < 3; j++)
          {
            model_matrix[j] = -left[j] * scales[i * 3 + 0];
            model_matrix[4 + j] = up[j] * scales[i * 3 + 1];
            model_matrix[8 + j] = forward[j] * scales[i * 3 + 2];
            model_matrix[12 + j] = positions[i * 3 + j];
          }
        model_matrix[15] = 1;
      }
      gr3_getviewmatrix(view);
      if (id == 1 && i == (n - 1))
        {
          pass_id = 1;
        }
      draw_mesh_softwarerendered(queues, mesh, model_matrix, view, colors + i * 3, scales + i * 3, width, height,
                                 pass_id, draw, i);
    }
  free(view);
  free(model_matrix);
}

/*!
 * First, this method transforms the vertices of the given mesh. Then it splits the mesh into n parts with n+1
 * being the number of available cores. Every part is then enqueued into certain queue belonging to a thread.
 * As soon as there is a part of a mesh enqueued, the worker threads dequeue it and begin the drawing process
 * by drawing this part into their pixmap.
 */
static int draw_mesh_softwarerendered(queue *queues[MAX_NUM_THREADS], int mesh, float *model, float *view,
                                      const float *colors_facs, const float *scales, int width, int height, int id,
                                      GR3_DrawList_t_ *draw, int draw_id)
{
  int thread_idx, i, j, numtri, tri_per_thread, index_start_end[MAX_NUM_THREADS + 1], rest, rest_distributed;
  matrix model_mat, view_mat, view_model, perspective, perspective_view_model, viewport;
  matrix3x3 model_mat_3x3, view_mat_3x3, model_view_mat_3x3;
  vector light_dir;
  color_float c_tmp;
  vertex_fp *vertices_fp;

  /* initialize transformation matrices */
  for (i = 0; i < 4; i++)
    {
      for (j = 0; j < 4; j++)
        {
          model_mat.mat[i * 4 + j] = model[j * 4 + i];
          view_mat.mat[i * 4 + j] = view[j * 4 + i];
        }
    }
  view_model = mat_mul_4x4(&view_mat, &model_mat);
  perspective = get_projection(width, height, context_struct_.vertical_field_of_view, context_struct_.zNear,
                               context_struct_.zFar, context_struct_.projection_type);
  perspective_view_model = mat_mul_4x4(&perspective, &view_model);
  viewport = matrix_viewport_trafo(width, height);
  for (i = 0; i < 3; i++)
    {
      for (j = 0; j < 3; j++)
        {
          model_mat_3x3.mat[i * 3 + j] = model_mat.mat[i * 4 + j];
          view_mat_3x3.mat[i * 3 + j] = view_mat.mat[i * 4 + j];
        }
    }
  if (context_struct_.option >= 0 && context_struct_.option <= 2)
    {
      matrix3x3 id = {{1, 0, 0, 0, 1, 0, 0, 0, 1}};
      model_view_mat_3x3 = id;
    }
  else
    {
      model_view_mat_3x3 = mat_mul_3x3(&view_mat_3x3, &model_mat_3x3);
    }
  light_dir.x = context_struct_.light_dir[0];
  light_dir.y = context_struct_.light_dir[1];
  light_dir.z = context_struct_.light_dir[2];

  /* default is the lighting coming from the camera, meaning the lighting comes from 0,0,1 referring to view Space */
  if (dot_vector(&light_dir, &light_dir) < 0.001)
    {
      light_dir.z = 1.0;
    }
  else
    {
      normalize_vector(&light_dir);
      mat_vec_mul_3x1(&view_mat_3x3, &light_dir);
    }
  vertices_fp = NULL;
  if (context_struct_.mesh_list_[mesh].data.number_of_indices != 0)
    {
      int num_vertices = context_struct_.mesh_list_[mesh].data.number_of_vertices;
      float *colors = context_struct_.mesh_list_[mesh].data.colors;
      float *normals = context_struct_.mesh_list_[mesh].data.normals;
      float *vertices = context_struct_.mesh_list_[mesh].data.vertices;
      int index = 0;
      vertex_fp tmp_v;
      vector normal_vector;
      draw->vertices_fp[draw_id] = malloc(sizeof(vertex_fp) * context_struct_.mesh_list_[mesh].data.number_of_indices);
      vertices_fp = draw->vertices_fp[draw_id];
      for (i = 0; i < num_vertices * 3; i += 3)
        {
          c_tmp.r = colors[i];
          c_tmp.g = colors[i + 1];
          c_tmp.b = colors[i + 2];
          c_tmp.a = 1.0f;
          tmp_v.x = vertices[i];
          tmp_v.y = vertices[i + 1];
          tmp_v.z = vertices[i + 2];
          tmp_v.w = 1.0;
          tmp_v.w_div = 1.0;
          tmp_v.c = c_tmp;
          normal_vector.x = normals[i] / scales[0];
          normal_vector.y = normals[i + 1] / scales[1];
          normal_vector.z = normals[i + 2] / scales[2];
          tmp_v.normal = normal_vector;
          index = (int)(i / 3);
          vertices_fp[index] = tmp_v;
        }
      for (i = 0; i < num_vertices; i++)
        {
          mat_vec_mul_4x1(&perspective_view_model, &vertices_fp[i]);
          divide_by_w(&vertices_fp[i]);
          mat_vec_mul_4x1(&viewport, &vertices_fp[i]);
          mat_vec_mul_3x1(&model_view_mat_3x3, &vertices_fp[i].normal);
        }

      numtri = context_struct_.mesh_list_[mesh].data.number_of_indices / 3;
      tri_per_thread = numtri / context_struct_.num_threads;
      index_start_end[0] = 0;
      index_start_end[context_struct_.num_threads] = context_struct_.mesh_list_[mesh].data.number_of_indices / 3;
    }
  else
    {
      numtri = context_struct_.mesh_list_[mesh].data.number_of_vertices / 3;
      tri_per_thread = numtri / context_struct_.num_threads;
      index_start_end[context_struct_.num_threads] = context_struct_.mesh_list_[mesh].data.number_of_vertices / 3;
      index_start_end[0] = 0;
    }
  rest = numtri % context_struct_.num_threads;
  rest_distributed = 0;
  for (i = 1; i < context_struct_.num_threads; i++)
    {
      index_start_end[i] = i * tri_per_thread + rest_distributed;
      if (rest >= 1)
        {
          index_start_end[i] += 1;
          rest -= 1;
          rest_distributed += 1;
        }
    }
  for (thread_idx = 0; thread_idx < context_struct_.num_threads; thread_idx++)
    {
      queue_enqueue(queues[thread_idx],
                    malloc_arg(thread_idx, mesh, perspective_view_model, viewport, model_view_mat_3x3, light_dir,
                               colors_facs, scales, width, height, id, index_start_end[thread_idx],
                               index_start_end[thread_idx + 1], vertices_fp));
    }
  return 1;
}

/*!
 * The currently implemented version of ssaa works by first rendering a pixmap in higher resolution and then
 * downsampling it for a smaller pixmap. Nearby pixels are condensed to one pixel by calculating their mean
 * color value.
 *
 * \param [in] pixels_high the higher resoluted pixmap
 * \param [in] pixels_low the lower resoluted pixmap to store the final image in
 * \param [in] width width of the final pixmap
 * \param [in] height height of the final pixmap
 * \param [in] ssaa_factor intensity of ssaa
 * */
static void downsample(unsigned char *pixels_high, unsigned char *pixels_low, int width, int height, int ssaa_factor)
{
  int ix, iy, j, k;
  color col, tmp;
  for (iy = 0; iy < height; iy += ssaa_factor)
    {
      for (ix = 0; ix < width; ix += ssaa_factor)
        {
          color_float col_f = {0.0f, 0.0f, 0.0f, 0.0f};
          for (j = 0; j < ssaa_factor; j++)
            {
              for (k = 0; k < ssaa_factor; k++)
                {
                  tmp.r = pixels_high[iy * width * 4 + ix * 4 + k * 4 + j * width * 4 + 0];
                  tmp.g = pixels_high[iy * width * 4 + ix * 4 + k * 4 + j * width * 4 + 1];
                  tmp.b = pixels_high[iy * width * 4 + ix * 4 + k * 4 + j * width * 4 + 2];
                  tmp.a = pixels_high[iy * width * 4 + ix * 4 + k * 4 + j * width * 4 + 3];
                  col_f.r += tmp.r / 255.0;
                  col_f.g += tmp.g / 255.0;
                  col_f.b += tmp.b / 255.0;
                  col_f.a += tmp.a / 255.0;
                }
            }
          mult_color(&col_f, 1.0 / (ssaa_factor * ssaa_factor), 1.0 / (ssaa_factor * ssaa_factor));
          col = color_float_to_color(col_f);
          pixels_low[iy / ssaa_factor * width / ssaa_factor * 4 + ix / ssaa_factor * 4 + 0] = col.r;
          pixels_low[iy / ssaa_factor * width / ssaa_factor * 4 + ix / ssaa_factor * 4 + 1] = col.g;
          pixels_low[iy / ssaa_factor * width / ssaa_factor * 4 + ix / ssaa_factor * 4 + 2] = col.b;
          pixels_low[iy / ssaa_factor * width / ssaa_factor * 4 + ix / ssaa_factor * 4 + 3] = col.a;
        }
    }
}

/*!
 * Terminates the software-renderer and deletes all the memory that was allocated to use it.
 * */
GR3API void gr3_terminateSR_()
{
  int i;
  args *arg;
  for (i = 0; i < context_struct_.num_threads; i++)
    {
      if (i != 0)
        {
          free(context_struct_.pixmaps[i]);
        }
      free(context_struct_.depth_buffers[i]);
      arg = malloc_arg(i, 0, MAT4x4_INIT_NUL, MAT4x4_INIT_NUL, MAT3x3_INIT_NUL, VECTOR3x1_INIT_NUL, NULL, NULL, 0, 0, 2,
                       0, 0, NULL);
      queue_enqueue(context_struct_.queues[i], arg);
#ifndef NO_THREADS
      pthread_join(context_struct_.threads[i], NULL);
#endif
      queue_destroy(context_struct_.queues[i]);
    }
  for (i = 0; i < context_struct_.mesh_list_capacity_; i++)
    {
      free(context_struct_.mesh_list_[i].data.vertices_fp);
    }
}
