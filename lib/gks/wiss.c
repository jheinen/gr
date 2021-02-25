
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#if !defined(VMS) && !defined(_WIN32)
#include <unistd.h>
#endif

#include "gks.h"
#include "gkscore.h"

#define SEGM_SIZE 262144 /* 256K */

#define COPY(s, n)                              \
  memmove(p->buffer + p->nbytes, (void *)s, n); \
  p->nbytes += n
#define PAD(n)                         \
  memset(p->buffer + p->nbytes, 0, n); \
  p->nbytes += n

#define RESOLVE(arg, type, nbytes) \
  arg = (type *)(s + sp);          \
  sp += nbytes

typedef struct ws_state_list_struct
{
  int conid, state;
  int segn;
  int empty;
  char *buffer;
  int size, nbytes;
} ws_state_list;

static ws_state_list *p;
static gks_state_list_t *gkss;
static int wkid = 1;
static int unused_variable = 0;

static void reallocate(int len)
{
  while (p->nbytes + len > p->size) p->size += SEGM_SIZE;

  p->buffer = (char *)gks_realloc(p->buffer, p->size + 1);
  if (p->buffer == NULL)
    {
      gks_perror("memory allocation failed");
      exit(1);
    }
}

#if 0

static
int cksum(void)
{
  int i, crc = 0;

  for (i = 0; i < p->nbytes; i++)
    {
      if (crc & 1)
        crc |= 0x10000;
      crc = ((crc >> 1) + p->buffer[i]) & 0xffff;
    }
  return crc;
}

#endif

