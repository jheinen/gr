#ifdef __unix__
#define _POSIX_C_SOURCE 199506L
#endif

#include <stdio.h>

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

DLLEXPORT void gks_x11plugin(int fctid, int dx, int dy, int dimx, int *i_arr, int len_f_arr_1, double *f_arr_1,
                             int len_f_arr_2, double *f_arr_2, int len_c_arr, char *c_arr, void **ptr);

#ifdef __cplusplus
}
#endif


#if !defined(NO_X11)

#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include <errno.h>
#include <signal.h>

#if !defined(VMS) && !defined(_WIN32)
#include <unistd.h>
#endif

#ifdef XSHM
#include <sys/ipc.h>
#include <sys/shm.h>
#endif

#include <sys/stat.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>

#ifndef NO_XFT
#include <X11/Xft/Xft.h>
#endif

#include <X11/Intrinsic.h>

#include <pthread.h>

#ifdef XSHM
#include <X11/extensions/XShm.h>
#endif

#endif

#if !defined(NO_X11)

#include "icon.bm"

#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif
#ifndef max
#define max(a, b) (((a) > (b)) ? (a) : (b))
#endif
#define nint(a) ((int)(a + 0.5))

#define WindowName "GKS 5"

#define DrawBorder 0
#define Undefined 0xffff

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define MAX_PIXMAP 512
#define MAX_SIZE 1000

#define MAX_POINTS 2048
#define MAX_SELECTIONS 100
#define PATTERNS 120
#define HATCH_STYLE 108

#define WHITE 255
#define THRESH 127
#define BLACK 0
#define INITERR(X, Y) (X - (Y ? WHITE : BLACK) + (THRESH - X) / 2)

#define LEFT (1 << 0)
#define RIGHT (1 << 1)
#define BOTTOM (1 << 2)
#define TOP (1 << 3)

#define CTRL_C 3
#define CTRL_D 4
#define CTRL_Z 26

#define WC_to_NDC(xw, yw, tnr, xn, yn) \
  xn = a[tnr] * (xw) + b[tnr];         \
  yn = c[tnr] * (yw) + d[tnr]

#define WC_to_NDC_rel(xw, yw, tnr, xn, yn) \
  xn = a[tnr] * (xw);                      \
  yn = c[tnr] * (yw)

#define NDC_to_WC(xn, yn, tnr, xw, yw) \
  xw = ((xn)-b[tnr]) / a[tnr];         \
  yw = ((yn)-d[tnr]) / c[tnr]

#define NDC_to_DC(xn, yn, xd, yd)      \
  xd = sint(p->a * (xn) + p->b + 0.5); \
  yd = sint(p->c * (yn) + p->d + 0.5); \
  update_bbox(xd, yd)

#define DC_to_NDC(xd, yd, xn, yn) \
  xn = ((xd)-p->b) / p->a;        \
  yn = ((yd)-p->d) / p->c;

#define CharXform(xrel, yrel, x, y)                    \
  x = cos_f[p->path] * (xrel)-sin_f[p->path] * (yrel); \
  y = sin_f[p->path] * (xrel) + cos_f[p->path] * (yrel);

static int idle = False;

#if !defined(NO_XFT) || defined(NO_FT)

static char *fonts[] = {"-%s-times-medium-r-normal--*-%d0-%d-%d-*-*-iso8859-1",
                        "-%s-times-medium-i-normal--*-%d0-%d-%d-*-*-iso8859-1",
                        "-%s-times-bold-r-normal--*-%d0-%d-%d-*-*-iso8859-1",
                        "-%s-times-bold-i-normal--*-%d0-%d-%d-*-*-iso8859-1",
                        "-%s-helvetica-medium-r-normal--*-%d0-%d-%d-*-*-iso8859-1",
                        "-%s-helvetica-medium-o-normal--*-%d0-%d-%d-*-*-iso8859-1",
                        "-%s-helvetica-bold-r-normal--*-%d0-%d-%d-*-*-iso8859-1",
                        "-%s-helvetica-bold-o-normal--*-%d0-%d-%d-*-*-iso8859-1",
                        "-%s-courier-medium-r-normal--*-%d0-%d-%d-*-*-iso8859-1",
                        "-%s-courier-medium-o-normal--*-%d0-%d-%d-*-*-iso8859-1",
                        "-%s-courier-bold-r-normal--*-%d0-%d-%d-*-*-iso8859-1",
                        "-%s-courier-bold-o-normal--*-%d0-%d-%d-*-*-iso8859-1",
                        "-%s-symbol-medium-r-normal--*-%d0-%d-%d-*-*-*-*",
                        "-%s-bookman-light-r-normal--*-%d0-%d-%d-*-*-iso8859-1",
                        "-%s-bookman-light-i-normal--*-%d0-%d-%d-*-*-iso8859-1",
                        "-%s-bookman-demibold-r-normal--*-%d0-%d-%d-*-*-iso8859-1",
                        "-%s-bookman-demibold-i-normal--*-%d0-%d-%d-*-*-iso8859-1",
                        "-%s-new century schoolbook-medium-r-normal--*-%d0-%d-%d-*-*-iso8859-1",
                        "-%s-new century schoolbook-medium-i-normal--*-%d0-%d-%d-*-*-iso8859-1",
                        "-%s-new century schoolbook-bold-r-normal--*-%d0-%d-%d-*-*-iso8859-1",
                        "-%s-new century schoolbook-bold-i-normal--*-%d0-%d-%d-*-*-iso8859-1",
                        "-%s-avantgarde-book-r-normal--*-%d0-%d-%d-*-*-iso8859-1",
                        "-%s-avantgarde-book-o-normal--*-%d0-%d-%d-*-*-iso8859-1",
                        "-%s-avantgarde-demibold-r-normal--*-%d0-%d-%d-*-*-iso8859-1",
                        "-%s-avantgarde-demibold-o-normal--*-%d0-%d-%d-*-*-iso8859-1",
                        "-%s-palatino-medium-r-normal--*-%d0-%d-%d-*-*-iso8859-1",
                        "-%s-palatino-medium-i-normal--*-%d0-%d-%d-*-*-iso8859-1",
                        "-%s-palatino-bold-r-normal--*-%d0-%d-%d-*-*-iso8859-1",
                        "-%s-palatino-bold-i-normal--*-%d0-%d-%d-*-*-iso8859-1",
                        "-%s-zapf chancery-medium-i-normal--*-%d0-%d-%d-*-*-iso8859-1",
                        "-%s-zapf dingbats-medium-r-normal--*-%d0-%d-%d-*-*-*-*"};
static int n_font = 31;

static char *foundry[] = {"*", "adobe", "urw"};
static int n_foundries = 3;

static char *urw_fonts[] = {"-urw-nimbus roman no9 l-medium-r-normal--*-%d0-%d-%d-*-*-iso8859-1",
                            "-urw-nimbus roman no9 l-medium-i-normal--*-%d0-%d-%d-*-*-iso8859-1",
                            "-urw-nimbus roman no9 l-bold-r-normal--*-%d0-%d-%d-*-*-iso8859-1",
                            "-urw-nimbus roman no9 l-bold-i-normal--*-%d0-%d-%d-*-*-iso8859-1",
                            "-urw-nimbus sans l-medium-r-normal--*-%d0-%d-%d-*-*-iso8859-1",
                            "-urw-nimbus sans l-medium-i-normal--*-%d0-%d-%d-*-*-iso8859-1",
                            "-urw-nimbus sans l-bold-r-normal--*-%d0-%d-%d-*-*-iso8859-1",
                            "-urw-nimbus sans l-bold-i-normal--*-%d0-%d-%d-*-*-iso8859-1",
                            "-urw-nimbus mono l-medium-r-normal--*-%d0-%d-%d-*-*-iso8859-1",
                            "-urw-nimbus mono l-medium-o-normal--*-%d0-%d-%d-*-*-iso8859-1",
                            "-urw-nimbus mono l-bold-r-normal--*-%d0-%d-%d-*-*-iso8859-1",
                            "-urw-nimbus mono l-bold-o-normal--*-%d0-%d-%d-*-*-iso8859-1",
                            "-urw-standard symbols l-medium-r-normal--*-%d0-%d-%d-*-*-*-*",
                            "-urw-urw bookman l-medium-r-normal--*-%d0-%d-%d-*-*-iso8859-1",
                            "-urw-urw bookman l-medium-i-normal--*-%d0-%d-%d-*-*-iso8859-1",
                            "-urw-urw bookman l-bold-r-normal--*-%d0-%d-%d-*-*-iso8859-1",
                            "-urw-urw bookman l-bold-i-normal--*-%d0-%d-%d-*-*-iso8859-1",
                            "-urw-century schoolbook l-medium-r-normal--*-%d0-%d-%d-*-*-iso8859-1",
                            "-urw-century schoolbook l-medium-i-normal--*-%d0-%d-%d-*-*-iso8859-1",
                            "-urw-century schoolbook l-bold-r-normal--*-%d0-%d-%d-*-*-iso8859-1",
                            "-urw-century schoolbook l-bold-i-normal--*-%d0-%d-%d-*-*-iso8859-1",
                            "-urw-urw gothic l-medium-r-normal--*-%d0-%d-%d-*-*-iso8859-1",
                            "-urw-urw gothic l-medium-o-normal--*-%d0-%d-%d-*-*-iso8859-1",
                            "-urw-urw gothic l-semibold-r-normal--*-%d0-%d-%d-*-*-iso8859-1",
                            "-urw-urw gothic l-semibold-o-normal--*-%d0-%d-%d-*-*-iso8859-1",
                            "-urw-urw palladio l-medium-r-normal--*-%d0-%d-%d-*-*-iso8859-1",
                            "-urw-urw palladio l-medium-i-normal--*-%d0-%d-%d-*-*-iso8859-1",
                            "-urw-urw palladio l-bold-r-normal--*-%d0-%d-%d-*-*-iso8859-1",
                            "-urw-urw palladio l-bold-i-normal--*-%d0-%d-%d-*-*-iso8859-1",
                            "-urw-urw chancery l-medium-i-normal--*-%d0-%d-%d-*-*-iso8859-1",
                            "-urw-dingbats-medium-r-normal--*-%d0-%d-%d-*-*-*-*"};

#ifndef NO_XFT

static char *base_fonts[] = {
    "Times",          "Helvetica",    "Courier", "Symbol",       "Bookman Old Style", "Century Schoolbook",
    "Century Gothic", "Book Antiqua", "Zapfino", "Zapf Dingbats"};

#endif

static int map[32] = {22, 9,  5, 14, 18, 26, 13, 1, 24, 11, 7, 16, 20, 28, 13, 3,
                      23, 10, 6, 15, 19, 27, 13, 2, 25, 12, 8, 17, 21, 29, 13, 4};

static double capheights[31] = {0.662, 0.660, 0.681, 0.662, 0.729, 0.729, 0.729, 0.729, 0.583, 0.583, 0.583,
                                0.583, 0.667, 0.681, 0.681, 0.681, 0.681, 0.722, 0.722, 0.722, 0.722, 0.739,
                                0.739, 0.739, 0.739, 0.694, 0.693, 0.683, 0.683, 0.587, 0.692};

#endif

#ifndef NO_XFT

static int adobe2utf[256] = {
    0,     1,    2,    3,     4,     5,    6,     7,     8,     9,     10,    11,    12,    13,    14,    15,
    16,    17,   18,   19,    20,    21,   22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
    32,    33,   8704, 35,    8707,  37,   38,    8715,  40,    41,    8727,  43,    44,    8722,  46,    47,
    48,    49,   50,   51,    52,    53,   54,    55,    56,    57,    58,    59,    60,    61,    62,    63,
    8773,  913,  914,  935,   8710,  917,  934,   915,   919,   921,   977,   922,   923,   924,   925,   927,
    928,   920,  929,  931,   932,   933,  962,   8486,  926,   936,   918,   91,    8756,  93,    8869,  95,
    63717, 945,  946,  967,   948,   949,  966,   947,   951,   953,   981,   954,   955,   181,   957,   959,
    960,   952,  961,  963,   964,   965,  118,   969,   958,   968,   950,   123,   124,   125,   8764,  127,
    128,   129,  130,  131,   132,   133,  134,   135,   136,   137,   138,   139,   140,   141,   142,   143,
    144,   145,  146,  147,   148,   149,  150,   151,   152,   153,   154,   155,   156,   157,   158,   159,
    160,   161,  162,  8804,  8260,  8734, 402,   9827,  9830,  9829,  9824,  8596,  8592,  8593,  8594,  8595,
    176,   177,  178,  8805,  215,   8733, 8706,  8226,  247,   8800,  8801,  8776,  8230,  63718, 63719, 8629,
    8501,  8465, 8476, 8472,  8855,  8853, 8709,  8745,  8746,  8835,  8839,  8836,  8834,  8838,  8712,  8713,
    8736,  8711, 0,    63193, 63195, 8719, 8730,  8901,  172,   8743,  8744,  8660,  8656,  8657,  8658,  8659,
    9674,  9001, 0,    63721, 63722, 8721, 63723, 63724, 63725, 63726, 63727, 63728, 63729, 63730, 0,     63732,
    240,   9002, 8747, 8992,  63733, 8993, 63734, 63735, 63736, 63737, 63738, 63739, 63740, 63741, 63742, 100000};

#endif

static char patterns[PATTERNS][33];
static Bool have_patterns = False;

#if !defined(NO_XFT) || defined(NO_FT)

static double xfac[4] = {0, 0, -0.5, -1};
static double yfac[6] = {0, -1.2, -1, -0.5, 0, 0.2};

static double sin_f[] = {0, 1, 0, -1};
static double cos_f[] = {1, 0, -1, 0};

#endif

static int predef_font[] = {1, 1, 1, -2, -3, -4};
static int predef_prec[] = {0, 1, 2, 2, 2, 2};
static int predef_ints[] = {0, 1, 3, 3, 3};
static int predef_styli[] = {1, 1, 1, 2, 3};

static XPoint *points = NULL;
static int max_points = MAX_POINTS;

typedef enum
{
  TypeNone,
  TypeLocal,
  TypeCrosshair,
  TypeCross,
  TypeRubberband,
  TypeRectangle,
  TypeDigital,
  TypeCircle
} pe_type;

typedef unsigned char byte;

typedef struct
{
  int type;
  short x1, y1, x2, y2;
} Segment;

typedef struct ws_state_list_struct
{
  pthread_t thread;
  pthread_attr_t attr;
  pthread_mutex_t mutex;
  int run, done;
  int wkid;
  int gif, rf, uil;
  Bool packed_ca;
  Widget widget;
  int conid, wstype;
  Display *dpy;
  Bool new_dpy;
  int fd;
  Screen *screen;
  Bool backing_store;
  unsigned long fg, bg;
  Visual *vis;
  int depth;
  Colormap cmap;
  Window win;
  Bool new_win;
  Atom wmDeleteMessage;
  pthread_t master_thread;
  Pixmap pixmap, drawable, icon_pixmap;
  Bool double_buf;
  int shape;
  XImage *shmimage;
#ifdef XSHM
  XShmSegmentInfo shminfo;
#endif
  GC gc, def, invert, clear;
  long event_mask;
  Cursor cursor, textcursor;
  int swidth, sheight, dpi, x, y, width, height;
  int selection, bb_update, num_bb, max_bb;
  Segment *bb, *bbox, bounding_box;
  double mwidth, mheight, resolution, magnification, window[4], viewport[4];
  int state, mapped;
  Bool empty;
  int path;
#if !defined(NO_XFT) || defined(NO_FT)
#ifndef NO_XFT
  XftFont *fstr[31][MAX_SIZE + 1], *cfont;
#else
  XFontStruct *fstr[31][MAX_SIZE + 1], *cfont;
#endif
#endif
  int capheight, font;
  Pixmap tile[MAX_COLOR][PATTERNS];
  Pixmap stipple[MAX_COLOR][PATTERNS];
  Bool ored_patterns;
  XColor color[MAX_COLOR];
#ifndef NO_XFT
  XftColor rendercolor[MAX_COLOR];
  Bool havecolor[MAX_COLOR];
#endif
  unsigned long pixels[MAX_COLOR];
  int ccolor;
  double red[MAX_COLOR], green[MAX_COLOR], blue[MAX_COLOR];
  double gray[MAX_COLOR];
  int ltype;
  unsigned int lwidth;
  double a, b, c, d;
  pe_type type;
  int px, py;
  char *error;
  int scalable_fonts;
  Bool xshm;
  Pixmap *frame;
  int nframes;
} ws_state_list;

typedef struct
{
  int ch;
  char seq[3];
  char alt_seq[3];
} compose_keys;

