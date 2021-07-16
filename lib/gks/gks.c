#ifdef __unix__
#define _POSIX_C_SOURCE 1
#endif

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include <locale.h>

#if !defined(VMS) && !defined(_WIN32)
#include <unistd.h>
#endif

#include "gks.h"
#include "gkscore.h"

#ifndef MAXPATHLEN
#define MAXPATHLEN 1024
#endif

#ifndef DBL_EPSILON
#define DBL_EPSILON 2.2204460492503131e-16
#endif

#define OK 0
#define MAX_POINTS 2048

static gks_state_list_t *s = NULL, *seg_state = NULL;

static int state = GKS_K_GKCL, api = 1;

static int i_arr[13];
static double f_arr_1[6], f_arr_2[6];
static char c_arr[1];
static int id = 0;

static gks_list_t *open_ws = NULL, *active_ws = NULL, *av_ws_types = NULL;

static ws_descr_t ws_types[] = {{2, GKS_K_METERS, 1.00000, 1.00000, 65536, 65536, 4, "mf", NULL},
                                {3, GKS_K_METERS, 1.00000, 1.00000, 65536, 65536, 5, "mf", NULL},
                                {5, GKS_K_METERS, 1.00000, 1.00000, 32767, 32767, 3, NULL, NULL},
                                {41, GKS_K_METERS, 0.33300, 0.28100, 1024, 864, 2, NULL, NULL},
                                {61, GKS_K_METERS, 0.28575, 0.19685, 6750, 4650, 0, "ps", NULL},
                                {62, GKS_K_METERS, 0.28575, 0.19685, 6750, 4650, 0, "ps", NULL},
                                {63, GKS_K_METERS, 0.28575, 0.19685, 6750, 4650, 0, "ps", NULL},
                                {64, GKS_K_METERS, 0.28575, 0.19685, 6750, 4650, 0, "ps", NULL},
                                {100, GKS_K_METERS, 0.25400, 0.19050, 1024, 768, 0, NULL, NULL},
                                {101, GKS_K_METERS, 0.28800, 0.19840, 810, 558, 0, "pdf", NULL},
                                {102, GKS_K_METERS, 0.28800, 0.19840, 810, 558, 0, "pdf", NULL},
                                {120, GKS_K_METERS, 0.25400, 0.19050, 1440, 1080, 0, "mov", NULL},
                                {121, GKS_K_METERS, 0.25400, 0.19050, 1440, 1080, 0, "mov", NULL},
                                {130, GKS_K_METERS, 0.25400, 0.19050, 1440, 1080, 0, "gif", NULL},
                                {140, GKS_K_METERS, 0.28575, 0.19685, 6750, 4650, 0, "png", NULL},
                                {141, GKS_K_METERS, 0.25400, 0.19050, 1024, 768, 0, NULL, NULL},
                                {142, GKS_K_METERS, 0.25400, 0.19050, 1024, 768, 0, NULL, NULL},
                                {143, GKS_K_METERS, 0.28575, 0.19685, 6750, 4650, 0, NULL, NULL},
                                {144, GKS_K_METERS, 0.28575, 0.19685, 6750, 4650, 0, "jpg", NULL},
                                {145, GKS_K_METERS, 0.28575, 0.19685, 6750, 4650, 0, "bmp", NULL},
                                {146, GKS_K_METERS, 0.28575, 0.19685, 6750, 4650, 0, "tif", NULL},
                                {150, GKS_K_METERS, 0.20320, 0.15240, 560, 420, 0, "six", NULL},
                                {160, GKS_K_METERS, 0.25400, 0.19050, 1440, 1080, 0, "mp4", NULL},
                                {161, GKS_K_METERS, 0.25400, 0.19050, 1440, 1080, 0, "webm", NULL},
                                {162, GKS_K_METERS, 0.25400, 0.19050, 1440, 1080, 0, "ogg", NULL},
                                {210, GKS_K_METERS, 0.33300, 0.28100, 1024, 864, 0, NULL, NULL},
                                {211, GKS_K_METERS, 0.33300, 0.28100, 1024, 864, 2, NULL, NULL},
                                {212, GKS_K_METERS, 0.33300, 0.28100, 1024, 864, 2, NULL, NULL},
                                {213, GKS_K_METERS, 0.33300, 0.28100, 1024, 864, 2, NULL, NULL},
                                {214, GKS_K_METERS, 0.33300, 0.28100, 1024, 864, 0, "rf", "GKS_RF"},
                                {215, GKS_K_METERS, 0.33300, 0.28100, 1024, 864, 0, "gif", "GKS_GIF"},
                                {216, GKS_K_METERS, 0.33300, 0.28100, 1024, 864, 0, "uil", "GKS_UIL"},
                                {217, GKS_K_METERS, 0.33300, 0.28100, 1024, 864, 0, NULL, NULL},
                                {218, GKS_K_METERS, 0.33300, 0.28100, 1024, 864, 0, "gif", NULL},
                                {301, GKS_K_METERS, 0.33300, 0.28100, 1024, 864, 0, NULL, NULL},
                                {314, GKS_K_METERS, 0.25400, 0.19050, 1024, 768, 0, "tex", NULL},
                                {320, GKS_K_METERS, 0.28575, 0.19685, 6750, 4650, 0, NULL, NULL},
                                {321, GKS_K_METERS, 0.28575, 0.19685, 6750, 4650, 0, NULL, NULL},
                                {322, GKS_K_METERS, 0.28575, 0.19685, 6750, 4650, 0, NULL, NULL},
                                {323, GKS_K_METERS, 0.28575, 0.19685, 6750, 4650, 0, NULL, NULL},
                                {371, GKS_K_METERS, 0.25400, 0.19050, 1024, 768, 0, NULL, NULL},
                                {380, GKS_K_METERS, 0.25400, 0.19050, 1024, 768, 0, NULL, NULL},
                                {381, GKS_K_METERS, 0.25400, 0.19050, 1024, 768, 0, NULL, NULL},
                                {382, GKS_K_METERS, 0.25400, 0.19050, 1024, 768, 0, "svg", NULL},
                                {390, GKS_K_METERS, 0.25400, 0.19050, 1024, 768, 0, "wmf", NULL},
                                {400, GKS_K_METERS, 0.28560, 0.17850, 1280, 800, 0, NULL, NULL},
                                {410, GKS_K_METERS, 0.28560, 0.17850, 1280, 800, 0, NULL, NULL},
                                {411, GKS_K_METERS, 0.28560, 0.17850, 1280, 800, 0, NULL, NULL},
                                {415, GKS_K_METERS, 0.28560, 0.17850, 1280, 800, 0, NULL, NULL},
                                {420, GKS_K_METERS, 0.25400, 0.19050, 1024, 768, 0, NULL, NULL}};

static int num_ws_types = sizeof(ws_types) / sizeof(ws_types[0]);

static int predef_font[6] = {1, 1, 1, -2, -3, -4};

static int predef_prec[6] = {0, 1, 2, 2, 2, 2};

static int predef_ints[5] = {0, 1, 3, 3, 3};

static int predef_styli[5] = {1, 1, 1, 2, 3};

static int gksgral_marker_types[14] = {-20, -15, -18, -5, -17, -3, -7, -1, -16, -19, -14, -12, -2, -6};

static int gksgral_fill_styles[6] = {7, 7, 1, 8, 8, 2};

static int gddm_fill_styles[6] = {4, 10, 3, 9, 2, 1};

extern int gks_errno;

static double *x = NULL, *y = NULL;

static int max_points = 0;

static void gks_ddlk(int fctid, int dx, int dy, int dimx, int *i_arr, int len_f_arr_1, double *f_arr_1, int len_f_arr_2,
                     double *f_arr_2, int len_c_arr, char *c_arr, void **ptr)
{
  gks_list_t *list;
  ws_list_t *ws;
  int have_id;

  switch (fctid)
    {
    case OPEN_WS:
    case CLOSE_WS:
    case ACTIVATE_WS:
    case DEACTIVATE_WS:
    case CLEAR_WS:
    case REDRAW_SEG_ON_WS:
    case UPDATE_WS:
    case SET_DEFERRAL_STATE:
    case MESSAGE:
    case SET_COLOR_REP:
    case SET_WS_WINDOW:
    case SET_WS_VIEWPORT:
    case ASSOC_SEG_WITH_WS:
    case COPY_SEG_TO_WS:
    case INITIALIZE_LOCATOR:
    case REQUEST_LOCATOR:
    case REQUEST_STROKE:
    case REQUEST_CHOICE:
    case REQUEST_STRING:
      have_id = 1;
      break;

    default:
      have_id = 0;
    }

  api = 0;
  list = open_ws;

  while (list != NULL)
    {
      ws = (ws_list_t *)list->ptr;

      if (i_arr[0] == ws->wkid || !have_id)
        {
          if (id != 0)
            {
              if (id != ws->wkid)
                {
                  list = list->next;
                  continue;
                }
            }
          ptr = &ws->ptr;

#ifndef EMSCRIPTEN
          switch (ws->wtype)
            {
            case 2:
              gks_drv_mo(fctid, dx, dy, dimx, i_arr, len_f_arr_1, f_arr_1, len_f_arr_2, f_arr_2, len_c_arr, c_arr, ptr);
              break;

            case 3:
              gks_drv_mi(fctid, dx, dy, dimx, i_arr, len_f_arr_1, f_arr_1, len_f_arr_2, f_arr_2, len_c_arr, c_arr, ptr);
              break;

            case 5:
              gks_drv_wiss(fctid, dx, dy, dimx, i_arr, len_f_arr_1, f_arr_1, len_f_arr_2, f_arr_2, len_c_arr, c_arr,
                           ptr);
              break;

            case 41:
              gks_drv_win(fctid, dx, dy, dimx, i_arr, len_f_arr_1, f_arr_1, len_f_arr_2, f_arr_2, len_c_arr, c_arr,
                          ptr);
              break;

            case 61:
            case 62:
            case 63:
            case 64:
              gks_drv_ps(fctid, dx, dy, dimx, i_arr, len_f_arr_1, f_arr_1, len_f_arr_2, f_arr_2, len_c_arr, c_arr, ptr);
              break;

            case 100:
              break;

            case 101:
            case 102:
              gks_drv_pdf(fctid, dx, dy, dimx, i_arr, len_f_arr_1, f_arr_1, len_f_arr_2, f_arr_2, len_c_arr, c_arr,
                          ptr);
              break;

            case 210:
            case 211:
            case 212:
            case 213:
            case 214:
            case 215:
            case 216:
            case 217:
            case 218:
              gks_x11_plugin(fctid, dx, dy, dimx, i_arr, len_f_arr_1, f_arr_1, len_f_arr_2, f_arr_2, len_c_arr, c_arr,
                             ptr);
              break;

            case 410:
            case 411:
              gks_drv_socket(fctid, dx, dy, dimx, i_arr, len_f_arr_1, f_arr_1, len_f_arr_2, f_arr_2, len_c_arr, c_arr,
                             ptr);
              break;

            case 415:
              gks_zmq_plugin(fctid, dx, dy, dimx, i_arr, len_f_arr_1, f_arr_1, len_f_arr_2, f_arr_2, len_c_arr, c_arr,
                             ptr);
              break;

            case 301:
              gks_drv_plugin(fctid, dx, dy, dimx, i_arr, len_f_arr_1, f_arr_1, len_f_arr_2, f_arr_2, len_c_arr, c_arr,
                             ptr);
              break;

            case 320:
            case 321:
            case 322:
            case 323:
              gks_gs_plugin(fctid, dx, dy, dimx, i_arr, len_f_arr_1, f_arr_1, len_f_arr_2, f_arr_2, len_c_arr, c_arr,
                            ptr);
              break;

            case 371:
              gks_gtk_plugin(fctid, dx, dy, dimx, i_arr, len_f_arr_1, f_arr_1, len_f_arr_2, f_arr_2, len_c_arr, c_arr,
                             ptr);
              break;

            case 380:
              gks_wx_plugin(fctid, dx, dy, dimx, i_arr, len_f_arr_1, f_arr_1, len_f_arr_2, f_arr_2, len_c_arr, c_arr,
                            ptr);
              break;

            case 381:
              gks_qt_plugin(fctid, dx, dy, dimx, i_arr, len_f_arr_1, f_arr_1, len_f_arr_2, f_arr_2, len_c_arr, c_arr,
                            ptr);
              break;

            case 382:
              gks_svg_plugin(fctid, dx, dy, dimx, i_arr, len_f_arr_1, f_arr_1, len_f_arr_2, f_arr_2, len_c_arr, c_arr,
                             ptr);
              break;

            case 390:
              gks_wmf_plugin(fctid, dx, dy, dimx, i_arr, len_f_arr_1, f_arr_1, len_f_arr_2, f_arr_2, len_c_arr, c_arr,
                             ptr);
              break;

            case 400:
              gks_quartz_plugin(fctid, dx, dy, dimx, i_arr, len_f_arr_1, f_arr_1, len_f_arr_2, f_arr_2, len_c_arr,
                                c_arr, ptr);
              break;

            case 420:
              gks_gl_plugin(fctid, dx, dy, dimx, i_arr, len_f_arr_1, f_arr_1, len_f_arr_2, f_arr_2, len_c_arr, c_arr,
                            ptr);
              break;

            case 140:
            case 141:
            case 142:
            case 143:
            case 144:
            case 145:
            case 146:
            case 150:
              gks_cairo_plugin(fctid, dx, dy, dimx, i_arr, len_f_arr_1, f_arr_1, len_f_arr_2, f_arr_2, len_c_arr, c_arr,
                               ptr);
              break;

            case 120:
            case 121:
            case 130:
            case 160:
            case 161:
            case 162:
              gks_video_plugin(fctid, dx, dy, dimx, i_arr, len_f_arr_1, f_arr_1, len_f_arr_2, f_arr_2, len_c_arr, c_arr,
                               ptr);
              break;

            case 314:
              gks_pgf_plugin(fctid, dx, dy, dimx, i_arr, len_f_arr_1, f_arr_1, len_f_arr_2, f_arr_2, len_c_arr, c_arr,
                             ptr);
              break;

            default:
              printf("GKS: %s\n", gks_function_name(fctid));
              break;
            }
#else
          gks_drv_js(fctid, dx, dy, dimx, i_arr, len_f_arr_1, f_arr_1, len_f_arr_2, f_arr_2, len_c_arr, c_arr, ptr);
#endif
        }
      list = list->next;
    }
  api = 1;
}

static int gks_parse_encoding(const char *encoding)
{
  int i, j;
  const char *utf8_aliases[] = {"utf8", "utf-8"};
  const char *latin1_aliases[] = {"latin1", "latin-1", "iso-8859-1", "iso8859-1", "iso 8859-1"};

  if (!encoding)
    {
      return 0;
    }

  for (i = 0; i < sizeof(utf8_aliases) / sizeof(char *); i++)
    {
      for (j = 0; utf8_aliases[i][j] == tolower(encoding[j]) && encoding[j] != 0; j++)
        {
        }
      if (encoding[j] == 0 && utf8_aliases[i][j] == 0)
        {
          return ENCODING_UTF8;
        }
    }

  for (i = 0; i < sizeof(latin1_aliases) / sizeof(char *); i++)
    {
      for (j = 0; latin1_aliases[i][j] == tolower(encoding[j]) && encoding[j] != 0; j++)
        {
        }
      if (encoding[j] == 0 && latin1_aliases[i][j] == 0)
        {
          return ENCODING_LATIN1;
        }
    }

  return 0;
}


static void gks_parse_env(void)
{
  static int did_report_invalid_encoding = 0;
  const char *env;

  env = gks_getenv("GLI_GKS");
  if (env != NULL)
    {
      if (!strcmp(env, "GRALGKS"))
        s->version = GRALGKS;
      else if (!strcmp(env, "GLIGKS"))
        s->version = GLIGKS;
    }
  else
    s->version = GKS5;

  if (s->input_encoding == 0)
    {
      env = gks_getenv("GKS_ENCODING");
      if (env)
        {
          s->input_encoding = gks_parse_encoding(gks_getenv("GKS_ENCODING"));
          if (s->input_encoding == 0 && did_report_invalid_encoding == 0)
            {
              gks_perror("Invalid value '%s' for GKS_ENCODING, please use either 'utf8' or 'latin1'.", env);
              did_report_invalid_encoding = 1;
            }
        }
    }
  if (s->input_encoding == 0)
    {
      s->input_encoding = (char *)gks_getenv("GKS_IGNORE_ENCODING") != NULL ? ENCODING_UTF8 : ENCODING_LATIN1;
    }

  if (gks_getenv("GKS_NO_EXIT_HANDLER") == NULL) atexit(gks_emergency_close);
}