static void write_item(int sgnum, int fctid, int dx, int dy, int dimx, int *i_arr, int len_farr_1, double *f_arr_1,
                       int len_farr_2, double *f_arr_2, int len_c_arr, char *c_arr)
{
  char s[132];
  int len = -1, slen, tp = 0;

  switch (fctid)
    {
    case 12: /* polyline */
    case 13: /* polymarker */
    case 15: /* fill area */

      len = 4 * sizeof(int) + 2 * i_arr[0] * sizeof(double);
      if (p->nbytes + len > p->size) reallocate(len);

      COPY(&len, sizeof(int));
      COPY(&sgnum, sizeof(int));
      COPY(&fctid, sizeof(int));
      COPY(i_arr, sizeof(int));
      COPY(f_arr_1, i_arr[0] * sizeof(double));
      COPY(f_arr_2, i_arr[0] * sizeof(double));
      break;

    case 14: /* text */

      len = 4 * sizeof(int) + 2 * sizeof(double) + 132;
      if (p->nbytes + len > p->size) reallocate(len);

      memset((void *)s, 0, 132);
      slen = strlen(c_arr);
      strncpy(s, c_arr, slen);

      COPY(&len, sizeof(int));
      COPY(&sgnum, sizeof(int));
      COPY(&fctid, sizeof(int));
      COPY(f_arr_1, sizeof(double));
      COPY(f_arr_2, sizeof(double));
      COPY(&slen, sizeof(int));
      COPY(s, 132);
      break;

    case 16:  /* cell array */
    case 201: /* draw image */

      len = (6 + dimx * dy) * sizeof(int) + 4 * sizeof(double);
      if (p->nbytes + len > p->size) reallocate(len);

      COPY(&len, sizeof(int));
      COPY(&sgnum, sizeof(int));
      COPY(&fctid, sizeof(int));
      COPY(f_arr_1, 2 * sizeof(double));
      COPY(f_arr_2, 2 * sizeof(double));
      COPY(&dx, sizeof(int));
      COPY(&dy, sizeof(int));
      COPY(&dimx, sizeof(int));
      tp = dimx * (dy - 1) + dx;
      COPY(i_arr, tp * sizeof(int));
      PAD((dimx - dx) * sizeof(int)); /* (dimx * dy - tp) elements */
      break;

    case 17: /* GDP */
      len = (3 + 3 + i_arr[2]) * sizeof(int) + 2 * i_arr[0] * sizeof(double);
      if (p->nbytes + len > p->size) reallocate(len);

      COPY(&len, sizeof(int));
      COPY(&sgnum, sizeof(int));
      COPY(&fctid, sizeof(int));
      COPY(i_arr, (3 + i_arr[2]) * sizeof(int));
      COPY(f_arr_1, i_arr[0] * sizeof(double));
      COPY(f_arr_2, i_arr[0] * sizeof(double));
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
    case 207: /* set border color index */
    case 208: /* select clipping transformation */

      len = 4 * sizeof(int);
      if (p->nbytes + len > p->size) reallocate(len);

      COPY(&len, sizeof(int));
      COPY(&sgnum, sizeof(int));
      COPY(&fctid, sizeof(int));
      COPY(i_arr, sizeof(int));
      break;

    case 27: /* set text font and precision */
    case 34: /* set text alignment */

      len = 5 * sizeof(int);
      if (p->nbytes + len > p->size) reallocate(len);

      COPY(&len, sizeof(int));
      COPY(&sgnum, sizeof(int));
      COPY(&fctid, sizeof(int));
      COPY(i_arr, 2 * sizeof(int));
      break;

    case 20:  /* set linewidth scale factor */
    case 24:  /* set marker size scale factor */
    case 28:  /* set character expansion factor */
    case 29:  /* set character spacing */
    case 31:  /* set character height */
    case 200: /* set text slant */
    case 203: /* set transparency */
    case 206: /* set border width */

      len = 3 * sizeof(int) + sizeof(double);
      if (p->nbytes + len > p->size) reallocate(len);

      COPY(&len, sizeof(int));
      COPY(&sgnum, sizeof(int));
      COPY(&fctid, sizeof(int));
      COPY(f_arr_1, sizeof(double));
      break;

    case 32: /* set character up vector */

      len = 3 * sizeof(int) + 2 * sizeof(double);
      if (p->nbytes + len > p->size) reallocate(len);

      COPY(&len, sizeof(int));
      COPY(&sgnum, sizeof(int));
      COPY(&fctid, sizeof(int));
      COPY(f_arr_1, sizeof(double));
      COPY(f_arr_2, sizeof(double));
      break;

    case 41: /* set aspect source flags */

      len = 3 * sizeof(int) + 13 * sizeof(int);
      if (p->nbytes + len > p->size) reallocate(len);

      COPY(&len, sizeof(int));
      COPY(&sgnum, sizeof(int));
      COPY(&fctid, sizeof(int));
      COPY(i_arr, 13 * sizeof(int));
      break;

    case 48: /* set color representation */

      len = 4 * sizeof(int) + 3 * sizeof(double);
      if (p->nbytes + len > p->size) reallocate(len);

      COPY(&len, sizeof(int));
      COPY(&sgnum, sizeof(int));
      COPY(&fctid, sizeof(int));
      COPY(&i_arr[1], sizeof(int));
      COPY(f_arr_1, 3 * sizeof(double));
      break;

    case 49: /* set window */
    case 50: /* set viewport */
    case 54: /* set workstation window */
    case 55: /* set workstation viewport */

      len = 4 * sizeof(int) + 4 * sizeof(double);
      if (p->nbytes + len > p->size) reallocate(len);

      COPY(&len, sizeof(int));
      COPY(&sgnum, sizeof(int));
      COPY(&fctid, sizeof(int));
      COPY(i_arr, sizeof(int));
      COPY(f_arr_1, 2 * sizeof(double));
      COPY(f_arr_2, 2 * sizeof(double));
      break;

    case 202: /* set shadow */

      len = 3 * sizeof(int) + 3 * sizeof(double);
      if (p->nbytes + len > p->size) reallocate(len);

      COPY(&len, sizeof(int));
      COPY(&sgnum, sizeof(int));
      COPY(&fctid, sizeof(int));
      COPY(f_arr_1, 3 * sizeof(double));
      break;

    case 204: /* set coord xform */

      len = 3 * sizeof(int) + 6 * sizeof(double);
      if (p->nbytes + len > p->size) reallocate(len);

      COPY(&len, sizeof(int));
      COPY(&sgnum, sizeof(int));
      COPY(&fctid, sizeof(int));
      COPY(f_arr_1, 6 * sizeof(double));
      break;
    }
}

static void delete_seg(char *str, int segn)
{
  char *s, *d;
  int sp = 0, *len, *sgnum, dp = 0, saved_sp;

  s = d = str;

  saved_sp = sp;
  RESOLVE(len, int, sizeof(int));

  while (*len)
    {
      RESOLVE(sgnum, int, sizeof(int));
      sp = saved_sp;

      if (*sgnum != 0 && segn != *sgnum)
        {
          if (sp > dp) memmove(d + dp, s + sp, *len);
          dp += *len;
        }
      sp += *len;

      saved_sp = sp;
      RESOLVE(len, int, sizeof(int));
    }

  if (sp > dp)
    {
      memset(d + dp, 0, sp - dp);
      p->nbytes = dp;
    }
}