static compose_keys key_bindings[] = {
    {34, "\" ", ""},   /* quotation mark */
    {35, "++", ""},    /* number sign */
    {39, "' ", ""},    /* apostrophe */
    {64, "AA", ""},    /* commercial at */
    {91, "((", ""},    /* opening bracket */
    {92, "//", "/<"},  /* backslash */
    {93, "))", ""},    /* closing bracket */
    {94, "^ ", ""},    /* circumflex accent */
    {96, "` ", ""},    /* grave accent */
    {123, "(-", ""},   /* opining brace */
    {124, "/^", ""},   /* vertical line */
    {125, ")-", ""},   /* closing brace */
    {126, "~ ", ""},   /* tilde */
    {160, "  ", ""},   /* no break space */
    {161, "!!", ""},   /* inverted ! */
    {162, "C/", "C|"}, /* cent sign */
    {163, "L-", "L="}, /* pound sign */
    {164, "XO", "X0"}, /* currency sign  */
    {165, "Y-", "Y="}, /* yen sign  */
    {166, "||", "!^"}, /* broken vertical bar */
    {167, "SO", "S!"}, /* section sign */
    {168, "\"\"", ""}, /* diaeresis */
    {169, "CO", "C0"}, /* copyright sign */
    {170, "A_", ""},   /* feminine ordinal */
    {171, "<<", ""},   /* open angle brackets */
    {172, "-,", ""},   /* logical not */
    {173, "-^", ""},   /* macron */
    {174, "RO", ""},   /* registered trademark */
    {175, "--", ""},   /* soft (syllable) hyphen */
    {176, "0^", ""},   /* degree sign */
    {177, "+-", ""},   /* plus or minus sign */
    {178, "2^", ""},   /* superscript 2 */
    {179, "3^", ""},   /* superscript 3 */
    {180, "''", ""},   /* acute accent */
    {181, "/U", ""},   /* micro sign */
    {182, "P!", ""},   /* paragraph sign */
    {183, ".^", ""},   /* middle dot  */
    {184, ", ", ""},   /* cedilla */
    {185, "1^", ""},   /* superscript 1 */
    {186, "O_", ""},   /* masculine ordinal */
    {187, ">>", ""},   /* closed angle brackets */
    {188, "14", ""},   /* fraction one-quarter */
    {189, "12", ""},   /* fraction one-half */
    {190, "34", ""},   /* three quarters */
    {191, "??", ""},   /* inverted ? */
    {192, "`A", ""},   /* A grave  */
    {193, "'A", ""},   /* A acute  */
    {194, "^A", ""},   /* A circumflex */
    {195, "~A", ""},   /* A tilde */
    {196, "\"A", ""},  /* A umlaut */
    {197, "A*", ""},   /* A ring */
    {198, "AE", ""},   /* A E diphthong */
    {199, "C,", ""},   /* C cedilla */
    {200, "`E", ""},   /* E grave */
    {201, "'E", ""},   /* E acute */
    {202, "^E", ""},   /* E circumflex */
    {203, "\"E", ""},  /* E umlaut */
    {204, "`I", ""},   /* I grave */
    {205, "'I", ""},   /* I acute */
    {206, "^I", ""},   /* I circumflex */
    {207, "\"I", ""},  /* I umlaut */
    {208, "-D", ""},   /* capital Icelandic Eth */
    {209, "~N", ""},   /* N tilde */
    {210, "`O", ""},   /* O grave */
    {211, "'O", ""},   /* O acute */
    {212, "^O", ""},   /* O circumflex */
    {213, "~O", ""},   /* O tilde */
    {214, "\"O", ""},  /* O umlaut */
    {215, "xx", ""},   /* multiplication sign */
    {216, "o/", ""},   /* O slash */
    {217, "`U", ""},   /* U grave */
    {218, "'U", ""},   /* U acute */
    {219, "^U", ""},   /* U circumflex */
    {220, "\"U", ""},  /* U umlaut */
    {221, "'Y", ""},   /* Y acute */
    {222, "TH", ""},   /* capital Icelandic thorn */
    {223, "ss", ""},   /* German small sharp s */
    {224, "`a", ""},   /* a grave */
    {225, "'a", ""},   /* a acute */
    {226, "^a", ""},   /* a circumflex */
    {227, "~a", ""},   /* a tilde */
    {228, "\"a", ""},  /* a umlaut */
    {229, "a*", ""},   /* a ring */
    {230, "ae", ""},   /* a e diphthong */
    {231, "c,", ""},   /* c cedilla  */
    {232, "`e", ""},   /* e grave */
    {233, "'e", ""},   /* e acute */
    {234, "^e", ""},   /* e circumflex */
    {235, "\"e", ""},  /* e umlaut */
    {236, "`i", ""},   /* i grave */
    {237, "'i", ""},   /* i acute */
    {238, "^i", ""},   /* i circumflex */
    {239, "\"i", ""},  /* i umlaut */
    {240, "-d", ""},   /* small Icelandic Eth */
    {241, "~n", ""},   /* n tilde  */
    {242, "`o", ""},   /* o grave */
    {243, "'o", ""},   /* o acute */
    {244, "^o", ""},   /* o circumflex */
    {245, "~o", ""},   /* o tilde */
    {246, "\"o", ""},  /* o umlaut */
    {247, "-:", ""},   /* division sign */
    {248, "o/", ""},   /* o slash  */
    {249, "`u", ""},   /* u grave */
    {250, "'u", ""},   /* u acute */
    {251, "^u", ""},   /* u circumflex */
    {252, "\"u", ""},  /* u umlaut */
    {253, "'y", ""},   /* y acute */
    {254, "th", ""},   /* small Icelandic thorn */
    {255, "\"y", ""}   /* y umlaut */
};
static int n_key = sizeof(key_bindings) / sizeof(key_bindings[0]);

static gks_state_list_t *gksl;
static double a[MAX_TNR], b[MAX_TNR], c[MAX_TNR], d[MAX_TNR];

static ws_state_list *p;
static int error_code, request_code, function_id;


static int *handler(Display *dpy, XErrorEvent *event)
{
  char str[80], request[40];

  if (event->error_code != error_code || event->request_code != request_code)
    {
      XGetErrorText(dpy, event->error_code, str, sizeof(str));
      fprintf(stderr, "X Protocol error detected by server: %s\n", str);

      sprintf(request, "XRequest.%d", event->request_code);
      XGetErrorDatabaseText(dpy, "", request, "unknown", str, sizeof(str));
      fprintf(stderr, "Failed request major op code %d (%s)\n", event->request_code, str);

      fprintf(stderr, "Invoked from within GKS function id %d\n", function_id);

      error_code = event->error_code;
      request_code = event->request_code;
    }

  return (NULL);
}


static int sint(double a)
{
  if (a > 65535)
    return 65535;
  else if (a < -65535)
    return -65535;
  else
    return (int)(a + 0.5);
}


static void seg_xform(double *x, double *y)
{
  double xx;

  xx = *x * gksl->mat[0][0] + *y * gksl->mat[0][1] + gksl->mat[2][0];
  *y = *x * gksl->mat[1][0] + *y * gksl->mat[1][1] + gksl->mat[2][1];
  *x = xx;
}


static void seg_xform_rel(double *x, double *y)
{
  double xx;

  xx = *x * gksl->mat[0][0] + *y * gksl->mat[0][1];
  *y = *x * gksl->mat[1][0] + *y * gksl->mat[1][1];
  *x = xx;
}


static void update_bbox(int x, int y)
{
  if (p->bb_update)
    {
      if (x < p->bb->x1) p->bb->x1 = x;
      if (x > p->bb->x2) p->bb->x2 = x;

      if (y < p->bb->y1) p->bb->y1 = y;
      if (y > p->bb->y2) p->bb->y2 = y;
    }
}


#define drawRect(x, y, w, h)                                                             \
  {                                                                                      \
    if (type == 0 && p->pixmap) XDrawRectangle(p->dpy, p->pixmap, p->def, x, y, w, h);   \
    if (!p->double_buf || type != 0) XDrawRectangle(p->dpy, p->win, p->def, x, y, w, h); \
  }

static void draw_bbox(int type, int xoff, int yoff)
{
  int xmin = p->bb->x1, xmax = p->bb->x2, ymin = p->bb->y2, ymax = p->bb->y1;
  /*
             4
       8 o---o----o 6
         |        |
       1 o        o 2
         |        |
       5 o---o----o 7
             3
   */
  if (type == 1 || type == 5 || type == 8) xmin += xoff;
  if (type == 2 || type == 6 || type == 7) xmax += xoff;
  if (type == 3 || type == 5 || type == 7) ymin += yoff;
  if (type == 4 || type == 6 || type == 8) ymax += yoff;

  drawRect(xmin - 1, ymax - 1, xmax - xmin + 2, ymin - ymax + 2) if (p->bb->type & (1 << 0))
      drawRect(xmin - 4, (ymin + ymax - 6) / 2, 6, 6) if (p->bb->type & (1 << 1))
          drawRect(xmax - 2, (ymin + ymax - 6) / 2, 6, 6) if (p->bb->type & (1 << 2))
              drawRect((xmin + xmax - 6) / 2, ymin - 2, 6, 6) if (p->bb->type & (1 << 3))
                  drawRect((xmin + xmax - 6) / 2, ymax - 4, 6, 6) if (p->bb->type & (1 << 4))
                      drawRect(xmin - 4, ymin - 2, 6, 6) if (p->bb->type & (1 << 5))
                          drawRect(xmax - 2, ymax - 4, 6, 6) if (p->bb->type & (1 << 6))
                              drawRect(xmax - 2, ymin - 2, 6, 6) if (p->bb->type & (1 << 7))
                                  drawRect(xmin - 4, ymax - 4, 6, 6)
}


static void set_clipping(Bool state)
{
  double clrt[4];
  int i, j;
  XRectangle rt;

  if (state && (gksl->clip_tnr != 0 || gksl->clip == GKS_K_CLIP))
    {
      memmove(clrt, gksl->viewport[gksl->clip_tnr != 0 ? gksl->clip_tnr : gksl->cntnr], 4 * sizeof(double));
      seg_xform(&clrt[0], &clrt[2]);
      seg_xform(&clrt[1], &clrt[3]);
      i = clrt[0] < clrt[1] ? 0 : 1;
      j = clrt[2] < clrt[3] ? 2 : 3;

      rt.x = (int)(p->a * clrt[i] + p->b);
      rt.y = (int)(p->c * clrt[5 - j] + p->d);
      rt.width = (int)(p->a * (clrt[1 - i] - clrt[i])) + 2;
      rt.height = (int)(p->c * (clrt[j] - clrt[5 - j])) + 2;

      XSetClipRectangles(p->dpy, p->gc, 0, 0, &rt, 1, Unsorted);
    }
  else
    XSetClipMask(p->dpy, p->gc, None);

  rt.x = 0;
  rt.y = 0;
  rt.width = p->width;
  rt.height = p->height;

  XSetClipRectangles(p->dpy, p->invert, 0, 0, &rt, 1, Unsorted);
}


static void expose_event(Widget widget, ws_state_list *p, XExposeEvent *event, Boolean *continue_to_dispatch)

/*
 *  Handle expose events
 */

{
  if (p->pixmap)
    {
      set_clipping(False);
      XCopyArea(p->dpy, p->pixmap, p->win, p->gc, event->x, event->y, event->width, event->height, event->x, event->y);
      set_clipping(True);
    }
}


static Display *open_display(void)

/*
 *  Open display
 */

{
  char *env, *ep, s[80];

  env = (char *)gks_getenv("GKS_CONID");
  if (env)
    if (!*env) env = NULL;
  if (!env) env = (char *)gks_getenv("GKSconid");

  if (p->wstype == 213)
    {
      if (env == NULL)
        {
          gks_perror("can't obtain widget id");
          return (NULL);
        }
      else
        sscanf(env, "%p", (void **)&p->widget);
    }

  if (p->widget == NULL)
    {
      if (p->wstype == 212)
        {
          if (env == NULL)
            {
              gks_perror("can't obtain pre-existing drawable");
              return (NULL);
            }
          else
            {
              if (sscanf(env, "%p!%ld", (void **)&p->dpy, (long int *)&p->win) != 2)
                {
                  ep = strchr(env, '!');
                  if (ep != NULL)
                    {
                      if (strncmp(++ep, "0x", 2) == 0)
                        sscanf(ep + 2, "%x", (unsigned int *)&p->win);
                      else
                        sscanf(ep, "%d", (int *)&p->win);
                    }
#ifdef _WIN32
                  if (*env == ':')
                    sprintf(s, "localhost%s", env);
                  else
                    strcpy(s, env);
#else
                  strcpy(s, env);
#endif
                  strtok(s, "!");
                  p->dpy = XOpenDisplay(s);
                  p->new_dpy = True;
                }
            }
        }
      else
        {
          if (!env) env = (char *)gks_getenv("DISPLAY");
          if (env != NULL)
            {
#ifdef _WIN32
              if (*env == ':')
                sprintf(s, "localhost%s", env);
              else
                strcpy(s, env);
#else
              strcpy(s, env);
#endif
              env = s;
            }

          p->dpy = XOpenDisplay(env);
          p->new_dpy = True;
        }

      if (p->new_dpy)
        {
          if (p->dpy == NULL)
            {
              gks_perror("can't open display on \"%s\"\n\
     Is your DISPLAY environment variable set correctly?\n\
     Did you enable X11 and TCP forwarding?\n",
                         env != NULL ? env : "");
              return (NULL);
            }
        }

      p->screen = XDefaultScreenOfDisplay(p->dpy);
    }
  else
    {
      p->dpy = XtDisplay(p->widget);
      p->new_dpy = False;
      p->screen = XtScreenOfObject(p->widget);
    }

  p->fd = ConnectionNumber(p->dpy);

  error_code = request_code = 0;
  if (p->wstype != 212) XSetErrorHandler((XErrorHandler)handler);

  p->backing_store = (XDoesBackingStore(p->screen) == Always) || ((char *)gks_getenv("GKS_BS") != NULL);

  p->mwidth = XWidthMMOfScreen(p->screen) * 0.001;
  p->mheight = XHeightMMOfScreen(p->screen) * 0.001;
  p->swidth = XWidthOfScreen(p->screen);
  p->sheight = XHeightOfScreen(p->screen);
  p->resolution = 0.5 * (p->mwidth / p->swidth + p->mheight / p->sheight);

  p->magnification = 1;

  if ((env = (char *)gks_getenv("GKS_DPI")) != NULL)
    p->dpi = atoi(env);
  else
    {
#if 0
      double dpi = 0.0254 / p->resolution;

      if (fabs(dpi - 75) < fabs(100 - dpi))
        p->dpi = 75;
      else
        p->dpi = 100;
#else
      p->dpi = 75;
#endif
    }

  p->ored_patterns = (char *)gks_getenv("GKS_TRANSPARENT_PATTERNS") != NULL;

  return (p->dpy);
}


static void set_colors(void)
{
  int i;

  for (i = 0; i < MAX_COLOR; i++)
    {
      gks_inq_rgb(i, &p->red[i], &p->green[i], &p->blue[i]);
      p->gray[i] = 0.3 * p->red[i] + 0.59 * p->green[i] + 0.11 * p->blue[i];
    }
}


/*******************************************************/
/* 24/32-bit TrueColor display color 'allocation' code */
/*******************************************************/

static int highbit(unsigned long ul)
{
  /* returns position of highest set bit in 'ul' as an integer (0-31),
     or -1 if none */

  int i;
  unsigned long hb;
  hb = 0x8000;
  hb = (hb << 16); /* hb = 0x80000000UL */
  for (i = 31; ((ul & hb) == 0) && i >= 0; i--, ul <<= 1)
    ;
  return i;
}


static int lowbit(unsigned long ul)
{
  /* returns position of lowest set bit in 'ul' as an integer (0-31),
     or -1 if none */

  int i;
  for (i = 0; ((ul & 1) == 0) && i <= 31; i++) ul >>= 1;
  return i;
}


static void alloc_color(XColor *color)
{
  unsigned long r, g, b, rmask, gmask, bmask;
  int rshift, gshift, bshift;

  /* shift r,g,b so that high bit of 16-bit color specification is
   * aligned with high bit of r,g,b-mask in visual,
   * AND each component with its mask,
   * and OR the three components together
   */

  r = color->red;
  g = color->green;
  b = color->blue;

  rmask = p->vis->red_mask;
  gmask = p->vis->green_mask;
  bmask = p->vis->blue_mask;

  rshift = 15 - highbit(rmask);
  gshift = 15 - highbit(gmask);
  bshift = 15 - highbit(bmask);

  /* shift the bits around */
  if (rshift < 0)
    r = r << (-rshift);
  else
    r = r >> rshift;

  if (gshift < 0)
    g = g << (-gshift);
  else
    g = g >> gshift;

  if (bshift < 0)
    b = b << (-bshift);
  else
    b = b >> bshift;

  r = r & rmask;
  g = g & gmask;
  b = b & bmask;

  color->pixel = r | g | b;

  /* put 'exact' colors into red,green,blue fields */
  /* shift the bits BACK to where they were, now that they've been masked */
  if (rshift < 0)
    r = r >> (-rshift);
  else
    r = r << rshift;

  if (gshift < 0)
    g = g >> (-gshift);
  else
    g = g << gshift;

  if (bshift < 0)
    b = b >> (-bshift);
  else
    b = b << bshift;

  color->red = r;
  color->green = g;
  color->blue = b;
}


static void allocate_colors(void)
{
  int i;

  p->vis = XDefaultVisualOfScreen(p->screen);
  p->depth = XDefaultDepthOfScreen(p->screen);
  p->cmap = XDefaultColormapOfScreen(p->screen);

  for (i = 0; i < MAX_COLOR; i++)
    {
      p->color[i].red = (unsigned short)(p->red[i] * 65535);
      p->color[i].green = (unsigned short)(p->green[i] * 65535);
      p->color[i].blue = (unsigned short)(p->blue[i] * 65535);

#if defined(__cplusplus) || defined(c_plusplus)
      if (p->vis->c_class == TrueColor)
#else
      if (p->vis->class == TrueColor)
#endif
        {
          alloc_color(&p->color[i]);
        }
      else if (!XAllocColor(p->dpy, p->cmap, &p->color[i]))
        {
          p->color[i].pixel = 0xffff;
        }
    }

  p->ccolor = Undefined;

  p->bg = p->color[0].pixel;
  p->fg = p->color[1].pixel;
}


#ifndef NO_XFT

static void allocate_rendercolors(void)
{
  XRenderColor rendercolor;
  int i;

  for (i = 0; i < MAX_COLOR; i++)
    {
      rendercolor.red = p->color[i].red;
      rendercolor.green = p->color[i].green;
      rendercolor.blue = p->color[i].blue;
      rendercolor.alpha = 65535;

      p->havecolor[i] = XftColorAllocValue(p->dpy, p->vis, p->cmap, &rendercolor, &p->rendercolor[i]);
    }
}