void gks_set_encoding(int encoding)
{
  if (state >= GKS_K_GKOP)
    {
      switch (encoding)
        {
        case 0:
        case ENCODING_LATIN1:
        case ENCODING_UTF8:
          s->input_encoding = encoding;
          break;
        default:
          gks_perror("Invalid value '%d' for encoding.", encoding);
          break;
        }
    }
  else
    {
      /* GKS not in proper state. GKS must be in the state GKOP */
      gks_report_error(SET_ENCODING, 8);
    }
}

void gks_inq_encoding(int *encoding)
{
  if (state >= GKS_K_GKOP)
    {
      *encoding = s->input_encoding;
    }
  else
    {
      /* GKS not in proper state. GKS must be in the state GKOP */
      gks_report_error(INQ_ENCODING, 8);
    }
}

void gks_init_gks(void)
{
  int i, tnr;

  if (s != NULL)
    {
      /* initialize aspect source flags */
      for (i = 0; i < 13; i++) s->asf[i] = GKS_K_ASF_BUNDLED;

      /* set default bundle table indices */
      s->lindex = 1;
      s->mindex = 1;
      s->tindex = 1;
      s->findex = 1;

      /* set default global output attributes */
      s->chh = 0.01;
      s->chup[0] = 0;
      s->chup[1] = 1;
      s->txp = GKS_K_TEXT_PATH_RIGHT;
      s->txal[0] = GKS_K_TEXT_HALIGN_NORMAL;
      s->txal[1] = GKS_K_TEXT_VALIGN_NORMAL;

      /* set default polyline attributes */
      s->ltype = GKS_K_LINETYPE_SOLID;
      s->lwidth = 1;
      s->plcoli = 1;

      /* set default polymarker attributes */
      s->mtype = GKS_K_MARKERTYPE_DOT;
      s->mszsc = 1;
      s->pmcoli = 1;

      /* set default text attributes */
      s->txfont = 1;
      s->txprec = GKS_K_TEXT_PRECISION_STRING;
      s->chxp = 1;
      s->chsp = 0;
      s->txcoli = 1;

      /* set default fill area attributes */
      s->ints = GKS_K_INTSTYLE_HOLLOW;
      s->styli = 1;
      s->facoli = 1;

      /* initialize normalization transformations */
      for (tnr = 0; tnr < MAX_TNR; tnr++)
        {
          s->window[tnr][0] = 0;
          s->window[tnr][1] = 1;
          s->window[tnr][2] = 0;
          s->window[tnr][3] = 1;
          s->viewport[tnr][0] = 0;
          s->viewport[tnr][1] = 1;
          s->viewport[tnr][2] = 0;
          s->viewport[tnr][3] = 1;
          gks_set_norm_xform(tnr, s->window[tnr], s->viewport[tnr]);
        }

      /* set segment transformation */
      s->mat[0][0] = 1;
      s->mat[0][1] = 0;
      s->mat[1][0] = 0;
      s->mat[1][1] = 1;
      s->mat[2][0] = 0;
      s->mat[2][1] = 0;

      s->opsg = 0;
      s->cntnr = 0;
      s->clip = GKS_K_CLIP;

      /* GKS extended attributes */
      s->input_encoding = 0;
      s->txslant = 0;
      s->shoff[0] = 0;
      s->shoff[1] = 0;
      s->blur = 0;
      s->alpha = 1;
      s->bwidth = 1;
      s->bcoli = 0;
      s->clip_tnr = 0;
      s->aspect_ratio = 1;

      s->callback = NULL;
    }
}

void gks_open_gks(int errfil)
{
  int i;
  ws_descr_t *ws;

  if (state == GKS_K_GKCL)
    {
      open_ws = NULL;
      active_ws = NULL;
      av_ws_types = NULL;
      for (i = 0; i < num_ws_types; i++)
        {
          ws = (ws_descr_t *)gks_malloc(sizeof(ws_descr_t));
          memmove(ws, ws_types + i, sizeof(ws_descr_t));
          av_ws_types = gks_list_add(av_ws_types, ws_types[i].wtype, ws);
        }

      s = (gks_state_list_t *)gks_malloc(sizeof(gks_state_list_t));

      /* parse GKS environment variables */
      gks_parse_env();

      /* open font database */
      s->fontfile = gks_open_font();

      /* miscellaneous flags */
      s->wiss = 0;

      gks_init_core(s);

      /* initialize GKS state */
      gks_init_gks();

      i_arr[0] = errfil;

      /* call the device driver link routine */
      gks_ddlk(OPEN_GKS, 1, 1, 1, i_arr, 0, f_arr_1, 0, f_arr_2, 0, c_arr, NULL);

      state = GKS_K_GKOP;

      /* ensure that floats are printed with dots */
      setlocale(LC_NUMERIC, "C");
    }
  else
    /* GKS not in proper state. GKS must be in the state GKCL */
    gks_report_error(OPEN_GKS, 1);
}

void gks_close_gks(void)
{
  if (state == GKS_K_GKOP)
    {
      /* call the device driver link routine */
      gks_ddlk(CLOSE_GKS, 0, 0, 0, i_arr, 0, f_arr_1, 0, f_arr_2, 0, c_arr, NULL);

      /* close font database */
      gks_close_font(s->fontfile);

      gks_list_free(av_ws_types);
      gks_free((void *)s);
      s = NULL;

      state = GKS_K_GKCL;
    }
  else
    /* GKS not in proper state. GKS must be in the state GKOP */
    gks_report_error(CLOSE_GKS, 2);
}

static int open_file(char *name, int wtype)
{
  int fd;

  if (name != NULL)
    {
      if (*name)
        {
          char *mode = wtype == GKS_K_WSTYPE_MI ? "r" : "w";
          fd = gks_open_file(name, mode);
        }
      else
        fd = 1;
    }
  else
    fd = 1;

  return fd < 0 ? 1 : fd;
}

void gks_open_ws(int wkid, char *path, int wtype)
{
  ws_list_t *ws;
  gks_list_t *element;
  ws_descr_t *descr;
  const char *env = NULL;

  if (state >= GKS_K_GKOP)
    {
      if (wkid > 0)
        {
          if (wtype == 0) wtype = gks_get_ws_type();

          if ((element = gks_list_find(av_ws_types, wtype)) != NULL)
            {
              descr = (ws_descr_t *)element->ptr;

              if (gks_list_find(open_ws, wkid) == NULL)
                {
                  if (wtype != 5 || s->wiss == 0)
                    {
                      ws = (ws_list_t *)gks_malloc(sizeof(ws_list_t));

                      ws->wkid = wkid;
                      if (path == NULL)
                        {
                          if (descr->type != NULL)
                            {
                              path = (char *)malloc(MAXPATHLEN * sizeof(char));
                              gks_filepath(path, NULL, descr->type, 1, 0);
                              ws->path = gks_strdup(path);
                              free(path);
                            }
                          else
                            ws->path = NULL;
                        }
                      else
                        ws->path = gks_strdup(path);
                      ws->wtype = wtype;
                      ws->conid = 0;

                      if (descr->env)
                        {
                          if ((env = gks_getenv(descr->env)))
                            {
                              if (ws->path != NULL) free(ws->path);
                              ws->path = gks_strdup(env);
                            }
                        }
                      if (ws->path)
                        {
                          if (*ws->path != '!')
                            {
                              if (wtype == 2 || wtype == 3 || wtype == 5 || /* mf and wiss */
                                  (wtype >= 61 && wtype <= 64) ||           /* ps */
                                  wtype == 101 || wtype == 102)             /* pdf */
                                {
                                  ws->conid = open_file(ws->path, wtype);
                                }
                            }
                          else
                            {
                              ws->conid = atoi(ws->path + 1);
                            }
                        }
                      else
                        {
                          ws->conid = 1;
                        }

                      /* add workstation identifier to the set of open
                         workstations */
                      open_ws = gks_list_add(open_ws, wkid, ws);

                      if (state == GKS_K_GKOP) state = GKS_K_WSOP;

                      /* parse GKS environment variables */
                      gks_parse_env();

                      i_arr[0] = wkid;
                      i_arr[1] = ws->conid;
                      i_arr[2] = wtype;

                      ws->ptr = (void *)s;

                      /* call the device driver link routine */
                      gks_ddlk(OPEN_WS, 3, 1, 3, i_arr, 0, f_arr_1, 0, f_arr_2, 1, ws->path, &ws->ptr);

                      if (i_arr[0] != 0 || i_arr[1] != 0)
                        {
                          ws_descr_t *p = (ws_descr_t *)element->ptr;

                          if (wtype == 5) s->wiss = 1;

#ifndef __EMSCRIPTEN__
                          if ((wtype >= 210 && wtype <= 213) || wtype == 218 || wtype == 41 || wtype == 381 ||
                              wtype == 400 || wtype == 411 || wtype == 420)
                            {
                              p->sizex = f_arr_1[0];
                              p->sizey = f_arr_2[0];
                              p->unitsx = i_arr[0];
                              p->unitsy = i_arr[1];
                            }
#else
                          p->sizex = f_arr_1[0];
                          p->sizey = f_arr_2[0];
                          p->unitsx = i_arr[0];
                          p->unitsy = i_arr[1];
#endif
                          ws->vp[0] = 0;
                          ws->vp[2] = 0;
                          if ((wtype >= 140 && wtype <= 146) || wtype == 150)
                            {
                              ws->vp[1] = 2400.0 / p->unitsx * p->sizex;
                              ws->vp[3] = 2400.0 / p->unitsy * p->sizey;
                            }
                          else
                            {
                              ws->vp[1] = 500.0 / p->unitsx * p->sizex;
                              ws->vp[3] = 500.0 / p->unitsy * p->sizey;
                            }
                        }
                      else
                        {
                          if (ws->conid && ws->conid != 1)
                            if (ws->path)
                              if (*ws->path != '!') gks_close_file(ws->conid);

                          if (ws->path) free(ws->path);

                          /* remove workstation identifier from the set of open
                             workstations */
                          open_ws = gks_list_del(open_ws, wkid);

                          if (open_ws == NULL) state = GKS_K_GKOP;

                          /* Open failed */
                          gks_report_error(OPEN_WS, 901);
                        }
                    }
                  else
                    /* WISS is already open */
                    gks_report_error(OPEN_WS, 28);
                }
              else
                /* specified workstation is open */
                gks_report_error(OPEN_WS, 24);
            }
          else
            /* specified workstation type is invalid */
            gks_report_error(OPEN_WS, 22);
        }
      else
        /* specified workstation identifier is invalid */
        gks_report_error(OPEN_WS, 20);
    }
  else
    /* GKS not in proper state. GKS must be in one of the
       states GKOP, WSOP, WSAC or SGOP */
    gks_report_error(OPEN_WS, 8);
}

void gks_close_ws(int wkid)
{
  gks_list_t *element;
  ws_list_t *ws;

  if (state >= GKS_K_WSOP)
    {
      if (wkid > 0)
        {
          if ((element = gks_list_find(open_ws, wkid)) != NULL)
            {
              ws = (ws_list_t *)element->ptr;

              if (gks_list_find(active_ws, wkid) == NULL)
                {
                  i_arr[0] = wkid;

                  /* call the device driver link routine */
                  gks_ddlk(CLOSE_WS, 1, 1, 1, i_arr, 0, f_arr_1, 0, f_arr_2, 0, c_arr, NULL);

                  if (ws->wtype == 5) s->wiss = 0;

                  if (ws->conid && ws->conid != 1)
                    if (ws->path)
                      if (*ws->path != '!') gks_close_file(ws->conid);

                  if (ws->path) free(ws->path);

                  /* remove workstation identifier from the set of open
                     workstations */
                  open_ws = gks_list_del(open_ws, wkid);

                  if (open_ws == NULL) state = GKS_K_GKOP;
                }
              else
                /* specified workstation is active */
                gks_report_error(CLOSE_WS, 29);
            }
          else
            /* specified workstation is not open */
            gks_report_error(CLOSE_WS, 25);
        }
      else
        /* specified workstation identifier is invalid */
        gks_report_error(CLOSE_WS, 20);
    }
  else
    /* GKS not in proper state. GKS must be in one of the
       states WSOP, WSAC or SGOP */
    gks_report_error(CLOSE_WS, 7);
}

void gks_activate_ws(int wkid)
{
  if (state == GKS_K_WSOP || state == GKS_K_WSAC)
    {
      if (wkid > 0)
        {
          if (gks_list_find(open_ws, wkid) != NULL)
            {
              if (gks_list_find(active_ws, wkid) == NULL)
                {
                  /* add workstation identifier to the set of active
                     workstations */
                  active_ws = gks_list_add(active_ws, wkid, NULL);

                  i_arr[0] = wkid;

                  /* call the device driver link routine */
                  gks_ddlk(ACTIVATE_WS, 1, 1, 1, i_arr, 0, f_arr_1, 0, f_arr_2, 0, c_arr, NULL);

                  if (state == GKS_K_WSOP) state = GKS_K_WSAC;
                }
              else
                /* specified workstation is active */
                gks_report_error(ACTIVATE_WS, 29);
            }
          else
            /* specified workstation is not open */
            gks_report_error(ACTIVATE_WS, 25);
        }
      else
        /* specified workstation identifier is invalid */
        gks_report_error(ACTIVATE_WS, 20);
    }
  else
    /* GKS not in proper state. GKS must be either in the state
       WSOP or in the state WSAC */
    gks_report_error(ACTIVATE_WS, 6);
}

void gks_deactivate_ws(int wkid)
{
  if (state == GKS_K_WSAC)
    {
      if (wkid > 0)
        {
          if (gks_list_find(active_ws, wkid) != NULL)
            {
              i_arr[0] = wkid;

              /* call the device driver link routine */
              gks_ddlk(DEACTIVATE_WS, 1, 1, 1, i_arr, 0, f_arr_1, 0, f_arr_2, 0, c_arr, NULL);

              /* remove workstation identifier from the set of active
                 workstations */
              active_ws = gks_list_del(active_ws, wkid);

              if (active_ws == NULL) state = GKS_K_WSOP;
            }
          else
            /* specified workstation is not active */
            gks_report_error(DEACTIVATE_WS, 30);
        }
      else
        /* specified workstation identifier is invalid */
        gks_report_error(DEACTIVATE_WS, 20);
    }
  else
    /* GKS not in proper state. GKS must be in the state WSAC */
    gks_report_error(DEACTIVATE_WS, 3);
}

void gks_configure_ws(int wkid)
{
  gks_list_t *element;
  ws_list_t *ws;

  if (state == GKS_K_WSOP || state == GKS_K_WSAC)
    {
      if (wkid > 0)
        {
          if ((element = gks_list_find(open_ws, wkid)) != NULL)
            {
              i_arr[0] = wkid;

              /* call the device driver link routine */
              gks_ddlk(CONFIGURE_WS, 1, 1, 1, i_arr, 0, f_arr_1, 0, f_arr_2, 0, c_arr, NULL);

              ws = (ws_list_t *)element->ptr;
              if ((element = gks_list_find(av_ws_types, ws->wtype)) != NULL)
                {
                  ws_descr_t *p = (ws_descr_t *)element->ptr;

                  p->sizex = f_arr_1[0];
                  p->sizey = f_arr_2[0];
                  p->unitsx = i_arr[0];
                  p->unitsy = i_arr[1];
                }
              else
                /* specified workstation type is invalid */
                gks_report_error(CONFIGURE_WS, 22);
            }
          else
            /* specified workstation is not open */
            gks_report_error(CONFIGURE_WS, 25);
        }
      else
        /* specified workstation identifier is invalid */
        gks_report_error(CONFIGURE_WS, 20);
    }
  else
    /* GKS not in proper state. GKS must be either in the state
       WSOP or in the state WSAC */
    gks_report_error(CONFIGURE_WS, 6);
}