void gks_drv_wiss(int fctid, int dx, int dy, int dimx, int *i_arr, int len_farr_1, double *f_arr_1, int len_farr_2,
                  double *f_arr_2, int len_c_arr, char *c_arr, void **ptr)
{
  p = (ws_state_list *)*ptr;

  switch (fctid)
    {
    case 2: /* open workstation */

      gkss = (gks_state_list_t *)*ptr;

      p = (ws_state_list *)gks_malloc(sizeof(ws_state_list));

      p->conid = i_arr[1];
      p->state = GKS_K_WS_INACTIVE;
      p->segn = 0;
      p->empty = 1;

      p->buffer = (char *)gks_malloc(SEGM_SIZE);
      p->size = SEGM_SIZE;
      p->nbytes = 0;

      *ptr = p;
      break;

    case 3: /* close workstation */

      free(p->buffer);
      free(p);

      p = NULL;
      break;

    case 4: /* activate workstation */

      p->state = GKS_K_WS_ACTIVE;
      break;

    case 5: /* deactivate workstation */

      p->state = GKS_K_WS_INACTIVE;
      break;

    case 6: /* clear workstation */

      p->nbytes = 0;
      p->empty = 1;
      memset(p->buffer, 0, p->size);
      break;

    case 8: /* update workstation */

      break;

    case 12:
    case 13:
    case 14:
    case 15:
    case 16:
    case 17:
      p->empty = 0;
    case 19:
    case 20:
    case 21:
    case 23:
    case 24:
    case 25:
    case 27:
    case 28:
    case 29:
    case 30:
    case 31:
    case 32:
    case 33:
    case 34:
    case 36:
    case 37:
    case 38:
    case 41:
    case 48:
    case 49:
    case 50:
    case 52:
    case 53:
    case 54:
    case 55:
    case 108:
    case 200:
    case 201:
    case 202:
    case 203:
    case 204:
    case 206:
    case 207:
    case 208:

      if (p->state == GKS_K_WS_ACTIVE)
        {
          if (p->segn != 0)
            {
              int len = 3 * sizeof(int) + sizeof(gks_state_list_t);

              if (p->nbytes == 0)
                {
                  int fctid = 2, sgnum = 0;

                  COPY(&len, sizeof(int));
                  COPY(&sgnum, sizeof(int));
                  COPY(&fctid, sizeof(int));
                  COPY(gkss, sizeof(gks_state_list_t));
                }

              write_item(p->segn, fctid, dx, dy, dimx, i_arr, len_farr_1, f_arr_1, len_farr_2, f_arr_2, len_c_arr,
                         c_arr);
            }
        }
      break;

    case 56: /* create segment */

      p->segn = i_arr[0];
      break;

    case 57: /* close segment */

      p->segn = 0;
      break;

    case 58: /* delete segment */

      delete_seg(p->buffer, i_arr[0]);
      break;
    }
}