#endif


#ifndef NO_XFT

static void free_rendercolors(void)
{
  int i;

  for (i = 0; i < MAX_COLOR; i++)
    if (p->havecolor[i]) XftColorFree(p->dpy, p->vis, p->cmap, &p->rendercolor[i]);
}

#endif


static void setup_xform(double *window, double *viewport)
{
  p->a = (p->width - 1) / (window[1] - window[0]);
  p->b = -window[0] * p->a;
  p->c = (p->height - 1) / (window[2] - window[3]);
  p->d = p->height - 1 - window[2] * p->c;
}


static void configure_viewport(XConfigureEvent *event)
{
  if (p->width != event->width || p->height != event->height)
    {
      p->width = event->width;
      p->height = event->height;

      if (p->pixmap)
        {
          XFreePixmap(p->dpy, p->pixmap);
          p->pixmap = XCreatePixmap(p->dpy, XRootWindowOfScreen(p->screen), p->width, p->height, p->depth);
          XFillRectangle(p->dpy, p->pixmap, p->clear, 0, 0, p->width, p->height);
        }
      if (p->drawable)
        {
          XFreePixmap(p->dpy, p->drawable);
          p->drawable = XCreatePixmap(p->dpy, XRootWindowOfScreen(p->screen), p->width, p->height, p->depth);
          XFillRectangle(p->dpy, p->drawable, p->clear, 0, 0, p->width, p->height);
        }
#ifdef XSHM
      free_shared_memory();
      create_shared_memory();
#endif
      p->viewport[0] = 0;
      p->viewport[1] = p->width * p->resolution;
      p->viewport[2] = 0;
      p->viewport[3] = p->height * p->resolution;

      p->window[0] = p->window[2] = 0;
      if (p->viewport[1] > p->viewport[3])
        {
          p->window[1] = 1;
          p->window[3] = p->viewport[3] / p->viewport[1];
        }
      else if (p->viewport[1] < p->viewport[3])
        {
          p->window[1] = p->viewport[1] / p->viewport[3];
          p->window[3] = 1;
        }
      else
        p->window[1] = p->window[3] = 1;

      setup_xform(p->window, p->viewport);
      set_clipping(True);
    }
}


static void create_window(int win)

/*
 *  Create a window
 */

{
  XSetWindowAttributes xswa;
  XWindowAttributes xwa;
  char icon_name[40];
  char *env, **argv = NULL;
  int argc = 0;
  XSizeHints *hints = NULL;
  unsigned long valuemask;

  /* Set up the window attributes. We want to set the event mask,
   * and the background pixel */

  xswa.background_pixel = p->bg;
  xswa.event_mask = StructureNotifyMask | ExposureMask;

  if (p->backing_store && ((char *)gks_getenv("GKS_BS") == NULL))
    xswa.backing_store = Always;
  else
    xswa.backing_store = NotUseful;

  xswa.colormap = p->cmap;
  xswa.border_pixel = p->bg;

  if (p->widget == NULL && p->wstype != 212)
    {
      p->new_win = True;

      p->x = 5 + win * 25;
      p->y = 100 + win * 25;

      if (p->uil < 0)
        {
          if ((env = (char *)gks_getenv("GKS_MAGSTEP")) != NULL) p->magnification = pow(1.2, atof(env));

          p->width = p->height = (int)(500 * p->magnification);
        }
      else
        p->width = p->height = 16;

      /* Create a window whose parent is the root window */

      p->win = XCreateWindow(p->dpy, XRootWindowOfScreen(p->screen), p->x, p->y, p->width, p->height, 0, p->depth,
                             InputOutput, p->vis,
                             CWBackPixel | CWEventMask | CWBackingStore | CWColormap | CWBorderPixel, &xswa);

      XSelectInput(p->dpy, p->win, xswa.event_mask);

      p->icon_pixmap =
          XCreatePixmapFromBitmapData(p->dpy, XRootWindowOfScreen(p->screen), (char *)icon_bits, icon_width,
                                      icon_height, XBlackPixelOfScreen(p->screen), XWhitePixelOfScreen(p->screen), 1);

      if (p->conid)
        sprintf(icon_name, "GKSwk %d", p->conid);
      else
        strcpy(icon_name, "GKSterm");

      if (gks_getenv("GKS_IGNORE_WM_DELETE_WINDOW") == NULL)
        {
          p->master_thread = pthread_self();
          p->wmDeleteMessage = XInternAtom(p->dpy, "WM_DELETE_WINDOW", False);
          XSetWMProtocols(p->dpy, p->win, &p->wmDeleteMessage, 1);
        }
      else
        p->master_thread = 0;

      XSetStandardProperties(p->dpy, p->win, WindowName, icon_name, p->icon_pixmap, argv, argc, hints);

      XStoreName(p->dpy, p->win, WindowName);
    }
  else
    {
      p->new_win = False;

      if (p->wstype != 212) p->win = XtWindow(p->widget);

      XGetWindowAttributes(p->dpy, p->win, &xwa);
      p->x = xwa.x;
      p->y = xwa.y;
      p->width = xwa.width;
      p->height = xwa.height;

      xswa.event_mask |= (xwa.all_event_masks | ButtonPressMask);

      valuemask = CWBackingStore;
      if (p->wstype != 212) valuemask = CWBackPixel | CWEventMask | CWBackingStore | CWColormap;

      XChangeWindowAttributes(p->dpy, p->win, valuemask, &xswa);
    }

  p->event_mask = xswa.event_mask;
}


static void set_WM_hints(void)
{
  XSizeHints hints;
  XWMHints wmhints;

  if (p->new_win)
    {
      hints.flags = PPosition | PSize;
      hints.x = p->x;
      hints.y = p->y;
      hints.width = p->width;
      hints.height = p->height;

      XSetNormalHints(p->dpy, p->win, &hints);

      if (p->gif >= 0 || p->rf >= 0)
        {
          wmhints.initial_state = IconicState;
          wmhints.flags = StateHint;

          XSetWMHints(p->dpy, p->win, &wmhints);
        }
    }
}


static void create_GC(void)

/*
 *  Create graphics context
 */

{
  XGCValues xgcv;

  xgcv.foreground = p->fg;
  xgcv.background = p->bg;
  p->gc = XCreateGC(p->dpy, p->win, GCForeground | GCBackground, &xgcv);

  p->def = XCreateGC(p->dpy, p->win, GCForeground | GCBackground, &xgcv);

  p->invert = XCreateGC(p->dpy, p->win, GCForeground | GCBackground, &xgcv);
  XSetFunction(p->dpy, p->invert, GXinvert);
  XSetForeground(p->dpy, p->invert, p->fg ^ p->bg);

  xgcv.foreground = p->bg;
  p->clear = XCreateGC(p->dpy, p->win, GCForeground | GCBackground, &xgcv);
}


static void free_GC(void)

/*
 *  Free graphics context
 */

{
  XFreeGC(p->dpy, p->clear);
  XFreeGC(p->dpy, p->invert);
  XFreeGC(p->dpy, p->def);
  XFreeGC(p->dpy, p->gc);
}


static void create_pixmap(void)

/*
 *  Create a pixmap
 */

{
  if (!p->backing_store || p->gif >= 0 || p->rf >= 0 || p->uil >= 0 || p->frame || p->double_buf)
    {
      p->pixmap = XCreatePixmap(p->dpy, XRootWindowOfScreen(p->screen), p->width, p->height, p->depth);
      XFillRectangle(p->dpy, p->pixmap, p->clear, 0, 0, p->width, p->height);
    }
  else
    p->pixmap = 0;
}


#ifdef XSHM

int XShmQueryExtension(Display *);

static void create_shared_memory(void)

/*
 *  Create X shared memory
 */

{
  if (!p->xshm)
    {
      p->shmimage = NULL;
      return;
    }

  if (XShmQueryExtension(p->dpy))
    {
      p->shmimage = XShmCreateImage(p->dpy, p->vis, p->depth, p->depth == 1 ? XYBitmap : ZPixmap, 0, &p->shminfo,
                                    p->width, p->height);

      p->shminfo.shmid = shmget(IPC_PRIVATE, p->shmimage->bytes_per_line * p->shmimage->height, IPC_CREAT | 0777);
      if (p->shminfo.shmid >= 0)
        {
          p->shminfo.shmaddr = (char *)shmat(p->shminfo.shmid, 0, 0);
        }

      p->shminfo.readOnly = False;
      XShmAttach(p->dpy, &p->shminfo);
      XSync(p->dpy, False);

      shmctl(p->shminfo.shmid, IPC_RMID, 0);
      p->shmimage->data = p->shminfo.shmaddr;
    }
  else
    p->shmimage = NULL;
}


static void free_shared_memory(void)

/*
 *  Free X shared memory
 */

{
  if (p->shmimage != NULL)
    {
      XShmDetach(p->dpy, &p->shminfo);
      XDestroyImage(p->shmimage);
      shmdt(p->shminfo.shmaddr);
    }
}

#endif /* XSHM */


static void initialize_arrays(void)

/*
 *  Initialize the arrays
 */

{
  int i, j;
  int pat, pa[33];

  if (!have_patterns)
    {
      for (i = 0; i < PATTERNS; i++)
        {
          pat = i;
          gks_inq_pattern_array(pat, pa);
          patterns[i][0] = (char)(*pa);
          for (j = 1; j <= *pa; j++) patterns[i][j] = (char)(~pa[j]);
        }
      have_patterns = True;
    }

#if !defined(NO_XFT) || defined(NO_FT)
#ifndef NO_XFT
  memset((void *)p->fstr, 0, n_font * (MAX_SIZE + 1) * sizeof(XftFont *));
#else
  memset((void *)p->fstr, 0, n_font * (MAX_SIZE + 1) * sizeof(XFontStruct *));
#endif
#endif
  memset((void *)p->tile, 0, MAX_COLOR * PATTERNS * sizeof(Pixmap));
  memset((void *)p->stipple, 0, MAX_COLOR * PATTERNS * sizeof(Pixmap));
}


static void free_tile_patterns(int color)

/*
 *  Free tile patterns
 */

{
  int style;

  for (style = 0; style < PATTERNS; style++)
    {
      if (p->tile[color][style] != 0)
        {
          XFreePixmap(p->dpy, p->tile[color][style]);
          XFreePixmap(p->dpy, p->stipple[color][style]);
          p->tile[color][style] = p->stipple[color][style] = 0;
        }
    }
}


static void create_cursor(void)

/*
 *  Create cursor
 */

{
  char *env;
  unsigned int shape = 0;

  if ((env = (char *)gks_getenv("GKS_XC")) != NULL) shape = atoi(env);
  if (!shape) shape = XC_draft_small;

  p->cursor = XCreateFontCursor(p->dpy, shape);
  p->textcursor = XCreateFontCursor(p->dpy, XC_xterm);
}


static void set_color_repr(int i, double r, double g, double b)
{
  p->red[i] = r;
  p->green[i] = g;
  p->blue[i] = b;
  p->gray[i] = 0.3 * r + 0.59 * g + 0.11 * b;

  if (i < MAX_COLOR)
    {
      p->color[i].red = (unsigned short)(r * 65535);
      p->color[i].green = (unsigned short)(g * 65535);
      p->color[i].blue = (unsigned short)(b * 65535);

      if (!XAllocColor(p->dpy, p->cmap, &p->color[i]))
        {
          p->color[i].pixel = 0xffff;
        }
    }

  if (i < 2)
    {
      p->bg = p->color[0].pixel;
      p->fg = p->color[1].pixel;
      XSetForeground(p->dpy, p->invert, p->fg ^ p->bg);
    }

  p->ccolor = Undefined;
}


static void set_color(int color)
{
  int i;

  i = color;
  if (i != p->ccolor)
    {
      XSetForeground(p->dpy, p->gc, p->color[i].pixel);
      p->ccolor = i;
    }
}


static void set_pattern(int color, int style)
{
  unsigned int w, h;
  char *pattern;

  if (color < MAX_COLOR && style > 0 && style < PATTERNS)
    {
      if (p->tile[color][style] == 0)
        {
          pattern = patterns[style];
          w = h = (*pattern == 32) ? 16 : *pattern;
          pattern++;
          p->tile[color][style] =
              XCreatePixmapFromBitmapData(p->dpy, p->win, pattern, w, h, p->color[color].pixel, p->bg, p->depth);
          p->stipple[color][style] =
              XCreatePixmapFromBitmapData(p->dpy, p->win, pattern, w, h, p->color[color].pixel, p->bg, 1);
        }

      if (p->ored_patterns)
        {
          XSetFillStyle(p->dpy, p->gc, FillStippled);
          XSetStipple(p->dpy, p->gc, p->stipple[color][style]);
        }
      else
        {
          XSetFillStyle(p->dpy, p->gc, FillTiled);
          XSetTile(p->dpy, p->gc, p->tile[color][style]);
        }
    }
  else
    XSetFillStyle(p->dpy, p->gc, FillSolid);
}


static void configure_event(XConfigureEvent *event)

/*
 *  Handle configure events
 */

{
  double req_aspect_ratio, cur_aspect_ratio;
  int width, height;

  if (p->widget || p->gif >= 0 || p->rf >= 0 || p->uil >= 0 || p->frame) return;

  p->x = event->x;
  p->y = event->y;
  if (event->width == p->width && event->height == p->height) return;

  width = event->width;
  height = event->height;

  p->viewport[0] = p->x * p->resolution;
  p->viewport[1] = p->viewport[0] + width * p->resolution;
  p->viewport[2] = (p->sheight - (p->y + height)) * p->resolution;
  p->viewport[3] = p->viewport[2] + height * p->resolution;

  req_aspect_ratio = (p->window[1] - p->window[0]) / (p->window[3] - p->window[2]);
  cur_aspect_ratio = (p->viewport[1] - p->viewport[0]) / (p->viewport[3] - p->viewport[2]);

  if (cur_aspect_ratio > req_aspect_ratio)
    {
      width = (int)(height * req_aspect_ratio);
      p->viewport[1] = p->viewport[0] + (p->viewport[3] - p->viewport[2]) * req_aspect_ratio;
    }
  else
    {
      height = (int)(width / req_aspect_ratio);
      p->viewport[3] = p->viewport[2] + (p->viewport[1] - p->viewport[0]) / req_aspect_ratio;
    }

  if (width != p->width || height != p->height)
    {
      p->width = width;
      p->height = height;

      if (p->pixmap)
        {
          XFreePixmap(p->dpy, p->pixmap);
          p->pixmap = XCreatePixmap(p->dpy, XRootWindowOfScreen(p->screen), p->width, p->height, p->depth);
          XFillRectangle(p->dpy, p->pixmap, p->clear, 0, 0, p->width, p->height);
        }
      if (p->drawable)
        {
          XFreePixmap(p->dpy, p->drawable);
          p->drawable = XCreatePixmap(p->dpy, XRootWindowOfScreen(p->screen), p->width, p->height, p->depth);
          XFillRectangle(p->dpy, p->drawable, p->clear, 0, 0, p->width, p->height);
        }
#ifdef XSHM
      free_shared_memory();
      create_shared_memory();
#endif
      setup_xform(p->window, p->viewport);
      set_clipping(True);
      return;
    }
  else
    return;
}


static void handle_expose_event(ws_state_list *p)

/*
 *  Handle expose events
 */

{
  if (p->pixmap)
    {
      set_clipping(False);
      XCopyArea(p->dpy, p->pixmap, p->win, p->gc, 0, 0, p->width, p->height, 0, 0);
      set_clipping(True);
      XSync(p->dpy, False);
    }
}


static void wait_for_expose(void)
{
  XEvent event;

  if (p->new_win)
    {
      do
        XWindowEvent(p->dpy, p->win, StructureNotifyMask, &event);
      while (event.xany.type != MapNotify && event.xany.type != ConfigureNotify);
      while (XCheckTypedWindowEvent(p->dpy, p->win, Expose, &event))
        ;
    }
}


static void map_window(void)

/*
 *  Map window
 */

{
  /* Windows are not visible until they are mapped - map this window */

  if (!p->mapped)
    {
      XMapWindow(p->dpy, p->win);
      p->mapped = True;

      if (p->gif < 0 && p->rf < 0) wait_for_expose();

      if (p->widget && !p->backing_store)
        XtAddEventHandler(p->widget, ExposureMask, False, (XtEventHandler)expose_event, p);
    }
}


static void unmap_window(void)

/*
 *  Unmap window
 */

{
  if (p->mapped)
    {
      if (p->widget && !p->backing_store)
        XtRemoveEventHandler(p->widget, ExposureMask, False, (XtEventHandler)expose_event, p);

      if (!p->widget && p->wstype != 212) XUnmapWindow(p->dpy, p->win);
      p->mapped = False;
    }
}


static void setup_norm_xform(int tnr, double *wn, double *vp)
{
  a[tnr] = (vp[1] - vp[0]) / (wn[1] - wn[0]);
  b[tnr] = vp[0] - wn[0] * a[tnr];
  c[tnr] = (vp[3] - vp[2]) / (wn[3] - wn[2]);
  d[tnr] = vp[2] - wn[2] * c[tnr];
}


static void init_norm_xform(void)
{
  int tnr;

  for (tnr = 0; tnr < MAX_TNR; tnr++) setup_norm_xform(tnr, gksl->window[tnr], gksl->viewport[tnr]);
}