void gks_clear_ws(int wkid, int cofl)
{
  if (state == GKS_K_WSOP || state == GKS_K_WSAC)
    {
      if (wkid > 0)
        {
          if (gks_list_find(open_ws, wkid) != NULL)
            {
              i_arr[0] = wkid;
              i_arr[1] = cofl;

              /* call the device driver link routine */
              gks_ddlk(CLEAR_WS, 2, 1, 2, i_arr, 0, f_arr_1, 0, f_arr_2, 0, c_arr, NULL);
            }
          else
            /* specified workstation is not open */
            gks_report_error(CLEAR_WS, 25);
        }
      else
        /* specified workstation identifier is invalid */
        gks_report_error(CLEAR_WS, 20);
    }
  else
    /* GKS not in proper state. GKS must be either in the state
       WSOP or in the state WSAC */
    gks_report_error(CLEAR_WS, 6);
}

void gks_redraw_seg_on_ws(int wkid)
{
  gks_state_list_t sl;

  if (state >= GKS_K_WSOP)
    {
      if (wkid > 0)
        {
          if (s->wiss)
            {
              if (gks_list_find(active_ws, wkid) != NULL)
                {
                  /* save GKS state, restore segment state */
                  memmove(&sl, s, sizeof(gks_state_list_t));
                  memmove(s, seg_state, sizeof(gks_state_list_t));

                  id = wkid;

                  /* call the WISS dispatch routine */
                  gks_wiss_dispatch(REDRAW_SEG_ON_WS, wkid, 0);

                  id = 0;

                  /* restore GKS state */
                  memmove(s, &sl, sizeof(gks_state_list_t));
                }
              else
                /* specified workstation is not active */
                gks_report_error(REDRAW_SEG_ON_WS, 30);
            }
          else
            /* WISS is not open */
            gks_report_error(REDRAW_SEG_ON_WS, 27);
        }
      else
        /* specified workstation identifier is invalid */
        gks_report_error(REDRAW_SEG_ON_WS, 20);
    }
  else
    /* GKS not in proper state. GKS must be in one of the
       states WSOP, WSAC or SGOP */
    gks_report_error(REDRAW_SEG_ON_WS, 7);
}

void gks_update_ws(int wkid, int regfl)
{
  if (state >= GKS_K_WSOP)
    {
      if (wkid > 0)
        {
          if (gks_list_find(open_ws, wkid) != NULL)
            {
              i_arr[0] = wkid;
              i_arr[1] = regfl;

              /* call the device driver link routine */
              gks_ddlk(UPDATE_WS, 2, 1, 2, i_arr, 0, f_arr_1, 0, f_arr_2, 0, c_arr, NULL);
            }
          else
            /* specified workstation is not open */
            gks_report_error(UPDATE_WS, 25);
        }
      else
        /* specified workstation identifier is invalid */
        gks_report_error(UPDATE_WS, 20);
    }
  else
    /* GKS not in proper state. GKS must be in one of the
       states WSOP, WSAC or SGOP */
    gks_report_error(UPDATE_WS, 7);
}

void gks_set_deferral_state(int wkid, int defmo, int regmo)
{
  if (state >= GKS_K_WSOP)
    {
      if (wkid > 0)
        {
          if (gks_list_find(open_ws, wkid) != NULL)
            {
              i_arr[0] = wkid;
              i_arr[1] = defmo;
              i_arr[2] = regmo;

              /* call the device driver link routine */
              gks_ddlk(SET_DEFERRAL_STATE, 3, 1, 3, i_arr, 0, f_arr_1, 0, f_arr_2, 0, c_arr, NULL);
            }
          else
            /* specified workstation is not open */
            gks_report_error(SET_DEFERRAL_STATE, 25);
        }
      else
        /* specified workstation identifier is invalid */
        gks_report_error(SET_DEFERRAL_STATE, 20);
    }
  else
    /* GKS not in proper state. GKS must be in one of the
       states WSOP, WSAC or SGOP */
    gks_report_error(SET_DEFERRAL_STATE, 7);
}

void gks_escape(int funid, int dimidr, int *idr, int maxodr, int *lenodr, int *odr)
{
  gks_perror("escape function %d not implemented", funid);
}

void gks_message(int wkid, char *message)
{
  if (state >= GKS_K_WSOP)
    {
      if (wkid > 0)
        {
          if (gks_list_find(open_ws, wkid) != NULL)
            {
              i_arr[0] = wkid;

              /* call the device driver link routine */
              gks_ddlk(MESSAGE, 1, 1, 1, i_arr, 0, f_arr_1, 0, f_arr_2, 1, message, NULL);
            }
          else
            /* specified workstation is not open */
            gks_report_error(MESSAGE, 25);
        }
      else
        /* specified workstation identifier is invalid */
        gks_report_error(MESSAGE, 20);
    }
  else
    /* GKS not in proper state. GKS must be in one of the
       states WSOP, WSAC or SGOP */
    gks_report_error(MESSAGE, 7);
}

void gks_polyline(int n, double *pxa, double *pya)
{
  if (state >= GKS_K_WSAC)
    {
      if (n >= 2)
        {
          i_arr[0] = n;

          /* call the device driver link routine */
          gks_ddlk(POLYLINE, 1, 1, 1, i_arr, n, pxa, n, pya, 0, c_arr, NULL);
        }
      else
        /* number of points is invalid */
        gks_report_error(POLYLINE, 100);
    }
  else
    /* GKS not in proper state. GKS must be either in the state
       WSAC or in the state SGOP */
    gks_report_error(POLYLINE, 5);
}

void gks_polymarker(int n, double *pxa, double *pya)
{
  if (state >= GKS_K_WSAC)
    {
      if (n >= 1)
        {
          i_arr[0] = n;

          /* call the device driver link routine */
          gks_ddlk(POLYMARKER, 1, 1, 1, i_arr, n, pxa, n, pya, 0, c_arr, NULL);
        }
      else
        /* number of points is invalid */
        gks_report_error(POLYMARKER, 100);
    }
  else
    /* GKS not in proper state. GKS must be either in the state
       WSAC or in the state SGOP */
    gks_report_error(POLYMARKER, 5);
}

void gks_ft_gdp(int n, double *px, double *py, int primid, int ldr, int *datrec)
{
  int saved_ints = s->ints, saved_facoli = s->facoli;
  double saved_bwidth = s->bwidth;

  gks_set_fill_int_style(GKS_K_INTSTYLE_SOLID);
  gks_set_fill_color_index(s->txcoli);
  gks_set_border_width(0);

  gks_gdp(n, px, py, primid, ldr, datrec);

  gks_set_border_width(saved_bwidth);
  gks_set_fill_color_index(saved_facoli);
  gks_set_fill_int_style(saved_ints);
}

void gks_text(double px, double py, char *str)
{
  if (state >= GKS_K_WSAC)
    {
      if (strlen(str) == 0)
        {
          /* ignore empty strings */
          return;
        }
      else if (strlen(str) < 132)
        {
          if (s->txprec != GKS_K_TEXT_PRECISION_OUTLINE)
            {
              /* double the string length as the longest utf8 representation of any latin1 character is two bytes long
               */
              char utf8_str[2 * 131 + 1];
              gks_input2utf8(str, utf8_str, s->input_encoding);

              f_arr_1[0] = px;
              f_arr_2[0] = py;

              /* call the device driver link routine */
              gks_ddlk(TEXT, 0, 0, 0, i_arr, 1, f_arr_1, 1, f_arr_2, 1, utf8_str, NULL);
            }
          else
            {
              if (s->input_encoding == ENCODING_LATIN1)
                {
                  char *utf8_str = gks_malloc(strlen(str) * 2 + 1);
                  gks_input2utf8(str, utf8_str, ENCODING_LATIN1);

                  gks_ft_text(px, py, utf8_str, s, gks_ft_gdp);
                }
              else
                gks_ft_text(px, py, str, s, gks_ft_gdp);
            }
        }
      else
        /* string is too long */
        gks_report_error(TEXT, 403);
    }
  else
    /* GKS not in proper state. GKS must be either in the state
       WSAC or in the state SGOP */
    gks_report_error(TEXT, 5);
}

void gks_fillarea(int n, double *pxa, double *pya)
{
  if (state >= GKS_K_WSAC)
    {
      if (n >= 3)
        {
          i_arr[0] = n;

          /* call the device driver link routine */
          gks_ddlk(FILLAREA, 1, 1, 1, i_arr, n, pxa, n, pya, 0, c_arr, NULL);
        }
      else
        /* number of points is invalid */
        gks_report_error(FILLAREA, 100);
    }
  else
    /* GKS not in proper state. GKS must be either in the state
       WSAC or in the state SGOP */
    gks_report_error(FILLAREA, 5);
}

static int check_range(double a, double b)
{
  double d;

  if (a != 0)
    d = a;
  else if (b != 0)
    d = b;
  else
    d = 1;

  return fabs((b - a) / d) * 0.000001 > DBL_EPSILON;
}

void gks_cellarray(double qx, double qy, double rx, double ry, int dimx, int dimy, int scol, int srow, int ncol,
                   int nrow, int *colia)
{
  if (state >= GKS_K_WSAC)
    {
      if (scol >= 1 && srow >= 1 && scol + ncol - 1 <= dimx && srow + nrow - 1 <= dimy)
        {
          /* Check whether the given coordinate range does not lead
             to loss of precision */
          if (check_range(qx, rx) && check_range(qy, ry))
            {
              gks_adjust_cellarray(&qx, &qy, &rx, &ry, &scol, &srow, &ncol, &nrow, dimx, dimy);

              if (ncol >= 1 && nrow >= 1)
                {
                  f_arr_1[0] = qx;
                  f_arr_2[0] = qy;
                  f_arr_1[1] = rx;
                  f_arr_2[1] = ry;

                  /* call the device driver link routine */
                  gks_ddlk(CELLARRAY, ncol, nrow, dimx, colia + (srow - 1) * dimx + scol - 1, 2, f_arr_1, 2, f_arr_2, 0,
                           c_arr, NULL);
                }
              else
                /* subimage limitation reached */
                gks_report_error(CELLARRAY, 404);
            }
          else
            /* rectangle definition is invalid */
            gks_report_error(CELLARRAY, 51);
        }
      else
        /* dimensions of color index array are invalid */
        gks_report_error(CELLARRAY, 91);
    }
  else
    /* GKS not in proper state. GKS must be either in the state
       WSAC or in the state SGOP */
    gks_report_error(CELLARRAY, 5);
}

void gks_gdp(int n, double *px, double *py, int primid, int ldr, int *datrec)
{
  int *dr, len;

  if (state >= GKS_K_WSAC)
    {
      if (n >= 1)
        {
          len = ldr + 3;
          dr = (int *)gks_malloc(len * sizeof(int));
          dr[0] = n;
          dr[1] = primid;
          dr[2] = ldr;
          memmove(dr + 3, datrec, ldr * sizeof(int));

          /* call the device driver link routine */
          gks_ddlk(GDP, len, 1, len, dr, n, px, n, py, 0, c_arr, NULL);

          free(dr);
        }
      else
        /* number of points is invalid */
        gks_report_error(GDP, 100);
    }
  else
    /* GKS not in proper state. GKS must be either in the state
       WSAC or in the state SGOP */
    gks_report_error(GDP, 5);
}

void gks_set_pline_index(int index)
{
  if (state >= GKS_K_GKOP)
    {
      if (index >= 1 && index <= 5)
        {
          s->lindex = i_arr[0] = index;

          /* call the device driver link routine */
          gks_ddlk(SET_PLINE_INDEX, 1, 1, 1, i_arr, 0, f_arr_1, 0, f_arr_2, 0, c_arr, NULL);
        }
      else
        /* polyline index is invalid */
        gks_report_error(SET_PLINE_INDEX, 60);
    }
  else
    /* GKS not in proper state. GKS must be in one of the states
       GKOP, WSOP, WSAC or SGOP */
    gks_report_error(SET_PLINE_INDEX, 8);
}

void gks_set_pline_linetype(int ltype)
{
  if (state >= GKS_K_GKOP)
    {
      if (ltype >= -30 && ltype <= GKS_K_LINETYPE_DASHED_DOTTED && ltype)
        {
          s->ltype = i_arr[0] = ltype;

          /* call the device driver link routine */
          gks_ddlk(SET_PLINE_LINETYPE, 1, 1, 1, i_arr, 0, f_arr_1, 0, f_arr_2, 0, c_arr, NULL);
        }
      else
        /* linetype is invalid */
        gks_report_error(SET_PLINE_LINETYPE, 62);
    }
  else
    /* GKS not in proper state. GKS must be in one of the states
       GKOP, WSOP, WSAC or SGOP */
    gks_report_error(SET_PLINE_LINETYPE, 8);
}

void gks_set_pline_linewidth(double lwidth)
{
  if (state >= GKS_K_GKOP)
    {
      if (lwidth != s->lwidth)
        {
          s->lwidth = f_arr_1[0] = lwidth;

          /* call the device driver link routine */
          gks_ddlk(SET_PLINE_LINEWIDTH, 0, 0, 0, i_arr, 1, f_arr_1, 0, f_arr_2, 0, c_arr, NULL);
        }
    }
  else
    /* GKS not in proper state. GKS must be in one of the states
       GKOP, WSOP, WSAC or SGOP */
    gks_report_error(SET_PLINE_LINEWIDTH, 8);
}

void gks_set_pline_color_index(int coli)
{
  if (state >= GKS_K_GKOP)
    {
      if (coli >= 0)
        {
          if (coli != s->plcoli)
            {
              s->plcoli = i_arr[0] = coli;

              /* call the device driver link routine */
              gks_ddlk(SET_PLINE_COLOR_INDEX, 1, 1, 1, i_arr, 0, f_arr_1, 0, f_arr_2, 0, c_arr, NULL);
            }
        }
      else
        /* color index is invalid */
        gks_report_error(SET_PLINE_COLOR_INDEX, 65);
    }
  else
    /* GKS not in proper state. GKS must be in one of the states
       GKOP, WSOP, WSAC or SGOP */
    gks_report_error(SET_PLINE_COLOR_INDEX, 8);
}

void gks_set_pmark_index(int index)
{
  if (state >= GKS_K_GKOP)
    {
      if (index >= 1 && index <= 5)
        {
          s->mindex = i_arr[0] = index;

          /* call the device driver link routine */
          gks_ddlk(SET_PMARK_INDEX, 1, 1, 1, i_arr, 0, f_arr_1, 0, f_arr_2, 0, c_arr, NULL);
        }
      else
        /* polymarker index is invalid */
        gks_report_error(SET_PMARK_INDEX, 64);
    }
  else
    /* GKS not in proper state. GKS must be in one of the states
       GKOP, WSOP, WSAC or SGOP */
    gks_report_error(SET_PMARK_INDEX, 8);
}

void gks_set_pmark_type(int mtype)
{
  if (state >= GKS_K_GKOP)
    {
      if (mtype >= -114 && mtype <= -101) mtype = gksgral_marker_types[mtype + 114];

      if (mtype >= -32 && mtype <= GKS_K_MARKERTYPE_DIAGONAL_CROSS && mtype)
        {
          if (mtype != s->mtype)
            {
              s->mtype = i_arr[0] = mtype;

              /* call the device driver link routine */
              gks_ddlk(SET_PMARK_TYPE, 1, 1, 1, i_arr, 0, f_arr_1, 0, f_arr_2, 0, c_arr, NULL);
            }
        }
      else
        /* marker type is invalid */
        gks_report_error(SET_PMARK_TYPE, 66);
    }
  else
    /* GKS not in proper state. GKS must be in one of the states
       GKOP, WSOP, WSAC or SGOP */
    gks_report_error(SET_PMARK_TYPE, 8);
}

