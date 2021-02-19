
#include <string.h>
#include <stdlib.h>

#include "gks.h"
#include "gkscore.h"

#define SEGM_SIZE 262144 /* 256K */

#define COPY(s, n)                              \
  memmove(d->buffer + d->nbytes, (void *)s, n); \
  d->nbytes += n
#define PAD(n)                         \
  memset(d->buffer + d->nbytes, 0, n); \
  d->nbytes += n


static void reallocate(gks_display_list_t *d, int len)
{
  while (d->nbytes + len > d->size) d->size += SEGM_SIZE;

  d->buffer = (char *)gks_realloc(d->buffer, d->size + 1);
}

static int purge(gks_display_list_t *d, char *t)
/*
   Clear display list preserving workstation specific functions.
   Return purged display list (t) and length (in bytes)
 */
{
  char *s;
  int i;
  int sp = 0, tp = 0, *len, fctid;
  static const char *attribute_buffer[MAX_ATTRIBUTE_FCTID + 1];
  static const char *color_buffer[MAX_COLOR];
  memset(attribute_buffer, 0, sizeof(char *) * (MAX_ATTRIBUTE_FCTID + 1));
  memset(color_buffer, 0, sizeof(char *) * MAX_COLOR);

  s = d->buffer;
  len = (int *)(s + sp);
  while (*len)
    {
      fctid = *(int *)(s + sp + sizeof(int));
      switch (fctid)
        {
        case 48: /* setcolorrep */
          {
            int colorind = *(int *)(s + sp + 2 * sizeof(int));
            if (colorind >= 0 && colorind < MAX_COLOR)
              {
                color_buffer[colorind] = s + sp;
              }
          }
          break;
        case 54: /* setwswindow */
        case 55: /* setwsviewport */
          attribute_buffer[fctid] = s + sp;
          break;
        default:
          break;
        }
      sp += *len;
      len = (int *)(s + sp);
    }
  for (i = 0; i < MAX_COLOR; i++)
    {
      if (color_buffer[i])
        {
          len = (int *)(color_buffer[i]);
          memmove(t + tp, color_buffer[i], *len);
          tp += *len;
        }
    }
  for (i = 0; i <= MAX_ATTRIBUTE_FCTID; i++)
    {
      if (attribute_buffer[i])
        {
          len = (int *)(attribute_buffer[i]);
          memmove(t + tp, attribute_buffer[i], *len);
          tp += *len;
        }
    }
  return tp;
}