static void interp(char *str, int segn)
{
  char *s;
  gks_state_list_t *gkss = NULL;
  int sp = 0, *len, *sgnum, *fctid, sx = 1, sy = 1;
  int *i_arr = NULL, *dx = NULL, *dy = NULL, *dimx = NULL, *len_c_arr = NULL;
  int *n = NULL, *primid = NULL, *ldr = NULL;
  double *f_arr_1 = NULL, *f_arr_2 = NULL;
  char *c_arr = NULL;
  int saved_sp;
  double mat[3][2];

  s = str;

  saved_sp = sp;
  RESOLVE(len, int, sizeof(int));

  while (*len)
    {
      RESOLVE(sgnum, int, sizeof(int));
      RESOLVE(fctid, int, sizeof(int));

      switch (*fctid)
        {
        case 2:

          RESOLVE(gkss, gks_state_list_t, sizeof(gks_state_list_t));
          unused_variable = gkss->ltype;
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
        case 207: /* set border color index */
        case 208: /* select clipping transformation */

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
        case 206: /* set border width */

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
          gks_perror("segment storage is corrupted (len=%d, fctid=%d)", *len, *fctid);
          exit(1);
        }

      if (sp != saved_sp + *len)
        {
          gks_perror("internal error (fctid=%d)", *fctid);
          exit(1);
        }

      if (segn == 0 || *sgnum == segn)
        {
          switch (*fctid)
            {
            case 12:
              gks_polyline(i_arr[0], f_arr_1, f_arr_2);
              break;
            case 13:
              gks_polymarker(i_arr[0], f_arr_1, f_arr_2);
              break;
            case 14:
              unused_variable = *len_c_arr;
              gks_text(f_arr_1[0], f_arr_2[0], c_arr);
              break;
            case 15:
              gks_fillarea(i_arr[0], f_arr_1, f_arr_2);
              break;
            case 16:
              gks_cellarray(f_arr_1[0], f_arr_2[0], f_arr_1[1], f_arr_2[1], *dx, *dy, sx, sy, *dimx, *dy, i_arr);
              break;
            case 17:
              gks_gdp(*n, f_arr_1, f_arr_2, *primid, *ldr, i_arr);
              break;

            case 19:
              gks_set_pline_linetype(i_arr[0]);
              break;
            case 20:
              gks_set_pline_linewidth(f_arr_1[0]);
              break;
            case 21:
              gks_set_pline_color_index(i_arr[0]);
              break;
            case 23:
              gks_set_pmark_type(i_arr[0]);
              break;
            case 24:
              gks_set_pmark_size(f_arr_1[0]);
              break;
            case 25:
              gks_set_pmark_color_index(i_arr[0]);
              break;
            case 27:
              gks_set_text_fontprec(i_arr[0], i_arr[1]);
              break;
            case 28:
              gks_set_text_expfac(f_arr_1[0]);
              break;
            case 29:
              gks_set_text_spacing(f_arr_1[0]);
              break;
            case 30:
              gks_set_text_color_index(i_arr[0]);
              break;
            case 31:
              gks_set_text_height(f_arr_1[0]);
              break;
            case 32:
              gks_set_text_upvec(f_arr_1[0], f_arr_2[0]);
              break;
            case 33:
              gks_set_text_path(i_arr[0]);
              break;
            case 34:
              gks_set_text_align(i_arr[0], i_arr[1]);
              break;
            case 36:
              gks_set_fill_int_style(i_arr[0]);
              break;
            case 37:
              gks_set_fill_style_index(i_arr[0]);
              break;
            case 38:
              gks_set_fill_color_index(i_arr[0]);
              break;
            case 41:
              gks_set_asf(i_arr);
              break;

            case 48:
              gks_set_color_rep(wkid, i_arr[0], f_arr_1[0], f_arr_1[1], f_arr_1[2]);
              break;

            case 49:
              gks_set_window(i_arr[0], f_arr_1[0], f_arr_1[1], f_arr_2[0], f_arr_2[1]);
              break;
            case 50:
              gks_set_viewport(i_arr[0], f_arr_1[0], f_arr_1[1], f_arr_2[0], f_arr_2[1]);
              break;
            case 52:
              gks_select_xform(i_arr[0]);
              break;
            case 53:
              gks_set_clipping(i_arr[0]);
              break;
            case 54:
              gks_set_ws_window(i_arr[0], f_arr_1[0], f_arr_1[1], f_arr_2[0], f_arr_2[1]);
              break;
            case 55:
              gks_set_ws_viewport(i_arr[0], f_arr_1[0], f_arr_1[1], f_arr_2[0], f_arr_2[1]);
              break;

            case 108:
              gks_set_resample_method(i_arr[0]);
              break;
            case 200:
              gks_set_text_slant(f_arr_1[0]);
              break;
            case 201:
              gks_draw_image(f_arr_1[0], f_arr_2[0], f_arr_1[1], f_arr_2[1], *dx, *dy, i_arr);
              break;
            case 202:
              gks_set_shadow(f_arr_1[0], f_arr_1[1], f_arr_1[2]);
              break;
            case 203:
              gks_set_transparency(f_arr_1[0]);
              break;
            case 204:
              mat[0][0] = f_arr_1[0];
              mat[0][1] = f_arr_1[1];
              mat[1][0] = f_arr_1[2];
              mat[1][1] = f_arr_1[3];
              mat[2][0] = f_arr_1[4];
              mat[2][1] = f_arr_1[5];
              gks_set_coord_xform(mat);
              break;
            case 206:
              gks_set_border_width(f_arr_1[0]);
              break;
            case 207:
              gks_set_border_color_index(i_arr[0]);
              break;
            case 208:
              gks_select_clip_xform(i_arr[0]);
              break;
            }
        }

      saved_sp = sp;
      RESOLVE(len, int, sizeof(int));
    }
}

void gks_wiss_dispatch(int fctid, int wkid, int segn)
{
  interp(p->buffer, segn);
}