void gks_set_pmark_size(double mszsc)
{
  if (state >= GKS_K_GKOP)
    {
      if (mszsc != s->mszsc)
        {
          s->mszsc = f_arr_1[0] = mszsc;

          /* call the device driver link routine */
          gks_ddlk(SET_PMARK_SIZE, 0, 0, 0, i_arr, 1, f_arr_1, 0, f_arr_2, 0, c_arr, NULL);
        }
    }
  else
    /* GKS not in proper state. GKS must be in one of the states
       GKOP, WSOP, WSAC or SGOP */
    gks_report_error(SET_PMARK_SIZE, 8);
}

void gks_set_pmark_color_index(int coli)
{
  if (state >= GKS_K_GKOP)
    {
      if (coli >= 0)
        {
          if (coli != s->pmcoli)
            {
              s->pmcoli = i_arr[0] = coli;

              /* call the device driver link routine */
              gks_ddlk(SET_PMARK_COLOR_INDEX, 1, 1, 1, i_arr, 0, f_arr_1, 0, f_arr_2, 0, c_arr, NULL);
            }
        }
      else
        /* color index is invalid */
        gks_report_error(SET_PMARK_COLOR_INDEX, 65);
    }
  else
    /* GKS not in proper state. GKS must be in one of the states
       GKOP, WSOP, WSAC or SGOP */
    gks_report_error(SET_PMARK_COLOR_INDEX, 8);
}

void gks_set_text_index(int index)
{
  if (state >= GKS_K_GKOP)
    {
      if (index >= 1 && index <= 6)
        {
          s->tindex = i_arr[0] = index;

          /* call the device driver link routine */
          gks_ddlk(SET_TEXT_INDEX, 1, 1, 1, i_arr, 0, f_arr_1, 0, f_arr_2, 0, c_arr, NULL);
        }
      else
        /* text index is invalid */
        gks_report_error(SET_TEXT_INDEX, 68);
    }
  else
    /* GKS not in proper state. GKS must be in one of the states
       GKOP, WSOP, WSAC or SGOP */
    gks_report_error(SET_TEXT_INDEX, 8);
}

void gks_set_text_fontprec(int font, int prec)
{
  if (state >= GKS_K_GKOP)
    {
      if (font != 0)
        {
          if (font != s->txfont || prec != s->txprec)
            {
              s->txfont = i_arr[0] = font;
              s->txprec = i_arr[1] = prec;

              /* call the device driver link routine */
              gks_ddlk(SET_TEXT_FONTPREC, 2, 1, 2, i_arr, 0, f_arr_1, 0, f_arr_2, 0, c_arr, NULL);
            }
        }
      else
        /* text font is invalid */
        gks_report_error(SET_TEXT_FONTPREC, 70);
    }
  else
    /* GKS not in proper state. GKS must be in one of the states
       GKOP, WSOP, WSAC or SGOP */
    gks_report_error(SET_TEXT_FONTPREC, 8);
}

void gks_set_text_expfac(double chxp)
{
  if (state >= GKS_K_GKOP)
    {
      if (chxp != 0)
        {
          if (chxp != s->chxp)
            {
              s->chxp = f_arr_1[0] = chxp;

              /* call the device driver link routine */
              gks_ddlk(SET_TEXT_EXPFAC, 0, 0, 0, i_arr, 1, f_arr_1, 0, f_arr_2, 0, c_arr, NULL);
            }
        }
      else
        /* character expansion factor is invalid */
        gks_report_error(SET_TEXT_EXPFAC, 72);
    }
  else
    /* GKS not in proper state. GKS must be in one of the states
       GKOP, WSOP, WSAC or SGOP */
    gks_report_error(SET_TEXT_EXPFAC, 8);
}

void gks_set_text_spacing(double chsp)
{
  if (state >= GKS_K_GKOP)
    {
      if (chsp != s->chsp)
        {
          s->chsp = f_arr_1[0] = chsp;

          /* call the device driver link routine */
          gks_ddlk(SET_TEXT_SPACING, 0, 0, 0, i_arr, 1, f_arr_1, 0, f_arr_2, 0, c_arr, NULL);
        }
    }
  else
    /* GKS not in proper state. GKS must be in one of the states
       GKOP, WSOP, WSAC or SGOP */
    gks_report_error(SET_TEXT_SPACING, 8);
}

void gks_set_text_color_index(int coli)
{
  if (state >= GKS_K_GKOP)
    {
      if (coli >= 0)
        {
          if (coli != s->txcoli)
            {
              s->txcoli = i_arr[0] = coli;

              /* call the device driver link routine */
              gks_ddlk(SET_TEXT_COLOR_INDEX, 1, 1, 1, i_arr, 0, f_arr_1, 0, f_arr_2, 0, c_arr, NULL);
            }
        }
      else
        /* color index is invalid */
        gks_report_error(SET_TEXT_COLOR_INDEX, 65);
    }
  else
    /* GKS not in proper state. GKS must be in one of the states
       GKOP, WSOP, WSAC or SGOP */
    gks_report_error(SET_TEXT_COLOR_INDEX, 8);
}

void gks_set_text_height(double chh)
{
  if (state >= GKS_K_GKOP)
    {
      if (chh > 0)
        {
          if (chh != s->chh)
            {
              s->chh = f_arr_1[0] = chh;

              /* call the device driver link routine */
              gks_ddlk(SET_TEXT_HEIGHT, 0, 0, 0, i_arr, 1, f_arr_1, 0, f_arr_2, 0, c_arr, NULL);
            }
        }
      else
        /* character height is invalid */
        gks_report_error(SET_TEXT_HEIGHT, 73);
    }
  else
    /* GKS not in proper state. GKS must be in one of the states
       GKOP, WSOP, WSAC or SGOP */
    gks_report_error(SET_TEXT_HEIGHT, 8);
}

void gks_set_text_upvec(double chux, double chuy)
{
  if (state >= GKS_K_GKOP)
    {
      if (fabs(chux) > FEPS || fabs(chuy) > FEPS)
        {
          if (chux != s->chup[0] || chuy != s->chup[1])
            {
              s->chup[0] = f_arr_1[0] = chux;
              s->chup[1] = f_arr_2[0] = chuy;

              /* call the device driver link routine */
              gks_ddlk(SET_TEXT_UPVEC, 0, 0, 0, i_arr, 1, f_arr_1, 1, f_arr_2, 0, c_arr, NULL);
            }
        }
      else
        /* character up vector is invalid */
        gks_report_error(SET_TEXT_UPVEC, 74);
    }
  else
    /* GKS not in proper state. GKS must be in one of the states
       GKOP, WSOP, WSAC or SGOP */
    gks_report_error(SET_TEXT_UPVEC, 8);
}

void gks_set_text_path(int txp)
{
  if (state >= GKS_K_GKOP)
    {
      if (txp != s->txp)
        {
          s->txp = i_arr[0] = txp;

          /* call the device driver link routine */
          gks_ddlk(SET_TEXT_PATH, 1, 1, 1, i_arr, 0, f_arr_1, 0, f_arr_2, 0, c_arr, NULL);
        }
    }
  else
    /* GKS not in proper state. GKS must be in one of the states
       GKOP, WSOP, WSAC or SGOP */
    gks_report_error(SET_TEXT_PATH, 8);
}

void gks_set_text_align(int txalh, int txalv)
{
  if (state >= GKS_K_GKOP)
    {
      if (txalh != s->txal[0] || txalv != s->txal[1])
        {
          s->txal[0] = i_arr[0] = txalh;
          s->txal[1] = i_arr[1] = txalv;

          /* call the device driver link routine */
          gks_ddlk(SET_TEXT_ALIGN, 2, 1, 2, i_arr, 0, f_arr_1, 0, f_arr_2, 0, c_arr, NULL);
        }
    }
  else
    /* GKS not in proper state. GKS must be in one of the states
       GKOP, WSOP, WSAC or SGOP */
    gks_report_error(SET_TEXT_ALIGN, 8);
}

void gks_set_fill_index(int index)
{
  if (state >= GKS_K_GKOP)
    {
      if (index >= 1 && index <= 5)
        {
          s->findex = i_arr[0] = index;

          /* call the device driver link routine */
          gks_ddlk(SET_FILL_INDEX, 1, 1, 1, i_arr, 0, f_arr_1, 0, f_arr_2, 0, c_arr, NULL);
        }
      else
        /* fill area index is invalid */
        gks_report_error(SET_FILL_INDEX, 75);
    }
  else
    /* GKS not in proper state. GKS must be in one of the states
       GKOP, WSOP, WSAC or SGOP */
    gks_report_error(SET_FILL_INDEX, 8);
}

void gks_set_fill_int_style(int ints)
{
  if (state >= GKS_K_GKOP)
    {
      if (ints != s->ints)
        {
          s->ints = i_arr[0] = ints;

          /* call the device driver link routine */
          gks_ddlk(SET_FILL_INT_STYLE, 1, 1, 1, i_arr, 0, f_arr_1, 0, f_arr_2, 0, c_arr, NULL);
        }
    }
  else
    /* GKS not in proper state. GKS must be in one of the states
       GKOP, WSOP, WSAC or SGOP */
    gks_report_error(SET_FILL_INT_STYLE, 8);
}

void gks_set_fill_style_index(int styli)
{
  if (state >= GKS_K_GKOP)
    {
      if (styli >= -106 && styli <= -101)
        styli = gksgral_fill_styles[styli + 106];
      else if (styli >= -6 && styli <= -1)
        styli = gddm_fill_styles[styli + 6];

      if (styli >= 0)
        {
          s->styli = i_arr[0] = styli;

          /* call the device driver link routine */
          gks_ddlk(SET_FILL_STYLE_INDEX, 1, 1, 1, i_arr, 0, f_arr_1, 0, f_arr_2, 0, c_arr, NULL);
        }
      else
        /* style index is invalid */
        gks_report_error(SET_FILL_STYLE_INDEX, 78);
    }
  else
    /* GKS not in proper state. GKS must be in one of the states
       GKOP, WSOP, WSAC or SGOP */
    gks_report_error(SET_FILL_STYLE_INDEX, 8);
}

void gks_set_fill_color_index(int coli)
{
  if (state >= GKS_K_GKOP)
    {
      if (coli >= 0)
        {
          if (coli != s->facoli)
            {
              s->facoli = i_arr[0] = coli;

              /* call the device driver link routine */
              gks_ddlk(SET_FILL_COLOR_INDEX, 1, 1, 1, i_arr, 0, f_arr_1, 0, f_arr_2, 0, c_arr, NULL);
            }
        }
      else
        /* color index is invalid */
        gks_report_error(SET_FILL_COLOR_INDEX, 65);
    }
  else
    /* GKS not in proper state. GKS must be in one of the states
       GKOP, WSOP, WSAC or SGOP */
    gks_report_error(SET_FILL_COLOR_INDEX, 8);
}

void gks_set_asf(int *flag)
{
  int i;

  if (state >= GKS_K_GKOP)
    {
      for (i = 0; i < 13; i++) s->asf[i] = i_arr[i] = flag[i];

      /* call the device driver link routine */
      gks_ddlk(SET_ASF, 13, 1, 13, i_arr, 0, f_arr_1, 0, f_arr_2, 0, c_arr, NULL);
    }
  else
    /* GKS not in proper state. GKS must be in one of the states
       GKOP, WSOP, WSAC or SGOP */
    gks_report_error(SET_ASF, 8);
}

void gks_set_color_rep(int wkid, int index, double red, double green, double blue)
{
  if (state >= GKS_K_GKOP)
    {
      if (wkid > 0)
        {
          if (gks_list_find(open_ws, wkid) != NULL)
            {
              if (index >= 0)
                {
                  if (red >= 0 && red <= 1 && green >= 0 && green <= 1 && blue >= 0 && blue <= 1)
                    {
                      gks_set_rgb(index, red, green, blue);

                      i_arr[0] = wkid;
                      i_arr[1] = index;
                      f_arr_1[0] = red;
                      f_arr_1[1] = green;
                      f_arr_1[2] = blue;

                      /* call the device driver link routine */
                      gks_ddlk(SET_COLOR_REP, 2, 1, 2, i_arr, 3, f_arr_1, 0, f_arr_2, 0, c_arr, NULL);
                    }
                  else
                    /* color is invalid */
                    gks_report_error(SET_COLOR_REP, 88);
                }
              else
                /* color index is invalid */
                gks_report_error(SET_COLOR_REP, 85);
            }
          else
            /* specified workstation is not open */
            gks_report_error(SET_COLOR_REP, 25);
        }
      else
        /* specified workstation identifier is invalid */
        gks_report_error(SET_COLOR_REP, 20);
    }
  else
    /* GKS not in proper state. GKS must be in one of the states
       GKOP, WSOP, WSAC or SGOP */
    gks_report_error(SET_COLOR_REP, 8);
}

void gks_set_window(int tnr, double xmin, double xmax, double ymin, double ymax)
{
  static int warn_about = 1;

  if (state >= GKS_K_GKOP)
    {
      if (tnr > 0 && tnr < MAX_TNR)
        {
          /* Check whether the given coordinate range does not lead
             to loss of precision in subsequent GKS functions. It must
             be ensured that there are at least 4 significant digits
             when applying normalization or device transformations
           */
          if (!check_range(xmin, xmax) || !check_range(ymin, ymax))
            {
              if (warn_about)
                {
                  fprintf(stderr, "GKS: Possible loss of precision in routine SET_WINDOW\n");
                  warn_about = 0;
                }
            }
          if (xmin < xmax && ymin < ymax)
            {
              i_arr[0] = tnr;
              s->window[tnr][0] = f_arr_1[0] = xmin;
              s->window[tnr][1] = f_arr_1[1] = xmax;
              s->window[tnr][2] = f_arr_2[0] = ymin;
              s->window[tnr][3] = f_arr_2[1] = ymax;
              gks_set_norm_xform(tnr, s->window[tnr], s->viewport[tnr]);

              /* call the device driver link routine */
              gks_ddlk(SET_WINDOW, 1, 1, 1, i_arr, 2, f_arr_1, 2, f_arr_2, 0, c_arr, NULL);
            }
          else
            /* rectangle definition is invalid */
            gks_report_error(SET_WINDOW, 51);
        }
      else
        /* transformation number is invalid */
        gks_report_error(SET_WINDOW, 50);
    }
  else
    /* GKS not in proper state. GKS must be in one of the states
       GKOP, WSOP, WSAC or SGOP */
    gks_report_error(SET_WINDOW, 8);
}

void gks_set_viewport(int tnr, double xmin, double xmax, double ymin, double ymax)
{
  if (state >= GKS_K_GKOP)
    {
      if (tnr > 0 && tnr < MAX_TNR)
        {
          if (xmin < xmax && ymin < ymax)
            {
              if (xmin >= 0 && xmax <= 1 && ymin >= 0 && ymax <= 1)
                {
                  i_arr[0] = tnr;
                  s->viewport[tnr][0] = f_arr_1[0] = xmin;
                  s->viewport[tnr][1] = f_arr_1[1] = xmax;
                  s->viewport[tnr][2] = f_arr_2[0] = ymin;
                  s->viewport[tnr][3] = f_arr_2[1] = ymax;
                  gks_set_norm_xform(tnr, s->window[tnr], s->viewport[tnr]);

                  /* call the device driver link routine */
                  gks_ddlk(SET_VIEWPORT, 1, 1, 1, i_arr, 2, f_arr_1, 2, f_arr_2, 0, c_arr, NULL);
                }
              else
                /* viewport is not within the NDC unit square */
                gks_report_error(SET_VIEWPORT, 52);
            }
          else
            /* rectangle definition is invalid */
            gks_report_error(SET_VIEWPORT, 51);
        }
      else
        /* transformation number is invalid */
        gks_report_error(SET_VIEWPORT, 50);
    }
  else
    /* GKS not in proper state. GKS must be in one of the states
       GKOP, WSOP, WSAC or SGOP */
    gks_report_error(SET_VIEWPORT, 8);
}

