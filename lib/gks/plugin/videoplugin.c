#if !defined(NO_AV) && !defined(NO_CAIRO)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef MAXPATHLEN
#define MAXPATHLEN 1024
#endif

#endif

#include "gks.h"
#include "gkscore.h"

#if !defined(NO_AV)
#include "vc.h"
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

DLLEXPORT void gks_videoplugin(int fctid, int dx, int dy, int dimx, int *i_arr, int len_f_arr_1, double *f_arr_1,
                               int len_f_arr_2, double *f_arr_2, int len_c_arr, char *c_arr, void **ptr);

#ifdef __cplusplus
}
#endif

#if !defined(NO_AV) && !defined(NO_CAIRO)
static gks_state_list_t *gkss;

typedef struct ws_state_list_t
{
  char *path;
  char *mem_path;
  int *mem;
  int width, height, framerate;
  int wtype;
  movie_t movie;
  frame_t frame;
  void *cairo_ws_state_list;
  int video_plugin_initialized;
  int user_defined_resolution;
  int video_flags;
} ws_state_list;

static ws_state_list *p;

static void close_page(void)
{
  if ((p->wtype == 120 || p->wtype == 121 || p->wtype == 130 || p->wtype == 160 || p->wtype == 161 ||
       p->wtype == 162) &&
      p->movie)
    {
      vc_movie_finish(p->movie);
    }
  gks_free(p->movie);
  gks_free(p->frame);
}

static void open_page()
{
  int width, height;
  width = p->mem[0];
  height = p->mem[1];
  char path[MAXPATHLEN];

  if (p->wtype == 120 || p->wtype == 121)
    {
      gks_filepath(path, p->path, "mov", 0, 0);
    }
  else if (p->wtype == 130)
    {
      gks_filepath(path, p->path, "gif", 0, 0);
    }
  else if (p->wtype == 160)
    {
      gks_filepath(path, p->path, "mp4", 0, 0);
    }
  else if (p->wtype == 161)
    {
      gks_filepath(path, p->path, "webm", 0, 0);
    }
  else if (p->wtype == 162)
    {
      gks_filepath(path, p->path, "ogg", 0, 0);
    }
  p->movie = vc_movie_create(path, p->framerate, 4000000, width, height, p->video_flags);
  p->frame = (frame_t)gks_malloc(sizeof(struct frame_t_));
}

static void write_page(void)
{
  int bg[3] = {255, 255, 255};
  int i, j, k;
  int width, height;
  unsigned char *mem;

  if (!p->movie)
    {
      open_page();
    }

  width = p->mem[0];
  height = p->mem[1];

  mem = *((unsigned char **)(p->mem + 3));
  for (i = 0; i < height; i++)
    {
      for (j = 0; j < width; j++)
        {
          long ind = (i * width + j) * 4;
          double alpha = mem[ind + 3] / 255.0;
          for (k = 0; k < 3; k++)
            {
              double col = mem[ind + k] * alpha + bg[k] * (1 - alpha) + 0.5;
              if (col > 255)
                {
                  col = 255;
                }
              mem[ind + k] = (unsigned char)col;
            }
        }
    }
  if (p->movie)
    {
      p->frame->data = mem;
      p->frame->width = width;
      p->frame->height = height;
      vc_movie_append_frame(p->movie, p->frame);
    }
  else
    {
      fprintf(stderr, "Failed to append video frame\n");
    }
}