void gks_dl_write_item(gks_display_list_t *d, int fctid, int dx, int dy, int dimx, int *i_arr, int len_f_arr_1,
                       double *f_arr_1, int len_f_arr_2, double *f_arr_2, int len_c_arr, char *c_arr,
                       gks_state_list_t *gkss)
{
  char s[132], *t = NULL;
  int len, slen, tp = 0;

  switch (fctid)
    {
    case 2: /* open workstation */

      d->state = GKS_K_WS_INACTIVE;
      d->buffer = (char *)gks_malloc(SEGM_SIZE + 1);
      d->size = SEGM_SIZE;
      d->nbytes = d->position = 0;
      d->empty = 1;

      len = 2 * sizeof(int) + sizeof(gks_state_list_t);

      COPY(&len, sizeof(int));
      COPY(&fctid, sizeof(int));
      COPY(gkss, sizeof(gks_state_list_t));
      break;

    case 3: /* close workstation */

      free(d->buffer);
      d->buffer = NULL;
      break;

    case 4: /* activate workstation */

      d->state = GKS_K_WS_ACTIVE;
      break;

    case 5: /* deactivate workstation */

      d->state = GKS_K_WS_INACTIVE;
      break;

    case 6: /* clear workstation */

      t = gks_malloc(d->size);
      tp = purge(d, t);
      d->nbytes = d->position = 0;

      len = 2 * sizeof(int) + sizeof(gks_state_list_t);
      fctid = 2;

      COPY(&len, sizeof(int));
      COPY(&fctid, sizeof(int));
      COPY(gkss, sizeof(gks_state_list_t));

      COPY(t, tp);
      free(t);
      break;

    case 12: /* polyline */
    case 13: /* polymarker */
    case 15: /* fill area */
      if (d->state == GKS_K_WS_ACTIVE)
        {
          len = 3 * sizeof(int) + 2 * i_arr[0] * sizeof(double);
          if (d->nbytes + len > d->size) reallocate(d, len);

          COPY(&len, sizeof(int));
          COPY(&fctid, sizeof(int));
          COPY(i_arr, sizeof(int));
          COPY(f_arr_1, i_arr[0] * sizeof(double));
          COPY(f_arr_2, i_arr[0] * sizeof(double));

          d->empty = 0;
        }
      break;

    case 14: /* text */

      if (d->state == GKS_K_WS_ACTIVE)
        {
          len = 3 * sizeof(int) + 2 * sizeof(double) + 132;
          if (d->nbytes + len > d->size) reallocate(d, len);

          memset((void *)s, 0, 132);
          slen = strlen(c_arr);
          strncpy(s, c_arr, slen);

          COPY(&len, sizeof(int));
          COPY(&fctid, sizeof(int));
          COPY(f_arr_1, sizeof(double));
          COPY(f_arr_2, sizeof(double));
          COPY(&slen, sizeof(int));
          COPY(s, 132);

          d->empty = 0;
        }
      break;

    case 16:  /* cell array */
    case 201: /* draw image */

      if (d->state == GKS_K_WS_ACTIVE)
        {
          len = (5 + dimx * dy) * sizeof(int) + 4 * sizeof(double);
          if (d->nbytes + len > d->size) reallocate(d, len);

          COPY(&len, sizeof(int));
          COPY(&fctid, sizeof(int));
          COPY(f_arr_1, 2 * sizeof(double));
          COPY(f_arr_2, 2 * sizeof(double));
          COPY(&dx, sizeof(int));
          COPY(&dy, sizeof(int));
          COPY(&dimx, sizeof(int));
          tp = dimx * (dy - 1) + dx;
          COPY(i_arr, tp * sizeof(int));
          PAD((dimx - dx) * sizeof(int)); /* (dimx * dy - tp) elements */

          d->empty = 0;
        }
      break;

    case 17: /* GDP */
      if (d->state == GKS_K_WS_ACTIVE)
        {
          len = (2 + 3 + i_arr[2]) * sizeof(int) + 2 * i_arr[0] * sizeof(double);
          if (d->nbytes + len > d->size) reallocate(d, len);

          COPY(&len, sizeof(int));
          COPY(&fctid, sizeof(int));
          COPY(i_arr, (3 + i_arr[2]) * sizeof(int));
          COPY(f_arr_1, i_arr[0] * sizeof(double));
          COPY(f_arr_2, i_arr[0] * sizeof(double));

          d->empty = 0;
        }
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

      len = 3 * sizeof(int);
      if (d->nbytes + len > d->size) reallocate(d, len);

      COPY(&len, sizeof(int));
      COPY(&fctid, sizeof(int));
      COPY(i_arr, sizeof(int));
      break;

    case 27: /* set text font and precision */
    case 34: /* set text alignment */

      len = 4 * sizeof(int);
      if (d->nbytes + len > d->size) reallocate(d, len);

      COPY(&len, sizeof(int));
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

      len = 2 * sizeof(int) + sizeof(double);
      if (d->nbytes + len > d->size) reallocate(d, len);

      COPY(&len, sizeof(int));
      COPY(&fctid, sizeof(int));
      COPY(f_arr_1, sizeof(double));
      break;

    case 32: /* set character up vector */

      len = 2 * sizeof(int) + 2 * sizeof(double);
      if (d->nbytes + len > d->size) reallocate(d, len);

      COPY(&len, sizeof(int));
      COPY(&fctid, sizeof(int));
      COPY(f_arr_1, sizeof(double));
      COPY(f_arr_2, sizeof(double));
      break;

    case 41: /* set aspect source flags */

      len = 2 * sizeof(int) + 13 * sizeof(int);
      if (d->nbytes + len > d->size) reallocate(d, len);

      COPY(&len, sizeof(int));
      COPY(&fctid, sizeof(int));
      COPY(i_arr, 13 * sizeof(int));
      break;

    case 48: /* set color representation */

      len = 3 * sizeof(int) + 3 * sizeof(double);
      if (d->nbytes + len > d->size) reallocate(d, len);

      COPY(&len, sizeof(int));
      COPY(&fctid, sizeof(int));
      COPY(&i_arr[1], sizeof(int));
      COPY(f_arr_1, 3 * sizeof(double));
      break;

    case 49: /* set window */
    case 50: /* set viewport */
    case 54: /* set workstation window */
    case 55: /* set workstation viewport */

      len = 3 * sizeof(int) + 4 * sizeof(double);
      if (d->nbytes + len > d->size) reallocate(d, len);

      COPY(&len, sizeof(int));
      COPY(&fctid, sizeof(int));
      COPY(i_arr, sizeof(int));
      COPY(f_arr_1, 2 * sizeof(double));
      COPY(f_arr_2, 2 * sizeof(double));
      break;

    case 202: /* set shadow */

      len = 2 * sizeof(int) + 3 * sizeof(double);
      if (d->nbytes + len >= d->size) reallocate(d, len);

      COPY(&len, sizeof(int));
      COPY(&fctid, sizeof(int));
      COPY(f_arr_1, 3 * sizeof(double));
      break;

    case 204: /* set coord xform */

      len = 2 * sizeof(int) + 6 * sizeof(double);
      if (d->nbytes + len >= d->size) reallocate(d, len);

      COPY(&len, sizeof(int));
      COPY(&fctid, sizeof(int));
      COPY(f_arr_1, 6 * sizeof(double));
      break;
    }

  if (d->buffer != NULL)
    {
      if (d->nbytes + 4 > d->size) reallocate(d, 4);

      memset(d->buffer + d->nbytes, 0, 4);
    }
}