void gks_select_xform(int tnr)
{
  if (state >= GKS_K_GKOP)
    {
      if (tnr >= 0 && tnr < MAX_TNR)
        {
          s->cntnr = i_arr[0] = tnr;

          /* call the device driver link routine */
          gks_ddlk(SELECT_XFORM, 1, 1, 1, i_arr, 0, f_arr_1, 0, f_arr_2, 0, c_arr, NULL);
        }
      else
        /* transformation number is invalid */
        gks_report_error(SELECT_XFORM, 50);
    }
  else
    /* GKS not in proper state. GKS must be in one of the states
       GKOP, WSOP, WSAC or SGOP */
    gks_report_error(SELECT_XFORM, 8);
}

void gks_set_clipping(int clsw)
{
  if (state >= GKS_K_GKOP)
    {
      if (clsw != s->clip)
        {
          s->clip = i_arr[0] = clsw;

          /* call the device driver link routine */
          gks_ddlk(SET_CLIPPING, 1, 1, 1, i_arr, 0, f_arr_1, 0, f_arr_2, 0, c_arr, NULL);
        }
    }
  else
    /* GKS not in proper state. GKS must be in one of the states
       GKOP, WSOP, WSAC or SGOP */
    gks_report_error(SET_CLIPPING, 8);
}

void gks_set_ws_window(int wkid, double xmin, double xmax, double ymin, double ymax)
{
  if (state >= GKS_K_WSOP)
    {
      if (wkid > 0)
        {
          if (gks_list_find(open_ws, wkid) != NULL)
            {
              if (xmin < xmax && ymin < ymax)
                {
                  if (xmin >= 0 && xmax <= 1 && ymin >= 0 && ymax <= 1)
                    {
                      i_arr[0] = wkid;
                      f_arr_1[0] = xmin;
                      f_arr_1[1] = xmax;
                      f_arr_2[0] = ymin;
                      f_arr_2[1] = ymax;

                      /* call the device driver link routine */
                      gks_ddlk(SET_WS_WINDOW, 1, 1, 1, i_arr, 2, f_arr_1, 2, f_arr_2, 0, c_arr, NULL);
                      s->aspect_ratio = (xmax - xmin) / (ymax - ymin);
                    }
                  else
                    /* workstation window is not within the NDC unit square */
                    gks_report_error(SET_WS_WINDOW, 53);
                }
              else
                /* rectangle definition is invalid */
                gks_report_error(SET_WS_WINDOW, 51);
            }
          else
            /* specified workstation is not open */
            gks_report_error(SET_WS_WINDOW, 25);
        }
      else
        /* specified workstation identifier is invalid */
        gks_report_error(SET_WS_WINDOW, 20);
    }
  else
    /* GKS not in proper state. GKS must be in one of the states
       WSOP, WSAC or SGOP */
    gks_report_error(SET_WS_WINDOW, 7);
}

void gks_set_ws_viewport(int wkid, double xmin, double xmax, double ymin, double ymax)
{
  gks_list_t *element;
  ws_list_t *ws;

  if (state >= GKS_K_WSOP)
    {
      if (wkid > 0)
        {
          if ((element = gks_list_find(open_ws, wkid)) != NULL)
            {
              if (xmin < xmax && ymin < ymax)
                {
                  i_arr[0] = wkid;
                  f_arr_1[0] = xmin;
                  f_arr_1[1] = xmax;
                  f_arr_2[0] = ymin;
                  f_arr_2[1] = ymax;
                  /* call the device driver link routine */
                  gks_ddlk(SET_WS_VIEWPORT, 1, 1, 1, i_arr, 2, f_arr_1, 2, f_arr_2, 0, c_arr, NULL);

                  ws = (ws_list_t *)element->ptr;
                  ws->vp[0] = xmin;
                  ws->vp[1] = xmax;
                  ws->vp[2] = xmin;
                  ws->vp[3] = ymax;
                }
              else
                /* rectangle definition is invalid */
                gks_report_error(SET_WS_VIEWPORT, 51);
            }
          else
            /* specified workstation is not open */
            gks_report_error(SET_WS_VIEWPORT, 25);
        }
      else
        /* specified workstation identifier is invalid */
        gks_report_error(SET_WS_VIEWPORT, 20);
    }
  else
    /* GKS not in proper state. GKS must be in one of the states
       WSOP, WSAC or SGOP */
    gks_report_error(SET_WS_VIEWPORT, 7);
}

void gks_create_seg(int segn)
{
  if (state == GKS_K_WSAC)
    {
      i_arr[0] = segn;

      /* call the device driver link routine */
      gks_ddlk(CREATE_SEG, 1, 1, 1, i_arr, 0, f_arr_1, 0, f_arr_2, 0, c_arr, NULL);

      s->opsg = segn;
      state = GKS_K_SGOP;

      /* save segment state */
      seg_state = (gks_state_list_t *)gks_malloc(sizeof(gks_state_list_t));
      memmove(seg_state, s, sizeof(gks_state_list_t));
    }
  else
    /* GKS not in proper state. GKS must be either in the state WSAC */
    gks_report_error(CREATE_SEG, 3);
}

void gks_close_seg(void)
{
  if (state == GKS_K_SGOP)
    {
      /* call the device driver link routine */
      gks_ddlk(CLOSE_SEG, 0, 0, 0, i_arr, 0, f_arr_1, 0, f_arr_2, 0, c_arr, NULL);

      state = GKS_K_WSAC;
      s->opsg = 0;
    }
  else
    /* GKS not in proper state. GKS must be either in the state SGOP */
    gks_report_error(CLOSE_SEG, 4);
}

void gks_delete_seg(int segn)
{
  if (state >= GKS_K_WSAC)
    {
      i_arr[0] = segn;

      /* call the device driver link routine */
      gks_ddlk(DELETE_SEG, 1, 1, 1, i_arr, 0, f_arr_1, 0, f_arr_2, 0, c_arr, NULL);
    }
  else
    /* GKS not in proper state. GKS must be in one of the
       states WSOP, WSAC or SGOP */
    gks_report_error(DELETE_SEG, 7);
}

void gks_assoc_seg_with_ws(int wkid, int segn)
{
  gks_state_list_t sl;

  if (state >= GKS_K_WSOP)
    {
      if (wkid > 0)
        {
          if (s->wiss)
            {
              if (gks_list_find(active_ws, wkid) != NULL)
                {
                  /* save GKS state, restore segment state */
                  memmove(&sl, s, sizeof(gks_state_list_t));
                  memmove(s, seg_state, sizeof(gks_state_list_t));

                  id = wkid;

                  /* call the WISS dispatch routine */
                  gks_wiss_dispatch(ASSOC_SEG_WITH_WS, wkid, segn);

                  id = 0;

                  /* restore GKS state */
                  memmove(s, &sl, sizeof(gks_state_list_t));
                }
              else
                /* specified workstation is not active */
                gks_report_error(ASSOC_SEG_WITH_WS, 30);
            }
          else
            /* WISS is not open */
            gks_report_error(ASSOC_SEG_WITH_WS, 27);
        }
      else
        /* specified workstation identifier is invalid */
        gks_report_error(ASSOC_SEG_WITH_WS, 20);
    }
  else
    /* GKS not in proper state. GKS must be in one of the
       states WSOP, WSAC or SGOP */
    gks_report_error(ASSOC_SEG_WITH_WS, 7);
}

void gks_copy_seg_to_ws(int wkid, int segn)
{
  gks_state_list_t sl;

  if (state >= GKS_K_WSOP)
    {
      if (wkid > 0)
        {
          if (s->wiss)
            {
              if (gks_list_find(active_ws, wkid) != NULL)
                {
                  /* save GKS state, restore segment state */
                  memmove(&sl, s, sizeof(gks_state_list_t));
                  memmove(s, seg_state, sizeof(gks_state_list_t));

                  id = wkid;

                  /* call the WISS dispatch routine */
                  gks_wiss_dispatch(COPY_SEG_TO_WS, wkid, segn);

                  id = 0;

                  /* restore GKS state */
                  memmove(s, &sl, sizeof(gks_state_list_t));
                }
              else
                /* specified workstation is not active */
                gks_report_error(COPY_SEG_TO_WS, 30);
            }
          else
            /* WISS is not open */
            gks_report_error(COPY_SEG_TO_WS, 27);
        }
      else
        /* specified workstation identifier is invalid */
        gks_report_error(COPY_SEG_TO_WS, 20);
    }
  else
    /* GKS not in proper state. GKS must be in one of the
       states WSOP, WSAC or SGOP */
    gks_report_error(COPY_SEG_TO_WS, 7);
}

void gks_set_seg_xform(int segn, double mat[3][2])
{
  int i, j;

  if (state >= GKS_K_GKOP)
    {
      for (i = 0; i < 3; i++)
        for (j = 0; j < 2; j++) s->mat[i][j] = mat[i][j];
    }
  else
    /* GKS not in proper state. GKS must be in one of the
       states GKOP, WSOP, WSAC or SGOP */
    gks_report_error(SET_SEG_XFORM, 8);
}

static int workstation_category(int wkid)
{
  gks_list_t *element;
  ws_list_t *ws;
  ws_descr_t *descr;

  if ((element = gks_list_find(open_ws, wkid)) != NULL)
    {
      ws = (ws_list_t *)element->ptr;

      element = gks_list_find(av_ws_types, ws->wtype);
      descr = (ws_descr_t *)element->ptr;

      return descr->wscat;
    }
  else
    return -1;
}

void gks_initialize_locator(int wkid, int lcdnr, int tnr, double px, double py, int pet, double xmin, double xmax,
                            double ymin, double ymax, int ldr, char *datrec)
{
  int wscat;

  if (state >= GKS_K_WSOP)
    {
      if (wkid > 0)
        {
          if (gks_list_find(open_ws, wkid) != NULL)
            {
              wscat = workstation_category(wkid);

              if (wscat == 1 || wscat == 2)
                {
                  i_arr[0] = wkid;
                  i_arr[1] = lcdnr;
                  i_arr[2] = tnr;
                  i_arr[3] = pet;
                  f_arr_1[0] = px;
                  f_arr_1[1] = xmin;
                  f_arr_1[2] = xmax;
                  f_arr_2[0] = py;
                  f_arr_2[1] = ymin;
                  f_arr_2[2] = ymax;

                  /* call the device driver link routine */
                  gks_ddlk(INITIALIZE_LOCATOR, 4, 1, 4, i_arr, 3, f_arr_1, 3, f_arr_2, 1, datrec, NULL);
                }
              else
                /* specified workstation is neither of category INPUT nor of
                   category OUTIN */
                gks_report_error(INITIALIZE_LOCATOR, 38);
            }
          else
            /* specified workstation is not open */
            gks_report_error(INITIALIZE_LOCATOR, 25);
        }
      else
        /* specified workstation identifier is invalid */
        gks_report_error(INITIALIZE_LOCATOR, 20);
    }
  else
    /* GKS not in proper state. GKS must be in one of the
       states WSOP, WSAC or SGOP */
    gks_report_error(INITIALIZE_LOCATOR, 7);
}

void gks_request_locator(int wkid, int lcdnr, int *stat, int *tnr, double *px, double *py)
{
  int wscat;

  if (state >= GKS_K_WSOP)
    {
      if (wkid > 0)
        {
          if (gks_list_find(open_ws, wkid) != NULL)
            {
              wscat = workstation_category(wkid);

              if (wscat == 1 || wscat == 2)
                {
                  i_arr[0] = wkid;
                  i_arr[1] = lcdnr;
                  f_arr_1[0] = px[0];
                  f_arr_2[0] = py[0];

                  /* call the device driver link routine */
                  gks_ddlk(REQUEST_LOCATOR, 2, 1, 2, i_arr, 1, f_arr_1, 1, f_arr_2, 0, c_arr, NULL);

                  *stat = i_arr[0];
                  *tnr = 0;
                  *px = f_arr_1[0];
                  *py = f_arr_2[0];
                }
              else
                /* specified workstation is neither of category INPUT nor of
                   category OUTIN */
                gks_report_error(REQUEST_LOCATOR, 38);
            }
          else
            /* specified workstation is not open */
            gks_report_error(REQUEST_LOCATOR, 25);
        }
      else
        /* specified workstation identifier is invalid */
        gks_report_error(REQUEST_LOCATOR, 20);
    }
  else
    /* GKS not in proper state. GKS must be in one of the
       states WSOP, WSAC or SGOP */
    gks_report_error(REQUEST_LOCATOR, 7);
}

void gks_request_stroke(int wkid, int skdnr, int n, int *stat, int *tnr, int *np, double *pxa, double *pya)
{
  int wscat;

  if (state >= GKS_K_WSOP)
    {
      if (wkid > 0)
        {
          if (gks_list_find(open_ws, wkid) != NULL)
            {
              wscat = workstation_category(wkid);

              if (wscat == 1 || wscat == 2)
                {
                  i_arr[0] = wkid;
                  i_arr[1] = skdnr;
                  i_arr[2] = n;

                  /* call the device driver link routine */
                  gks_ddlk(REQUEST_STROKE, 3, 1, 3, i_arr, n, pxa, n, pya, 0, c_arr, NULL);

                  *stat = i_arr[0];
                  *tnr = 0;
                  *np = i_arr[2];
                }
              else
                /* specified workstation is neither of category INPUT nor of
                   category OUTIN */
                gks_report_error(REQUEST_STROKE, 38);
            }
          else
            /* specified workstation is not open */
            gks_report_error(REQUEST_STROKE, 25);
        }
      else
        /* specified workstation identifier is invalid */
        gks_report_error(REQUEST_STROKE, 20);
    }
  else
    /* GKS not in proper state. GKS must be in one of the
       states WSOP, WSAC or SGOP */
    gks_report_error(REQUEST_STROKE, 7);
}

void gks_request_choice(int wkid, int chdnr, int *stat, int *chnr)
{
  int wscat;

  if (state >= GKS_K_WSOP)
    {
      if (wkid > 0)
        {
          if (gks_list_find(open_ws, wkid) != NULL)
            {
              wscat = workstation_category(wkid);

              if (wscat == 1 || wscat == 2)
                {
                  i_arr[0] = wkid;
                  i_arr[1] = chdnr;

                  /* call the device driver link routine */
                  gks_ddlk(REQUEST_CHOICE, 2, 1, 2, i_arr, 0, f_arr_1, 0, f_arr_2, 0, c_arr, NULL);

                  *stat = i_arr[0];
                  *chnr = i_arr[1];
                }
              else
                /* specified workstation is neither of category INPUT nor of
                   category OUTIN */
                gks_report_error(REQUEST_CHOICE, 38);
            }
          else
            /* specified workstation is not open */
            gks_report_error(REQUEST_CHOICE, 25);
        }
      else
        /* specified workstation identifier is invalid */
        gks_report_error(REQUEST_CHOICE, 20);
    }
  else
    /* GKS not in proper state. GKS must be in one of the
       states WSOP, WSAC or SGOP */
    gks_report_error(REQUEST_CHOICE, 7);
}

void gks_request_string(int wkid, int stdnr, int *stat, int *lostr, char *str)
{
  int wscat;

  if (state >= GKS_K_WSOP)
    {
      if (wkid > 0)
        {
          if (gks_list_find(open_ws, wkid) != NULL)
            {
              wscat = workstation_category(wkid);

              if (wscat == 1 || wscat == 2)
                {
                  i_arr[0] = wkid;
                  i_arr[1] = stdnr;

                  /* call the device driver link routine */
                  gks_ddlk(REQUEST_STRING, 2, 1, 2, i_arr, 0, f_arr_1, 0, f_arr_2, 1, str, NULL);

                  *stat = i_arr[0];
                  *lostr = i_arr[1];
                }
              else
                /* specified workstation is neither of category INPUT nor of
                   category OUTIN */
                gks_report_error(REQUEST_STRING, 38);
            }
          else
            /* specified workstation is not open */
            gks_report_error(REQUEST_STRING, 25);
        }
      else
        /* specified workstation identifier is invalid */
        gks_report_error(REQUEST_STRING, 20);
    }
  else
    /* GKS not in proper state. GKS must be in one of the
       states WSOP, WSAC or SGOP */
    gks_report_error(REQUEST_STRING, 7);
}