static void draw_marker(double xn, double yn, int mtype, double mscale)
{
  int r, d, x, y, i;
  int pc, op;
  XPoint points[16];
  double scale, xr, yr;

#include "marker.h"

  r = (int)(3 * mscale);
  d = 2 * r;
  scale = 0.01 * mscale / 3.0;

  xr = r;
  yr = 0;
  seg_xform_rel(&xr, &yr);
  r = nint(sqrt(xr * xr + yr * yr));

  NDC_to_DC(xn, yn, x, y);

  update_bbox(x - r, y - r);
  update_bbox(x + r, y + r);

  pc = 0;
  mtype = (d > 1) ? mtype + marker_off : marker_off + 1;

  do
    {
      op = marker[mtype][pc];
      switch (op)
        {

        case 1: /* point */
          if (p->pixmap) XDrawPoint(p->dpy, p->pixmap, p->gc, x, y);
          if (p->selection) XDrawPoint(p->dpy, p->drawable, p->gc, x, y);
          if (!p->double_buf) XDrawPoint(p->dpy, p->win, p->gc, x, y);
          break;

        case 2: /* line */
          for (i = 0; i < 2; i++)
            {
              xr = scale * marker[mtype][pc + 2 * i + 1];
              yr = -scale * marker[mtype][pc + 2 * i + 2];
              seg_xform_rel(&xr, &yr);
              points[i].x = nint(x - xr);
              points[i].y = nint(y + yr);
            }
          if (p->pixmap) XDrawLines(p->dpy, p->pixmap, p->gc, points, 2, CoordModeOrigin);
          if (p->selection) XDrawLines(p->dpy, p->drawable, p->gc, points, 2, CoordModeOrigin);
          if (!p->double_buf) XDrawLines(p->dpy, p->win, p->gc, points, 2, CoordModeOrigin);
          pc += 4;
          break;

        case 3: /* polygon */
          for (i = 0; i < marker[mtype][pc + 1]; i++)
            {
              xr = scale * marker[mtype][pc + 2 + 2 * i];
              yr = -scale * marker[mtype][pc + 3 + 2 * i];
              seg_xform_rel(&xr, &yr);
              points[i].x = nint(x - xr);
              points[i].y = nint(y + yr);
            }
          if (p->pixmap) XDrawLines(p->dpy, p->pixmap, p->gc, points, marker[mtype][pc + 1], CoordModeOrigin);
          if (p->selection) XDrawLines(p->dpy, p->drawable, p->gc, points, marker[mtype][pc + 1], CoordModeOrigin);
          if (!p->double_buf) XDrawLines(p->dpy, p->win, p->gc, points, marker[mtype][pc + 1], CoordModeOrigin);
          pc += 1 + 2 * marker[mtype][pc + 1];
          break;

        case 4: /* filled polygon */
          for (i = 0; i < marker[mtype][pc + 1]; i++)
            {
              xr = scale * marker[mtype][pc + 2 + 2 * i];
              yr = -scale * marker[mtype][pc + 3 + 2 * i];
              seg_xform_rel(&xr, &yr);
              points[i].x = nint(x - xr);
              points[i].y = nint(y + yr);
            }
          if (p->pixmap)
            XFillPolygon(p->dpy, p->pixmap, p->gc, points, marker[mtype][pc + 1], Complex, CoordModeOrigin);
          if (p->selection)
            XFillPolygon(p->dpy, p->drawable, p->gc, points, marker[mtype][pc + 1], Complex, CoordModeOrigin);
          if (!p->double_buf)
            XFillPolygon(p->dpy, p->win, p->gc, points, marker[mtype][pc + 1], Complex, CoordModeOrigin);
          pc += 1 + 2 * marker[mtype][pc + 1];
          break;

        case 5: /* hollow polygon */
          for (i = 0; i < marker[mtype][pc + 1]; i++)
            {
              xr = scale * marker[mtype][pc + 2 + 2 * i];
              yr = -scale * marker[mtype][pc + 3 + 2 * i];
              seg_xform_rel(&xr, &yr);
              points[i].x = nint(x - xr);
              points[i].y = nint(y + yr);
            }
          if (p->pixmap)
            XFillPolygon(p->dpy, p->pixmap, p->clear, points, marker[mtype][pc + 1], Complex, CoordModeOrigin);
          if (p->selection)
            XFillPolygon(p->dpy, p->drawable, p->clear, points, marker[mtype][pc + 1], Complex, CoordModeOrigin);
          if (!p->double_buf)
            XFillPolygon(p->dpy, p->win, p->clear, points, marker[mtype][pc + 1], Complex, CoordModeOrigin);
          pc += 1 + 2 * marker[mtype][pc + 1];
          break;

        case 6: /* arc */
          if (p->pixmap) XDrawArc(p->dpy, p->pixmap, p->gc, x - r, y - r, d, d, 0, 360 * 64);
          if (p->selection) XDrawArc(p->dpy, p->drawable, p->gc, x - r, y - r, d, d, 0, 360 * 64);
          if (!p->double_buf) XDrawArc(p->dpy, p->win, p->gc, x - r, y - r, d, d, 0, 360 * 64);
          break;

        case 7: /* filled arc */
          if (p->pixmap) XFillArc(p->dpy, p->pixmap, p->gc, x - r, y - r, d, d, 0, 360 * 64);
          if (p->selection) XFillArc(p->dpy, p->drawable, p->gc, x - r, y - r, d, d, 0, 360 * 64);
          if (!p->double_buf) XFillArc(p->dpy, p->win, p->gc, x - r, y - r, d, d, 0, 360 * 64);
          break;

        case 8: /* hollow arc */
          if (p->pixmap) XFillArc(p->dpy, p->pixmap, p->clear, x - r, y - r, d, d, 0, 360 * 64);
          if (p->selection) XFillArc(p->dpy, p->drawable, p->clear, x - r, y - r, d, d, 0, 360 * 64);
          if (!p->double_buf) XFillArc(p->dpy, p->win, p->clear, x - r, y - r, d, d, 0, 360 * 64);
          break;
        }
      pc++;
    }
  while (op != 0);
}


static void draw_points(int n, double *px, double *py, int tnr)
{
  int i;
  double xn, yn;

  if (n > max_points)
    {
      points = (XPoint *)realloc(points, n * sizeof(XPoint));
      max_points = n;
    }

  for (i = 0; i < n; i++)
    {
      WC_to_NDC(px[i], py[i], tnr, xn, yn);
      seg_xform(&xn, &yn);
      NDC_to_DC(xn, yn, points[i].x, points[i].y);
    }

  if (p->pixmap) XDrawPoints(p->dpy, p->pixmap, p->gc, points, n, CoordModeOrigin);
  if (p->selection) XDrawPoints(p->dpy, p->drawable, p->gc, points, n, CoordModeOrigin);
  if (!p->double_buf) XDrawPoints(p->dpy, p->win, p->gc, points, n, CoordModeOrigin);
}


static void marker_routine(int n, double *px, double *py, int tnr, int mtype, double mscale)
{
  double clrt[4] = {0, 1, 0, 1};
  double x, y;
  int i, xd, yd;
  Bool draw;

  if (gksl->clip == GKS_K_CLIP || mtype != GKS_K_MARKERTYPE_DOT)
    {
      if (gksl->clip == GKS_K_CLIP)
        {
          memmove(clrt, gksl->viewport[gksl->cntnr], 4 * sizeof(double));
          seg_xform(&clrt[0], &clrt[2]);
          seg_xform(&clrt[1], &clrt[3]);
        }
      set_clipping(False);
      for (i = 0; i < n; i++)
        {
          WC_to_NDC(px[i], py[i], tnr, x, y);
          seg_xform(&x, &y);
          NDC_to_DC(x, y, xd, yd);

          if (gksl->clip == GKS_K_CLIP)
            draw = x >= clrt[0] && x <= clrt[1] && y >= clrt[2] && y <= clrt[3];
          else
            draw = True;

          if (draw) draw_marker(x, y, mtype, mscale);
        }
      set_clipping(True);
    }
  else
    draw_points(n, px, py, tnr);
}


static void set_line_attr(int linetype, double linewidth)
{
  int i, list[10];
  char dash_list[10];

  unsigned int width;
  int n;

  if (linewidth > 1)
    width = (unsigned int)(linewidth + 0.5);
  else
    width = 0;

  if (linetype != p->ltype || width != p->lwidth)
    {
      if (linetype != GKS_K_LINETYPE_SOLID)
        {
          gks_get_dash_list(linetype, linewidth, list);
          for (i = 0; i < 10; i++) dash_list[i] = (int)list[i];

          XSetLineAttributes(p->dpy, p->gc, width, LineOnOffDash, CapNotLast, JoinRound);
          n = (int)dash_list[0];
          XSetDashes(p->dpy, p->gc, 0, &dash_list[1], n);
        }
      else
        XSetLineAttributes(p->dpy, p->gc, width, LineSolid, CapNotLast, JoinRound);

      p->ltype = linetype;
      p->lwidth = width;
    }
}


static int clip_code(int x, int y)
{
  int code = 0;

  if (x < 0)
    code = LEFT;
  else if (x > p->width)
    code = RIGHT;

  if (y < 0)
    code |= BOTTOM;
  else if (y > p->height)
    code |= TOP;

  return code;
}


static void clip_line(int *x0, int *x1, int *y0, int *y1, Bool *visible, Bool *clip)
{
  int c, c0, c1;
  int x = 0, y = 0;

  c0 = clip_code(*x0, *y0);
  c1 = clip_code(*x1, *y1);

  *clip = c1;
  *visible = False;

  while (c0 | c1)
    {
      if (c0 & c1) return;
      c = c0 ? c0 : c1;

      if (c & LEFT)
        {
          x = 0;
          y = (int)(*y0 - (*y1 - *y0) * (double)(*x0) / (*x1 - *x0));
        }
      else if (c & RIGHT)
        {
          x = p->width;
          y = (int)(*y0 + (*y1 - *y0) * (double)(p->width - *x0) / (*x1 - *x0));
        }
      else if (c & BOTTOM)
        {
          x = (int)(*x0 - (*x1 - *x0) * (double)(*y0) / (*y1 - *y0));
          y = 0;
        }
      else if (c & TOP)
        {
          x = (int)(*x0 + (*x1 - *x0) * (double)(p->height - *y0) / (*y1 - *y0));
          y = p->height;
        }

      if (c == c0)
        {
          *x0 = x;
          *y0 = y;
          c0 = clip_code(x, y);
        }
      else
        {
          *x1 = x;
          *y1 = y;
          c1 = clip_code(x, y);
        }
    }
  *visible = True;
}


static void line_routine(int n, double *px, double *py, int linetype, int tnr)
{
  double x1, y1;
  int i, j, npoints, m;
  int ix0, iy0, ix1, iy1, x, y;
  Bool visible, clip;

  if (n > max_points)
    {
      points = (XPoint *)realloc(points, n * sizeof(XPoint));
      max_points = n;
    }

  WC_to_NDC(px[0], py[0], tnr, x1, y1);
  seg_xform(&x1, &y1);
  NDC_to_DC(x1, y1, ix1, iy1);

  npoints = 0;
  m = linetype ? n : n + 1;

  for (j = 1; j < m; j++)
    {
      i = j < n ? j : 0;

      ix0 = ix1;
      iy0 = iy1;

      WC_to_NDC(px[i], py[i], tnr, x1, y1);
      seg_xform(&x1, &y1);
      NDC_to_DC(x1, y1, ix1, iy1);

      x = ix1;
      y = iy1;
      clip_line(&ix0, &ix1, &iy0, &iy1, &visible, &clip);

      if (visible)
        {
          if (!npoints)
            {
              points[0].x = ix0;
              points[0].y = iy0;
              npoints = 1;
            }

          points[npoints].x = ix1;
          points[npoints].y = iy1;
          npoints++;

          if (clip)
            {
              if (p->pixmap) XDrawLines(p->dpy, p->pixmap, p->gc, points, npoints, 0);
              if (p->selection) XDrawLines(p->dpy, p->drawable, p->gc, points, npoints, 0);
              if (!p->double_buf) XDrawLines(p->dpy, p->win, p->gc, points, npoints, 0);
              npoints = 0;
            }

          if (npoints == MAX_POINTS)
            {
              if (p->pixmap) XDrawLines(p->dpy, p->pixmap, p->gc, points, npoints, 0);
              if (p->selection) XDrawLines(p->dpy, p->drawable, p->gc, points, npoints, 0);
              if (!p->double_buf) XDrawLines(p->dpy, p->win, p->gc, points, npoints, 0);

              --npoints;
              points[0].x = points[npoints].x;
              points[0].y = points[npoints].y;
              npoints = 1;
            }
        }

      ix1 = x;
      iy1 = y;
    }

  if (npoints > 1)
    {
      if (p->pixmap) XDrawLines(p->dpy, p->pixmap, p->gc, points, npoints, 0);
      if (p->selection) XDrawLines(p->dpy, p->drawable, p->gc, points, npoints, 0);
      if (!p->double_buf) XDrawLines(p->dpy, p->win, p->gc, points, npoints, 0);
    }
}


static void polyline(int n, double *px, double *py)
{
  int ln_type, ln_color;
  double ln_width;

  ln_type = gksl->asf[0] ? gksl->ltype : gksl->lindex;
  ln_width = gksl->asf[1] ? gksl->lwidth : 1;
  ln_color = gksl->asf[2] ? gksl->plcoli : 1;

  set_color(ln_color);
  set_line_attr(ln_type, ln_width);

  line_routine(n, px, py, ln_type, gksl->cntnr);
}


static void polymarker(int n, double *px, double *py)
{
  int mk_type, mk_color;
  double mk_size;

  mk_type = gksl->asf[3] ? gksl->mtype : gksl->mindex;
  mk_size = gksl->asf[4] ? gksl->mszsc : 1;
  mk_color = gksl->asf[5] ? gksl->pmcoli : 1;

  set_color(mk_color);
  set_line_attr(GKS_K_LINETYPE_SOLID, 1.0);

  marker_routine(n, px, py, gksl->cntnr, mk_type, mk_size);
}


static void fill_routine(int n, double *px, double *py, int tnr)
{
  double x, y;
  int i, npoints;

  if (n > max_points)
    {
      points = (XPoint *)realloc(points, n * sizeof(XPoint));
      max_points = n;
    }

  npoints = n;
  for (i = 0; i < n; i++)
    {
      WC_to_NDC(px[i], py[i], tnr, x, y);
      seg_xform(&x, &y);
      NDC_to_DC(x, y, points[i].x, points[i].y);
    }

  if (npoints > 1)
    {
      if (p->pixmap) XFillPolygon(p->dpy, p->pixmap, p->gc, points, npoints, p->shape, CoordModeOrigin);
      if (p->selection) XFillPolygon(p->dpy, p->drawable, p->gc, points, npoints, p->shape, CoordModeOrigin);
      if (!p->double_buf) XFillPolygon(p->dpy, p->win, p->gc, points, npoints, p->shape, CoordModeOrigin);
    }
}


static void fill_area(int n, double *px, double *py)
{
  int fl_inter, fl_style, fl_color;
  int ln_type;

  fl_inter = gksl->asf[10] ? gksl->ints : predef_ints[gksl->findex - 1];
  fl_style = gksl->asf[11] ? gksl->styli : predef_styli[gksl->findex - 1];
  fl_color = gksl->asf[12] ? gksl->facoli : 1;

  set_color(fl_color);
  set_line_attr(GKS_K_LINETYPE_SOLID, 1.0);

  if (fl_inter == GKS_K_INTSTYLE_SOLID)
    {
      fill_routine(n, px, py, gksl->cntnr);
    }
  else if (fl_inter == GKS_K_INTSTYLE_PATTERN || fl_inter == GKS_K_INTSTYLE_HATCH)
    {
      if (fl_inter == GKS_K_INTSTYLE_HATCH)
        set_pattern(p->ccolor, fl_style + HATCH_STYLE);
      else
        set_pattern(p->ccolor, fl_style);
      fill_routine(n, px, py, gksl->cntnr);
      set_pattern(p->ccolor, 0);
    }
  else
    {
      ln_type = DrawBorder;
      line_routine(n, px, py, ln_type, gksl->cntnr);
    }
}


static void x_draw_string(Display *display, Drawable d, GC gc, int x, int y, char *string, int length)
{
#ifndef NO_XFT
  XftDraw *draw;
  XftColor *color = NULL;
  unsigned int *s32;
  int i, j;

  draw = XftDrawCreate(display, d, p->vis, p->cmap);
  if (p->havecolor[p->ccolor]) color = &p->rendercolor[p->ccolor];

  if (p->font == 12) /* Symbol */
    {
      s32 = (unsigned int *)gks_malloc(length * sizeof(unsigned int));
      for (i = 0; i < length; i++)
        {
          j = (int)string[i];
          if (j < 0) j += 256;
          s32[i] = adobe2utf[j];
        }
      XftDrawString32(draw, color, p->cfont, x, y, s32, length);
      free(s32);
    }
  else
    XftDrawString8(draw, color, p->cfont, x, y, (unsigned char *)string, length);

  XftDrawDestroy(draw);
#else
  XDrawString(display, d, gc, x, y, string, length);
#endif
}


#if !defined(NO_XFT) || defined(NO_FT)