void gks_videoplugin(int fctid, int dx, int dy, int dimx, int *ia, int lr1, double *r1, int lr2, double *r2, int lc,
                     char *chars, void **ptr)
{
  p = (ws_state_list *)*ptr;

  switch (fctid)
    {
    case 2:
      /* open workstation */

      gkss = (gks_state_list_t *)*ptr;

      p = (ws_state_list *)gks_malloc(sizeof(ws_state_list));
      p->cairo_ws_state_list = *ptr;
      p->video_plugin_initialized = 0;
      p->movie = NULL;
      p->mem = NULL;
      p->wtype = ia[2];
      p->path = chars;
      p->video_flags = 0;
      *ptr = p;

      long width, height, framerate, num_args;
      char *env;
      width = height = framerate = -1;
      env = (char *)gks_getenv("GKS_VIDEO_OPTS");
      if (env)
        {
          /* GKS_VIDEO_OPTS=<width>x<height>@<framerate> */
          num_args = sscanf(env, "%ldx%ld@%ld", &width, &height, &framerate);
          if (num_args == 0 && !(strlen(env) >= 3 && strcmp(env + strlen(env) - 3, "@x2") == 0))
            {
              /* GKS_VIDEO_OPTS invalid */
              fprintf(stderr, "Failed to parse GKS_VIDEO_OPTS. Expected '<width>x<height>@<framerate>', "
                              "'<width>x<height>' or '<framerate>'\n");
              exit(1);
            }
          else if (num_args == 1)
            {
              /* GKS_VIDEO_OPTS=<framerate> */
              framerate = width;
              width = -1;
              height = -1;
            }
          else if (num_args == 2)
            {
              /* GKS_VIDEO_OPTS=<width>x<height> */
              framerate = -1;
            }
          if (strlen(env) >= 3 && strcmp(env + strlen(env) - 3, "@x2") == 0)
            {
              /* Allow @x2 suffix to GKS_VIDEO_OPTS enabling HiDPI mov videos */
              if (p->wtype == 120)
                {
                  p->video_flags |= VC_FLAGS_MOV_HIDPI;
                }
              else
                {
                  fprintf(stderr, "HiDPI is only supported when using mov files.\n");
                  exit(1);
                }
            }
        }
      if (p->wtype == 121)
        {
          p->video_flags |= VC_FLAGS_MOV_HIDPI;
        }

      p->framerate = 24;
      p->width = 720;
      p->height = 720;
      p->user_defined_resolution = 0;

      if (framerate > 0)
        {
          p->framerate = (int)framerate;
        }
      if (width > 0)
        {
          p->width = (int)width;
          p->user_defined_resolution = 1;
        }
      if (height > 0)
        {
          p->height = (int)height;
          p->user_defined_resolution = 1;
        }

      p->mem_path = (char *)gks_malloc(MAXPATHLEN);
      p->mem = (int *)gks_malloc(3 * sizeof(int) + sizeof(unsigned char *));
      p->mem[0] = p->width;
      p->mem[1] = p->height;
      p->mem[2] = 144;
      *((unsigned char **)(p->mem + 3)) = NULL;

      sprintf(p->mem_path, "!resizable@%p.mem", (void *)p->mem);
      chars = p->mem_path;
      /* set wstype for cairo png in memory */
      ia[2] = 143;

      p->video_plugin_initialized = 1;
      break;
    default:;
    }

  if (p && p->user_defined_resolution && fctid == 55)
    {
      /* Ignore setwsviewport if the video resolution is user defined */
      return;
    }

  if (p && p->video_plugin_initialized)
    {
      gks_cairo_plugin(fctid, dx, dy, dimx, ia, lr1, r1, lr2, r2, lc, chars, &(p->cairo_ws_state_list));
    }

  switch (fctid)
    {
    case 2:
      /* open workstation */
      if (p->mem_path)
        {
          gks_free(p->mem_path);
          p->mem_path = NULL;
        }
      break;
    case 3:
      /* close workstation */
      if (!p)
        {
          break;
        }
      p->video_plugin_initialized = 0;
      close_page();
      if (p->mem)
        {
          unsigned char *mem_ptr = *((unsigned char **)(p->mem + 3));
          if (mem_ptr)
            {
              gks_free(mem_ptr);
            }
          gks_free(p->mem);
        }
      gks_free(p);
      break;
    case 8:
      /* update workstation */
      if (ia[1] & GKS_K_WRITE_PAGE_FLAG)
        {
          write_page();
        }
      break;
    default:;
    }
}

#else

void gks_videoplugin(int fctid, int dx, int dy, int dimx, int *ia, int lr1, double *r1, int lr2, double *r2, int lc,
                     char *chars, void **ptr)
{
  if (fctid == 2)
    {
      gks_perror("Video support not compiled in");
      ia[0] = 0;
    }
}

#endif