void gks_read_item(int wkid, int lenidr, int maxodr, char *odr)
{
  gks_list_t *element;
  ws_list_t *ws;

  if (state >= GKS_K_WSOP)
    {
      if (wkid > 0)
        {
          if ((element = gks_list_find(open_ws, wkid)) != NULL)
            {
              ws = (ws_list_t *)element->ptr;
              if (ws->wtype == 3)
                {
                  i_arr[0] = wkid;
                  i_arr[1] = lenidr;
                  i_arr[2] = maxodr;

                  /* call the device driver link routine */
                  gks_ddlk(READ_ITEM, 3, 1, 3, i_arr, 0, f_arr_1, 0, f_arr_2, maxodr, odr, NULL);
                }
              else
                /* specified workstation is not of category MI */
                gks_report_error(READ_ITEM, 34);
            }
          else
            /* specified workstation is not open */
            gks_report_error(READ_ITEM, 25);
        }
      else
        /* specified workstation identifier is invalid */
        gks_report_error(READ_ITEM, 20);
    }
  else
    /* GKS not in proper state. GKS must be in one of the
       states WSOP, WSAC or SGOP */
    gks_report_error(READ_ITEM, 7);
}

void gks_get_item(int wkid, int *type, int *lenodr)
{
  gks_list_t *element;
  ws_list_t *ws;

  if (state >= GKS_K_WSOP)
    {
      if (wkid > 0)
        {
          if ((element = gks_list_find(open_ws, wkid)) != NULL)
            {
              ws = (ws_list_t *)element->ptr;
              if (ws->wtype == 3)
                {
                  i_arr[0] = wkid;

                  /* call the device driver link routine */
                  gks_ddlk(GET_ITEM, 1, 1, 1, i_arr, 0, f_arr_1, 0, f_arr_2, 0, c_arr, NULL);

                  *type = i_arr[0];
                  *lenodr = i_arr[1];
                }
              else
                /* specified workstation is not of category MI */
                gks_report_error(GET_ITEM, 34);
            }
          else
            /* specified workstation is not open */
            gks_report_error(GET_ITEM, 25);
        }
      else
        /* specified workstation identifier is invalid */
        gks_report_error(GET_ITEM, 20);
    }
  else
    /* GKS not in proper state. GKS must be in one of the
       states WSOP, WSAC or SGOP */
    gks_report_error(GET_ITEM, 7);
}

void gks_interpret_item(int type, int lenidr, int dimidr, char *idr)
{
  if (state >= GKS_K_WSOP)
    {
      if (type >= 0)
        {
          if (lenidr >= 8)
            {
              if (dimidr >= 1)
                {
                  i_arr[0] = type;
                  i_arr[1] = lenidr;
                  i_arr[2] = dimidr;

                  /* call the device driver link routine */
                  gks_ddlk(INTERPRET_ITEM, 3, 1, 3, i_arr, 0, f_arr_1, 0, f_arr_2, dimidr, idr, NULL);
                }
              else
                /* metafile item is invalid */
                gks_report_error(INTERPRET_ITEM, 163);
            }
          else
            /* item length is invalid */
            gks_report_error(INTERPRET_ITEM, 161);
        }
      else
        /* item type is not a valid GKS item */
        gks_report_error(INTERPRET_ITEM, 164);
    }
  else
    /* GKS not in proper state. GKS must be in one of the
       states WSOP, WSAC or SGOP */
    gks_report_error(INTERPRET_ITEM, 7);
}

void gks_eval_xform_matrix(double fx, double fy, double transx, double transy, double phi, double scalex, double scaley,
                           int coord, double tran[3][2])
{
  double cosf, sinf;

  if (state >= GKS_K_GKOP)
    {
      if (coord == GKS_K_COORDINATES_WC)
        {
          double xorg = 0, yorg = 0;

          gks_WC_to_NDC(s->cntnr, &xorg, &yorg);

          gks_WC_to_NDC(s->cntnr, &fx, &fy);
          gks_WC_to_NDC(s->cntnr, &transx, &transy);

          transx -= xorg;
          transy -= yorg;
        }

      cosf = cos(phi);
      sinf = sin(phi);

      tran[0][0] = scalex * cosf;
      tran[1][0] = scalex * sinf;
      tran[0][1] = -scaley * sinf;
      tran[1][1] = scaley * cosf;
      tran[2][0] = fx + transx - fx * tran[0][0] - fy * tran[0][1];
      tran[2][1] = fy + transx - fx * tran[1][0] - fy * tran[1][1];
    }
  else
    /* GKS not in proper state. GKS must be in one of the
       states GKOP, WSOP, WSAC or SGOP */
    gks_report_error(EVAL_XFORM_MATRIX, 8);
}

void gks_inq_operating_state(int *opsta)
{
  *opsta = state;
}

void gks_inq_level(int *errind, int *lev)
{
  *errind = GKS_K_NO_ERROR;
  *lev = GKS_K_LEVEL_0A;
}

void gks_inq_wstype(int n, int *errind, int *number, int *wtype)
{
  if (n >= 1 && n <= num_ws_types)
    {
      *errind = GKS_K_NO_ERROR;
      *number = num_ws_types;
      *wtype = ws_types[n - 1].wtype;
    }
  else
    *errind = GKS_K_ERROR;
}

void gks_inq_max_xform(int *errind, int *maxtnr)
{
  *errind = GKS_K_NO_ERROR;
  *maxtnr = MAX_TNR - 1;
}

void gks_inq_open_ws(int n, int *errind, int *ol, int *wkid)
{
  gks_list_t *list;
  int num = 0;

  if (n >= 1)
    {
      list = open_ws;
      while (list != NULL)
        {
          if (n == ++num) *wkid = list->item;

          list = list->next;
        }

      *errind = GKS_K_NO_ERROR;
      *ol = num;
    }
  else
    *errind = GKS_K_ERROR;
}

void gks_inq_active_ws(int n, int *errind, int *ol, int *wkid)
{
  gks_list_t *list;
  int num = 0;

  if (n >= 1)
    {
      list = active_ws;
      while (list != NULL)
        {
          if (n == ++num) *wkid = list->item;

          list = list->next;
        }

      *errind = GKS_K_NO_ERROR;
      *ol = num;
    }
  else
    *errind = GKS_K_ERROR;
}

void gks_inq_segn_ws(int wkid, int n, int *errind, int *ol, int *segn)
{
  *errind = GKS_K_NO_ERROR;
  *ol = 0;
  *segn = 0;
}

void gks_inq_color_rep(int wkid, int index, int type, int *errind, double *red, double *green, double *blue)
{
  *errind = GKS_K_NO_ERROR;
  gks_inq_rgb(index, red, green, blue);
}

void gks_inq_pline_linetype(int *errind, int *ltype)
{
  *errind = GKS_K_NO_ERROR;
  if (api)
    *ltype = s->asf[0] == GKS_K_ASF_INDIVIDUAL ? s->ltype : s->lindex;
  else
    *ltype = s->ltype;
}

void gks_inq_pline_linewidth(int *errind, double *lwidth)
{
  *errind = GKS_K_NO_ERROR;
  if (api)
    *lwidth = s->asf[1] == GKS_K_ASF_INDIVIDUAL ? s->lwidth : 1.0;
  else
    *lwidth = s->lwidth;
}

void gks_inq_pline_color_index(int *errind, int *coli)
{
  *errind = GKS_K_NO_ERROR;
  if (api)
    *coli = s->asf[2] == GKS_K_ASF_INDIVIDUAL ? s->plcoli : 1;
  else
    *coli = s->plcoli;
}

void gks_inq_pmark_type(int *errind, int *mtype)
{
  *errind = GKS_K_NO_ERROR;
  if (api)
    *mtype = s->asf[3] == GKS_K_ASF_INDIVIDUAL ? s->mtype : s->mindex;
  else
    *mtype = s->mtype;
}

void gks_inq_pmark_size(int *errind, double *mszsc)
{
  *errind = GKS_K_NO_ERROR;
  if (api)
    *mszsc = s->asf[4] == GKS_K_ASF_INDIVIDUAL ? s->mszsc : 1.0;
  else
    *mszsc = s->mszsc;
}

void gks_inq_pmark_color_index(int *errind, int *coli)
{
  *errind = GKS_K_NO_ERROR;
  if (api)
    *coli = s->asf[5] == GKS_K_ASF_INDIVIDUAL ? s->pmcoli : 1;
  else
    *coli = s->pmcoli;
}

void gks_inq_text_fontprec(int *errind, int *font, int *prec)
{
  *errind = GKS_K_NO_ERROR;
  if (api)
    {
      if (s->asf[6] == GKS_K_ASF_INDIVIDUAL)
        {
          *font = s->txfont;
          *prec = s->txprec;
        }
      else
        {
          *font = predef_font[s->tindex - 1];
          *prec = predef_prec[s->tindex - 1];
        }
    }
  else
    {
      *font = s->txfont;
      *prec = s->txprec;
    }
}

void gks_inq_text_expfac(int *errind, double *chxp)
{
  *errind = GKS_K_NO_ERROR;
  if (api)
    *chxp = s->asf[7] == GKS_K_ASF_INDIVIDUAL ? s->chxp : 1.0;
  else
    *chxp = s->chxp;
}

void gks_inq_text_spacing(int *errind, double *chsp)
{
  *errind = GKS_K_NO_ERROR;
  if (api)
    *chsp = s->asf[8] == GKS_K_ASF_INDIVIDUAL ? s->chsp : 0.0;
  else
    *chsp = s->chsp;
}

void gks_inq_text_color_index(int *errind, int *coli)
{
  *errind = GKS_K_NO_ERROR;
  if (api)
    *coli = s->asf[9] == GKS_K_ASF_INDIVIDUAL ? s->txcoli : 1;
  else
    *coli = s->txcoli;
}

void gks_inq_text_height(int *errind, double *chh)
{
  *errind = GKS_K_NO_ERROR;
  *chh = s->chh;
}

void gks_inq_text_upvec(int *errind, double *chux, double *chuy)
{
  *errind = GKS_K_NO_ERROR;
  *chux = s->chup[0];
  *chuy = s->chup[1];
}

void gks_inq_text_path(int *errind, int *txp)
{
  *errind = GKS_K_NO_ERROR;
  *txp = s->txp;
}

void gks_inq_text_align(int *errind, int *txalh, int *txalv)
{
  *errind = GKS_K_NO_ERROR;
  *txalh = s->txal[0];
  *txalv = s->txal[1];
}

void gks_inq_fill_int_style(int *errind, int *ints)
{
  *errind = GKS_K_NO_ERROR;
  if (api)
    *ints = s->asf[10] == GKS_K_ASF_INDIVIDUAL ? s->ints : predef_ints[s->findex - 1];
  else
    *ints = s->ints;
}

void gks_inq_fill_style_index(int *errind, int *styli)
{
  *errind = GKS_K_NO_ERROR;
  if (api)
    *styli = s->asf[11] == GKS_K_ASF_INDIVIDUAL ? s->styli : predef_styli[s->findex - 1];
  else
    *styli = s->styli;
}

void gks_inq_fill_color_index(int *errind, int *coli)
{
  *errind = GKS_K_NO_ERROR;
  if (api)
    *coli = s->asf[12] == GKS_K_ASF_INDIVIDUAL ? s->facoli : 1;
  else
    *coli = s->facoli;
}

void gks_inq_open_segn(int *errind, int *segn)
{
  if (state == GKS_K_SGOP)
    {
      *errind = GKS_K_NO_ERROR;
      *segn = s->opsg;
    }
  else
    *errind = GKS_K_ERROR;
}

void gks_inq_current_xformno(int *errind, int *tnr)
{
  *errind = GKS_K_NO_ERROR;
  *tnr = s->cntnr;
}

void gks_inq_xform(int tnr, int *errind, double *wn, double *vp)
{
  int i;

  if (tnr >= 0 && tnr < MAX_TNR)
    {
      *errind = GKS_K_NO_ERROR;

      for (i = 0; i < 4; i++)
        {
          wn[i] = s->window[tnr][i];
          vp[i] = s->viewport[tnr][i];
        }
    }
  else
    *errind = GKS_K_ERROR;
}

void gks_inq_clip(int *errind, int *clsw, double *clrt)
{
  int i, tnr;

  *errind = GKS_K_NO_ERROR;
  *clsw = s->clip;

  tnr = (s->clip == GKS_K_CLIP) ? s->cntnr : 0;
  for (i = 0; i < 4; i++) clrt[i] = s->viewport[tnr][i];
}

void gks_inq_ws_conntype(int wkid, int *errind, int *conid, int *wtype)
{
  gks_list_t *element;
  ws_list_t *ws;

  if ((element = gks_list_find(open_ws, wkid)) != NULL)
    {
      ws = (ws_list_t *)element->ptr;

      *errind = GKS_K_NO_ERROR;
      *conid = ws->conid;
      *wtype = ws->wtype;
    }
  else
    *errind = GKS_K_ERROR;
}

void gks_inq_ws_category(int wtype, int *errind, int *wscat)
{
  gks_list_t *element;
  ws_descr_t *ws;

  if ((element = gks_list_find(av_ws_types, wtype)) != NULL)
    {
      ws = (ws_descr_t *)element->ptr;

      *errind = GKS_K_NO_ERROR;
      *wscat = ws->wscat;
    }
  else
    *errind = GKS_K_ERROR;
}

void gks_inq_text_extent(int wkid, double px, double py, char *str, int *errind, double *cpx, double *cpy, double *tx,
                         double *ty)
{
  double bx[9], by[9];
  int i;

  if (gks_list_find(open_ws, wkid) != NULL && strlen(str) != 0)
    {
      if (s->txprec != GKS_K_TEXT_PRECISION_OUTLINE)
        {
          /* double the string length as the longest utf8 representation of any latin1 character is two bytes long */
          char *utf8_str = gks_malloc(strlen(str) * 2 + 1);
          gks_input2utf8(str, utf8_str, s->input_encoding);

          gks_util_inq_text_extent(px, py, utf8_str, strlen(utf8_str), cpx, cpy, tx, ty);

          gks_free(utf8_str);
        }
      else
        {
          if (s->input_encoding == ENCODING_LATIN1)
            {
              char *utf8_str = gks_malloc(strlen(str) * 2 + 1);
              gks_input2utf8(str, utf8_str, ENCODING_LATIN1);

              gks_ft_inq_text_extent(px, py, utf8_str, s, gks_ft_gdp, bx, by);
            }
          else
            gks_ft_inq_text_extent(px, py, str, s, gks_ft_gdp, bx, by);

          for (i = 0; i < 4; i++)
            {
              tx[i] = bx[i];
              ty[i] = by[i];
            }
          *cpx = bx[8];
          *cpy = by[8];
        }
      *errind = GKS_K_NO_ERROR;
    }
  else
    *errind = GKS_K_ERROR;
}

void gks_inq_max_ds_size(int wtype, int *errind, int *dcunit, double *rx, double *ry, int *lx, int *ly)
{
  gks_list_t *element;
  ws_descr_t *ws;

  if ((element = gks_list_find(av_ws_types, wtype)) != NULL)
    {
      ws = (ws_descr_t *)element->ptr;

      *errind = GKS_K_NO_ERROR;
      *dcunit = ws->dcunit;
      *rx = ws->sizex;
      *ry = ws->sizey;
      *lx = ws->unitsx;
      *ly = ws->unitsy;
    }
  else
    *errind = GKS_K_ERROR;
}