static void draw_string(int x, int y, int width, int height, char *chars, int nchars)
{
  Pixmap src, dest;
  XImage *from, *to;
  int ascent, descent, w = 0, h = 0;
  int i, j, ii = 0, jj = 0;
  unsigned long pixel;

  height += p->cfont->descent + 2;
  ascent = p->cfont->ascent;
  descent = p->cfont->descent;

  switch (p->path)
    {
    case 0:
      w = width;
      h = height;
      break;
    case 1:
      x = x - height + descent;
      y -= width;
      w = height;
      h = width;
      break;
    case 2:
      x -= width;
      y -= descent;
      w = width;
      h = height;
      break;
    case 3:
      x -= descent;
      w = height;
      h = width;
      break;
    }

  if (p->path != 0)
    {
      set_clipping(False);

      src = XCreatePixmap(p->dpy, XRootWindowOfScreen(p->screen), width, height, p->depth);
      from = XGetImage(p->dpy, src, 0, 0, width, height, AllPlanes, ZPixmap);

      dest = XCreatePixmap(p->dpy, XRootWindowOfScreen(p->screen), w, h, p->depth);
      XCopyArea(p->dpy, p->pixmap ? p->pixmap : p->win, dest, p->gc, x, y, w, h, 0, 0);
      to = XGetImage(p->dpy, dest, 0, 0, w, h, AllPlanes, ZPixmap);

      for (i = 0; i < width; i++)
        for (j = 0; j < height; j++)
          {
            switch (p->path)
              {
              case 1:
                ii = j;
                jj = h - i - 1;
                break;
              case 2:
                ii = w - i - 1;
                jj = h - j - 1;
                break;
              case 3:
                ii = w - j - 1;
                jj = i;
                break;
              }
            pixel = XGetPixel(to, ii, jj);
            XPutPixel(from, i, j, pixel);
          }

      XPutImage(p->dpy, src, p->gc, from, 0, 0, 0, 0, width, height);

      x_draw_string(p->dpy, src, p->gc, 0, height - descent, chars, nchars);

      XDestroyImage(from);
      from = XGetImage(p->dpy, src, 0, 0, width, height, AllPlanes, ZPixmap);

      for (i = 0; i < width; i++)
        for (j = 0; j < height; j++)
          {
            switch (p->path)
              {
              case 1:
                ii = j;
                jj = h - i - 1;
                break;
              case 2:
                ii = w - i - 1;
                jj = h - j - 1;
                break;
              case 3:
                ii = w - j - 1;
                jj = i;
                break;
              }
            pixel = XGetPixel(from, i, j);
            XPutPixel(to, ii, jj, pixel);
            update_bbox(x + ii, y + jj);
          }

      set_clipping(True);

      if (p->pixmap) XPutImage(p->dpy, p->pixmap, p->gc, to, 0, 0, x, y, w, h);
      if (p->selection) XPutImage(p->dpy, p->drawable, p->gc, to, 0, 0, x, y, w, h);
      if (!p->double_buf) XPutImage(p->dpy, p->win, p->gc, to, 0, 0, x, y, w, h);

      XDestroyImage(to);
      XFreePixmap(p->dpy, dest);
      XDestroyImage(from);
      XFreePixmap(p->dpy, src);
    }
  else
    {
      if (p->pixmap) x_draw_string(p->dpy, p->pixmap, p->gc, x, y, chars, nchars);
      if (p->selection) x_draw_string(p->dpy, p->drawable, p->gc, x, y, chars, nchars);
      if (!p->double_buf) x_draw_string(p->dpy, p->win, p->gc, x, y, chars, nchars);

      update_bbox(x, y + descent);
      update_bbox(x + width, y + descent - ascent);
    }
}


static void text_routine(double x, double y, int nchars, char *chars)
{
  int xorg, yorg, width, height;
  double xrel, yrel, ax, ay;
  int tx_prec;
#ifndef NO_XFT
  XGlyphInfo extents;
  unsigned int *s32;
  int i, j;
#endif

  NDC_to_DC(x, y, xorg, yorg);

  /* Compute text extent */

#ifndef NO_XFT
  if (p->font == 12) /* Symbol */
    {
      s32 = (unsigned int *)gks_malloc(nchars * sizeof(unsigned int));
      for (i = 0; i < nchars; i++)
        {
          j = (int)chars[i];
          if (j < 0) j += 256;
          s32[i] = adobe2utf[j];
        }
      XftTextExtents32(p->dpy, p->cfont, s32, nchars, &extents);
      free(s32);
    }
  else
    XftTextExtents8(p->dpy, p->cfont, (unsigned char *)chars, nchars, &extents);

  width = extents.xOff;
#else
  width = XTextWidth(p->cfont, chars, nchars);
#endif
  height = p->capheight;

  tx_prec = gksl->asf[6] ? gksl->txprec : predef_prec[gksl->tindex - 1];

  if (tx_prec == GKS_K_TEXT_PRECISION_STRING)
    {

      /* Align the text */

      xrel = width * xfac[gksl->txal[0]];
      if (gksl->txal[1] != GKS_K_TEXT_VALIGN_BOTTOM)
        yrel = height * yfac[gksl->txal[1]];
      else
        yrel = p->cfont->descent;

      CharXform(xrel, yrel, ax, ay);

      xorg += (int)ax;
      yorg -= (int)ay;
    }

  if (width > 0 && height > 0) draw_string(xorg, yorg, width, height, chars, nchars);
}


#ifndef NO_XFT
#define load_font(d, f) XftFontOpenXlfd(d, DefaultScreen(p->dpy), f)
#else
#define load_font(d, f) XLoadQueryFont(d, f)
#endif


static void try_load_font(int font, int size, char *fontname)
{
  int f;
#ifndef NO_XFT
  int family, weight, slant;
  XftPattern *font_pattern;
  XftPattern *match_pattern;
  XftResult match_result;
#endif

  sprintf(fontname, urw_fonts[font], size, p->dpi, p->dpi);
  p->fstr[font][size] = load_font(p->dpy, fontname);

  if (p->fstr[font][size] == NULL)
    {
      for (f = 0; f < n_foundries; f++)
        {
          sprintf(fontname, fonts[font], foundry[f], size, p->dpi, p->dpi);
          p->fstr[font][size] = load_font(p->dpy, fontname);

          if (p->fstr[font][size] != NULL) break;
        }
    }
#ifndef NO_XFT
  f = font + 1;
  if (f < 30)
    {
      if (f > 13) f += 3;
      family = (f - 1) / 4;
      weight = (f % 4 == 1 || f % 4 == 2) ? XFT_WEIGHT_MEDIUM : XFT_WEIGHT_BOLD;
      slant = (f % 4 == 2 || f % 4 == 0) ? XFT_SLANT_ITALIC : XFT_SLANT_ROMAN;
    }
  else
    {
      family = 8 + f - 30;
      weight = XFT_WEIGHT_MEDIUM;
      slant = XFT_SLANT_ROMAN;
    }

  font_pattern = XftPatternCreate();
  XftPatternAddString(font_pattern, XFT_FAMILY, base_fonts[family]);
  XftPatternAddInteger(font_pattern, XFT_WEIGHT, weight);
  XftPatternAddInteger(font_pattern, XFT_SLANT, slant);
  XftPatternAddDouble(font_pattern, XFT_PIXEL_SIZE, (double)size);

  match_pattern = XftFontMatch(p->dpy, DefaultScreen(p->dpy), font_pattern, &match_result);
  p->fstr[font][size] = XftFontOpenPattern(p->dpy, match_pattern);
  XftPatternDestroy(font_pattern);
#endif
}


static void verify_font_capabilities(void)
{
  int font = 2, size = MAX_SIZE;
  char fontname[256];
#ifndef NO_XFT
  XGlyphInfo extents;
  unsigned int s32[1];
#endif

  try_load_font(font, size, fontname);

  p->scalable_fonts = p->fstr[font][size] != NULL;

#ifndef NO_XFT
  if (p->scalable_fonts)
    {
      for (font = 0; font < n_font; font++)
        {
          if (p->fstr[font][size] == NULL) try_load_font(font, size, fontname);

          if (p->fstr[font][size] != NULL)
            {
              if (font == 12) /* Symbol */
                {
                  s32[0] = adobe2utf[(unsigned int)'A'];
                  XftTextExtents32(p->dpy, p->fstr[font][size], s32, 1, &extents);
                }
              else
                XftTextExtents8(p->dpy, p->fstr[font][size], (unsigned char *)"A", 1, &extents);

              if (extents.height != 0) capheights[font] = (double)extents.height / MAX_SIZE;
            }
        }
    }
#endif
}


static void set_font(int font)
{
  int size, angle;
  char fontname[256];
  double scale, ux, uy, rad;
  double width, height, capheight, points;

  font = abs(font);
  if (font >= 101 && font <= 131)
    font -= 100;
  else if (font >= 1 && font <= 32)
    font = map[font - 1];
  else
    font = 9;

  font = font - 1;
  p->font = font;

  WC_to_NDC_rel(gksl->chup[0], gksl->chup[1], gksl->cntnr, ux, uy);
  seg_xform_rel(&ux, &uy);

  rad = -atan2(ux, uy);
  angle = (int)(rad * 180 / M_PI + 0.5);
  if (angle < 0) angle += 360;
  p->path = ((angle + 45) / 90) % 4;

  scale = sqrt(gksl->chup[0] * gksl->chup[0] + gksl->chup[1] * gksl->chup[1]);
  ux = gksl->chup[0] / scale * gksl->chh;
  uy = gksl->chup[1] / scale * gksl->chh;
  WC_to_NDC_rel(ux, uy, gksl->cntnr, ux, uy);

  width = 0;
  height = sqrt(ux * ux + uy * uy);
  seg_xform_rel(&width, &height);

  height = sqrt(width * width + height * height);
  capheight = height * (fabs(p->c) + 1);

  if (p->scalable_fonts)
    points = 10 * capheight / capheights[font];
  else
    points = 10 * capheight * 100.0 / p->dpi;

  p->capheight = nint(capheight);

  if (p->scalable_fonts)
    {
      size = nint(points / 10);
      if (size < 1) size = 1;
      if (size > MAX_SIZE) size = MAX_SIZE;
    }
  else
    {
      if (points <= 90)
        size = 8;
      else if (points <= 110)
        size = 10;
      else if (points <= 130)
        size = 12;
      else if (points <= 160)
        size = 14;
      else if (points <= 210)
        size = 18;
      else
        size = 24;
    }

  if (p->fstr[font][size] == NULL)
    {
      try_load_font(font, size, fontname);

      if (p->fstr[font][size] == NULL)
        {
          gks_perror("unable to load font %s", fontname);
          p->fstr[font][size] = load_font(p->dpy, "variable");

          if (p->fstr[font][size] == NULL) p->fstr[font][size] = load_font(p->dpy, "fixed");
        }
    }

  if (p->fstr[font][size] != NULL)
    {
      p->cfont = p->fstr[font][size];
#ifdef NO_XFT
      XSetFont(p->dpy, p->gc, p->cfont->fid);
#endif
    }
}

#endif


static unsigned long rgb2pixel(int rgb)
{
  unsigned long r, g, b, rmask, gmask, bmask;
  int rshift, gshift, bshift;

  r = (rgb & 0xff) << 8;
  g = (rgb & 0xff00);
  b = (rgb & 0xff0000) >> 8;

  rmask = p->vis->red_mask;
  gmask = p->vis->green_mask;
  bmask = p->vis->blue_mask;

  rshift = 15 - highbit(rmask);
  gshift = 15 - highbit(gmask);
  bshift = 15 - highbit(bmask);

  if (rshift < 0)
    r = r << (-rshift);
  else
    r = r >> rshift;

  if (gshift < 0)
    g = g << (-gshift);
  else
    g = g >> gshift;

  if (bshift < 0)
    b = b << (-bshift);
  else
    b = b >> bshift;

  r = r & rmask;
  g = g & gmask;
  b = b & bmask;

  return r | g | b;
}


static unsigned long pixel2rgb(int pixel)
{
  unsigned long r, g, b, rmask, gmask, bmask;
  int rshift, gshift, bshift;

  rmask = p->vis->red_mask;
  gmask = p->vis->green_mask;
  bmask = p->vis->blue_mask;

  rshift = lowbit(rmask);
  gshift = lowbit(gmask);
  bshift = lowbit(bmask);

  r = (pixel & rmask) >> rshift;
  g = (pixel & gmask) >> gshift;
  b = (pixel & bmask) >> bshift;

  r = (r & 0xff);
  g = (g & 0xff) << 8;
  b = (b & 0xff) << 16;

  return r | g | b;
}


static void draw_image(int x, int y, int width, int height, byte *ba, Bool true_color)
{
  Pixmap dest;
  XImage *to;
  int i, j;
  unsigned long pixel, rgb;
  int r, g, b;
  double a, red, green, blue;

  set_clipping(False);

  dest = XCreatePixmap(p->dpy, XRootWindowOfScreen(p->screen), width, height, p->depth);
  XCopyArea(p->dpy, p->pixmap ? p->pixmap : p->win, dest, p->gc, x, y, width, height, 0, 0);
  to = XGetImage(p->dpy, dest, 0, 0, width, height, AllPlanes, ZPixmap);

  if (!true_color)
    {
      red = 255.0 * p->red[p->ccolor];
      green = 255.0 * p->green[p->ccolor];
      blue = 255.0 * p->blue[p->ccolor];
    }

  for (j = 0; j < height; j++)
    for (i = 0; i < width; i++)
      {
        pixel = XGetPixel(to, i, j);
        rgb = pixel2rgb(pixel);
        if (true_color)
          {
            a = (double)ba[3] / 255.0;
            r = (int)((double)ba[0] * a + (1 - a) * ((rgb & 0xff)));
            g = (int)((double)ba[1] * a + (1 - a) * ((rgb & 0xff00) >> 8));
            b = (int)((double)ba[2] * a + (1 - a) * ((rgb & 0xff0000) >> 16));
            ba += 4;
          }
        else
          {
            a = (double)ba[0] / 255.0;
            r = (int)(red * a + (1 - a) * ((rgb & 0xff)));
            g = (int)(green * a + (1 - a) * ((rgb & 0xff00) >> 8));
            b = (int)(blue * a + (1 - a) * ((rgb & 0xff0000) >> 16));
            ba += 1;
          }
        rgb = r + (g << 8) + (b << 16);
        pixel = rgb2pixel(rgb);
        XPutPixel(to, i, j, pixel);
      }

  if (p->pixmap) XPutImage(p->dpy, p->pixmap, p->gc, to, 0, 0, x, y, width, height);
  if (p->selection) XPutImage(p->dpy, p->drawable, p->gc, to, 0, 0, x, y, width, height);
  if (!p->double_buf) XPutImage(p->dpy, p->win, p->gc, to, 0, 0, x, y, width, height);

  XDestroyImage(to);
  XFreePixmap(p->dpy, dest);

  set_clipping(True);
}


#if defined(NO_XFT) && !defined(NO_FT)

static void ft_text_routine(double px, double py, int nchars, char *chars)
{
  int x, y, w, h;
  byte *bitmap;
  char *s, ch;
  int i, n;
  size_t len;

  NDC_to_DC(px, py, x, y);
  w = p->width;
  y = p->height - y;

  s = (char *)gks_malloc(nchars * 3 + 1);
  n = 0;
  for (i = 0; i < nchars; i++)
    {
      ch = (chars[i] < 0) ? chars[i] + 256 : chars[i];
      if (p->font == 12)
        gks_symbol2utf(ch, s + n, &len);
      else
        {
          s[n] = ch;
          len = 1;
        }
      n += len;
      s[n] = '\0';
    }

  bitmap = gks_ft_get_bitmap(&x, &y, &w, &h, gksl, s, n);
  if (bitmap != NULL)
    {
      draw_image(x, (p->height - y) - h, w, h, bitmap, False);
      free(bitmap);
    }
  free(s);
}

#endif


static void text(double px, double py, int nchars, char *chars)
{
  int tx_font, tx_prec, tx_color;
  double x, y;

  tx_font = gksl->asf[6] ? gksl->txfont : predef_font[gksl->tindex - 1];
  tx_prec = gksl->asf[6] ? gksl->txprec : predef_prec[gksl->tindex - 1];
  tx_color = gksl->asf[9] ? gksl->txcoli : 1;

#if !defined(NO_XFT) || defined(NO_FT)
  if (tx_prec != GKS_K_TEXT_PRECISION_STROKE) set_font(tx_font);
#endif

  set_color(tx_color);
  set_line_attr(GKS_K_LINETYPE_SOLID, 1.0);

  if (tx_prec == GKS_K_TEXT_PRECISION_STRING)
    {
      WC_to_NDC(px, py, gksl->cntnr, x, y);
      seg_xform(&x, &y);

#if defined(NO_XFT) && !defined(NO_FT)
      ft_text_routine(x, y, nchars, chars);
#else
      text_routine(x, y, nchars, chars);
#endif
    }
  else
    gks_emul_text(px, py, nchars, chars, line_routine, fill_routine);
}


static void update(void)
{
  XEvent event;

  if (p->state == GKS_K_WS_ACTIVE)
    {
      if (!p->widget && p->wstype != 212 && !p->backing_store)
        {
          while (XPending(p->dpy))
            {
              XNextEvent(p->dpy, &event);
              if (event.type == Expose) expose_event(p->widget, p, (XExposeEvent *)&event, NULL);
            }
        }
      else
        {
          if (!p->new_win)
            if (XCheckTypedWindowEvent(p->dpy, p->win, ConfigureNotify, &event))
              configure_viewport((XConfigureEvent *)&event);

          XSync(p->dpy, False);
        }
    }
}


static void message(int nchars, char *chars)
{
  x_draw_string(p->dpy, p->win, p->invert, 10, 20, chars, nchars);
}


static void write_gif_word(int w)
{
  byte c;

  c = (w & 0xff);
  gks_write_file(p->gif, (void *)&c, 1);
  c = ((w >> 8) & 0xff);
  gks_write_file(p->gif, (void *)&c, 1);
}


