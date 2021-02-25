#ifndef GR3_SR_H_INCLUDED
#define GR3_SR_H_INCLUDED
#ifndef _MSC_VER
#include <unistd.h>
#else
#define NO_THREADS 1
#include <io.h>
#endif
#ifndef NO_THREADS
#include <pthread.h>
#endif
#include "gr.h"
#include "gr3.h"
#include "gr3_internals.h"
#define PI 3.14159265358979323846
#ifndef NO_THREADS
#define MAX_NUM_THREADS 256
#else
#define MAX_NUM_THREADS 1
#endif
#define SUCCESS 0
#define ERR_INVAL 1
#define ERR_NOMEM 2

struct queue_node_s
{
  struct queue_node_s *next;
  void *data;
};

struct queue_s
{
#ifndef NO_THREADS
  pthread_mutex_t lock;
  pthread_cond_t cond;
#endif
  struct queue_node_s *front;
  struct queue_node_s *back;
};
typedef struct queue_s queue;

/* defines the queue for a mesh to take the mesh parts out and
 * the area of the pixmaps to be merged by the thread */
struct queue_merge_area
{
  queue *queue;
  int width;
  int starty;
  int endy;
};

typedef struct
{
  float x;
  float y;
  float z;
} vector;

typedef struct
{
  float mat[16];
} matrix;

typedef struct
{
  float mat[9];
} matrix3x3;

typedef struct
{
  unsigned char r;
  unsigned char g;
  unsigned char b;
  unsigned char a;
} color;

typedef struct
{
  float r;
  float g;
  float b;
  float a;
} color_float;

typedef struct
{
  float x;
  float y;
  float z;
  float w;
  float w_div;
  color_float c;
  vector normal;
} vertex_fp;

typedef struct
{
  int thread_idx;
  int mesh;
  matrix model_view_perspective;
  matrix viewport;
  matrix3x3 model_view_3x3;
  vector light_dir;
  const float *colors;
  const float *scales;
  int width;
  int height;
  int id;
  int idxstart;
  int idxend;
  vertex_fp *vertices_fp;
} args;

GR3API int gr3_initSR_();
GR3API void gr3_getpixmap_softwarerendered(char *pixmap, int width, int height, int ssaa_factor);
GR3API void gr3_terminateSR_();

#endif