void gks_inq_vp_size(int wkid, int *errind, int *width, int *height, double *device_pixel_ratio)
{
  gks_list_t *element;
  ws_list_t *ws;
  ws_descr_t *descr;
  double *vp;

  if ((element = gks_list_find(open_ws, wkid)) != NULL)
    {
      ws = (ws_list_t *)element->ptr;

      switch (ws->wtype)
        {
#ifndef EMSCRIPTEN
        case 381:
          gks_qt_plugin(INQ_WS_STATE, 2, 1, 2, i_arr, 1, f_arr_1, 0, f_arr_2, 0, c_arr, &ws->ptr);
          break;

        case 400:
          gks_quartz_plugin(INQ_WS_STATE, 2, 1, 2, i_arr, 1, f_arr_1, 0, f_arr_2, 0, c_arr, &ws->ptr);
          break;

        case 411:
          gks_drv_socket(INQ_WS_STATE, 2, 1, 2, i_arr, 1, f_arr_1, 0, f_arr_2, 0, c_arr, &ws->ptr);
          break;
#endif
        default:
          element = gks_list_find(av_ws_types, ws->wtype);
          descr = (ws_descr_t *)element->ptr;

          i_arr[0] = (int)((ws->vp[1] - ws->vp[0]) / descr->sizex * descr->unitsx + 0.5);
          i_arr[1] = (int)((ws->vp[3] - ws->vp[2]) / descr->sizey * descr->unitsy + 0.5);
          if (ws->wtype == 101 || ws->wtype == 102 || ws->wtype == 382) /* PDF or SVG */
            f_arr_1[0] = 4.0;
          else
            f_arr_1[0] = 1.0;
        }

      *errind = GKS_K_NO_ERROR;
      vp = s->viewport[s->cntnr];
      if (i_arr[0] == 0 && i_arr[1] == 0)
        {
          element = gks_list_find(av_ws_types, ws->wtype);
          descr = (ws_descr_t *)element->ptr;
          i_arr[0] = (int)((ws->vp[1] - ws->vp[0]) / descr->sizex * descr->unitsx + 0.5);
          i_arr[1] = (int)((ws->vp[3] - ws->vp[2]) / descr->sizey * descr->unitsy + 0.5);
        }
      if (s->aspect_ratio > 1)
        {
          *width = i_arr[0] * (vp[1] - vp[0]);
          *height = i_arr[1] * (vp[3] - vp[2]) * s->aspect_ratio;
        }
      else
        {
          *width = i_arr[0] * (vp[1] - vp[0]) / s->aspect_ratio;
          *height = i_arr[1] * (vp[3] - vp[2]);
        }
      *device_pixel_ratio = f_arr_1[0];
    }
  else
    *errind = GKS_K_ERROR;
}

void gks_emergency_close(void)
{
  static int closing = 0;

  if (!closing)
    {
      closing = 1;

      if (state == GKS_K_SGOP) gks_close_seg();

      if (state == GKS_K_WSAC)
        {
          while (active_ws != NULL) gks_deactivate_ws(active_ws->item);
        }

      if (state == GKS_K_WSOP)
        {
          while (open_ws != NULL) gks_close_ws(open_ws->item);
        }

      if (state == GKS_K_GKOP) gks_close_gks();

      closing = 0;
    }
}

void gks_set_text_slant(double slant)
{
  if (state >= GKS_K_GKOP)
    {
      s->txslant = f_arr_1[0] = slant;

      /* call the device driver link routine */
      gks_ddlk(SET_TEXT_SLANT, 0, 0, 0, i_arr, 1, f_arr_1, 0, f_arr_2, 0, c_arr, NULL);
    }
  else
    /* GKS not in proper state. GKS must be in one of the states
       GKOP, WSOP, WSAC or SGOP */
    gks_report_error(SET_TEXT_SLANT, 8);
}

void gks_set_shadow(double offsetx, double offsety, double blur)
{
  if (state >= GKS_K_GKOP)
    {
      s->shoff[0] = f_arr_1[0] = offsetx;
      s->shoff[1] = f_arr_1[1] = offsety;
      s->blur = f_arr_1[2] = blur;

      /* call the device driver link routine */
      gks_ddlk(SET_SHADOW, 0, 0, 0, i_arr, 3, f_arr_1, 0, f_arr_2, 0, c_arr, NULL);
    }
  else
    /* GKS not in proper state. GKS must be in one of the states
       GKOP, WSOP, WSAC or SGOP */
    gks_report_error(SET_SHADOW, 8);
}

void gks_set_transparency(double alpha)
{
  if (state >= GKS_K_GKOP)
    {
      s->alpha = f_arr_1[0] = alpha;

      /* call the device driver link routine */
      gks_ddlk(SET_TRANSPARENCY, 0, 0, 0, i_arr, 1, f_arr_1, 0, f_arr_2, 0, c_arr, NULL);
    }
  else
    /* GKS not in proper state. GKS must be in one of the states
       GKOP, WSOP, WSAC or SGOP */
    gks_report_error(SET_TRANSPARENCY, 8);
}

void gks_set_coord_xform(double mat[3][2])
{
  if (state >= GKS_K_GKOP)
    {
      f_arr_1[0] = mat[0][0];
      f_arr_1[1] = mat[0][1];
      f_arr_1[2] = mat[1][0];
      f_arr_1[3] = mat[1][1];
      f_arr_1[4] = mat[2][0];
      f_arr_1[5] = mat[2][1];

      /* call the device driver link routine */
      gks_ddlk(SET_COORD_XFORM, 0, 0, 0, i_arr, 6, f_arr_1, 0, f_arr_2, 0, c_arr, NULL);
    }
  else
    /* GKS not in proper state. GKS must be in one of the
       states GKOP, WSOP, WSAC or SGOP */
    gks_report_error(SET_COORD_XFORM, 8);
}

void gks_begin_selection(int index, int kind)
{
  if (state >= GKS_K_WSAC)
    {
      i_arr[0] = index;
      i_arr[1] = kind;

      /* call the device driver link routine */
      gks_ddlk(BEGIN_SELECTION, 2, 1, 2, i_arr, 0, f_arr_1, 0, f_arr_2, 0, c_arr, NULL);
    }
  else
    /* GKS not in proper state. GKS must be either in the state
       WSAC or in the state SGOP */
    gks_report_error(BEGIN_SELECTION, 5);
}

void gks_end_selection(void)
{
  if (state >= GKS_K_WSAC)
    {
      /* call the device driver link routine */
      gks_ddlk(END_SELECTION, 0, 0, 0, i_arr, 0, f_arr_1, 0, f_arr_2, 0, c_arr, NULL);
    }
  else
    /* GKS not in proper state. GKS must be either in the state
       WSAC or in the state SGOP */
    gks_report_error(END_SELECTION, 5);
}

void gks_move_selection(double x, double y)
{
  if (state >= GKS_K_WSAC)
    {
      f_arr_1[0] = x;
      f_arr_2[0] = y;

      /* call the device driver link routine */
      gks_ddlk(MOVE_SELECTION, 0, 0, 0, i_arr, 1, f_arr_1, 1, f_arr_2, 0, c_arr, NULL);
    }
  else
    /* GKS not in proper state. GKS must be either in the state
       WSAC or in the state SGOP */
    gks_report_error(MOVE_SELECTION, 5);
}

void gks_resize_selection(int kind, double x, double y)
{
  if (state >= GKS_K_WSAC)
    {
      i_arr[0] = kind;
      f_arr_1[0] = x;
      f_arr_2[0] = y;

      /* call the device driver link routine */
      gks_ddlk(RESIZE_SELECTION, 1, 1, 1, i_arr, 1, f_arr_1, 1, f_arr_2, 0, c_arr, NULL);
    }
  else
    /* GKS not in proper state. GKS must be either in the state
       WSAC or in the state SGOP */
    gks_report_error(RESIZE_SELECTION, 5);
}

void gks_draw_image(double x, double y, double scalex, double scaley, int width, int height, int *data)
{
  if (state >= GKS_K_WSAC)
    {
      if (width >= 1 && height >= 1)
        {
          if (data != NULL)
            {
              f_arr_1[0] = x;
              f_arr_2[0] = y;
              f_arr_1[1] = scalex;
              f_arr_2[1] = scaley;

              /* call the device driver link routine */
              gks_ddlk(DRAW_IMAGE, width, height, width, data, 2, f_arr_1, 2, f_arr_2, 0, c_arr, NULL);
            }
          else
            /* invalid image data pointer */
            gks_report_error(DRAW_IMAGE, 402);
        }
      else
        /* dimensions of image are invalid */
        gks_report_error(DRAW_IMAGE, 401);
    }
  else
    /* GKS not in proper state. GKS must be either in the state
       WSAC or in the state SGOP */
    gks_report_error(DRAW_IMAGE, 5);
}

void gks_inq_bbox(int *errind, double *xmin, double *xmax, double *ymin, double *ymax)
{
  if (state >= GKS_K_WSAC)
    {
      f_arr_1[0] = f_arr_1[1] = 0;
      f_arr_2[0] = f_arr_2[1] = 0;

      /* call the device driver link routine */
      gks_ddlk(INQ_BBOX, 0, 0, 0, i_arr, 0, f_arr_1, 0, f_arr_2, 0, c_arr, NULL);

      *errind = GKS_K_NO_ERROR;
      *xmin = f_arr_1[0];
      *xmax = f_arr_1[1];
      *ymin = f_arr_2[0];
      *ymax = f_arr_2[1];
    }
  else
    *errind = GKS_K_ERROR;
}

void gks_inq_text_slant(int *errind, double *slant)
{
  *errind = GKS_K_NO_ERROR;
  *slant = s->txslant;
}

void gks_set_callback(char *(*callback)(const char *arg))
{
  s->callback = callback;
}

static void gksrealloc(int n)
{
  if (n > max_points)
    {
      x = (double *)realloc(x, sizeof(double) * n);
      y = (double *)realloc(y, sizeof(double) * n);
      max_points = n;
    }
}

int gopengks(Gfile *errfile, Glong memory)
{
  int errfil;

  errfil = (errfile != NULL) ? fileno(errfile) : 0;

  gks_open_gks(errfil);

  if (gks_errno == 0)
    {
      x = (double *)malloc(sizeof(double) * MAX_POINTS);
      y = (double *)malloc(sizeof(double) * MAX_POINTS);
      max_points = MAX_POINTS;
    }
  return gks_errno;
}

int gclosegks(void)
{
  gks_close_gks();

  if (gks_errno == 0)
    {
      free(y);
      free(x);
      max_points = 0;
    }
  return gks_errno;
}

int gopenws(Gint workstation_id, Gconn *connection, Gwstype *type)
{
  int wkid = workstation_id, wstype = *type;

  gks_open_ws(wkid, connection, wstype);

  return gks_errno;
}

int gclosews(Gint workstation_id)
{
  int wkid = workstation_id;

  gks_close_ws(wkid);

  return gks_errno;
}

int gactivatews(Gint workstation_id)
{
  int wkid = workstation_id;

  gks_activate_ws(wkid);

  return gks_errno;
}

int gdeactivatews(Gint workstation_id)
{
  int wkid = workstation_id;

  gks_deactivate_ws(wkid);

  return gks_errno;
}

int gconfigurews(Gint workstation_id)
{
  int wkid = workstation_id;

  gks_configure_ws(wkid);

  return gks_errno;
}

int gclearws(Gint workstation_id, Gclrflag clearflag)
{
  int wkid = workstation_id, cofl = clearflag;

  gks_clear_ws(wkid, cofl);

  return gks_errno;
}

int gupdatews(Gint workstation_id, Gregen regenflag)
{
  int wkid = workstation_id, refl;

  refl = regenflag == GPOSTPONE ? 0 : 1;
  gks_update_ws(wkid, refl);

  return gks_errno;
}

int gpolyline(Gint n, Gpoint *points)
{
  int i, npoints = n;

  gksrealloc(n);

  for (i = 0; i < n; i++)
    {
      x[i] = points[i].x;
      y[i] = points[i].y;
    }
  gks_polyline(npoints, x, y);

  return gks_errno;
}

int gpolymarker(Gint n, Gpoint *points)
{
  int i, npoints = n;

  gksrealloc(n);

  for (i = 0; i < n; i++)
    {
      x[i] = points[i].x;
      y[i] = points[i].y;
    }
  gks_polymarker(npoints, x, y);

  return gks_errno;
}

int gtext(Gpoint *position, Gchar *string)
{
  double qx, qy;
  char *chars;

  qx = position->x;
  qy = position->y;
  chars = string;
  gks_text(qx, qy, chars);

  return gks_errno;
}

int gfillarea(Gint n, Gpoint *points)
{
  int i, npoints = n;

  gksrealloc(n);

  for (i = 0; i < n; i++)
    {
      x[i] = points[i].x;
      y[i] = points[i].y;
    }
  gks_fillarea(npoints, x, y);

  return gks_errno;
}

int gcellarray(Grect *rectangle, Gidim *dimensions, Gint *color)
{
  double qx, qy, rx, ry;
  int dx, dy, scol, srow, ncol, nrow, *colia = color;

  qx = rectangle->ul.x;
  qy = rectangle->ul.y;
  rx = rectangle->lr.x;
  ry = rectangle->lr.y;
  dx = dimensions->x_dim;
  dy = dimensions->y_dim;
  scol = 1;
  srow = 1;
  ncol = dx;
  nrow = dy;

  gks_cellarray(qx, qy, rx, ry, dx, dy, scol, srow, ncol, nrow, colia);

  return gks_errno;
}

int gsetasf(Gasfs *asfs)
{
  int flag[13];

  flag[0] = asfs->ln_type;
  flag[1] = asfs->ln_width;
  flag[2] = asfs->ln_colour;
  flag[3] = asfs->mk_type;
  flag[4] = asfs->mk_size;
  flag[5] = asfs->mk_colour;
  flag[6] = asfs->tx_fp;
  flag[7] = asfs->tx_exp;
  flag[8] = asfs->tx_space;
  flag[9] = asfs->tx_colour;
  flag[10] = asfs->fl_inter;
  flag[11] = asfs->fl_style;
  flag[12] = asfs->fl_colour;

  gks_set_asf(flag);

  return gks_errno;
}

int gsetlineind(Gint index)
{
  int pli = index;

  gks_set_pline_index(pli);

  return gks_errno;
}

int gsetlinetype(Gint type)
{
  int ltype = type;

  gks_set_pline_linetype(ltype);

  return gks_errno;
}

int gsetlinewidth(Gfloat width)
{
  double lwidth = width;

  gks_set_pline_linewidth(lwidth);

  return gks_errno;
}

int gsetlinecolourind(Gint colour)
{
  int coli = colour;

  gks_set_pline_color_index(coli);

  return gks_errno;
}

int gsetmarkerind(Gint index)
{
  int pmi = index;

  gks_set_pmark_index(pmi);

  return gks_errno;
}

int gsetmarkertype(Gint type)
{
  int mtype = type;

  gks_set_pmark_type(mtype);

  return gks_errno;
}

int gsetmarkersize(Gfloat size)
{
  double mszsc = size;

  gks_set_pmark_size(mszsc);

  return gks_errno;
}

int gsetmarkercolourind(Gint colour)
{
  int coli = colour;

  gks_set_pmark_color_index(coli);

  return gks_errno;
}

int gsettextind(Gint index)
{
  int txi = index;

  gks_set_text_index(txi);

  return gks_errno;
}

int gsettextfontprec(Gtxfp *txfp)
{
  int font = txfp->font, prec = txfp->prec;

  gks_set_text_fontprec(font, prec);

  return gks_errno;
}

int gsetcharexpan(Gfloat exp)
{
  double chxp = exp;

  gks_set_text_expfac(chxp);

  return gks_errno;
}

int gsetcharspace(Gfloat spacing)
{
  double chsp = spacing;

  gks_set_text_spacing(chsp);

  return gks_errno;
}

int gsettextcolourind(Gint colour)
{
  int coli = colour;

  gks_set_text_color_index(coli);

  return gks_errno;
}

int gsetcharheight(Gfloat height)
{
  double chh = height;

  gks_set_text_height(chh);

  return gks_errno;
}

int gsetcharup(Gpoint *charup)
{
  double chux, chuy;

  chux = charup->x;
  chuy = charup->y;

  gks_set_text_upvec(chux, chuy);

  return gks_errno;
}

int gsettextpath(Gtxpath text_path)
{
  int txp = text_path;

  gks_set_text_path(txp);

  return gks_errno;
}

int gsettextalign(Gtxalign *txalign)
{
  int alh, alv;

  alh = txalign->hor;
  alv = txalign->ver;

  gks_set_text_align(alh, alv);

  return gks_errno;
}

int gsetfillind(Gint index)
{
  int fai = index;

  gks_set_fill_index(fai);

  return gks_errno;
}

int gsetfillintstyle(Gflinter style)
{
  int ints = style;

  gks_set_fill_int_style(ints);

  return gks_errno;
}