static void pixmap_to_gif(void)
{
  int size, besize;
  byte c, r, g, b, *pix, *ppix, *beimage;
  int i, j, k, coli, mcolor;
  XImage *image;
  int BitsPerPixel, ColorMapSize, InitCodeSize;
  unsigned long pixel;

  image = XGetImage(p->dpy, p->pixmap, 0, 0, p->width, p->height, AllPlanes, ZPixmap);

  size = p->width * p->height;
  pix = ppix = (byte *)gks_malloc(sizeof(byte) * size);
  beimage = (byte *)gks_malloc(sizeof(byte) * size * 3 / 2); /* worst case */

  if (pix != NULL && beimage != NULL)
    {
      mcolor = 0;
      for (j = 0; j < p->height; j++)
        {
          for (i = 0; i < p->width; i++)
            {
              pixel = XGetPixel(image, i, j);
              coli = 0;
              for (k = 0; k < MAX_COLOR; k++)
                {
                  if (pixel == p->color[k].pixel)
                    {
                      coli = k;
                      break;
                    }
                }
              *ppix++ = coli;
              if (coli > mcolor) mcolor = coli;
            }
        }

      for (BitsPerPixel = 1; BitsPerPixel < 8; BitsPerPixel++)
        if ((1 << BitsPerPixel) > mcolor) break;

      /* write the GIF header */

      gks_write_file(p->gif, p->wstype == 218 ? (char *)"GIF89a" : (char *)"GIF87a", 6);

      write_gif_word(p->width); /* screen descriptor */
      write_gif_word(p->height);

      c = 0x80;                /* yes, there is a color map */
      c |= (8 - 1) << 4;       /* OR in the color resolution (8) */
      c |= (0) << 3;           /* no, the colors are not sorted */
      c |= (BitsPerPixel - 1); /* OR in the # of bits per pixel */
      gks_write_file(p->gif, &c, 1);

      c = 0x0;
      gks_write_file(p->gif, &c, 1); /* background color index */
      gks_write_file(p->gif, &c, 1); /* pixel aspect ratio */

      /* write colormap */

      ColorMapSize = 1 << BitsPerPixel;

      for (i = 0; i < ColorMapSize; i++)
        {
          r = (byte)(255 * p->red[i]);
          g = (byte)(255 * p->green[i]);
          b = (byte)(255 * p->blue[i]);
          gks_write_file(p->gif, &r, 1);
          gks_write_file(p->gif, &g, 1);
          gks_write_file(p->gif, &b, 1);
        }

      /* write extension block */

      if (p->wstype == 218)
        { /* transparent GIF? */
          c = 0x21;
          gks_write_file(p->gif, &c, 1); /* extension block ID (fixed value) */
          c = 0xf9;
          gks_write_file(p->gif, &c, 1); /* graphic control label (fixed) */
          c = 0x4;
          gks_write_file(p->gif, &c, 1); /* block size (fixed) */
          c = 0x0;                       /* no disposal method, no user flag */
          c |= 0x1;                      /* transparent color on */
          gks_write_file(p->gif, &c, 1);
          write_gif_word(0); /* delay time */
          c = 0x0;
          gks_write_file(p->gif, &c, 1); /* transparent color index = 0 */
          gks_write_file(p->gif, &c, 1); /* block terminator = 0 */
        }

      /* write image block */

      c = ',';
      gks_write_file(p->gif, &c, 1); /* image separator */

      write_gif_word(0);         /* image left offset */
      write_gif_word(0);         /* image top offset */
      write_gif_word(p->width);  /* image width */
      write_gif_word(p->height); /* image height */

      c = 0x0;
      gks_write_file(p->gif, &c, 1); /* no interlace, sort, color table */

      InitCodeSize = max(BitsPerPixel, 2);
      gks_compress(InitCodeSize + 1, pix, size, beimage, &besize);

      c = InitCodeSize;
      gks_write_file(p->gif, &c, 1);
      if (gks_write_file(p->gif, beimage, besize) != besize)
        {
          gks_perror("can't write GIF file");
          perror("write");
        }

      free(beimage);
      free(pix);
    }
  else
    gks_perror("can't allocate temporary storage");

  c = '\0';
  gks_write_file(p->gif, &c, 1); /* write out a zero-length packet */
  c = ';';
  gks_write_file(p->gif, &c, 1); /* terminator */

  XDestroyImage(image);
}


static int compress_rle(byte *pix, int size, byte *beimage)
{
  byte c = 0, pc = 0;
  int besize, count, i, j;

  besize = 0;
  count = 0;
  for (i = 0; i < size; ++i)
    {
      c = *pix++;
      if (count > 0)
        {
          if (pc != c)
            {
              if (count == 1 && pc == 128)
                {
                  beimage[besize++] = 128;
                  beimage[besize++] = 0;
                  count = 0;
                }
              else if (count > 2 || pc == 128)
                {
                  beimage[besize++] = 128;
                  beimage[besize++] = count - 1;
                  beimage[besize++] = pc;
                  count = 0;
                }
              else
                {
                  for (j = 0; j < count; ++j) beimage[besize++] = pc;
                  count = 0;
                }
            }
        }
      pc = c;
      ++count;
      if (count == 256)
        {
          beimage[besize++] = 128;
          beimage[besize++] = count - 1;
          beimage[besize++] = c;
          count = 0;
        }
    }
  if (count > 0)
    {
      if (count == 1 && c == 128)
        {
          beimage[besize++] = 128;
          beimage[besize++] = 0;
        }
      if (count > 2 || c == 128)
        {
          beimage[besize++] = 128;
          beimage[besize++] = count - 1;
          beimage[besize++] = c;
        }
      else
        {
          for (j = 0; j < count; ++j) beimage[besize++] = c;
        }
    }

  return besize;
}


static void write_rf_long(long l)
{
  byte c;

  c = ((l >> 24) & 0xff);
  gks_write_file(p->rf, &c, 1);
  c = ((l >> 16) & 0xff);
  gks_write_file(p->rf, &c, 1);
  c = ((l >> 8) & 0xff);
  gks_write_file(p->rf, &c, 1);
  c = (l & 0xff);
  gks_write_file(p->rf, &c, 1);
}


#define RAS_MAGIC 0x59a66a95
#define RT_BYTE_ENCODED 2 /* Run-length compression of bytes */
#define RMT_EQUAL_RGB 1

static void pixmap_to_rf(void)
{
  XImage *image;
  int linesize, size, besize, depth = 8;
  byte *pix, *ppix, *beimage;
  int i, j, k, coli;
  byte rmap[MAX_COLOR], gmap[MAX_COLOR], bmap[MAX_COLOR];
  unsigned long pixel;

  image = XGetImage(p->dpy, p->pixmap, 0, 0, p->width, p->height, AllPlanes, ZPixmap);

  linesize = p->width;
  if (linesize % 2) linesize++;

  size = linesize * p->height;

  pix = ppix = (byte *)gks_malloc(sizeof(byte) * size);
  beimage = (byte *)gks_malloc(sizeof(byte) * size * 3 / 2); /* worst case */

  if (pix != NULL && beimage != NULL)
    {
      for (j = 0; j < p->height; j++)
        {
          for (i = 0; i < p->width; i++)
            {
              pixel = XGetPixel(image, i, j);
              coli = 0;
              for (k = 0; k < MAX_COLOR; k++)
                {
                  if (pixel == p->color[k].pixel)
                    {
                      coli = k;
                      break;
                    }
                }
              *ppix++ = coli;
            }
          if (linesize != p->width) *ppix++ = 0;
        }

      besize = compress_rle(pix, size, beimage);

      /* write the header */

      write_rf_long(RAS_MAGIC);
      write_rf_long(p->width);
      write_rf_long(p->height);
      write_rf_long(depth);
      write_rf_long(besize);
      write_rf_long(RT_BYTE_ENCODED);
      write_rf_long(RMT_EQUAL_RGB);
      write_rf_long(3 * MAX_COLOR);

      for (i = 0; i < MAX_COLOR; i++)
        {
          rmap[i] = (byte)(255 * p->red[i]);
          gmap[i] = (byte)(255 * p->green[i]);
          bmap[i] = (byte)(255 * p->blue[i]);
        }

      /* write the colormap */

      gks_write_file(p->rf, rmap, MAX_COLOR);
      gks_write_file(p->rf, gmap, MAX_COLOR);
      gks_write_file(p->rf, bmap, MAX_COLOR);

      /* write the image */

      if (gks_write_file(p->rf, beimage, besize) != besize)
        {
          gks_perror("can't write Sun rle rasterfile");
          perror("write");
        }

      free(beimage);
      free(pix);
    }
  else
    gks_perror("can't allocate temporary storage");

  XDestroyImage(image);
}


#define UIL_HEADER \
  "\
value\n\
    white : color ('white');\n\
    black : color ('black');\n\
    red : color ('red');\n\
    green : color ('green');\n\
    blue : color ('blue');\n\
    cyan : color ('cyan');\n\
    yellow : color ('yellow');\n\
    magenta : color ('magenta');\n\
\n\
value\n\
    color_map : color_table (\n\
        white = '`',\n\
        black = 'd',\n\
        red = 'r',\n\
        green = 'g',\n\
        blue = 'b',\n\
        cyan = 'c',\n\
        yellow = 'y',\n\
        magenta = 'm'\n\
    );\n"


static void pixmap_to_uil(void)
{
  static char *icon_name;
  XImage *image;
  int i, j, k, n = 8, pix;
  unsigned long pixel;

  static char letter[] = {'`', 'd', 'r', 'g', 'b', 'c', 'y', 'm'};

  icon_name = (char *)gks_getenv("GKS_ICON");
  image = XGetImage(p->dpy, p->pixmap, 0, 0, p->width, p->height, AllPlanes, ZPixmap);

  gks_write_file(p->uil, (void *)"\n", 1);
  if (icon_name != NULL)
    gks_write_file(p->uil, icon_name, strlen(icon_name));
  else
    gks_write_file(p->uil, (void *)"(unknown)", 9);
  gks_write_file(p->uil, (void *)" : icon (color_table = color_map", 32);
  for (j = 0; j < p->height; j++)
    {
      gks_write_file(p->uil, (void *)",\n    '", 7);
      for (i = 0; i < p->width; i++)
        {
          pixel = XGetPixel(image, i, j);
          pix = 0;
          for (k = 0; k < n; k++)
            {
              if (pixel == p->color[k].pixel)
                {
                  pix = k;
                  break;
                }
            }
          gks_write_file(p->uil, &letter[pix], 1);
        }
      gks_write_file(p->uil, (void *)"'", 1);
    }
  gks_write_file(p->uil, (void *)"\n    );\n", 8);

  XDestroyImage(image);
}


static void set_frame_header(int frame)
{
  char header[32];

  sprintf(header, "Frame #%d\n", frame);
  XStoreName(p->dpy, p->win, header);
}


static void pixmap_loop(void)
{
  int this_frame = 0, inc = 1;
  XEvent event;
  Bool run = True, step = False;

  XSelectInput(p->dpy, p->win, ButtonPressMask);
  XSetClipMask(p->dpy, p->gc, None);
  XSynchronize(p->dpy, True);

  /* Be sure that the window is mapped */

  XMapWindow(p->dpy, p->win);

  for (; p->nframes > 0;)
    {
      if (run || step)
        {
          XCopyArea(p->dpy, p->frame[this_frame], p->win, p->gc, 0, 0, p->width, p->height, 0, 0);
          this_frame += inc;
          if (this_frame == 0 || this_frame == p->nframes - 1) inc = -inc;
          step = False;
          set_frame_header(this_frame);
        }

      while (XPending(p->dpy))
        {
          XNextEvent(p->dpy, &event);
          if (event.type == ButtonPress)
            {
              if (event.xbutton.button == Button1)
                run = !run;
              else if (event.xbutton.button == Button2)
                step = True;
              else
                goto stop;
            }
        }
    }
stop:
  this_frame = p->nframes;
  while (this_frame--) XFreePixmap(p->dpy, p->frame[this_frame]);
  free(p->frame);

  p->pixmap = 0;
}


#define COPY_BODY(type)                                                                                       \
  int i, j, ix, iy, nbytes;                                                                                   \
  int *ilptr, *ipptr;                                                                                         \
  byte *blptr, *bpptr, *packed_colia;                                                                         \
  type *elptr, *epptr, tmp, *tmpptr;                                                                          \
  type pixel[MAX_COLOR];                                                                                      \
                                                                                                              \
  if (!true_color)                                                                                            \
    {                                                                                                         \
      for (i = 0; i < MAX_COLOR; i++)                                                                         \
        {                                                                                                     \
          if (p->depth != 1)                                                                                  \
            pixel[i] = (type)p->color[i].pixel;                                                               \
          else                                                                                                \
            pixel[i] = i;                                                                                     \
        }                                                                                                     \
    }                                                                                                         \
                                                                                                              \
  if (p->packed_ca)                                                                                           \
    {                                                                                                         \
      packed_colia = (byte *)colia;                                                                           \
                                                                                                              \
      if (dx != dimx || w != dx || h != dy || w != bytes_per_line)                                            \
        {                                                                                                     \
          elptr = epptr = ba;                                                                                 \
                                                                                                              \
          for (j = 0; j < h; j++, elptr += bytes_per_line)                                                    \
            {                                                                                                 \
              iy = (dy * j) / h;                                                                              \
              epptr = elptr;                                                                                  \
              blptr = packed_colia + (iy * dimx);                                                             \
              for (i = 0; i < w; i++, epptr++)                                                                \
                {                                                                                             \
                  ix = (dx * i) / w;                                                                          \
                  bpptr = blptr + ix;                                                                         \
                  *epptr = pixel[*bpptr];                                                                     \
                }                                                                                             \
            }                                                                                                 \
        }                                                                                                     \
      else                                                                                                    \
        {                                                                                                     \
          nbytes = w * h;                                                                                     \
          epptr = ba;                                                                                         \
          bpptr = packed_colia;                                                                               \
          for (i = 0; i < nbytes; i++) *epptr++ = pixel[*bpptr++];                                            \
        }                                                                                                     \
    }                                                                                                         \
  else                                                                                                        \
    {                                                                                                         \
      if (dx != dimx || w != dx || h != dy || w != bytes_per_line)                                            \
        {                                                                                                     \
          elptr = epptr = ba;                                                                                 \
                                                                                                              \
          for (j = 0; j < h; j++, elptr += bytes_per_line)                                                    \
            {                                                                                                 \
              iy = (dy * j) / h;                                                                              \
              epptr = elptr;                                                                                  \
              ilptr = colia + (iy * dimx);                                                                    \
              for (i = 0; i < w; i++, epptr++)                                                                \
                {                                                                                             \
                  ix = (dx * i) / w;                                                                          \
                  ipptr = ilptr + ix;                                                                         \
                  *epptr = true_color ? *ipptr : pixel[FIX_COLORIND(*ipptr)];                                 \
                }                                                                                             \
            }                                                                                                 \
        }                                                                                                     \
      else                                                                                                    \
        {                                                                                                     \
          nbytes = w * h;                                                                                     \
          epptr = ba;                                                                                         \
          ipptr = colia;                                                                                      \
          for (i = 0; i < nbytes; i++, ipptr++) *epptr++ = true_color ? *ipptr : pixel[FIX_COLORIND(*ipptr)]; \
        }                                                                                                     \
    }                                                                                                         \
                                                                                                              \
  if (swapx)                                                                                                  \
    {                                                                                                         \
      ix = 0;                                                                                                 \
      w /= 2;                                                                                                 \
      for (j = 0; j < h; j++)                                                                                 \
        {                                                                                                     \
          for (i = 0; i < w; i++)                                                                             \
            {                                                                                                 \
              tmp = ba[i + ix];                                                                               \
              ba[i + ix] = ba[ix + w - i];                                                                    \
              ba[ix + w - i] = tmp;                                                                           \
            }                                                                                                 \
        }                                                                                                     \
      ix += bytes_per_line;                                                                                   \
    }                                                                                                         \
                                                                                                              \
  if (swapy)                                                                                                  \
    {                                                                                                         \
      tmpptr = (type *)gks_malloc(w * sizeof(type));                                                          \
                                                                                                              \
      elptr = ba;                                                                                             \
      epptr = ba + h * bytes_per_line;                                                                        \
      h /= 2;                                                                                                 \
                                                                                                              \
      for (j = 0; j < h; j++)                                                                                 \
        {                                                                                                     \
          epptr -= bytes_per_line;                                                                            \
          memmove(tmpptr, elptr, w * sizeof(type));                                                           \
          memmove(elptr, epptr, w * sizeof(type));                                                            \
          memmove(epptr, tmpptr, w * sizeof(type));                                                           \
          elptr += bytes_per_line;                                                                            \
        }                                                                                                     \
                                                                                                              \
      free(tmpptr);                                                                                           \
    }


static void copy32(int dx, int dy, int dimx, int *colia, int w, int h, int bytes_per_line, int *ba, Bool swapx,
                   Bool swapy, int true_color)
{
  COPY_BODY(int)
}


static void copy16(int dx, int dy, int dimx, int *colia, int w, int h, int bytes_per_line, short int *ba, Bool swapx,
                   Bool swapy, int true_color)
{
  COPY_BODY(short int)
}


static void copy8(int dx, int dy, int dimx, int *colia, int w, int h, int bytes_per_line, byte *ba, Bool swapx,
                  Bool swapy, int true_color)
{
  COPY_BODY(byte)
}