int gsetfillstyle(Gint index)
{
  int styli = index;

  gks_set_fill_style_index(styli);

  return gks_errno;
}

int gsetfillcolourind(Gint colour)
{
  int coli = colour;

  gks_set_fill_color_index(coli);

  return gks_errno;
}

int gsetcolourrep(Gint workstation_id, Gint index, Gcobundl *rep)
{
  int wkid = workstation_id, coli = index;
  double r, g, b;

  r = rep->red;
  g = rep->green;
  b = rep->blue;

  gks_set_color_rep(wkid, coli, r, g, b);

  return gks_errno;
}

int gsetwindow(Gint transform, Glimit *window)
{
  int tnr = transform;
  double xmin, xmax, ymin, ymax;

  xmin = window->xmin;
  xmax = window->xmax;
  ymin = window->ymin;
  ymax = window->ymax;

  gks_set_window(tnr, xmin, xmax, ymin, ymax);

  return gks_errno;
}

int gsetviewport(Gint transform, Glimit *viewport)
{
  int tnr = transform;
  double xmin, xmax, ymin, ymax;

  xmin = viewport->xmin;
  xmax = viewport->xmax;
  ymin = viewport->ymin;
  ymax = viewport->ymax;

  gks_set_viewport(tnr, xmin, xmax, ymin, ymax);

  return gks_errno;
}

int gselntran(Gint transform)
{
  int tnr = transform;

  gks_select_xform(tnr);

  return gks_errno;
}

int gsetclip(Gclip indicator)
{
  int clsw = indicator;

  gks_set_clipping(clsw);

  return gks_errno;
}

int gsetwswindow(Gint workstation_id, Glimit *window)
{
  int wkid = workstation_id;
  double xmin, xmax, ymin, ymax;

  xmin = window->xmin;
  xmax = window->xmax;
  ymin = window->ymin;
  ymax = window->ymax;

  gks_set_ws_window(wkid, xmin, xmax, ymin, ymax);

  return gks_errno;
}

int gsetwsviewport(Gint workstation_id, Glimit *viewport)
{
  int wkid = workstation_id;
  double xmin, xmax, ymin, ymax;

  xmin = viewport->xmin;
  xmax = viewport->xmax;
  ymin = viewport->ymin;
  ymax = viewport->ymax;

  gks_set_ws_viewport(wkid, xmin, xmax, ymin, ymax);

  return gks_errno;
}

int gcreateseg(Gint segment_name)
{
  int segn = segment_name;

  gks_create_seg(segn);

  return gks_errno;
}

int gcopysegws(Gint workstation_id, Gint segment_name)
{
  int wkid = workstation_id, segn = segment_name;

  gks_copy_seg_to_ws(wkid, segn);

  return gks_errno;
}

int gredrawsegws(Gint workstation_id)
{
  int wkid = workstation_id;

  gks_redraw_seg_on_ws(wkid);

  return gks_errno;
}

int gcloseseg(void)
{
  gks_close_seg();

  return gks_errno;
}

int gevaltran(Gpoint *ppoint, Gpoint *pshift, Gfloat angle, Gscale *pscale, Gcsw coord, Gfloat result[2][3])
{
  double x0, y0, tx, ty, phi, fx, fy;
  int isw, i, j;
  double tran[3][2];

  x0 = ppoint->x;
  y0 = ppoint->y;
  tx = pshift->x;
  ty = pshift->y;
  phi = angle;
  fx = pscale->x_scale;
  fy = pscale->y_scale;
  isw = coord;

  gks_eval_xform_matrix(x0, y0, tx, ty, phi, fx, fy, isw, tran);

  for (i = 0; i < 2; i++)
    for (j = 0; j < 3; j++) result[i][j] = tran[j][i];

  return gks_errno;
}

int gsetsegtran(Gint segment_name, Gfloat segtran[2][3])
{
  int segn = segment_name, i, j;
  double tran[3][2];

  for (i = 0; i < 2; i++)
    for (j = 0; j < 3; j++) tran[j][i] = segtran[i][j];

  gks_set_seg_xform(segn, tran);

  return gks_errno;
}

int ginqopst(Gint *state)
{
  int opsta;

  gks_inq_operating_state(&opsta);

  *state = opsta;

  return OK;
}

int ginqlevelgks(Gint *level, Gint *error_status)
{
  int errind, lev;

  gks_inq_level(&errind, &lev);

  *level = lev;
  *error_status = errind;

  return OK;
}

int ginqmaxntrannum(Gint *maxtran, Gint *error_status)
{
  int errind, maxtnr;

  gks_inq_max_xform(&errind, &maxtnr);

  *maxtran = maxtnr;
  *error_status = errind;

  return OK;
}

int ginqcharheight(Gfloat *height, Gint *error_status)
{
  int errind;
  double chh;

  gks_inq_text_height(&errind, &chh);

  *height = chh;
  *error_status = errind;

  return OK;
}

int ginqcharup(Gpoint *up, Gint *error_status)
{
  double chux, chuy;
  int errind;

  gks_inq_text_upvec(&errind, &chux, &chuy);

  up->x = chux;
  up->y = chuy;
  *error_status = errind;

  return OK;
}

int ginqtextpath(Gtxpath *text_path, Gint *error_status)
{
  int errind, path;

  gks_inq_text_path(&errind, &path);

  *text_path = (Gtxpath)path;
  *error_status = errind;

  return OK;
}

int ginqtextalign(Gtxalign *align, Gint *error_status)
{
  int errind, alh, alv;

  gks_inq_text_align(&errind, &alh, &alv);

  align->hor = (Gtxhor)alh;
  align->ver = (Gtxver)alv;

  return OK;
}

int ginqlinetype(Gint *type, Gint *error_status)
{
  int errind, ltype;

  gks_inq_pline_linetype(&errind, &ltype);

  *type = ltype;
  *error_status = errind;

  return OK;
}

int ginqlinewidth(Gfloat *width, Gint *error_status)
{
  int errind;
  double lwidth;

  gks_inq_pline_linewidth(&errind, &lwidth);

  *width = lwidth;
  *error_status = errind;

  return OK;
}

int ginqlinecolourind(Gint *index, Gint *error_status)
{
  int errind, coli;

  gks_inq_pline_color_index(&errind, &coli);

  *index = coli;
  *error_status = errind;

  return OK;
}

int ginqmarkertype(Gint *level, Gint *error_status)
{
  int errind, mtype;

  gks_inq_pmark_type(&errind, &mtype);

  *level = mtype;
  *error_status = errind;

  return OK;
}

int ginqmarkersize(Gfloat *mksize, Gint *error_status)
{
  int errind;
  double mszsc;

  gks_inq_pmark_size(&errind, &mszsc);

  *mksize = mszsc;
  *error_status = errind;

  return OK;
}

int ginqmarkercolourind(Gint *index, Gint *error_status)
{
  int errind, coli;

  gks_inq_pmark_color_index(&errind, &coli);

  *index = coli;
  *error_status = errind;

  return OK;
}

int ginqtextfontprec(Gtxfp *txfp, Gint *error_status)
{
  int errind, font, prec;

  gks_inq_text_fontprec(&errind, &font, &prec);

  txfp->font = font;
  txfp->prec = (Gtxprec)prec;
  *error_status = errind;

  return OK;
}

int ginqcharexpan(Gfloat *chexp, Gint *error_status)
{
  int errind;
  double chxp;

  gks_inq_text_expfac(&errind, &chxp);

  *chexp = chxp;
  *error_status = errind;

  return OK;
}

int ginqcharspace(Gfloat *chspc, Gint *error_status)
{
  int errind;
  double chsp;

  gks_inq_text_spacing(&errind, &chsp);

  *chspc = chsp;
  *error_status = errind;

  return OK;
}

int ginqtextcolourind(Gint *index, Gint *error_status)
{
  int errind, coli;

  gks_inq_text_color_index(&errind, &coli);

  *index = coli;
  *error_status = errind;

  return OK;
}

int ginqfillintstyle(Gint *style, Gint *error_status)
{
  int ints, errind;

  gks_inq_fill_int_style(&errind, &ints);

  *style = ints;
  *error_status = errind;

  return OK;
}

int ginqfillstyle(Gint *index, Gint *error_status)
{
  int styli, errind;

  gks_inq_fill_style_index(&errind, &styli);

  *index = styli;
  *error_status = errind;

  return OK;
}

int ginqfillcolourind(Gint *index, Gint *error_status)
{
  int errind, coli;

  gks_inq_fill_color_index(&errind, &coli);

  *index = coli;
  *error_status = errind;

  return OK;
}

int ginqcurntrannum(Gint *tran, Gint *error_status)
{
  int errind, tnr;

  gks_inq_current_xformno(&errind, &tnr);

  *tran = tnr;
  *error_status = errind;

  return OK;
}

int ginqntran(Gint num, Gtran *tran, Gint *error_status)
{
  int tnr = num, errind;
  double wn[4], vp[4];

  gks_inq_xform(tnr, &errind, wn, vp);

  tran->w.xmin = wn[0];
  tran->w.xmax = wn[1];
  tran->w.ymin = wn[2];
  tran->w.ymax = wn[3];
  tran->v.xmin = vp[0];
  tran->v.xmax = vp[1];
  tran->v.ymin = vp[2];
  tran->v.ymax = vp[3];
  *error_status = errind;

  return OK;
}

int ginqclip(Gcliprect *clipping, Gint *error_status)
{
  int errind, clsw;
  double clrt[4];

  gks_inq_clip(&errind, &clsw, clrt);

  clipping->rec.xmin = clrt[0];
  clipping->rec.xmax = clrt[1];
  clipping->rec.ymin = clrt[2];
  clipping->rec.ymax = clrt[3];
  clipping->ind = (Gclip)clsw;
  *error_status = errind;

  return OK;
}

int ginqwscategory(Gwstype *workstation_type, Gint *cat, Gint *error_status)
{
  int wstype = *workstation_type;
  int errind, category;

  gks_inq_ws_category(wstype, &errind, &category);

  *cat = category;
  *error_status = errind;

  return OK;
}

int ginqdisplaysize(Gwstype *workstation_type, Gdspsize *dspsz, Gint *error_status)
{
  int wstype = *workstation_type;
  int errind, units, ras_x, ras_y;
  double px, py;

  gks_inq_max_ds_size(wstype, &errind, &units, &px, &py, &ras_x, &ras_y);

  dspsz->units = (Gdevunits)units;
  dspsz->device.x = px;
  dspsz->device.y = py;
  dspsz->raster.x = ras_x;
  dspsz->raster.y = ras_y;
  *error_status = errind;

  return OK;
}

int ginqtextextent(Gint workstation_id, Gpoint *position, Gchar *string, Gextent *extent, Gint *error_status)
{
  int wkid = workstation_id, errind;
  double qx, qy, cpx, cpy, tx[4], ty[4];
  char *chars;

  qx = position->x;
  qy = position->y;
  chars = string;
  gks_inq_text_extent(wkid, qx, qy, chars, &errind, &cpx, &cpy, tx, ty);

  extent->concat.x = cpx;
  extent->concat.y = cpy;
  extent->corner_1.x = tx[0];
  extent->corner_1.y = ty[0];
  extent->corner_2.x = tx[1];
  extent->corner_2.y = ty[1];
  extent->corner_3.x = tx[2];
  extent->corner_3.y = ty[2];
  extent->corner_4.x = tx[3];
  extent->corner_4.y = ty[3];

  return OK;
}

int ginqnameopenseg(Gint *segment_name, Gint *error_status)
{
  int errind, segn;

  gks_inq_open_segn(&errind, &segn);

  *segment_name = segn;
  *error_status = errind;

  return OK;
}

int gemergencyclosegks(void)
{
  gks_emergency_close();

  return OK;
}

double gks_precision(void)
{
  return FEPS;
}

/*!
 * Set the resample method for up and downscaling. The default method is nearest neighbour.
 *
 * \param[in] flag Resample method, valid options are GKS_K_RESAMPLE_DEFAULT, GKS_K_RESAMPLE_LINEAR,
 * GKS_K_RESAMPLE_NEAREST and GKS_K_RESAMPLE_LANCZOS, or combinations of the GKS_K_UPSAMPLE_* and GKS_K_DOWNSAMPLE_*
 * flags
 */
void gks_set_resample_method(unsigned int flag)
{
  if (state >= GKS_K_GKOP)
    {
      unsigned int vertical_upsampling_method = (flag >> 0u) & 0xffu;
      unsigned int horizontal_upsampling_method = (flag >> 8u) & 0xffu;
      unsigned int vertical_downsampling_method = (flag >> 16u) & 0xffu;
      unsigned int horizontal_downsampling_method = (flag >> 24u) & 0xffu;
      if ((vertical_upsampling_method <= 3) && (horizontal_upsampling_method <= 3) &&
          (vertical_downsampling_method <= 3) && (horizontal_downsampling_method <= 3))
        {
          s->resample_method = flag;
          i_arr[0] = (int)flag;

          /* call the device driver link routine */
          gks_ddlk(SET_RESAMPLE_METHOD, 1, 1, 1, i_arr, 0, f_arr_1, 0, f_arr_2, 0, c_arr, NULL);
        }
      else
        {
          gks_report_error(SET_RESAMPLE_METHOD, 501);
        }
    }
  else
    {
      gks_report_error(SET_RESAMPLE_METHOD, 8);
    }
}

/*!
 * Inquire the resample flag status.
 *
 * \returns Resample flag
 */
void gks_inq_resample_method(unsigned int *flag)
{
  *flag = s->resample_method;
}

void gks_set_border_width(double bwidth)
{
  if (state >= GKS_K_GKOP)
    {
      if (bwidth != s->bwidth)
        {
          s->bwidth = f_arr_1[0] = bwidth;

          /* call the device driver link routine */
          gks_ddlk(SET_BORDER_WIDTH, 0, 0, 0, i_arr, 1, f_arr_1, 0, f_arr_2, 0, c_arr, NULL);
        }
    }
  else
    /* GKS not in proper state. GKS must be in one of the states
       GKOP, WSOP, WSAC or SGOP */
    gks_report_error(SET_BORDER_WIDTH, 8);
}

void gks_inq_border_width(int *errind, double *bwidth)
{
  *errind = GKS_K_NO_ERROR;
  *bwidth = s->bwidth;
}

void gks_set_border_color_index(int coli)
{
  if (state >= GKS_K_GKOP)
    {
      if (coli >= 0)
        {
          if (coli != s->bcoli)
            {
              s->bcoli = i_arr[0] = coli;

              /* call the device driver link routine */
              gks_ddlk(SET_BORDER_COLOR_INDEX, 1, 1, 1, i_arr, 0, f_arr_1, 0, f_arr_2, 0, c_arr, NULL);
            }
        }
      else
        /* color index is invalid */
        gks_report_error(SET_BORDER_COLOR_INDEX, 65);
    }
  else
    /* GKS not in proper state. GKS must be in one of the states
       GKOP, WSOP, WSAC or SGOP */
    gks_report_error(SET_BORDER_COLOR_INDEX, 8);
}

void gks_inq_border_color_index(int *errind, int *coli)
{
  *errind = GKS_K_NO_ERROR;
  *coli = s->bcoli;
}

void gks_select_clip_xform(int tnr)
{
  if (state >= GKS_K_GKOP)
    {
      if (tnr >= 0 && tnr < MAX_TNR)
        {
          s->clip_tnr = i_arr[0] = tnr;

          /* call the device driver link routine */
          gks_ddlk(SELECT_CLIP_XFORM, 1, 1, 1, i_arr, 0, f_arr_1, 0, f_arr_2, 0, c_arr, NULL);
        }
      else
        /* transformation number is invalid */
        gks_report_error(SELECT_CLIP_XFORM, 50);
    }
  else
    /* GKS not in proper state. GKS must be in one of the states
       GKOP, WSOP, WSAC or SGOP */
    gks_report_error(SELECT_CLIP_XFORM, 8);
}

void gks_inq_clip_xform(int *errind, int *tnr)
{
  *errind = GKS_K_NO_ERROR;
  *tnr = s->clip_tnr;
}

void *gks_state(void)
{
  return (void *)s;
}