static void pixmap_to_bitmap(int w, int h, byte *ba)
{
  byte *pix, *mbuffer, *bbuffer, mvalue, *first;
  int i, j, k, graylevel, error, bit, row_size;
  int *lerr, *cerr, *terr, *error1, *error2;

  static unsigned char bit_flag[] = {1, 2, 4, 8, 16, 32, 64, 128};

  pix = ba;
  for (j = 0; j < h; j++)
    for (i = 0; i < w; i++)
      {
        *pix = (byte)(p->gray[*pix] * (WHITE - BLACK));
        pix++;
      }

  /* Allocate space for error arrays */

  error1 = (int *)calloc(w + 2, sizeof(int));
  error2 = (int *)calloc(w + 2, sizeof(int));
  bbuffer = mbuffer = (byte *)calloc(w * h, sizeof(byte));

  cerr = &error1[1];
  lerr = &error2[1];
  for (error = 0, i = 0; i < w;)
    { /* The top border */
      mvalue = 0x00;
      for (j = 0; (j < 8) && (i < w); j++, i++)
        {
          graylevel = (int)ba[i] + error;
          bit = graylevel > THRESH ? WHITE : BLACK;
          if (bit) mvalue |= (0x01 << j);
          error = graylevel - bit;
          lerr[i] = (THRESH - (int)bit) / 2;
        }
      *mbuffer++ = ~mvalue;
    }

  /*  Process the rest.            1 5 3
   *  Error distribution:          7 x
   */
  for (j = 1; j < h; j++)
    {
      pix = &ba[j * w];
      first = mbuffer;
      for (i = 0; i < w;)
        {
          mvalue = 0x00;
          for (bit = 0; (bit < 8) && (i < w); bit++, i++)
            {
              graylevel = (int)pix[i] + (int)(lerr[i - 1] + (5 * lerr[i]) + (3 * lerr[i + 1]) + (7 * cerr[i - 1])) / 16;
              if (graylevel > THRESH)
                {
                  mvalue |= (0x01 << bit);
                  cerr[i] = graylevel - WHITE;
                }
              else
                cerr[i] = graylevel;
            }
          *mbuffer++ = ~mvalue;
        }
      cerr[-1] = INITERR((int)pix[-1], (*first & 0x01));

      /* Swap error buffers */
      terr = error1;
      error1 = error2;
      error2 = terr;
      cerr = &error1[1];
      lerr = &error2[1];
    }

  row_size = (w + 7) / 8;
  for (j = 0; j < h; j++)
    for (i = 0; i < w; i++)
      {
        k = row_size * j + i / 8;
        if (*(bbuffer + k) & bit_flag[i % 8])
          *(ba + k) |= bit_flag[i % 8];
        else
          *(ba + k) &= ~bit_flag[i % 8];
      }

  free(bbuffer);
  free(error2);
  free(error1);
}


static void int64to32(int *a, int length)
{
  int i;
  char *from, *to;

  to = from = (char *)a;
  for (i = 0; i < length; i++)
    {
      to[0] = from[7];
      to[1] = from[6];
      to[2] = from[5];
      to[3] = from[4];
      to += 4;
      from += 8;
    }
}


static void int64to16(short int *a, int length)
{
  int i;
  char *from, *to;

  to = from = (char *)a;
  for (i = 0; i < length; i++)
    {
      to[0] = from[7];
      to[1] = from[6];
      to += 2;
      from += 8;
    }
}


static void cell_array(double xmin, double xmax, double ymin, double ymax, int dx, int dy, int dimx, int *colia,
                       int true_color)
{
  XImage *image = NULL;

  double x1, y1, x2, y2;
  int ix1, ix2, iy1, iy2;
  int x, y, w, h, bytes_per_line = 0, bitmap_pad, size;
  byte *ba = NULL;
  Bool swapx, swapy;

  WC_to_NDC(xmin, ymax, gksl->cntnr, x1, y1);
  seg_xform(&x1, &y1);
  NDC_to_DC(x1, y1, ix1, iy1);

  WC_to_NDC(xmax, ymin, gksl->cntnr, x2, y2);
  seg_xform(&x2, &y2);
  NDC_to_DC(x2, y2, ix2, iy2);

  w = abs(ix2 - ix1);
  h = abs(iy2 - iy1);
  if (w == 0 || h == 0) return;
  x = min(ix1, ix2);
  y = min(iy1, iy2);

#ifdef XSHM
  if (p->shmimage != NULL && w == p->width && h == p->height)
    {
      image = p->shmimage;
      ba = (byte *)p->shminfo.shmaddr;
      bytes_per_line = p->shmimage->bytes_per_line;
    }
#endif
  bitmap_pad = (p->depth > 16 ? 32 : (p->depth > 8 ? 16 : 8));
  if (image == NULL)
    {
      size = w * h * bitmap_pad / 8;
      if (sizeof(int) == 8) size *= 2;
      ba = (byte *)gks_malloc(size);
      bytes_per_line = w;
    }

  if (ba != NULL)
    {
      swapx = (xmin > xmax) ? True : False;
      swapy = (ymin < ymax) ? True : False;

      if (bitmap_pad == 32)
        {
          copy32(dx, dy, dimx, colia, w, h, bytes_per_line, (int *)ba, swapx, swapy, true_color);
          if (sizeof(int) == 8) int64to32((int *)ba, w * h);
        }
      else if (bitmap_pad == 16)
        {
          copy16(dx, dy, dimx, colia, w, h, bytes_per_line, (short int *)ba, swapx, swapy, true_color);
          if (sizeof(short int) == 8) int64to16((short int *)ba, w * h);
        }
      else
        copy8(dx, dy, dimx, colia, w, h, bytes_per_line, ba, swapx, swapy, true_color);

      if (p->depth == 1) pixmap_to_bitmap(w, h, ba);

      if (true_color && bitmap_pad == 32)
        {
          draw_image(x, y, w, h, ba, True);
          return;
        }

#ifdef XSHM
      if (image != NULL)
        {
          XShmPutImage(p->dpy, p->win, p->gc, image, 0, 0, x, y, w, h, True);
          XSync(p->dpy, False);
          return;
        }
#endif
      image = XCreateImage(p->dpy, p->vis, p->depth, p->depth == 1 ? XYBitmap : ZPixmap, 0, (char *)ba, w, h,
                           bitmap_pad, 0);
      if (image)
        {
          if (p->pixmap) XPutImage(p->dpy, p->pixmap, p->gc, image, 0, 0, x, y, w, h);
          if (p->selection) XPutImage(p->dpy, p->drawable, p->gc, image, 0, 0, x, y, w, h);
          if (!p->double_buf) XPutImage(p->dpy, p->win, p->gc, image, 0, 0, x, y, w, h);
          XSync(p->dpy, False);
          /*
           * Note: `XDestroyImage' frees both the image structure and the
           * data pointed to by the image structure (ba)
           */
#ifdef VMS
          free(ba);
#endif
          XDestroyImage(image);
        }
      else
        gks_perror("unable to create a %dx%d image", w, h);
    }
  else
    gks_perror("can't allocate %dx%d data array", w, h);
}


static void resize_window(void)
{
  int x, y, width, height;

  if (p->uil < 0)
    {
      width = nint((p->viewport[1] - p->viewport[0]) / p->resolution * p->magnification);
      height = nint((p->viewport[3] - p->viewport[2]) / p->resolution * p->magnification);
    }
  else
    {
      width = nint((p->viewport[1] - p->viewport[0]) * 100);
      height = nint((p->viewport[3] - p->viewport[2]) * 100);
    }

  x = 4 + nint(p->viewport[0] / p->resolution);
  y = p->sheight - height - 4 - nint(p->viewport[2] / p->resolution);

  if (width != p->width || height != p->height || x != p->x || y != p->y)
    {
      p->x = x;
      p->y = y;
      p->width = width;
      p->height = height;

      if (p->new_win)
        {
          XMoveWindow(p->dpy, p->win, p->x, p->y);
          XResizeWindow(p->dpy, p->win, p->width, p->height);
        }
      else
        XResizeWindow(p->dpy, p->win, p->width, p->height);

      if (p->pixmap)
        {
          XFreePixmap(p->dpy, p->pixmap);
          p->pixmap = XCreatePixmap(p->dpy, XRootWindowOfScreen(p->screen), p->width, p->height, p->depth);
          XFillRectangle(p->dpy, p->pixmap, p->clear, 0, 0, p->width, p->height);
        }
      if (p->drawable)
        {
          XFreePixmap(p->dpy, p->drawable);
          p->drawable = XCreatePixmap(p->dpy, XRootWindowOfScreen(p->screen), p->width, p->height, p->depth);
          XFillRectangle(p->dpy, p->drawable, p->clear, 0, 0, p->width, p->height);
        }
#ifdef XSHM
      free_shared_memory();
      create_shared_memory();
#endif
    }
}


static void display_cursor(int x, int y)
{
  int xorg, yorg, width, height;
  int dx, dy, r, d;
  char str[16];

  if (x == Undefined && y == Undefined) return;

  switch (p->type)
    {
    case TypeNone:
    case TypeCross:
      break;

    case TypeLocal:
    case TypeCrosshair:
      XDrawLine(p->dpy, p->win, p->invert, 0, y, p->width, y);
      XDrawLine(p->dpy, p->win, p->invert, x, 0, x, p->height);
      break;

    case TypeRubberband:
      XDrawLine(p->dpy, p->win, p->invert, p->px, p->py, x, y);
      break;

    case TypeRectangle:
      xorg = min(p->px, x);
      yorg = min(p->py, y);
      width = abs(p->px - x);
      height = abs(p->py - y);
      XDrawRectangle(p->dpy, p->win, p->invert, xorg, yorg, width, height);
      break;

    case TypeDigital:
      sprintf(str, "(%d %d)", x, y);
      x_draw_string(p->dpy, p->win, p->invert, p->px, p->py, str, strlen(str));
      break;

    case TypeCircle:
      dx = p->px - x;
      dy = p->py - y;
      r = (int)(sqrt((double)(dx * dx + dy * dy)) + 0.5);
      d = 2 * r;
      if (r != 0) XDrawArc(p->dpy, p->win, p->invert, p->px - r, p->py - r, d, d, 0, 360 * 64);
      break;
    }
}


static void get_pointer(int *n, double *x, double *y, int *state, int *term)
{
  Window focus, root_win, child_win;
  XEvent event;
  int np, revert, xold, yold;
  KeySym keysym;
  char str[10];
  int inc, xcur, ycur, xwin, ywin;
  static XComposeStatus status = {NULL, 0};
  unsigned int mask, old_mask;
  int event_mask =
      ButtonPressMask | PointerMotionMask | KeyPressMask | KeyReleaseMask | StructureNotifyMask | ExposureMask;

  XGetInputFocus(p->dpy, &focus, &revert);

  XDefineCursor(p->dpy, p->win, p->cursor);
  XRaiseWindow(p->dpy, p->win);

  if (p->new_win)
    {
      while (XCheckTypedWindowEvent(p->dpy, p->win, ConfigureNotify, &event))
        ;
    }

  np = 0;

  xold = p->px;
  yold = p->py;
  old_mask = 0;
  if (xold == Undefined || yold == Undefined)
    {
      XQueryPointer(p->dpy, p->win, &root_win, &child_win, &xcur, &ycur, &xold, &yold, &mask);
    }
  display_cursor(xold, yold);

  XSelectInput(p->dpy, p->win, event_mask);

  *term = 0;

  do
    {
      *state = Undefined;
      inc = 1;

#ifndef _WIN32
      if (p->wstype == 212)
        {
          struct timespec delay = {0, 200000000}; /* 200 ms */
          nanosleep(&delay, NULL);
        }
#endif

      do
        {
          if (p->wstype == 212)
            {
              while (True)
                {
                  old_mask = mask;
                  XQueryPointer(p->dpy, p->win, &root_win, &child_win, &xcur, &ycur, &xwin, &ywin, &mask);
                  if (xwin != xold || ywin != yold || mask != old_mask) break;
                }

              switch (mask)
                {
                case Button1Mask:
                case Button2Mask:
                  event.xany.type = ButtonPress;
                  event.xbutton.button = (mask == Button1Mask) ? Button1 : Button2;
                  event.xbutton.x = xwin;
                  event.xbutton.y = ywin;
                  break;
                default:
                  event.xany.type = MotionNotify;
                  event.xmotion.x = xwin;
                  event.xmotion.y = ywin;
                }
            }
          else
            XWindowEvent(p->dpy, p->win, event_mask, &event);

          switch (event.xany.type)
            {
            case Expose:
              handle_expose_event(p);
              xold = yold = Undefined;
              break;

            case ButtonPress:
              xcur = event.xbutton.x;
              ycur = event.xbutton.y;

              DC_to_NDC(xcur, ycur, *x++, *y++);

              if (event.xbutton.button == Button1)
                {
                  np++;
                  *state = GKS_K_STATUS_OK;
                }
              else if (event.xbutton.button == Button2 || event.xbutton.button == Button3)
                {
                  *state = GKS_K_STATUS_NONE;
                }
              break;

            case MotionNotify:
              display_cursor(xold, yold);

              xcur = event.xmotion.x;
              ycur = event.xmotion.y;
              display_cursor(xcur, ycur);

              xold = xcur;
              yold = ycur;
              break;

            case KeyPress:
              xcur = xold;
              ycur = yold;
              display_cursor(xold, yold);

              XLookupString((XKeyEvent *)&event, str, 9, &keysym, &status);

              switch (keysym)
                {
                case XK_Shift_L:
                case XK_Shift_R:
                  inc = 10;
                  break;
                case XK_Left:
                  xcur -= inc;
                  break;
                case XK_Right:
                  xcur += inc;
                  break;
                case XK_Up:
                  ycur -= inc;
                  break;
                case XK_Down:
                  ycur += inc;
                  break;
                case XK_Control_L:
                case XK_Control_R:
                case XK_Caps_Lock:
                case XK_Shift_Lock:
                case XK_Meta_L:
                case XK_Meta_R:
                case XK_Alt_L:
                case XK_Alt_R:
                case XK_Multi_key:
                  break;

                default:
                  if (*str != CTRL_C && *str != CTRL_D && *str != CTRL_Z)
                    {
                      DC_to_NDC(xcur, ycur, *x++, *y++);
                      np++;
                      *state = GKS_K_STATUS_OK;
                    }
                  else
                    *state = GKS_K_STATUS_NONE;

                  *term = *str;
                  break;
                }

              XWarpPointer(p->dpy, None, p->win, 0, 0, 0, 0, xcur, ycur);

              display_cursor(xcur, ycur);

              xold = xcur;
              yold = ycur;
              break;

            case KeyRelease:
              XLookupString((XKeyEvent *)&event, str, 9, &keysym, &status);

              if (keysym == XK_Shift_L || keysym == XK_Shift_R) inc = 1;
              break;

            case ConfigureNotify:
              p->empty = False;
              configure_event((XConfigureEvent *)&event);
              if (p->empty) xold = yold = Undefined;

              *state = Undefined;
              break;
            }
        }
      while (*state < 0);
    }
  while (np < *n && *state != GKS_K_STATUS_NONE);

  display_cursor(xold, yold);

  XSetInputFocus(p->dpy, focus, revert, CurrentTime);
  XRaiseWindow(p->dpy, p->win);

  XUndefineCursor(p->dpy, p->win);
  XSync(p->dpy, False);

  *n = np;
  if (*n > 1) *state = GKS_K_STATUS_OK;

  p->px = xcur;
  p->py = ycur;
}


static int lookup_string(char *str)
{
  int i;
  char s1[3], s2[3];

  s1[0] = str[0];
  s1[1] = str[1];
  s1[2] = '\0';
  s2[0] = str[1];
  s2[1] = str[0];
  s2[2] = '\0';

  for (i = 0; i < n_key; i++)
    {
      if (strcmp(s1, key_bindings[i].seq) == 0 || strcmp(s2, key_bindings[i].seq) == 0 ||
          strcmp(s1, key_bindings[i].alt_seq) == 0 || strcmp(s2, key_bindings[i].alt_seq) == 0)
        return (key_bindings[i].ch);
    }

  return (0);
}


static int dispatch_character(XKeyEvent *event, char *text)
{
  KeySym keysym;
  static char str[10], seq[3];
  static XComposeStatus compose_status = {NULL, 0};
  static char recall_buffer[256] = "";

  x_draw_string(p->dpy, p->win, p->invert, p->px, p->py, text, strlen(text));

  if (event)
    {
      XLookupString(event, str, 9, &keysym, &compose_status);

#ifndef VMS
      if (keysym == XK_Multi_key)
        compose_status.chars_matched = 1;

      else if (keysym >= XK_space && keysym <= XK_asciitilde)
        {
          switch (compose_status.chars_matched)
            {
            case 1:
              seq[0] = (char)keysym;
              compose_status.chars_matched++;
              keysym = 0;
              break;

            case 2:
              seq[1] = (char)keysym;
              seq[2] = '\0';
              keysym = lookup_string(seq);

            default:
              compose_status.chars_matched = 0;
              break;
            }
        }
#endif
      if ((keysym >= XK_space && keysym <= XK_asciitilde) || (keysym >= XK_nobreakspace && keysym <= XK_ydiaeresis))
        {
          str[0] = (char)keysym;
          str[1] = '\0';
          strcat(text, str);
        }

      else if (keysym == XK_Delete)
        {
          if (*text) text[strlen(text) - 1] = '\0';
        }
      else if (keysym == XK_Up)
        strcpy(text, recall_buffer);

      x_draw_string(p->dpy, p->win, p->invert, p->px, p->py, text, strlen(text));

      if (keysym == XK_Return) strcpy(recall_buffer, text);
    }
  else
    keysym = XK_space;

  return (keysym);
}


static void draw_text_box(void)

{
  int x, y;

  x = p->px - 5;
  y = p->py + 5;

  XDrawLine(p->dpy, p->win, p->invert, x, y, x, y - 20);
  XDrawLine(p->dpy, p->win, p->invert, x, y, x + 100, y);
}


static void get_string(int *n, char *chars, int *state)
{
  XEvent event;
  Window focus;
  int done, revert;
  char text[256];
  int event_mask = ButtonPressMask | PointerMotionMask | KeyPressMask | ExposureMask;

  XGetInputFocus(p->dpy, &focus, &revert);

  XDefineCursor(p->dpy, p->win, p->textcursor);
  XRaiseWindow(p->dpy, p->win);

  draw_text_box();

  XSelectInput(p->dpy, p->win, event_mask);

  strcpy(text, "");
  done = FALSE;

  do
    {
      XWindowEvent(p->dpy, p->win, event_mask, &event);

      switch (event.type)
        {
        case Expose:
          handle_expose_event(p);
          break;

        case KeyPress:
          if (dispatch_character((XKeyEvent *)&event, text) == XK_Return)
            {
              strcpy(chars, text);
              done = TRUE;
            }
          break;

        case ButtonPress:
          if (event.xbutton.button == Button1)
            {
              strcpy(chars, text);
              done = TRUE;
            }
          break;
        }
    }

  while (!done && event.type != ButtonPress);

  *n = strlen(text);
  dispatch_character(NULL, text);

  draw_text_box();

  XSetInputFocus(p->dpy, focus, revert, CurrentTime);
  XRaiseWindow(p->dpy, p->win);

  XUndefineCursor(p->dpy, p->win);
  XSync(p->dpy, False);

  if (done)
    *state = GKS_K_STATUS_OK;
  else
    *state = GKS_K_STATUS_NONE;
}

#endif


#if !defined(NO_X11)

static void lock(void)
{
  if (p->new_win) pthread_mutex_lock(&p->mutex);
}

static void unlock(void)
{
  if (p->new_win) pthread_mutex_unlock(&p->mutex);
}

static void *event_loop(void *arg)
{
  ws_state_list *p = (ws_state_list *)arg;
  XEvent event;

  p->run = 1;
  while (p->run)
    {
      {
        struct timespec delay = {0, 10000000}; /* 10 ms */
        nanosleep(&delay, NULL);
      }

      if (idle && p->run)
        {
          if (pthread_mutex_trylock(&p->mutex) == 0)
            {
              if (XCheckTypedWindowEvent(p->dpy, p->win, Expose, &event))
                handle_expose_event(p);
              else if (XCheckTypedWindowEvent(p->dpy, p->win, ClientMessage, &event))
                {
                  if (event.xclient.data.l[0] == p->wmDeleteMessage)
                    {
                      if (p->master_thread != 0)
                        {
#ifdef SIGUSR1
                          pthread_kill(p->master_thread, SIGUSR1);
#endif
                          p->run = 0;
                        }
                    }
                }
              pthread_mutex_unlock(&p->mutex);
            }
        }
    }
  p->done = 1;

  pthread_exit(0);
}


void gks_x11plugin(int fctid, int dx, int dy, int dimx, int *ia, int lr1, double *r1, int lr2, double *r2, int lc,
                   char *chars, void **ptr)
{
  static int win = 0;

  idle = False;

  p = (ws_state_list *)*ptr;

  switch (function_id = fctid)
    {

    case 2:
      /*
       *  Open workstation
       */
      /* Get GKS state list address */

      gksl = (gks_state_list_t *)*ptr;

      gks_init_core(gksl);

      p = (ws_state_list *)gks_malloc(sizeof(ws_state_list));

      p->wkid = ia[0];

      p->packed_ca = gks_getenv("GKS_PACKED_CELL_ARRAY") ? True : False;
      p->double_buf = gks_getenv("GKS_DOUBLE_BUF") ? True : False;
      p->shape = gks_getenv("GKS_CONVEX_SHAPE") ? Convex : Complex;
      p->widget = (Widget)NULL;
      p->conid = ia[1];
      p->wstype = ia[2];
      p->gif = -1;
      p->rf = -1;
      p->uil = -1;
      p->frame = NULL;

      switch (p->wstype)
        {

        case 230:
        case 231:
        case 232:
        case 233:
          p->wstype -= 20;
        case 210:
        case 211:
        case 212:
        case 213:
          if ((unsigned)ia[1] >= 100 + 100)
            {
              p->widget = (Widget)*ptr;
            }
          break;

        case 214:
        case 215:
        case 216:
        case 218:
          break;

        case 217:
          p->frame = (Pixmap *)gks_malloc(MAX_PIXMAP * sizeof(Pixmap));
          p->nframes = 0;
          break;
        }

      p->ccolor = Undefined;

      p->error = NULL;

      if (open_display() == NULL)
        {
          free(p);
          ia[0] = ia[1] = 0;
          return;
        }

      if (points == NULL)
        {
          max_points = MAX_POINTS;
          points = (XPoint *)gks_malloc(max_points * sizeof(XPoint));
        }

      set_colors();
      allocate_colors();
#ifndef NO_XFT
      allocate_rendercolors();
#endif

      if (p->wstype == 215 || p->wstype == 218)
        p->gif = p->conid;
      else if (p->wstype == 214)
        p->rf = p->conid;
      else if (p->wstype == 216)
        {
          p->uil = p->conid;
          gks_write_file(p->uil, (void *)UIL_HEADER, sizeof(UIL_HEADER));
        }

      create_window(win);
      set_WM_hints();
      create_GC();
      create_pixmap();
#ifdef XSHM
      create_shared_memory();
#endif
      initialize_arrays();
#if !defined(NO_XFT) || defined(NO_FT)
      verify_font_capabilities();
#endif

      create_cursor();

      p->state = GKS_K_WS_INACTIVE;
      p->mapped = False;

      p->ltype = GKS_K_LINETYPE_SOLID;
      p->lwidth = 0;

      /* Setup default device transformation */

      p->viewport[0] = 0;
      p->viewport[1] = p->width * p->resolution;
      p->viewport[2] = 0;
      p->viewport[3] = p->height * p->resolution;

      p->window[0] = p->window[2] = 0;
      if (p->viewport[1] > p->viewport[3])
        {
          p->window[1] = 1;
          p->window[3] = p->viewport[3] / p->viewport[1];
        }
      else if (p->viewport[1] < p->viewport[3])
        {
          p->window[1] = p->viewport[1] / p->viewport[3];
          p->window[3] = 1;
        }
      else
        p->window[1] = p->window[3] = 1;

      setup_xform(p->window, p->viewport);

      p->selection = p->bb_update = False;
      p->num_bb = p->max_bb = 0;
      p->bb = p->bbox = NULL;
      p->bounding_box.x1 = p->bounding_box.y1 = 32767;
      p->bounding_box.x2 = p->bounding_box.y2 = -32767;
      p->drawable = 0;

      p->type = TypeLocal;
      p->px = Undefined;
      p->py = Undefined;

      /* Return state list address, screen width and height */

      *ptr = p;

      r1[0] = p->mwidth;
      r2[0] = p->mheight;
      ia[0] = p->swidth;
      ia[1] = p->sheight;

      p->xshm = ((char *)gks_getenv("GKS_XSHM") != NULL) ? True : False;

      if (p->new_win)
        win++;
      else
        XClearWindow(p->dpy, p->win);

      init_norm_xform();
      set_clipping(True);

      if (p->new_win)
        {
          pthread_mutex_init(&p->mutex, NULL);
          if (p->wstype == 210 || p->wstype == 211)
            {
              pthread_attr_init(&p->attr);
              pthread_attr_setdetachstate(&p->attr, PTHREAD_CREATE_DETACHED);
              if (pthread_create(&p->thread, &p->attr, event_loop, (void *)p)) perror("pthread_create");
              pthread_attr_destroy(&p->attr);
            }
        }
      break;

    case 3:
      /*
       *  Close workstation
       */
      if (p->new_win)
        {
          if (p->run)
            {
              /* pthread_join didn't work for any reason */
              p->done = 0;
              while (!p->done)
                {
                  p->run = 0;
                  {
                    struct timespec delay = {0, 10000000}; /* 10 ms */
                    nanosleep(&delay, NULL);
                  }
                }
            }
        }

      lock();
      if (p->gif >= 0)
        pixmap_to_gif();
      else if (p->rf >= 0)
        pixmap_to_rf();

      else if (p->frame)
        if (p->nframes > 1) pixmap_loop();

      if (p->new_win) unmap_window();
#ifdef XSHM
      free_shared_memory();
#endif
      if (p->pixmap) XFreePixmap(p->dpy, p->pixmap);
      if (p->drawable) XFreePixmap(p->dpy, p->drawable);
      if (p->bbox) free(p->bbox);

      free_GC();
#ifndef NO_XFT
      free_rendercolors();
#endif

      if (p->new_win)
        {
          XDestroyWindow(p->dpy, p->win);
          win--;

          XFreePixmap(p->dpy, p->icon_pixmap);
        }
      unlock();
      if (p->new_dpy) XCloseDisplay(p->dpy);

      if (p->new_win) pthread_mutex_destroy(&p->mutex);

      free(p);
      return;

    case 4:
      /*
       *  Activate workstation
       */
      p->state = GKS_K_WS_ACTIVE;
      break;

    case 5:
      /*
       *  Deactivate workstation
       */
      p->state = GKS_K_WS_INACTIVE;
      break;

    case 6:
      /*
       *  Clear workstation
       */
      lock();
      if (p->uil >= 0)
        pixmap_to_uil();

      else if (p->frame)
        {
          if (p->nframes != MAX_PIXMAP)
            {
              p->frame[p->nframes++] = p->pixmap;
              set_frame_header(p->nframes);
              create_pixmap();
            }
        }

      if (p->pixmap) XFillRectangle(p->dpy, p->pixmap, p->clear, 0, 0, p->width, p->height);
      if (p->drawable) XFillRectangle(p->dpy, p->drawable, p->clear, 0, 0, p->width, p->height);
      if (!p->double_buf) XClearWindow(p->dpy, p->win);

      p->empty = True;

      if (!p->frame) XSync(p->dpy, False);
      unlock();
      break;

    case 8:
      /*
       *  Update workstation
       */
      lock();
      if (p->double_buf && (ia[1] & GKS_K_PERFORM_FLAG)) handle_expose_event(p);

      update();

      if (p->state == GKS_K_WS_ACTIVE)
        {
          if (p->error)
            {
              gks_perror(p->error);
              p->error = NULL;
            }
        }
      unlock();
      break;

    case 10:
      /*
       *  Message
       */
      lock();
      if (!p->mapped) map_window();

      if (p->state == GKS_K_WS_ACTIVE) message(strlen(chars), chars);
      unlock();
      break;

    case 12:
      /*
       *  Polyline
       */
      lock();
      if (!p->mapped) map_window();

      if (p->state == GKS_K_WS_ACTIVE)
        {
          polyline(*ia, r1, r2);
        }
      unlock();
      break;

    case 13:
      /*
       *  Polymarker
       */
      lock();
      if (!p->mapped) map_window();

      if (p->state == GKS_K_WS_ACTIVE)
        {
          polymarker(*ia, r1, r2);
        }
      unlock();
      break;

    case 14:
      /*
       *  Text
       */
      lock();
      if (!p->mapped) map_window();

      if (p->state == GKS_K_WS_ACTIVE)
        {
          text(*r1, *r2, strlen(chars), chars);
        }
      unlock();
      break;

    case 15:
      /*
       *  Fill Area
       */
      lock();
      if (!p->mapped) map_window();

      if (p->state == GKS_K_WS_ACTIVE)
        {
          fill_area(*ia, r1, r2);
        }
      unlock();
      break;

    case 16:
    case DRAW_IMAGE:
      /*
       *  Cell Array
       */
      lock();
      if (!p->mapped) map_window();

      if (p->state == GKS_K_WS_ACTIVE)
        {
          int true_color = function_id == DRAW_IMAGE;

          cell_array(r1[0], r1[1], r2[0], r2[1], dx, dy, dimx, ia, true_color);
        }
      unlock();
      break;

    case 17:
      /*
       * GDP
       */
      gks_perror("GDP primitive not supported for X11");
      break;

    case 48:
      /*
       *  Set color representation
       */
      lock();
      if (ia[1] >= 0 && ia[1] < MAX_COLOR)
        {
          set_color_repr(ia[1], r1[0], r1[1], r1[2]);
          free_tile_patterns(ia[1]);
        }
      unlock();
      break;

    case 49:
      /*
       *  Set window
       */
      lock();
      setup_norm_xform(*ia, gksl->window[*ia], gksl->viewport[*ia]);
      set_clipping(True);
      unlock();
      break;

    case 50:
      /*
       *  Set viewport
       */
      lock();
      setup_norm_xform(*ia, gksl->window[*ia], gksl->viewport[*ia]);
      set_clipping(True);
      unlock();
      break;

    case 52:
      /*
       *  Select normalization transformation
       */
      lock();
      set_clipping(True);
      unlock();
      break;

    case 53:
      /*
       *  Set clipping indicator
       */
      lock();
      set_clipping(True);
      unlock();
      break;

    case 54:
      /*
       *  Set workstation window
       */
      lock();
      p->window[0] = r1[0];
      p->window[1] = r1[1];
      p->window[2] = r2[0];
      p->window[3] = r2[1];

      setup_xform(p->window, p->viewport);
      set_clipping(True);
      unlock();
      break;

    case 55:
      /*
       *  Set workstation viewport
       */
      {
        double max_width, max_height;

        lock();
        p->viewport[0] = r1[0];
        p->viewport[1] = r1[1];
        p->viewport[2] = r2[0];
        p->viewport[3] = r2[1];

        if (p->gif >= 0 || p->rf >= 0)
          {
            max_width = p->resolution * 1280;
            max_height = p->resolution * 1024;
          }
        else if (p->new_win)
          {
            max_width = p->mwidth;
            max_height = p->mheight;
          }
        else
          {
            max_width = 10.0;
            max_height = 10.0;
          }
        if (p->uil < 0) gks_fit_ws_viewport(p->viewport, max_width, max_height, 0.0075);

        resize_window();
        set_WM_hints();

        setup_xform(p->window, p->viewport);
        set_clipping(True);
        unlock();
        break;
      }

    case 69:
      /*
       *  Initialize locator
       */
      lock();
      p->type = (pe_type)ia[3];
      NDC_to_DC(r1[0], r2[0], p->px, p->py);
      unlock();
      break;

    case 81:
      /*
       *  Request locator
       */
      {
        int n;

        lock();
        if (!p->mapped) map_window();

        n = 1;
        get_pointer(&n, r1, r2, &ia[0], &ia[3]);
        unlock();
        break;
      }

    case 82:
      /*
       *  Request stroke
       */
      lock();
      if (!p->mapped) map_window();

      get_pointer(&ia[2], r1, r2, &ia[0], &ia[3]);
      unlock();
      break;

    case 86:
      /*
       *  Request string
       */
      lock();
      if (!p->mapped) map_window();
      get_string(&ia[1], chars, &ia[0]);
      unlock();
      break;

    case BEGIN_SELECTION:
      if (ia[0] != 1 && ia[0] != p->num_bb + 1)
        {
          gks_perror("invalid selection number");
          break;
        }
      lock();
      p->num_bb = ia[0];
      if (!p->drawable)
        {
          p->drawable = XCreatePixmap(p->dpy, XRootWindowOfScreen(p->screen), p->width, p->height, p->depth);
          XFillRectangle(p->dpy, p->drawable, p->clear, 0, 0, p->width, p->height);
        }
      if (p->num_bb > p->max_bb)
        {
          p->max_bb += MAX_SELECTIONS;
          p->bbox = (Segment *)realloc(p->bbox, p->max_bb * sizeof(Segment));
        }
      p->selection = p->bb_update = True;
      p->bb = p->bbox + p->num_bb - 1;
      p->bb->type = ia[1];
      p->bb->x1 = p->bb->y1 = 32767;
      p->bb->x2 = p->bb->y2 = -32767;
      unlock();
      break;

    case END_SELECTION:
      lock();
      draw_bbox(0, 0, 0);
      p->selection = p->bb_update = False;
      unlock();
      break;

    case MOVE_SELECTION:
      lock();
      if (p->drawable)
        {
          int i, xoff, yoff;

          xoff = sint(p->a * r1[0] + 0.5);
          yoff = sint(p->c * r2[0] + 0.5);

          set_clipping(False);
          XCopyArea(p->dpy, p->pixmap, p->win, p->gc, 0, 0, p->width, p->height, 0, 0);
          for (i = 0; i < p->num_bb; i++)
            {
              p->bb = p->bbox + i;
              XCopyArea(p->dpy, p->drawable, p->win, p->gc, p->bb->x1, p->bb->y1, p->bb->x2 - p->bb->x1 + 1,
                        p->bb->y2 - p->bb->y1 + 1, p->bb->x1 + xoff, p->bb->y1 + yoff);
            }
          set_clipping(True);
        }
      unlock();
      break;

    case RESIZE_SELECTION:
      lock();
      if (p->drawable)
        {
          int i, xoff, yoff;

          xoff = sint(p->a * r1[0] + 0.5);
          yoff = sint(p->c * r2[0] + 0.5);

          set_clipping(False);
          XCopyArea(p->dpy, p->pixmap, p->win, p->gc, 0, 0, p->width, p->height, 0, 0);
          for (i = 0; i < p->num_bb; i++)
            {
              p->bb = p->bbox + i;
              draw_bbox(ia[0], xoff, yoff);
            }
          set_clipping(True);
        }
      unlock();
      break;

    case INQ_BBOX:
      if (p->selection)
        p->bb = p->bbox + p->num_bb - 1;
      else
        p->bb = &p->bounding_box;

      DC_to_NDC(p->bb->x1, p->bb->y2, r1[0], r2[0]);
      DC_to_NDC(p->bb->x2, p->bb->y1, r1[1], r2[1]);

      if (!p->selection)
        {
          p->bb->x1 = p->bb->y1 = 32767;
          p->bb->x2 = p->bb->y2 = -32767;
        }
      p->bb_update = True;
      break;
    }

  idle = True;
}

#else

void gks_x11plugin(int fctid, int dx, int dy, int dimx, int *ia, int lr1, double *r1, int lr2, double *r2, int lc,
                   char *chars, void **ptr)
{
  if (fctid == 2)
    {
      gks_perror("X11 support not compiled in");
      ia[0] = 0;
    }
}

#endif
