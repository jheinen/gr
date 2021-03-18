#ifdef __unix__
#define _XOPEN_SOURCE
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>
#ifdef _MSC_VER
typedef __int64 int64_t;
#else
#include <stdint.h>
#endif
#include <signal.h>

#if !defined(VMS) && !defined(_WIN32)
#include <unistd.h>
#endif

#ifdef _WIN32
#include <windows.h>
#include <wchar.h>
#define TMPDIR "C:\\Users\\%USERNAME%\\AppData\\Local\\Temp"
#define DIRDELIM "\\"
#define WIN32_LEAN_AND_MEAN 1
#else
#define TMPDIR "/tmp"
#define DIRDELIM "/"
#endif

#ifdef isnan
#define is_nan(a) isnan(a)
#else
#define is_nan(x) ((x) != (x))
#endif

#ifndef NAN
#define NAN (0.0 / 0.0)
#endif

#include "gks.h"
#include "gkscore.h"
#include "gr.h"
#include "gr_version.h"
#include "text.h"
#include "spline.h"
#include "gridit.h"
#include "contour.h"
#include "contourf.h"
#include "strlib.h"
#include "io.h"
#include "md5.h"
#include "cm.h"
#include "boundary.h"

#ifndef R_OK
#define R_OK 4
#endif

#define GR_UNUSED(param) (void)param

#ifndef M_PI
#define M_PI (3.141592653589793)
#endif

typedef struct
{
  int index;
  double red, green, blue;
} color_t;

typedef struct
{
  double xmin, xmax, ymin, ymax;
} rect_t;

typedef struct
{
  double a, b, c, d;
} norm_xform;

typedef struct
{
  int scale_options;
  double xmin, xmax, ymin, ymax, zmin, zmax, a, b, c, d, e, f;
} linear_xform;

typedef struct
{
  double zmin, zmax;
  int phi, delta;
  double a1, a2, b, c1, c2, c3, d;
} world_xform;

typedef struct
{
  double camera_pos_x, camera_pos_y, camera_pos_z;
  double up_x, up_y, up_z;
  double focus_point_x, focus_point_y, focus_point_z;
  double s_x, s_y, s_z;
  double x_axis_scale, y_axis_scale, z_axis_scale;
} transformation_xform;

typedef struct
{
  double left, right, bottom, top;
  double near_plane, far_plane, fov; /* only used for perspectiv */
  int projection_type;
} projection_xform;

typedef struct
{
  double xmin, xmax, ymin, ymax, zmin, zmax;
} interaction_xform;

typedef struct
{
  double x, y, z;
} point_3d;

typedef struct
{
  int sign;
  double x0, x1, y0, y1, z0, z1;
  double xmin, xmax;
  int initialize;
  double *buf, *ymin, *ymax;
} hlr_t;

typedef struct
{
  int ltype;
  double lwidth;
  int plcoli;
  int mtype;
  double mszsc;
  int pmcoli;
  int txfont, txprec;
  double chxp;
  double chsp;
  int txcoli;
  double chh;
  double chup[2];
  int txp;
  int txal[2];
  int ints;
  int styli;
  int facoli;
  int tnr;
  double wn[4], vp[4];
  int scale_options;
  double bwidth;
  int bcoli;
  int clip_tnr;
} state_list;

typedef struct
{
  int a, b, c;
  double sp;
} triangle_with_distance;

static norm_xform nx = {1, 0, 1, 0};

static linear_xform lx = {0, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0};

static world_xform wx = {0, 1, 60, 60, 0, 0, 0, 0, 0, 0, 0};

static transformation_xform tx = {0, 2, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 1, 1, 1};

static projection_xform gpx = {-1, 1, -1, 1, 1, 0, 45, GR_PROJECTION_DEFAULT};

static interaction_xform ix = {-1, 1, -1, 1, -1, 1};

static hlr_t hlr = {1, 0, 1, 0, 1, 0, 1, 0, 1, 1, NULL, NULL, NULL};

static int predef_colors[20] = {9, 2, 0, 1, 16, 3, 15, 8, 6, 10, 11, 4, 12, 13, 14, 7, 5, 17, 18, 19};

#define MAX_SAVESTATE 16

static state_list *state = NULL;

#define MAX_CONTEXT 8

static state_list *ctx, *app_context[MAX_CONTEXT] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};

static void (*previous_handler)(int);

static int autoinit = 1, double_buf = 0, state_saved = 0, def_color = 0;

static const char *display = NULL;

static double vxmin = 0.2, vxmax = 0.9, vymin = 0.2, vymax = 0.9;

static double cxl, cxr, cyf, cyb, czb, czt;

static int arrow_style = 0;

static double arrow_size = 1;

static int flag_printing = 0, flag_graphics = 0;

#define DEFAULT_FIRST_COLOR 8
#define DEFAULT_LAST_COLOR 79

static int first_color = DEFAULT_FIRST_COLOR, last_color = DEFAULT_LAST_COLOR;

#define MAX_COLOR 1256

static unsigned int rgb[MAX_COLOR], used[MAX_COLOR];

#define MAX_TICKS 500

#define check_autoinit \
  if (autoinit) initgks()

#define check_tick_marks(amin, amax, atick, axis)             \
  do                                                          \
    if ((amax - amin) / atick > MAX_TICKS)                    \
      {                                                       \
        atick = gr_tick(amin, amax);                          \
        fprintf(stderr, "auto-adjust %c tick marks\n", axis); \
      }                                                       \
  while (0)

#define nint(x) (int)((x) + 0.5)
#define round(x) (x < 0 ? ceil(x - .5) : floor(x + .5))
#define iround(x) ((int)round(x))
#define gauss(x) floor(x)
#define igauss(x) ((int)gauss(x))
#define trunc(x) (((x) > 0) ? floor(x) : ceil(x))

#define NDC 0
#define WC 1

#define POINT_INC 2048

/* Path definitions */
#define STOP 0
#define MOVETO 1
#define LINETO 2
#define CURVE3 3
#define CURVE4 4
#define CLOSEPOLY 0x4f

#define RESOLUTION_X 4096
#define BACKGROUND 0

#ifndef FLT_MAX
#define FLT_MAX 1.701411735e+38
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define OPTION_X_LOG (1 << 0)
#define OPTION_Y_LOG (1 << 1)
#define OPTION_Z_LOG (1 << 2)

#define OPTION_FLIP_X (1 << 3)
#define OPTION_FLIP_Y (1 << 4)
#define OPTION_FLIP_Z (1 << 5)

#define LEFT (1 << 0)
#define RIGHT (1 << 1)
#define FRONT (1 << 2)
#define BACK (1 << 3)
#define BOTTOM (1 << 4)
#define TOP (1 << 5)

#define SPEC_LINE (1 << 0)
#define SPEC_MARKER (1 << 1)
#define SPEC_COLOR (1 << 2)

#define XML_HEADER "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>\n"
#define GR_HEADER "<gr>\n"
#define GR_TRAILER "</gr>\n"

typedef enum
{
  OPTION_LINES,
  OPTION_MESH,
  OPTION_FILLED_MESH,
  OPTION_Z_SHADED_MESH,
  OPTION_COLORED_MESH,
  OPTION_CELL_ARRAY,
  OPTION_SHADED_MESH
} surface_option_t;

typedef enum
{
  MODEL_RGB,
  MODEL_HSV
} color_model_t;

typedef struct
{
  char *format;
  double width, height;
} format_t;

#ifndef max
#define max(a, b) ((a) > (b) ? (a) : (b))
#endif

#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif

#define arc(angle) (M_PI * (angle) / 180.0)
#define deg(rad) ((rad)*180.0 / M_PI)

static unsigned char *opcode = NULL;

static double *xpath = NULL, *xpoint = NULL, *ypath = NULL, *ypoint = NULL, *zpoint = NULL;

static int npoints = 0, maxpath = 0, npath = 0;

static int *code = NULL;

/*  0 - 20    21 - 45    46 - 70    71 - 90           rot/  */
static int rep_table[16][3] = {
    /*   tilt  */
    {2, 2, 2}, {2, 2, 2}, {1, 3, 2}, {1, 3, 3}, /*  0 - 20 */
    {2, 0, 0}, {2, 0, 0}, {1, 3, 2}, {1, 3, 1}, /* 21 - 45 */
    {2, 0, 0}, {0, 0, 0}, {1, 3, 0}, {1, 3, 1}, /* 46 - 70 */
    {0, 0, 0}, {0, 0, 0}, {1, 3, 0}, {1, 3, 1}  /* 71 - 90 */
};

static int axes_rep[4][3] = {{1, 1, 1}, {1, 1, 2}, {0, 0, 0}, {0, 0, 3}};

static int angle[4] = {20, 45, 70, 90};

static format_t formats[] = {
    {"A4", 0.210, 0.297},        {"B5", 0.176, 0.250},      {"Letter", 0.216, 0.279}, {"Legal", 0.216, 0.356},
    {"Executive", 0.191, 0.254}, {"A0", 0.841, 1.189},      {"A1", 0.594, 0.841},     {"A2", 0.420, 0.594},
    {"A3", 0.297, 0.420},        {"A5", 0.148, 0.210},      {"A6", 0.105, 0.148},     {"A7", 0.074, 0.105},
    {"A8", 0.052, 0.074},        {"A9", 0.037, 0.052},      {"B0", 1.000, 1.414},     {"B1", 0.500, 0.707},
    {"B10", 0.031, 0.044},       {"B2", 0.500, 0.707},      {"B3", 0.353, 0.500},     {"B4", 0.250, 0.353},
    {"B6", 0.125, 0.176},        {"B7", 0.088, 0.125},      {"B8", 0.062, 0.088},     {"B9", 0.044, 0.062},
    {"C5E", 0.163, 0.229},       {"Comm10E", 0.105, 0.241}, {"DLE", 0.110, 0.220},    {"Folio", 0.210, 0.330},
    {"Ledger", 0.432, 0.279},    {"Tabloid", 0.279, 0.432}, {NULL, 0.000, 0.000}};

static int vertex_list[18][25] = {
    {3, -10, 80, 0, 100, 10, 80, 2, 0, 100, 0, -100, 0},
    {3, -7, 80, 0, 100, 7, 80, 2, 0, 100, 0, -100, 0},
    {6, 0, 85, -10, 80, 0, 100, 10, 80, 0, 85, 0, -100, 0},
    {-4, 0, 85, -10, 80, 0, 100, 10, 80, 2, 0, 85, 0, -100, 0},
    {6, 0, 80, -10, 80, 0, 100, 10, 80, 0, 80, 0, -100, 0},
    {-4, 0, 80, -10, 80, 0, 100, 10, 80, 2, 0, 80, 0, -100, 0},
    {6, 0, 75, -10, 80, 0, 100, 10, 80, 0, 75, 0, -100, 0},
    {-4, 0, 75, -10, 80, 0, 100, 10, 80, 2, 0, 75, 0, -100, 0},
    {3, -10, 80, 0, 100, 10, 80, 2, 0, 100, 0, -100, 3, -10, -80, 0, -100, 10, -80, 0},
    {3, -7, 80, 0, 100, 7, 80, 2, 0, 100, 0, -100, 3, -7, -80, 0, -100, 7, -80, 0},
    {10, 0, 85, -10, 80, 0, 100, 10, 80, 0, 85, 0, -85, -10, -80, 0, -100, 10, -80, 0, -85, 0},
    {-4, 0, 85, -10, 80, 0, 100, 10, 80, 2, 0, 85, 0, -85, -4, -10, -80, 0, -100, 10, -80, 0, -85, 0},
    {10, 0, 80, -10, 80, 0, 100, 10, 80, 0, 80, 0, -80, -10, -80, 0, -100, 10, -80, 0, -80, 0},
    {-4, 0, 80, -10, 80, 0, 100, 10, 80, 2, 0, 80, 0, -80, -4, -10, -80, 0, -100, 10, -80, 0, -80, 0},
    {10, 0, 75, -10, 80, 0, 100, 10, 80, 0, 75, 0, -75, -10, -80, 0, -100, 10, -80, 0, -75, 0},
    {-4, 0, 75, -10, 80, 0, 100, 10, 80, 2, 0, 75, 0, -75, -4, -10, -80, 0, -100, 10, -80, 0, -75, 0},
    {3, -10, 80, 0, 100, 10, 80, 2, -1, 98, -1, -100, 2, 1, 98, 1, -100, 0},
    {3, -10, 80, 0, 100, 10, 80, 2, -1, 98, -1, -98, 3, -10, -80, 0, -100, 10, -80, 2, 1, 98, 1, -98, 0}};

static int colormap = 0;

static int cmap[48][72] = {
    {/* COLORMAP_UNIFORM */
     0x2020df, 0x3020df, 0x4020df, 0x5020df, 0x6020df, 0x7120df, 0x8020df, 0x9120df, 0xa120df, 0xb120df, 0xc120df,
     0xd120df, 0xdf20de, 0xdf20cd, 0xdf20bd, 0xdf20ad, 0xdf209d, 0xdf208d, 0xdf207d, 0xdf206d, 0xdf205d, 0xdf204c,
     0xdf203c, 0xdf202c, 0xdf2420, 0xdf3420, 0xdf4420, 0xdf5420, 0xdf6420, 0xdf7520, 0xdf8420, 0xdf9420, 0xdfa520,
     0xdfb520, 0xdfc520, 0xdfd520, 0xd9df20, 0xc9df20, 0xb9df20, 0xa9df20, 0x99df20, 0x89df20, 0x79df20, 0x69df20,
     0x59df20, 0x49df20, 0x38df20, 0x28df20, 0x20df28, 0x20df38, 0x20df48, 0x20df58, 0x20df68, 0x20df78, 0x20df88,
     0x20df98, 0x20dfa9, 0x20dfb9, 0x20dfc9, 0x20dfd9, 0x20d6df, 0x20c6df, 0x20b5df, 0x20a5df, 0x2095df, 0x2085df,
     0x2075df, 0x2065df, 0x2055df, 0x2045df, 0x2034df, 0x2024df},

    {/* COLORMAP_TEMPERATURE */
     0x000080, 0x000090, 0x0000a0, 0x0000b1, 0x0000c1, 0x0000d1, 0x0000e1, 0x0000f2, 0x0000ff, 0x0002ff, 0x0011ff,
     0x001fff, 0x002dff, 0x003cff, 0x004aff, 0x0058ff, 0x0066ff, 0x0074ff, 0x0084ff, 0x0092ff, 0x00a0ff, 0x00afff,
     0x00bdff, 0x00cbff, 0x00daff, 0x01e7f5, 0x0df7ea, 0x19ffde, 0x25ffd2, 0x31ffc7, 0x3bffbb, 0x47ffb0, 0x53ffa4,
     0x5fff98, 0x6aff8d, 0x76ff81, 0x81ff76, 0x8dff6a, 0x98ff5f, 0xa4ff53, 0xb0ff47, 0xbbff3b, 0xc7ff31, 0xd2ff25,
     0xdeff19, 0xeaff0d, 0xf5f701, 0xffeb00, 0xffdd00, 0xffd000, 0xffc200, 0xffb500, 0xffa800, 0xff9b00, 0xff8d00,
     0xff8000, 0xff7200, 0xff6500, 0xff5800, 0xff4a00, 0xff3d00, 0xff3000, 0xff2300, 0xff1500, 0xf20800, 0xe20000,
     0xd10000, 0xc10000, 0xb10000, 0xa00000, 0x900000, 0x800000},

    {/* COLORMAP_GRAYSCALE */
     0x000000, 0x040404, 0x070707, 0x0b0b0b, 0x0e0e0e, 0x121212, 0x161616, 0x191919, 0x1d1d1d, 0x202020, 0x242424,
     0x282828, 0x2b2b2b, 0x2f2f2f, 0x323232, 0x363636, 0x393939, 0x3d3d3d, 0x414141, 0x444444, 0x484848, 0x4b4b4b,
     0x4f4f4f, 0x535353, 0x565656, 0x5a5a5a, 0x5d5d5d, 0x616161, 0x656565, 0x686868, 0x6c6c6c, 0x6f6f6f, 0x737373,
     0x777777, 0x7a7a7a, 0x7e7e7e, 0x808080, 0x848484, 0x878787, 0x8b8b8b, 0x8f8f8f, 0x929292, 0x969696, 0x999999,
     0x9d9d9d, 0xa1a1a1, 0xa4a4a4, 0xa8a8a8, 0xababab, 0xafafaf, 0xb3b3b3, 0xb6b6b6, 0xbababa, 0xbdbdbd, 0xc1c1c1,
     0xc5c5c5, 0xc8c8c8, 0xcccccc, 0xcfcfcf, 0xd3d3d3, 0xd6d6d6, 0xdadada, 0xdedede, 0xe1e1e1, 0xe5e5e5, 0xe8e8e8,
     0xececec, 0xf0f0f0, 0xf3f3f3, 0xf7f7f7, 0xfafafa, 0xfefefe},

    {/* COLORMAP_GLOWING */
     0x000000, 0x580400, 0x690700, 0x730b00, 0x7c0e00, 0x831200, 0x891600, 0x8f1900, 0x941d00, 0x982000, 0x9c2400,
     0xa02800, 0xa32b00, 0xa72f00, 0xaa3200, 0xad3601, 0xaf3901, 0xb23d01, 0xb54101, 0xb74401, 0xba4802, 0xbc4b02,
     0xbe4f02, 0xc05303, 0xc25603, 0xc45a04, 0xc65d04, 0xc86105, 0xca6506, 0xcc6807, 0xce6c08, 0xcf6f09, 0xd1730a,
     0xd2770c, 0xd47a0d, 0xd67e0f, 0xd78010, 0xd88413, 0xda8714, 0xdb8b17, 0xdd8f1a, 0xde921c, 0xdf961f, 0xe19921,
     0xe29d25, 0xe3a129, 0xe4a42c, 0xe6a830, 0xe7ab34, 0xe8af39, 0xeab33d, 0xeab643, 0xecba49, 0xedbd4e, 0xeec154,
     0xefc55a, 0xf0c861, 0xf1cc68, 0xf2cf70, 0xf3d378, 0xf4d680, 0xf5da89, 0xf6de92, 0xf7e19c, 0xf8e5a6, 0xf9e8b0,
     0xfaecbb, 0xfbf0c7, 0xfcf3d3, 0xfdf7e0, 0xfefaed, 0xfffefb},

    {/* COLORMAP_RAINBOWLIKE */
     0x800000, 0x8d0000, 0x9c0000, 0xaa0000, 0xb80000, 0xc70000, 0xd50000, 0xe40000, 0xf20000, 0xff0100, 0xff1000,
     0xff1e00, 0xff2c00, 0xff3b00, 0xff4900, 0xff5700, 0xff6600, 0xff7400, 0xff8200, 0xff9000, 0xff9e00, 0xffad00,
     0xffbb00, 0xffc900, 0xffd800, 0xffe600, 0xfff500, 0xfbff04, 0xedff12, 0xdeff21, 0xd0ff2f, 0xc2ff3d, 0xb3ff4c,
     0xa5ff5a, 0x97ff68, 0x88ff77, 0x7bff84, 0x6cff93, 0x5effa1, 0x50ffaf, 0x41ffbe, 0x33ffcc, 0x25ffda, 0x16ffe9,
     0x08fff7, 0x00f8ff, 0x00eaff, 0x00dcff, 0x00cdff, 0x00bfff, 0x00b1ff, 0x00a2ff, 0x0094ff, 0x0086ff, 0x0078ff,
     0x006aff, 0x005bff, 0x004dff, 0x003fff, 0x0030ff, 0x0022ff, 0x0014ff, 0x0005ff, 0x0000f6, 0x0000e8, 0x0000d9,
     0x0000cb, 0x0000bc, 0x0000ae, 0x0000a0, 0x000091, 0x000083},

    {/* COLORMAP_GEOLOGIC */
     0x5555ff, 0x5158f8, 0x4e5af1, 0x4a5de9, 0x475fe2, 0x4362db, 0x3f65d4, 0x3c68cd, 0x386bc6, 0x356dbe, 0x3170b7,
     0x2d73b0, 0x2a75a9, 0x2678a2, 0x237a9a, 0x1f7d93, 0x1c808c, 0x188385, 0x14867f, 0x118878, 0x0d8b70, 0x0a8d69,
     0x069062, 0x02935b, 0x039556, 0x0d985a, 0x189a5d, 0x239d61, 0x2ea065, 0x38a368, 0x43a66c, 0x4ea86f, 0x59ab73,
     0x64ae77, 0x6eb07a, 0x79b37e, 0x83b580, 0x8eb884, 0x98bb87, 0xa3be8b, 0xaec18f, 0xb9c392, 0xc4c696, 0xcec899,
     0xd9cb9d, 0xe4cea1, 0xefd0a4, 0xf9d3a8, 0xfed2a6, 0xfacd9f, 0xf6c798, 0xf3c291, 0xefbc89, 0xecb782, 0xe8b27c,
     0xe4ac75, 0xe1a76e, 0xdda267, 0xda9c5f, 0xd69758, 0xd39251, 0xcf8c4a, 0xcb8743, 0xc8823b, 0xc47c34, 0xc1772d,
     0xbd7126, 0xb96c1f, 0xb66718, 0xb26110, 0xaf5c09, 0xab5602},

    {/* COLORMAP_GREENSCALE */
     0x000000, 0x045800, 0x076900, 0x0b7300, 0x0e7c00, 0x128300, 0x168900, 0x198f00, 0x1d9400, 0x209800, 0x249c00,
     0x28a000, 0x2ba300, 0x2fa700, 0x32aa00, 0x36ad01, 0x39af01, 0x3db201, 0x41b501, 0x44b701, 0x48ba02, 0x4bbc02,
     0x4fbe02, 0x53c003, 0x56c203, 0x5ac404, 0x5dc604, 0x61c805, 0x65ca06, 0x68cc07, 0x6cce08, 0x6fcf09, 0x73d10a,
     0x77d20c, 0x7ad40d, 0x7ed60f, 0x80d710, 0x84d813, 0x87da14, 0x8bdb17, 0x8fdd1a, 0x92de1c, 0x96df1f, 0x99e121,
     0x9de225, 0xa1e329, 0xa4e42c, 0xa8e630, 0xabe734, 0xafe839, 0xb3ea3d, 0xb6ea43, 0xbaec49, 0xbded4e, 0xc1ee54,
     0xc5ef5a, 0xc8f061, 0xccf168, 0xcff270, 0xd3f378, 0xd6f480, 0xdaf589, 0xdef692, 0xe1f79c, 0xe5f8a6, 0xe8f9b0,
     0xecfabb, 0xf0fbc7, 0xf3fcd3, 0xf7fde0, 0xfafeed, 0xfefffb},

    {/* COLORMAP_CYANSCALE */
     0x000000, 0x005804, 0x006907, 0x00730b, 0x007c0e, 0x008312, 0x008916, 0x008f19, 0x00941d, 0x009820, 0x009c24,
     0x00a028, 0x00a32b, 0x00a72f, 0x00aa32, 0x01ad36, 0x01af39, 0x01b23d, 0x01b541, 0x01b744, 0x02ba48, 0x02bc4b,
     0x02be4f, 0x03c053, 0x03c256, 0x04c45a, 0x04c65d, 0x05c861, 0x06ca65, 0x07cc68, 0x08ce6c, 0x09cf6f, 0x0ad173,
     0x0cd277, 0x0dd47a, 0x0fd67e, 0x10d780, 0x13d884, 0x14da87, 0x17db8b, 0x1add8f, 0x1cde92, 0x1fdf96, 0x21e199,
     0x25e29d, 0x29e3a1, 0x2ce4a4, 0x30e6a8, 0x34e7ab, 0x39e8af, 0x3deab3, 0x43eab6, 0x49ecba, 0x4eedbd, 0x54eec1,
     0x5aefc5, 0x61f0c8, 0x68f1cc, 0x70f2cf, 0x78f3d3, 0x80f4d6, 0x89f5da, 0x92f6de, 0x9cf7e1, 0xa6f8e5, 0xb0f9e8,
     0xbbfaec, 0xc7fbf0, 0xd3fcf3, 0xe0fdf7, 0xedfefa, 0xfbfffe},

    {/* COLORMAP_BLUESCALE */
     0x000000, 0x000458, 0x000769, 0x000b73, 0x000e7c, 0x001283, 0x001689, 0x00198f, 0x001d94, 0x002098, 0x00249c,
     0x0028a0, 0x002ba3, 0x002fa7, 0x0032aa, 0x0136ad, 0x0139af, 0x013db2, 0x0141b5, 0x0144b7, 0x0248ba, 0x024bbc,
     0x024fbe, 0x0353c0, 0x0356c2, 0x045ac4, 0x045dc6, 0x0561c8, 0x0665ca, 0x0768cc, 0x086cce, 0x096fcf, 0x0a73d1,
     0x0c77d2, 0x0d7ad4, 0x0f7ed6, 0x1080d7, 0x1384d8, 0x1487da, 0x178bdb, 0x1a8fdd, 0x1c92de, 0x1f96df, 0x2199e1,
     0x259de2, 0x29a1e3, 0x2ca4e4, 0x30a8e6, 0x34abe7, 0x39afe8, 0x3db3ea, 0x43b6ea, 0x49baec, 0x4ebded, 0x54c1ee,
     0x5ac5ef, 0x61c8f0, 0x68ccf1, 0x70cff2, 0x78d3f3, 0x80d6f4, 0x89daf5, 0x92def6, 0x9ce1f7, 0xa6e5f8, 0xb0e8f9,
     0xbbecfa, 0xc7f0fb, 0xd3f3fc, 0xe0f7fd, 0xedfafe, 0xfbfeff},

    {/* COLORMAP_MAGENTASCALE */
     0x000000, 0x040058, 0x070069, 0x0b0073, 0x0e007c, 0x120083, 0x160089, 0x19008f, 0x1d0094, 0x200098, 0x24009c,
     0x2800a0, 0x2b00a3, 0x2f00a7, 0x3200aa, 0x3601ad, 0x3901af, 0x3d01b2, 0x4101b5, 0x4401b7, 0x4802ba, 0x4b02bc,
     0x4f02be, 0x5303c0, 0x5603c2, 0x5a04c4, 0x5d04c6, 0x6105c8, 0x6506ca, 0x6807cc, 0x6c08ce, 0x6f09cf, 0x730ad1,
     0x770cd2, 0x7a0dd4, 0x7e0fd6, 0x8010d7, 0x8413d8, 0x8714da, 0x8b17db, 0x8f1add, 0x921cde, 0x961fdf, 0x9921e1,
     0x9d25e2, 0xa129e3, 0xa42ce4, 0xa830e6, 0xab34e7, 0xaf39e8, 0xb33dea, 0xb643ea, 0xba49ec, 0xbd4eed, 0xc154ee,
     0xc55aef, 0xc861f0, 0xcc68f1, 0xcf70f2, 0xd378f3, 0xd680f4, 0xda89f5, 0xde92f6, 0xe19cf7, 0xe5a6f8, 0xe8b0f9,
     0xecbbfa, 0xf0c7fb, 0xf3d3fc, 0xf7e0fd, 0xfaedfe, 0xfefbff},

    {/* COLORMAP_REDSCALE */
     0x000000, 0x580004, 0x690007, 0x73000b, 0x7c000e, 0x830012, 0x890016, 0x8f0019, 0x94001d, 0x980020, 0x9c0024,
     0xa00028, 0xa3002b, 0xa7002f, 0xaa0032, 0xad0136, 0xaf0139, 0xb2013d, 0xb50141, 0xb70144, 0xba0248, 0xbc024b,
     0xbe024f, 0xc00353, 0xc20356, 0xc4045a, 0xc6045d, 0xc80561, 0xca0665, 0xcc0768, 0xce086c, 0xcf096f, 0xd10a73,
     0xd20c77, 0xd40d7a, 0xd60f7e, 0xd71080, 0xd81384, 0xda1487, 0xdb178b, 0xdd1a8f, 0xde1c92, 0xdf1f96, 0xe12199,
     0xe2259d, 0xe329a1, 0xe42ca4, 0xe630a8, 0xe734ab, 0xe839af, 0xea3db3, 0xea43b6, 0xec49ba, 0xed4ebd, 0xee54c1,
     0xef5ac5, 0xf061c8, 0xf168cc, 0xf270cf, 0xf378d3, 0xf480d6, 0xf589da, 0xf692de, 0xf79ce1, 0xf8a6e5, 0xf9b0e8,
     0xfabbec, 0xfbc7f0, 0xfcd3f3, 0xfde0f7, 0xfeedfa, 0xfffbfe},

    {/* COLORMAP_FLAME */
     0x80ffff, 0x72ffff, 0x63ffff, 0x55ffff, 0x47ffff, 0x38ffff, 0x2affff, 0x1bffff, 0x0dffff, 0x00feff, 0x00efff,
     0x00e1ff, 0x00d3ff, 0x00c4ff, 0x00b6ff, 0x00a8ff, 0x0099ff, 0x008bff, 0x007dff, 0x006fff, 0x0061ff, 0x0052ff,
     0x0044ff, 0x0036ff, 0x0027ff, 0x0019ff, 0x000aff, 0x0400fb, 0x1200ed, 0x2100de, 0x2f00d0, 0x3d00c2, 0x4c00b3,
     0x5a00a5, 0x680097, 0x770088, 0x84007b, 0x93006c, 0xa1005e, 0xaf0050, 0xbe0041, 0xcc0033, 0xda0025, 0xe90016,
     0xf70008, 0xff0700, 0xff1500, 0xff2300, 0xff3200, 0xff4000, 0xff4e00, 0xff5d00, 0xff6b00, 0xff7900, 0xff8700,
     0xff9500, 0xffa400, 0xffb200, 0xffc000, 0xffcf00, 0xffdd00, 0xffeb00, 0xfffa00, 0xffff09, 0xffff17, 0xffff26,
     0xffff34, 0xffff43, 0xffff51, 0xffff5f, 0xffff6e, 0xffff7c},

    {/* COLORMAP_BROWNSCALE */
     0x8c2600, 0x8e2a00, 0x8f2c00, 0x913000, 0x933200, 0x943500, 0x963900, 0x973b00, 0x993f00, 0x9b4100, 0x9c4500,
     0x9e4800, 0xa04b00, 0xa14e00, 0xa35100, 0xa45400, 0xa65700, 0xa85a00, 0xa95d00, 0xab6000, 0xad6300, 0xae6700,
     0xb06900, 0xb16d00, 0xb36f00, 0xb57200, 0xb67500, 0xb87800, 0xba7c00, 0xbb7e00, 0xbd8200, 0xbe8400, 0xc08800,
     0xc18b00, 0xc38e00, 0xc59100, 0xc69300, 0xc89700, 0xc99900, 0xcb9d00, 0xcda000, 0xcea300, 0xd0a600, 0xd1a900,
     0xd3ac00, 0xd5af00, 0xd6b200, 0xd8b500, 0xd9b800, 0xdbbb00, 0xddbf00, 0xdec100, 0xe0c500, 0xe1c700, 0xe3cb00,
     0xe5ce00, 0xe6d000, 0xe8d400, 0xe9d600, 0xebda00, 0xeddd00, 0xeee000, 0xf0e300, 0xf2e600, 0xf3e900, 0xf5ec00,
     0xf6ef00, 0xf8f200, 0xfaf500, 0xfbf800, 0xfdfb00, 0xfffe00},

    {/* COLORMAP_PILATUS */
     0x000008, 0x000120, 0x010239, 0x010552, 0x02096d, 0x020e86, 0x04129c, 0x0515ad, 0x0619bf, 0x071ccf, 0x0920e1,
     0x0a21f1, 0x1629fb, 0x333efb, 0x5459fb, 0x7579fb, 0x9698fc, 0xb8b9fd, 0xc1c9eb, 0xb7cdca, 0xaecfaa, 0xa4d48b,
     0x9ad66b, 0x90da4f, 0x93dd49, 0x9ce04f, 0xa5e155, 0xade45c, 0xb6e562, 0xbfe968, 0xc9ec6e, 0xd3ee71, 0xddf375,
     0xe8f67a, 0xf2f97d, 0xfcfc81, 0xfff77f, 0xfeef7a, 0xfee575, 0xfedb70, 0xfed16b, 0xfec767, 0xfec25d, 0xfdc150,
     0xfdc144, 0xfdc03a, 0xfdbf33, 0xfdbe2d, 0xf7b72b, 0xedac28, 0xe3a225, 0xda9823, 0xd18e21, 0xc7831e, 0xc7782c,
     0xcb6c4f, 0xcf6276, 0xd359a1, 0xd64fcb, 0xda49f3, 0xda53fc, 0xda60fc, 0xda70fc, 0xda7efc, 0xdb8efc, 0xdb9dfd,
     0xe1acfd, 0xe6bdfd, 0xeccdfd, 0xf3defe, 0xf8eefe, 0xffffff},

    {/* COLORMAP_AUTUMN */
     0xff0000, 0xff0400, 0xff0700, 0xff0b00, 0xff0e00, 0xff1200, 0xff1600, 0xff1900, 0xff1d00, 0xff2000, 0xff2400,
     0xff2800, 0xff2b00, 0xff2f00, 0xff3200, 0xff3600, 0xff3900, 0xff3d00, 0xff4100, 0xff4400, 0xff4800, 0xff4b00,
     0xff4f00, 0xff5300, 0xff5600, 0xff5a00, 0xff5d00, 0xff6100, 0xff6500, 0xff6800, 0xff6c00, 0xff6f00, 0xff7300,
     0xff7700, 0xff7a00, 0xff7e00, 0xff8100, 0xff8500, 0xff8800, 0xff8c00, 0xff9000, 0xff9300, 0xff9700, 0xff9a00,
     0xff9e00, 0xffa200, 0xffa500, 0xffa900, 0xffac00, 0xffb000, 0xffb400, 0xffb700, 0xffbb00, 0xffbe00, 0xffc200,
     0xffc600, 0xffc900, 0xffcd00, 0xffd000, 0xffd400, 0xffd700, 0xffdb00, 0xffdf00, 0xffe200, 0xffe600, 0xffe900,
     0xffed00, 0xfff100, 0xfff400, 0xfff800, 0xfffb00, 0xffff00},

    {/* COLORMAP_BONE */
     0x000000, 0x040305, 0x060609, 0x0a0a0d, 0x0c0c11, 0x101016, 0x12131b, 0x16161e, 0x191923, 0x1c1c27, 0x201f2c,
     0x232330, 0x262634, 0x292939, 0x2c2c3d, 0x2f2f42, 0x323246, 0x35354a, 0x39394f, 0x3c3b53, 0x3f3f58, 0x43425c,
     0x454560, 0x494965, 0x4b4b69, 0x4f4f6e, 0x515271, 0x555675, 0x575b78, 0x5b5f7b, 0x5e637e, 0x616781, 0x656c84,
     0x687088, 0x6b748b, 0x6e798e, 0x717d91, 0x748194, 0x778697, 0x7b8a9a, 0x7e8e9e, 0x8192a0, 0x8497a4, 0x879ba7,
     0x8aa0aa, 0x8ea4ae, 0x90a8b0, 0x94adb4, 0x97b1b6, 0x9ab5ba, 0x9dbabc, 0xa0bec0, 0xa4c2c3, 0xa6c6c6, 0xaccaca,
     0xb1cdcd, 0xb5d0d0, 0xbad3d3, 0xbfd6d6, 0xc4d9d9, 0xc9dcdc, 0xcee0df, 0xd3e3e3, 0xd8e6e6, 0xdde9e9, 0xe1eced,
     0xe6efef, 0xebf3f3, 0xf0f5f5, 0xf5f9f9, 0xfbfcfb, 0xffffff},

    {/* COLORMAP_COOL */
     0x00ffff, 0x04fbff, 0x07f8ff, 0x0bf4ff, 0x0ef1ff, 0x12edff, 0x16e9ff, 0x19e6ff, 0x1de2ff, 0x20dfff, 0x24dbff,
     0x28d7ff, 0x2bd4ff, 0x2fd0ff, 0x32cdff, 0x36c9ff, 0x39c6ff, 0x3dc2ff, 0x41beff, 0x44bbff, 0x48b7ff, 0x4bb4ff,
     0x4fb0ff, 0x53acff, 0x56a9ff, 0x5aa5ff, 0x5da2ff, 0x619eff, 0x659aff, 0x6897ff, 0x6c93ff, 0x6f90ff, 0x738cff,
     0x7788ff, 0x7a85ff, 0x7e81ff, 0x817eff, 0x857aff, 0x8877ff, 0x8c73ff, 0x906fff, 0x936cff, 0x9768ff, 0x9a65ff,
     0x9e61ff, 0xa25dff, 0xa55aff, 0xa956ff, 0xac53ff, 0xb04fff, 0xb44bff, 0xb748ff, 0xbb44ff, 0xbe41ff, 0xc23dff,
     0xc639ff, 0xc936ff, 0xcd32ff, 0xd02fff, 0xd42bff, 0xd728ff, 0xdb24ff, 0xdf20ff, 0xe21dff, 0xe619ff, 0xe916ff,
     0xed12ff, 0xf10eff, 0xf40bff, 0xf807ff, 0xfb04ff, 0xff00ff},

    {/* COLORMAP_COPPER */
     0x000000, 0x050302, 0x090503, 0x0e0905, 0x120b07, 0x160e09, 0x1b100b, 0x1f140c, 0x24170e, 0x281910, 0x2c1c12,
     0x301e14, 0x352215, 0x3a2517, 0x3e2719, 0x432a1b, 0x472d1c, 0x4b301e, 0x503320, 0x543522, 0x593824, 0x5e3b25,
     0x623e27, 0x664129, 0x6a432b, 0x6f462d, 0x73492e, 0x784c30, 0x7d4f32, 0x805134, 0x855436, 0x895737, 0x8e5a39,
     0x925d3b, 0x975f3d, 0x9b623f, 0xa06540, 0xa46842, 0xa96b44, 0xad6d46, 0xb27048, 0xb67349, 0xbb764b, 0xbe784d,
     0xc37b4f, 0xc87f51, 0xcc8152, 0xd18454, 0xd58656, 0xd98958, 0xdd8d5a, 0xe28f5b, 0xe7925d, 0xeb945f, 0xf09861,
     0xf49b63, 0xf89d64, 0xfda066, 0xffa267, 0xffa669, 0xffa86b, 0xffab6d, 0xffae6f, 0xffb170, 0xffb472, 0xffb674,
     0xffb976, 0xffbc78, 0xffbf79, 0xffc27b, 0xffc47d, 0xffc77f},

    {/* COLORMAP_GRAY */
     0x000000, 0x040404, 0x070707, 0x0b0b0b, 0x0e0e0e, 0x121212, 0x161616, 0x191919, 0x1d1d1d, 0x202020, 0x242424,
     0x282828, 0x2b2b2b, 0x2f2f2f, 0x323232, 0x363636, 0x393939, 0x3d3d3d, 0x414141, 0x444444, 0x484848, 0x4b4b4b,
     0x4f4f4f, 0x535353, 0x565656, 0x5a5a5a, 0x5d5d5d, 0x616161, 0x656565, 0x686868, 0x6c6c6c, 0x6f6f6f, 0x737373,
     0x777777, 0x7a7a7a, 0x7e7e7e, 0x818181, 0x858585, 0x888888, 0x8c8c8c, 0x909090, 0x939393, 0x979797, 0x9a9a9a,
     0x9e9e9e, 0xa2a2a2, 0xa5a5a5, 0xa9a9a9, 0xacacac, 0xb0b0b0, 0xb4b4b4, 0xb7b7b7, 0xbbbbbb, 0xbebebe, 0xc2c2c2,
     0xc6c6c6, 0xc9c9c9, 0xcdcdcd, 0xd0d0d0, 0xd4d4d4, 0xd7d7d7, 0xdbdbdb, 0xdfdfdf, 0xe2e2e2, 0xe6e6e6, 0xe9e9e9,
     0xededed, 0xf1f1f1, 0xf4f4f4, 0xf8f8f8, 0xfbfbfb, 0xffffff},

    {/* COLORMAP_HOT */
     0x0b0000, 0x140000, 0x1e0000, 0x270000, 0x300000, 0x3a0000, 0x430000, 0x4c0000, 0x560000, 0x600000, 0x690000,
     0x730000, 0x7b0000, 0x850000, 0x8f0000, 0x980000, 0xa10000, 0xab0000, 0xb40000, 0xbe0000, 0xc80000, 0xd00000,
     0xda0000, 0xe30000, 0xec0000, 0xf60000, 0xff0100, 0xff0a00, 0xff1400, 0xff1d00, 0xff2600, 0xff3000, 0xff3a00,
     0xff4200, 0xff4c00, 0xff5500, 0xff5f00, 0xff6900, 0xff7200, 0xff7b00, 0xff8500, 0xff8f00, 0xff9700, 0xffa100,
     0xffaa00, 0xffb400, 0xffbd00, 0xffc700, 0xffd000, 0xffda00, 0xffe300, 0xffed00, 0xfff600, 0xfffe01, 0xffff0f,
     0xffff1d, 0xffff2b, 0xffff39, 0xffff47, 0xffff56, 0xffff63, 0xffff71, 0xffff80, 0xffff8e, 0xffff9d, 0xffffaa,
     0xffffb8, 0xffffc7, 0xffffd5, 0xffffe2, 0xfffff1, 0xffffff},

    {/* COLORMAP_HSV */
     0xff0000, 0xff1600, 0xff2a00, 0xff4000, 0xff5500, 0xff6a00, 0xff7f00, 0xff9500, 0xffa900, 0xffbf00, 0xffd400,
     0xffe900, 0xfaf900, 0xeaff00, 0xd5ff00, 0xc0ff00, 0xaaff00, 0x96ff00, 0x80ff00, 0x6bff00, 0x56ff00, 0x40ff00,
     0x2bff00, 0x16ff00, 0x06ff05, 0x00ff15, 0x00ff29, 0x00ff3f, 0x00ff54, 0x00ff69, 0x00ff7e, 0x00ff94, 0x00ffa9,
     0x00ffbe, 0x00ffd4, 0x00ffe8, 0x00fffd, 0x00ebff, 0x00d6ff, 0x00c1ff, 0x00acff, 0x0096ff, 0x0081ff, 0x006cff,
     0x0057ff, 0x0041ff, 0x002cff, 0x0017ff, 0x0506ff, 0x1300ff, 0x2800ff, 0x3e00ff, 0x5300ff, 0x6800ff, 0x7e00ff,
     0x9300ff, 0xa800ff, 0xbd00ff, 0xd200ff, 0xe700ff, 0xf900fb, 0xff00ed, 0xff00d7, 0xff00c1, 0xff00ad, 0xff0097,
     0xff0082, 0xff006d, 0xff0058, 0xff0042, 0xff002e, 0xff0018},

    {/* COLORMAP_JET */
     0x000080, 0x000090, 0x0000a0, 0x0000b1, 0x0000c1, 0x0000d1, 0x0000e1, 0x0000f2, 0x0000ff, 0x0002ff, 0x0011ff,
     0x001fff, 0x002dff, 0x003cff, 0x004aff, 0x0058ff, 0x0066ff, 0x0074ff, 0x0084ff, 0x0092ff, 0x00a0ff, 0x00afff,
     0x00bdff, 0x00cbff, 0x00daff, 0x01e7f5, 0x0df7ea, 0x19ffde, 0x25ffd2, 0x31ffc7, 0x3bffbb, 0x47ffb0, 0x53ffa4,
     0x5fff98, 0x6aff8d, 0x76ff81, 0x81ff76, 0x8dff6a, 0x98ff5f, 0xa4ff53, 0xb0ff47, 0xbbff3b, 0xc7ff31, 0xd2ff25,
     0xdeff19, 0xeaff0d, 0xf5f701, 0xffeb00, 0xffdd00, 0xffd000, 0xffc200, 0xffb500, 0xffa800, 0xff9b00, 0xff8d00,
     0xff8000, 0xff7200, 0xff6500, 0xff5800, 0xff4a00, 0xff3d00, 0xff3000, 0xff2300, 0xff1500, 0xf20800, 0xe20000,
     0xd10000, 0xc10000, 0xb10000, 0xa00000, 0x900000, 0x800000},

    {/* COLORMAP_PINK */
     0x1e0000, 0x301717, 0x3d2323, 0x472b2b, 0x513131, 0x593737, 0x613d3d, 0x684141, 0x6e4646, 0x754a4a, 0x7b4e4e,
     0x805252, 0x855555, 0x8b5959, 0x905c5c, 0x956060, 0x9a6262, 0x9e6666, 0xa26969, 0xa66c6c, 0xab6f6f, 0xae7272,
     0xb37474, 0xb77777, 0xba7979, 0xbe7c7c, 0xc27e7e, 0xc38480, 0xc58982, 0xc68e85, 0xc89387, 0xc99789, 0xcb9c8c,
     0xcda18e, 0xcea590, 0xd0a992, 0xd1ad94, 0xd2b296, 0xd4b599, 0xd5b99a, 0xd7bd9c, 0xd8c09e, 0xdac4a0, 0xdbc7a2,
     0xdccba4, 0xdecfa6, 0xdfd2a7, 0xe1d6a9, 0xe2d8ab, 0xe3dcad, 0xe5e0af, 0xe6e2b0, 0xe7e6b2, 0xe8e8b4, 0xeaeab9,
     0xebebbd, 0xececc2, 0xeeeec7, 0xefefcb, 0xf0f0d0, 0xf1f1d4, 0xf3f3d8, 0xf4f4dd, 0xf5f5e1, 0xf7f7e5, 0xf8f8e8,
     0xf9f9ed, 0xfafaf1, 0xfbfbf4, 0xfdfdf8, 0xfefefb, 0xffffff},

    {/* COLORMAP_SPECTRAL */
     0x000000, 0x210027, 0x43004d, 0x650073, 0x79008a, 0x7e008f, 0x820093, 0x870098, 0x66009d, 0x4000a2, 0x1900a7,
     0x0000af, 0x0000bd, 0x0000cc, 0x0000da, 0x001bdd, 0x003cdd, 0x005edd, 0x007add, 0x0083dd, 0x008cdd, 0x0096dd,
     0x009cd3, 0x00a1c5, 0x00a6b6, 0x00aaa9, 0x00aa9f, 0x00aa95, 0x00aa8c, 0x00a771, 0x00a24b, 0x009e24, 0x009a00,
     0x00a300, 0x00ac00, 0x00b600, 0x00c000, 0x00ca00, 0x00d300, 0x00dc00, 0x00e600, 0x00f000, 0x00fa00, 0x15ff00,
     0x49ff00, 0x7eff00, 0xb3ff00, 0xc7fb00, 0xd6f600, 0xe4f100, 0xf0eb00, 0xf4e100, 0xf9d800, 0xfece00, 0xffc100,
     0xffb300, 0xffa500, 0xff9100, 0xff6500, 0xff3a00, 0xff0f00, 0xf90000, 0xef0000, 0xe50000, 0xdc0000, 0xd70000,
     0xd30000, 0xcf0000, 0xcc2020, 0xcc5959, 0xcc9393, 0xcccccc},

    {/* COLORMAP_SPRING */
     0xff00ff, 0xff04fb, 0xff07f8, 0xff0bf4, 0xff0ef1, 0xff12ed, 0xff16e9, 0xff19e6, 0xff1de2, 0xff20df, 0xff24db,
     0xff28d7, 0xff2bd4, 0xff2fd0, 0xff32cd, 0xff36c9, 0xff39c6, 0xff3dc2, 0xff41be, 0xff44bb, 0xff48b7, 0xff4bb4,
     0xff4fb0, 0xff53ac, 0xff56a9, 0xff5aa5, 0xff5da2, 0xff619e, 0xff659a, 0xff6897, 0xff6c93, 0xff6f90, 0xff738c,
     0xff7788, 0xff7a85, 0xff7e81, 0xff817e, 0xff857a, 0xff8877, 0xff8c73, 0xff906f, 0xff936c, 0xff9768, 0xff9a65,
     0xff9e61, 0xffa25d, 0xffa55a, 0xffa956, 0xffac53, 0xffb04f, 0xffb44b, 0xffb748, 0xffbb44, 0xffbe41, 0xffc23d,
     0xffc639, 0xffc936, 0xffcd32, 0xffd02f, 0xffd42b, 0xffd728, 0xffdb24, 0xffdf20, 0xffe21d, 0xffe619, 0xffe916,
     0xffed12, 0xfff10e, 0xfff40b, 0xfff807, 0xfffb04, 0xffff00},

    {/* COLORMAP_SUMMER */
     0x008066, 0x048266, 0x078366, 0x0b8566, 0x0e8766, 0x128966, 0x168b66, 0x198c66, 0x1d8e66, 0x209066, 0x249266,
     0x289366, 0x2b9566, 0x2f9766, 0x329966, 0x369b66, 0x399c66, 0x3d9e66, 0x41a066, 0x44a266, 0x48a366, 0x4ba666,
     0x4fa766, 0x53a966, 0x56ab66, 0x5aad66, 0x5dae66, 0x61b066, 0x65b266, 0x68b366, 0x6cb666, 0x6fb766, 0x73b966,
     0x77bb66, 0x7abd66, 0x7ebf66, 0x81c066, 0x85c266, 0x88c466, 0x8cc666, 0x90c866, 0x93c966, 0x97cb66, 0x9acd66,
     0x9ecf66, 0xa2d166, 0xa5d266, 0xa9d466, 0xacd666, 0xb0d866, 0xb4da66, 0xb7db66, 0xbbdd66, 0xbedf66, 0xc2e166,
     0xc6e366, 0xc9e466, 0xcde666, 0xd0e866, 0xd4ea66, 0xd7eb66, 0xdbed66, 0xdfef66, 0xe2f166, 0xe6f366, 0xe9f466,
     0xedf666, 0xf1f866, 0xf4fa66, 0xf8fc66, 0xfbfd66, 0xffff66},

    {/* COLORMAP_WINTER */
     0x0000ff, 0x0004fd, 0x0007fc, 0x000bfa, 0x000ef8, 0x0012f6, 0x0016f4, 0x0019f3, 0x001df1, 0x0020ef, 0x0024ed,
     0x0028eb, 0x002bea, 0x002fe8, 0x0032e6, 0x0036e4, 0x0039e3, 0x003de1, 0x0041df, 0x0044dd, 0x0048db, 0x004bda,
     0x004fd8, 0x0053d6, 0x0056d4, 0x005ad2, 0x005dd1, 0x0061cf, 0x0065cd, 0x0068cb, 0x006cc9, 0x006fc8, 0x0073c6,
     0x0077c3, 0x007ac2, 0x007ec0, 0x0081bf, 0x0085bd, 0x0088bb, 0x008cb9, 0x0090b7, 0x0093b6, 0x0097b3, 0x009ab2,
     0x009eb0, 0x00a2ae, 0x00a5ad, 0x00a9ab, 0x00aca9, 0x00b0a7, 0x00b4a6, 0x00b7a3, 0x00bba2, 0x00bea0, 0x00c29e,
     0x00c69c, 0x00c99b, 0x00cd99, 0x00d097, 0x00d495, 0x00d793, 0x00db92, 0x00df90, 0x00e28e, 0x00e68c, 0x00e98b,
     0x00ed89, 0x00f187, 0x00f485, 0x00f883, 0x00fb82, 0x00ff80},

    {/* COLORMAP_GIST_EARTH */
     0x000000, 0x030049, 0x050070, 0x070975, 0x091175, 0x0c1976, 0x0f2177, 0x112977, 0x133178, 0x153a78, 0x184179,
     0x1b487a, 0x1d4f7a, 0x1f567b, 0x215d7c, 0x24647c, 0x27697d, 0x296f7e, 0x2b747e, 0x2d7a7f, 0x308080, 0x31827c,
     0x338477, 0x358672, 0x37886e, 0x388a6a, 0x3a8c66, 0x3c8e61, 0x3e915c, 0x3f9258, 0x419554, 0x429650, 0x44994b,
     0x499a46, 0x509d48, 0x589f4a, 0x61a14c, 0x69a34e, 0x70a551, 0x78a652, 0x7ea853, 0x85a954, 0x8aab56, 0x90ac56,
     0x96ae58, 0x9cb059, 0xa2b15a, 0xa8b35b, 0xaeb45c, 0xb4b65d, 0xb8b55f, 0xb9b360, 0xbbaf61, 0xbcac62, 0xbea963,
     0xbfa664, 0xc1a367, 0xc5a570, 0xc9a77a, 0xcdaa82, 0xd0ad8b, 0xd5b194, 0xd9b59c, 0xddbaa6, 0xe1bfb0, 0xe5c6ba,
     0xe9cdc5, 0xedd4d0, 0xf1dcdb, 0xf5e6e5, 0xfaf0f0, 0xfdfbfb},

    {/* COLORMAP_GIST_HEAT */
     0x000000, 0x060000, 0x0b0000, 0x100000, 0x150000, 0x1b0000, 0x210000, 0x260000, 0x2c0000, 0x300000, 0x360000,
     0x3c0000, 0x410000, 0x470000, 0x4c0000, 0x510000, 0x560000, 0x5b0000, 0x610000, 0x660000, 0x6c0000, 0x720000,
     0x770000, 0x7c0000, 0x810000, 0x870000, 0x8c0000, 0x920000, 0x970000, 0x9c0000, 0xa20000, 0xa70000, 0xad0000,
     0xb20000, 0xb70000, 0xbc0000, 0xc20400, 0xc80b00, 0xcd1200, 0xd21900, 0xd82000, 0xdc2800, 0xe32f00, 0xe73600,
     0xed3d00, 0xf34400, 0xf84b00, 0xfe5300, 0xff5a00, 0xff6100, 0xff6800, 0xff6f00, 0xff7700, 0xff7e00, 0xff850b,
     0xff8c19, 0xff9328, 0xff9a36, 0xffa244, 0xffa953, 0xffb061, 0xffb76f, 0xffbe7e, 0xffc68c, 0xffcd9a, 0xffd4a9,
     0xffdbb7, 0xffe2c6, 0xffe9d4, 0xfff1e2, 0xfff8f1, 0xffffff},

    {/* COLORMAP_GIST_NCAR */
     0x000080, 0x001a5e,    0x00343c, 0x004e1b, 0x00551d, 0x003d58, 0x002594, 0x000dcf, 0x0019100, 0x004cff, 0x007eff,
     0x00b1ff, 0x00caff,    0x00daff, 0x00eaff, 0x00faf3, 0x00fedb, 0x00fcc4, 0x00faac, 0x00fa8f,  0x00fb6a, 0x00fd45,
     0x00fe1f, 0x17f8 - 01, 0x2eeb00, 0x45de00, 0x5cd000, 0x6ad800, 0x71e400, 0x78f000, 0x7ffb07,  0x8fff15, 0x9fff24,
     0xb0ff32, 0xc0ff37,    0xd1ff29, 0xe0ff1b, 0xf1ff0c, 0xfff700, 0xffee00, 0xffe500, 0xffdb00,  0xffd303, 0xffca07,
     0xffc10b, 0xffb50e,    0xff970a, 0xff7a07, 0xff5d04, 0xff4300, 0xff3200, 0xff2000, 0xff0f00,  0xff0029, 0xff0069,
     0xff00a9, 0xff00e8,    0xec09ff, 0xd416ff, 0xbb22ff, 0xa230ff, 0xb144fa, 0xc65af6, 0xdc70f2,  0xec83ef, 0xee94f1,
     0xf1a5f3, 0xf4b5f5,    0xf6c6f7, 0xf9d7fa, 0xfbe7fc, 0xfef8fe},

    {/* COLORMAP_GIST_RAINBOW */
     0xff0029, 0xff0015, 0xff0002, 0xff1100, 0xff2400, 0xff3800, 0xff4b00, 0xff5f00, 0xff7200, 0xff8600, 0xff9900,
     0xffac00, 0xffbf00, 0xffd300, 0xffe600, 0xfffa00, 0xf1ff00, 0xdeff00, 0xcaff00, 0xb7ff00, 0xa3ff00, 0x90ff00,
     0x7cff00, 0x69ff00, 0x55ff00, 0x42ff00, 0x2fff00, 0x1bff00, 0x08ff00, 0x00ff0c, 0x00ff1f, 0x00ff32, 0x00ff46,
     0x00ff58, 0x00ff6d, 0x00ff80, 0x00ff93, 0x00ffa6, 0x00ffb9, 0x00ffcc, 0x00ffe0, 0x00fff3, 0x00f7ff, 0x00e4ff,
     0x00d0ff, 0x00bdff, 0x00a9ff, 0x0096ff, 0x0082ff, 0x006fff, 0x005bff, 0x0048ff, 0x0034ff, 0x0021ff, 0x000dff,
     0x0700ff, 0x1a00ff, 0x2e00ff, 0x4100ff, 0x5500ff, 0x6800ff, 0x7b00ff, 0x8f00ff, 0xa300ff, 0xb600ff, 0xca00ff,
     0xdd00ff, 0xf100ff, 0xff00fa, 0xff00e6, 0xff00d3, 0xff00bf},

    {/* COLORMAP_GIST_STERN */
     0x000000, 0x420407, 0x83070e, 0xc50b16, 0xff0e1d, 0xeb1224, 0xd9162b, 0xc71932, 0xb51d39, 0xa32041, 0x912448,
     0x7f284f, 0x6e2b56, 0x5c2f5d, 0x4a3265, 0x38366c, 0x273973, 0x153d7a, 0x454181, 0x444488, 0x484890, 0x4b4b97,
     0x4f4f9e, 0x5353a5, 0x5656ac, 0x5a5ab4, 0x5d5dbb, 0x6161c2, 0x6565c9, 0x6868d0, 0x6c6cd7, 0x6f6fdf, 0x7373e6,
     0x7777ed, 0x7a7af4, 0x7e7efb, 0x8181f8, 0x8585e9, 0x8888d9, 0x8c8cca, 0x9090ba, 0x9393ab, 0x97979c, 0x9a9a8c,
     0x9e9e7d, 0xa2a26d, 0xa5a55e, 0xa9a94f, 0xacac40, 0xb0b031, 0xb4b422, 0xb7b712, 0xbbbb03, 0xbebe0b, 0xc2c219,
     0xc6c626, 0xc9c934, 0xcdcd41, 0xd0d04f, 0xd4d45d, 0xd7d76a, 0xdbdb77, 0xdfdf85, 0xe2e293, 0xe6e6a1, 0xe9e9ae,
     0xededbb, 0xf1f1c8, 0xf4f4d6, 0xf8f8e4, 0xfbfbf2, 0xffffff},

    {/* COLORMAP_AFMHOT */
     0x000000, 0x070000, 0x0e0000, 0x160000, 0x1d0000, 0x240000, 0x2b0000, 0x320000, 0x390000, 0x410000, 0x480000,
     0x4f0000, 0x560000, 0x5d0000, 0x650000, 0x6c0000, 0x730000, 0x7a0000, 0x810100, 0x880800, 0x901000, 0x971800,
     0x9e1f00, 0xa52600, 0xac2d00, 0xb43300, 0xbb3b00, 0xc24200, 0xc94900, 0xd05100, 0xd75900, 0xdf6000, 0xe66700,
     0xed6e00, 0xf47400, 0xfb7c00, 0xff8304, 0xff8b0b, 0xff9212, 0xff9919, 0xffa020, 0xffa628, 0xffaf2f, 0xffb536,
     0xffbd3d, 0xffc444, 0xffcc4b, 0xffd353, 0xffda5a, 0xffe161, 0xffe768, 0xffef6f, 0xfff577, 0xfffe7e, 0xffff85,
     0xffff8c, 0xffff93, 0xffff9a, 0xffffa2, 0xffffa9, 0xffffb0, 0xffffb7, 0xffffbe, 0xffffc6, 0xffffcd, 0xffffd4,
     0xffffdb, 0xffffe2, 0xffffe9, 0xfffff1, 0xfffff8, 0xffffff},

    {/* COLORMAP_BRG */
     0x0000ff, 0x0700f8, 0x0e00f1, 0x1600e9, 0x1d00e2, 0x2400db, 0x2b00d4, 0x3200cd, 0x3900c6, 0x4100be, 0x4800b7,
     0x4f00b0, 0x5600a9, 0x5d00a2, 0x65009a, 0x6c0093, 0x73008c, 0x7a0085, 0x81007e, 0x880077, 0x90006f, 0x970068,
     0x9e0061, 0xa5005a, 0xac0053, 0xb4004b, 0xbb0044, 0xc2003d, 0xc90036, 0xd0002f, 0xd70028, 0xdf0020, 0xe60019,
     0xed0012, 0xf4000b, 0xfb0004, 0xfb0400, 0xf40b00, 0xed1200, 0xe61900, 0xdf2000, 0xd72800, 0xd02f00, 0xc93600,
     0xc23d00, 0xbb4400, 0xb44b00, 0xac5300, 0xa55a00, 0x9e6100, 0x976800, 0x906f00, 0x887700, 0x817e00, 0x7a8500,
     0x738c00, 0x6c9300, 0x659a00, 0x5da200, 0x56a900, 0x4fb000, 0x48b700, 0x41be00, 0x39c600, 0x32cd00, 0x2bd400,
     0x24db00, 0x1de200, 0x16e900, 0x0ef100, 0x07f800, 0x00ff00},

    {/* COLORMAP_BWR */
     0x0000ff, 0x0707ff, 0x0e0eff, 0x1616ff, 0x1d1dff, 0x2424ff, 0x2b2bff, 0x3232ff, 0x3939ff, 0x4141ff, 0x4848ff,
     0x4f4fff, 0x5656ff, 0x5d5dff, 0x6565ff, 0x6c6cff, 0x7373ff, 0x7a7aff, 0x8181ff, 0x8888ff, 0x9090ff, 0x9797ff,
     0x9e9eff, 0xa5a5ff, 0xacacff, 0xb4b4ff, 0xbbbbff, 0xc2c2ff, 0xc9c9ff, 0xd0d0ff, 0xd7d7ff, 0xdfdfff, 0xe6e6ff,
     0xededff, 0xf4f4ff, 0xfbfbff, 0xfffbfb, 0xfff4f4, 0xffeded, 0xffe6e6, 0xffdfdf, 0xffd7d7, 0xffd0d0, 0xffc9c9,
     0xffc2c2, 0xffbbbb, 0xffb4b4, 0xffacac, 0xffa5a5, 0xff9e9e, 0xff9797, 0xff9090, 0xff8888, 0xff8181, 0xff7a7a,
     0xff7373, 0xff6c6c, 0xff6565, 0xff5d5d, 0xff5656, 0xff4f4f, 0xff4848, 0xff4141, 0xff3939, 0xff3232, 0xff2b2b,
     0xff2424, 0xff1d1d, 0xff1616, 0xff0e0e, 0xff0707, 0xff0000},

    {/* COLORMAP_COOLWARM */
     0x3b4cc0, 0x3f52c6, 0x4358cb, 0x485fd1, 0x4b65d5, 0x506bda, 0x5571df, 0x5977e3, 0x5e7de7, 0x6283ea, 0x6788ee,
     0x6b8ef1, 0x7093f3, 0x7698f6, 0x7a9df8, 0x80a3fa, 0x85a7fc, 0x89acfd, 0x8eb1fe, 0x93b5fe, 0x98b9ff, 0x9ebdff,
     0xa2c1ff, 0xa6c5fe, 0xabc8fd, 0xb1cbfc, 0xb6cdfa, 0xbad0f8, 0xbfd2f6, 0xc3d5f4, 0xc7d7f0, 0xcbd8ee, 0xcfdaea,
     0xd4dbe6, 0xd7dce3, 0xdbdcdf, 0xdedcda, 0xe2dad5, 0xe5d7d0, 0xe9d5cb, 0xecd3c5, 0xeed0c0, 0xf1cdba, 0xf2cab5,
     0xf4c6af, 0xf5c1a9, 0xf6bea4, 0xf7b99e, 0xf7b498, 0xf7b093, 0xf7ab8d, 0xf7a688, 0xf5a081, 0xf59b7c, 0xf39577,
     0xf18f71, 0xf08a6c, 0xed8367, 0xeb7c61, 0xe8765c, 0xe56f57, 0xe26952, 0xde624d, 0xda5948, 0xd65244, 0xd14a40,
     0xcd423b, 0xc93936, 0xc43032, 0xbe252e, 0xb9142b, 0xb40426},

    {/* COLORMAP_CMRMAP */
     0x000000, 0x05050e, 0x08081d, 0x0d0d2b, 0x111139, 0x161648, 0x191956, 0x1e1e65, 0x232373, 0x272681, 0x2b2688,
     0x2f268f, 0x342696, 0x38269d, 0x3c26a5, 0x4126ac, 0x4526b3, 0x4926ba, 0x4f27be, 0x5828b7, 0x602aaf, 0x692ba8,
     0x712ca1, 0x7a2e9a, 0x822f93, 0x8a318b, 0x943284, 0x9d347c, 0xa93571, 0xb43668, 0xc0385e, 0xcb3953, 0xd73b49,
     0xe33c3f, 0xed3e36, 0xf93f2b, 0xfe4424, 0xfb4b20, 0xf7521b, 0xf55917, 0xf26013, 0xef680f, 0xec6f0a, 0xea7605,
     0xe77d02, 0xe68402, 0xe68b05, 0xe69308, 0xe69a0a, 0xe6a10d, 0xe6a810, 0xe6af13, 0xe6b716, 0xe6be19, 0xe6c322,
     0xe6c62e, 0xe6cb39, 0xe6d045, 0xe6d450, 0xe6d85c, 0xe6dc68, 0xe6e172, 0xe6e57e, 0xe8e88c, 0xebeb9a, 0xededa9,
     0xf1f1b7, 0xf4f4c6, 0xf6f6d4, 0xf9f9e2, 0xfcfcf1, 0xffffff},

    {/* COLORMAP_CUBEHELIX */
     0x000000, 0x060206, 0x0b040c, 0x100713, 0x13091a, 0x160d21, 0x191128, 0x1a142f, 0x1b1936, 0x1b1e3c, 0x1a2441,
     0x192946, 0x182f49, 0x17364c, 0x163c4e, 0x15424e, 0x15484e, 0x154e4d, 0x16544b, 0x175a49, 0x1a6046, 0x1d6443,
     0x22693f, 0x276d3b, 0x2d7038, 0x347435, 0x3d7632, 0x467830, 0x50792f, 0x5a7a2f, 0x657a30, 0x707b31, 0x7b7a35,
     0x867a39, 0x907a3e, 0x9b7945, 0xa5794d, 0xae7956, 0xb6795f, 0xbe796a, 0xc47a75, 0xc97b80, 0xce7d8c, 0xd18097,
     0xd382a3, 0xd486ae, 0xd589b9, 0xd48ec3, 0xd393cc, 0xd198d4, 0xcf9edc, 0xcca4e2, 0xcaaae8, 0xc8b1ec, 0xc5b8ef,
     0xc3bef2, 0xc2c5f3, 0xc1ccf3, 0xc1d1f3, 0xc2d7f3, 0xc4ddf2, 0xc6e1f1, 0xcae6f0, 0xcdeaef, 0xd3eeef, 0xd8f1ef,
     0xdef4ef, 0xe4f7f1, 0xebf9f3, 0xf2fbf6, 0xf9fdfb, 0xffffff},

    {/* COLORMAP_GNUPLOT */
     0x000000, 0x1e0017, 0x2b002d, 0x340043, 0x3d0058, 0x44006d, 0x4a0081, 0x500094, 0x5600a6, 0x5b01b6, 0x6001c6,
     0x6501d3, 0x6901de, 0x6d02e9, 0x7102f2, 0x7502f8, 0x7a03fb, 0x7d03fe, 0x8104ff, 0x8405fe, 0x8706fa, 0x8a06f4,
     0x8e08ed, 0x9109e4, 0x940ad9, 0x970bcc, 0x9a0cbe, 0x9d0eae, 0xa0109d, 0xa3118b, 0xa61377, 0xa81563, 0xab174d,
     0xad1938, 0xb01c21, 0xb31f0b, 0xb52100, 0xb82400, 0xba2800, 0xbd2a00, 0xc02e00, 0xc23100, 0xc43500, 0xc63800,
     0xc93d00, 0xcb4100, 0xcd4500, 0xd04a00, 0xd14f00, 0xd45400, 0xd65900, 0xd85e00, 0xda6500, 0xdc6a00, 0xde7000,
     0xe17700, 0xe27d00, 0xe58400, 0xe68b00, 0xe99300, 0xea9a00, 0xeca200, 0xeeaa00, 0xf0b300, 0xf2bb00, 0xf4c400,
     0xf6cd00, 0xf8d600, 0xf9e000, 0xfbeb00, 0xfdf400, 0xffff00},

    {/* COLORMAP_GNUPLOT2 */
     0x000000, 0x00000e, 0x00001d, 0x00002b, 0x000039, 0x000048, 0x000056, 0x000065, 0x000073, 0x000081, 0x000090,
     0x00009e, 0x0000ac, 0x0000bb, 0x0000c9, 0x0000d7, 0x0000e6, 0x0000f4, 0x0300ff, 0x0e00ff, 0x1900ff, 0x2400ff,
     0x3000ff, 0x3b00ff, 0x4700ff, 0x5100ff, 0x5d00ff, 0x6800ff, 0x7300ff, 0x7e00ff, 0x8901fe, 0x9509f6, 0xa010ef,
     0xac17e8, 0xb61ee1, 0xc225da, 0xcd2dd2, 0xd834cb, 0xe33bc4, 0xee42bd, 0xfa49b6, 0xff51ae, 0xff58a7, 0xff5fa0,
     0xff6699, 0xff6d92, 0xff748b, 0xff7c83, 0xff837c, 0xff8a75, 0xff916e, 0xff9867, 0xffa05f, 0xffa758, 0xffae51,
     0xffb54a, 0xffbc43, 0xffc33c, 0xffcb34, 0xffd22d, 0xffd926, 0xffe01f, 0xffe718, 0xffef10, 0xfff609, 0xfffd02,
     0xffff1f, 0xffff4b, 0xffff78, 0xffffa5, 0xffffd2, 0xffffff},

    {/* COLORMAP_OCEAN */
     0x008000, 0x007a04, 0x007507, 0x006f0b, 0x006a0e, 0x006512, 0x005f16, 0x005a19, 0x00541d, 0x004f20, 0x004a24,
     0x004428, 0x003f2b, 0x003a2f, 0x003532, 0x002f36, 0x002a39, 0x00243d, 0x001f41, 0x001944, 0x001448, 0x000f4b,
     0x00094f, 0x000453, 0x000156, 0x00075a, 0x000d5d, 0x001261, 0x001865, 0x001d68, 0x00226c, 0x00286f, 0x002d73,
     0x003277, 0x00377a, 0x003e7e, 0x004281, 0x004885, 0x004d88, 0x00538c, 0x005890, 0x005d93, 0x006397, 0x00689a,
     0x006e9e, 0x0072a2, 0x0078a5, 0x007ea9, 0x0784ac, 0x1288b0, 0x1d8eb4, 0x2893b7, 0x3298bb, 0x3d9ebe, 0x48a3c2,
     0x53a9c6, 0x5daec9, 0x68b4cd, 0x73b9d0, 0x7ebfd4, 0x88c3d7, 0x93c9db, 0x9ecfdf, 0xa9d4e2, 0xb4dae6, 0xbedfe9,
     0xc9e4ed, 0xd4e9f1, 0xdfeef4, 0xe9f4f8, 0xf4fafb, 0xffffff},

    {/* COLORMAP_RAINBOW */
     0x8000ff, 0x790bff, 0x7217ff, 0x6a21fe, 0x632dfe, 0x5c38fd, 0x5543fd, 0x4e4dfc, 0x4758fb, 0x3f63fa, 0x386df9,
     0x3078f7, 0x2981f6, 0x228bf4, 0x1b94f3, 0x149df1, 0x0da6ef, 0x06aeed, 0x01b6eb, 0x08bfe9, 0x10c6e6, 0x18cce4,
     0x1fd3e1, 0x26d9de, 0x2ddedc, 0x33e4d9, 0x3be9d6, 0x42edd3, 0x49f2cf, 0x51f4cc, 0x59f8c9, 0x60fac6, 0x67fcc2,
     0x6efdbf, 0x74feba, 0x7cffb6, 0x83ffb3, 0x8bfeae, 0x92fdaa, 0x99fca6, 0xa0faa1, 0xa6f89d, 0xaff498, 0xb5f294,
     0xbded8f, 0xc4e98b, 0xcce486, 0xd3de81, 0xdad97c, 0xe1d377, 0xe7cc73, 0xefc66d, 0xf5bf68, 0xfeb663, 0xffae5e,
     0xffa658, 0xff9d53, 0xff944e, 0xff8b48, 0xff8143, 0xff783e, 0xff6d38, 0xff6332, 0xff582d, 0xff4d27, 0xff4322,
     0xff381c, 0xff2d17, 0xff2111, 0xff170b, 0xff0b06, 0xff0000},

    {/* COLORMAP_SEISMIC */
     0x00004d, 0x000057, 0x000061, 0x00006b, 0x000075, 0x00007f, 0x000089, 0x000093, 0x00009d, 0x0000a7, 0x0000b1,
     0x0000bb, 0x0000c5, 0x0000cf, 0x0000d9, 0x0000e4, 0x0000ed, 0x0000f7, 0x0303ff, 0x1212ff, 0x2020ff, 0x2f2fff,
     0x3d3dff, 0x4b4bff, 0x5a5aff, 0x6868ff, 0x7777ff, 0x8585ff, 0x9393ff, 0xa2a2ff, 0xb0b0ff, 0xbebeff, 0xcdcdff,
     0xdbdbff, 0xe9e9ff, 0xf8f8ff, 0xfff8f8, 0xffe9e9, 0xffdbdb, 0xffcdcd, 0xffbebe, 0xffb0b0, 0xffa2a2, 0xff9393,
     0xff8585, 0xff7777, 0xff6868, 0xff5a5a, 0xff4b4b, 0xff3d3d, 0xff2f2f, 0xff2020, 0xff1212, 0xff0303, 0xfa0000,
     0xf30000, 0xec0000, 0xe40000, 0xdd0000, 0xd60000, 0xcf0000, 0xc80000, 0xc10000, 0xb90000, 0xb20000, 0xab0000,
     0xa30000, 0x9d0000, 0x950000, 0x8e0000, 0x870000, 0x800000},

    {/* COLORMAP_TERRAIN */
     0x333399, 0x2f3da3, 0x2a46ac, 0x2450b6, 0x2059bf, 0x1b63c9, 0x176dd3, 0x1276dc, 0x0d7fe5, 0x0889ef, 0x0393f9,
     0x009cf8, 0x00a3e2, 0x00aacd, 0x00b2b7, 0x00b9a2, 0x00c08c, 0x00c777, 0x03cd66, 0x12cf69, 0x20d36d, 0x2fd56f,
     0x3dd872, 0x4bdb75, 0x5ade78, 0x68e17b, 0x77e37d, 0x85e781, 0x93ea84, 0xa2ec86, 0xb0ef89, 0xbef28c, 0xcdf58f,
     0xdbf791, 0xe9fb95, 0xf8fe98, 0xfbfa97, 0xf4f193, 0xede890, 0xe6df8b, 0xdfd687, 0xd7cc84, 0xd0c380, 0xc9ba7c,
     0xc2b178, 0xbba874, 0xb49e71, 0xac956c, 0xa58c69, 0x9e8365, 0x977a60, 0x90715d, 0x886859, 0x815e55, 0x85635c,
     0x8c6c65, 0x93756e, 0x9a7e78, 0xa28882, 0xa9918c, 0xb09a95, 0xb7a39f, 0xbeaca8, 0xc6b6b2, 0xcdbfbc, 0xd4c8c5,
     0xdbd1cf, 0xe2dad8, 0xe9e4e2, 0xf1edec, 0xf8f6f5, 0xffffff},

    {/* COLORMAP_VIRIDIS */
     0x440154, 0x46065a, 0x460b5e, 0x471164, 0x481768, 0x481b6d, 0x482070, 0x482475, 0x482979, 0x472d7b, 0x46327e,
     0x453681, 0x443a83, 0x423f85, 0x414387, 0x3f4788, 0x3e4b89, 0x3c4f8a, 0x3a538b, 0x39568c, 0x375b8d, 0x355e8d,
     0x33628d, 0x31668e, 0x30698e, 0x2e6d8e, 0x2d708e, 0x2c738e, 0x2a778e, 0x297a8e, 0x277e8e, 0x26818e, 0x25848e,
     0x24888e, 0x228b8d, 0x218f8d, 0x20928c, 0x1f958b, 0x1f988b, 0x1e9c89, 0x1fa088, 0x1fa287, 0x21a685, 0x23a983,
     0x26ad81, 0x2ab07f, 0x2eb37c, 0x34b679, 0x39b977, 0x3fbc73, 0x45c06f, 0x4cc26c, 0x54c568, 0x5bc864, 0x63cb5f,
     0x6bcd5b, 0x73d056, 0x7bd250, 0x85d44a, 0x8ed645, 0x96d83f, 0xa0da39, 0xa9dc33, 0xb3dd2c, 0xbddf26, 0xc6e020,
     0xd0e11c, 0xd9e319, 0xe3e418, 0xece51b, 0xf5e61f, 0xfde725},

    {/* COLORMAP_INFERNO */
     0x000004, 0x020109, 0x030210, 0x060419, 0x090620, 0x0d0829, 0x110a31, 0x160b39, 0x1c0c43, 0x220c4b, 0x280b53,
     0x2e0a5a, 0x340a5f, 0x3a0964, 0x410a67, 0x470b6a, 0x4d0c6b, 0x520e6d, 0x58106e, 0x5d126e, 0x64156e, 0x69166e,
     0x6f196e, 0x751a6e, 0x7a1d6d, 0x801f6c, 0x86216b, 0x8c2369, 0x912568, 0x972766, 0x9c2964, 0xa22b62, 0xa82e5f,
     0xad305c, 0xb3325a, 0xb83556, 0xbe3853, 0xc33b4f, 0xc73e4c, 0xcc4248, 0xd14644, 0xd54a41, 0xda4e3c, 0xde5237,
     0xe25734, 0xe65d2f, 0xe9612b, 0xec6726, 0xef6d22, 0xf1731d, 0xf37918, 0xf67e14, 0xf8850f, 0xf98b0b, 0xfa9207,
     0xfb9806, 0xfc9f07, 0xfca60b, 0xfcad11, 0xfcb418, 0xfbbb20, 0xfac228, 0xf9c831, 0xf8d03b, 0xf6d746, 0xf4de51,
     0xf3e55d, 0xf1eb6c, 0xf1f17a, 0xf3f689, 0xf7fa98, 0xfcffa4},

    {/* COLORMAP_PLASMA */
     0x0d0887, 0x18068b, 0x20068f, 0x280592, 0x2e0595, 0x350498, 0x3b049a, 0x41049d, 0x48039f, 0x4d02a1, 0x5302a3,
     0x5901a5, 0x5e01a6, 0x6400a7, 0x6900a8, 0x6f00a8, 0x7401a8, 0x7a02a8, 0x7f04a8, 0x8405a7, 0x8a09a5, 0x8e0ca4,
     0x9410a2, 0x99159f, 0x9d189d, 0xa21d9a, 0xa62098, 0xab2494, 0xaf2991, 0xb32c8e, 0xb7318a, 0xbb3488, 0xbf3984,
     0xc33d80, 0xc6417d, 0xca457a, 0xcc4977, 0xd04d73, 0xd35271, 0xd6556d, 0xda596a, 0xdc5d67, 0xdf6263, 0xe26561,
     0xe56a5d, 0xe76f5a, 0xe97257, 0xec7754, 0xef7b51, 0xf0804e, 0xf3844b, 0xf48948, 0xf68f44, 0xf79342, 0xf9983e,
     0xfa9d3b, 0xfba238, 0xfca735, 0xfdad33, 0xfdb22f, 0xfeb72c, 0xfebd2a, 0xfdc328, 0xfdc927, 0xfcce25, 0xfbd424,
     0xfada24, 0xf8e025, 0xf6e626, 0xf4ed27, 0xf1f327, 0xf0f921},

    {/* COLORMAP_MAGMA */
     0x000004, 0x020109, 0x030310, 0x060518, 0x08071f, 0x0c0926, 0x110b2e, 0x140e36, 0x19103e, 0x1d1148, 0x221150,
     0x281159, 0x2d1161, 0x341068, 0x390f6f, 0x400f74, 0x461078, 0x4c117a, 0x52137c, 0x57157e, 0x5d177f, 0x631a80,
     0x681c81, 0x6e1d81, 0x732081, 0x792282, 0x7f2482, 0x842681, 0x8a2981, 0x902a81, 0x952c80, 0x9b2e7f, 0xa1307e,
     0xa7317d, 0xad347c, 0xb3367a, 0xb83779, 0xbf3a77, 0xc43c75, 0xca3e72, 0xd0416f, 0xd5446d, 0xdb476a, 0xdf4b68,
     0xe44f64, 0xe95362, 0xec5860, 0xf05f5e, 0xf2645c, 0xf56b5c, 0xf7715c, 0xf9785d, 0xfa7f5e, 0xfb8660, 0xfc8c63,
     0xfd9367, 0xfd9a6a, 0xfea06e, 0xfea872, 0xfeae77, 0xfeb57b, 0xfebb81, 0xfec286, 0xfec98c, 0xfecf92, 0xfed698,
     0xfddc9e, 0xfde3a4, 0xfdeaaa, 0xfcf0b2, 0xfcf6b8, 0xfcfdbf}};

static double sizex = 0;

static int regeneration_flags = 0;

static char *xcalloc(int count, int size)
{
  char *result = (char *)calloc(count, size);
  if (!result)
    {
      fprintf(stderr, "out of virtual memory\n");
      abort();
    }
  return (result);
}

static char *xmalloc(int size)
{
  char *result = (char *)malloc(size);
  if (!result)
    {
      fprintf(stderr, "out of virtual memory\n");
      abort();
    }
  return (result);
}

static char *xrealloc(void *ptr, int size)
{
  char *result = (char *)realloc(ptr, size);
  if (!result)
    {
      fprintf(stderr, "out of virtual memory\n");
      abort();
    }
  return (result);
}

static void reallocate(int npoints)
{
  while (npoints >= maxpath) maxpath += POINT_INC;

  opcode = (unsigned char *)xrealloc(opcode, maxpath * sizeof(unsigned char));
  xpath = (double *)xrealloc(xpath, maxpath * sizeof(double));
  xpoint = (double *)xrealloc(xpoint, maxpath * sizeof(double));
  ypath = (double *)xrealloc(ypath, maxpath * sizeof(double));
  ypoint = (double *)xrealloc(ypoint, maxpath * sizeof(double));
  zpoint = (double *)xrealloc(zpoint, maxpath * sizeof(double));
  code = (int *)xrealloc(code, maxpath * sizeof(int));
}

static double x_lin(double x)
{
  double result;

  if (OPTION_X_LOG & lx.scale_options)
    {
      if (x > 0)
        result = lx.a * log10(x) + lx.b;
      else
        result = NAN;
    }
  else
    result = x;

  if (OPTION_FLIP_X & lx.scale_options) result = lx.xmax - result + lx.xmin;

  return (result);
}

static double y_lin(double y)
{
  double result;

  if (OPTION_Y_LOG & lx.scale_options)
    {
      if (y > 0)
        result = lx.c * log10(y) + lx.d;
      else
        result = NAN;
    }
  else
    result = y;

  if (OPTION_FLIP_Y & lx.scale_options) result = lx.ymax - result + lx.ymin;

  return (result);
}

static double z_lin(double z)
{
  double result;

  if (OPTION_Z_LOG & lx.scale_options)
    {
      if (z > 0)
        result = lx.e * log10(z) + lx.f;
      else
        result = NAN;
    }
  else
    result = z;

  if (OPTION_FLIP_Z & lx.scale_options) result = lx.zmax - result + lx.zmin;

  return (result);
}

static double x_log(double x)
{
  if (OPTION_FLIP_X & lx.scale_options) x = lx.xmax - x + lx.xmin;

  if (OPTION_X_LOG & lx.scale_options)
    return (pow(10.0, (double)((x - lx.b) / lx.a)));
  else
    return (x);
}

static double y_log(double y)
{
  if (OPTION_FLIP_Y & lx.scale_options) y = lx.ymax - y + lx.ymin;

  if (OPTION_Y_LOG & lx.scale_options)
    return (pow(10.0, (double)((y - lx.d) / lx.c)));
  else
    return (y);
}

static double z_log(double z)
{
  if (OPTION_FLIP_Z & lx.scale_options) z = lx.zmax - z + lx.zmin;

  if (OPTION_Z_LOG & lx.scale_options)
    return (pow(10.0, (double)((z - lx.f) / lx.e)));
  else
    return (z);
}

static double atan_2(double x, double y)
{
  double a;

  if (y == 0)
    if (x < 0)
      a = -M_PI;
    else
      a = M_PI;
  else
    a = atan(x / y);

  return (a);
}

static void apply_world_xform(double *x, double *y, double *z)
{
  double xw, yw, zw;

  if (gpx.projection_type == GR_PROJECTION_DEFAULT)
    {
      xw = wx.a1 * *x + wx.a2 * *y + wx.b;
      yw = wx.c1 * *x + wx.c2 * *y + wx.c3 * *z + wx.d;
    }
  else
    {
      double fov = gpx.fov * M_PI / 180; /* camera angle of perspektiv projection */
      double xaspect = (vxmax - vxmin) / (vymax - vymin);
      double yaspect = 1.0 / xaspect;
      if (xaspect < 1.0)
        {
          xaspect = 1.0;
        }
      else
        {
          yaspect = 1.0;
        }
      double F[3] = {tx.focus_point_x - tx.camera_pos_x, tx.focus_point_y - tx.camera_pos_y,
                     tx.focus_point_z - tx.camera_pos_z}; /* direction between camera and focus point */
      double norm_func = sqrt(F[0] * F[0] + F[1] * F[1] + F[2] * F[2]);
      double f[3] = {F[0] / norm_func, F[1] / norm_func, F[2] / norm_func};

      double x_val = *x * tx.x_axis_scale;
      double y_val = *y * tx.y_axis_scale;
      double z_val = *z * tx.z_axis_scale;

      /* transformation */
      xw = (x_val - tx.camera_pos_x) * tx.s_x + (y_val - tx.camera_pos_y) * tx.s_y + (z_val - tx.camera_pos_z) * tx.s_z;
      yw = (x_val - tx.camera_pos_x) * tx.up_x + (y_val - tx.camera_pos_y) * tx.up_y +
           (z_val - tx.camera_pos_z) * tx.up_z;
      zw = (tx.camera_pos_x - x_val) * f[0] + (tx.camera_pos_y - y_val) * f[1] + (tx.camera_pos_z - z_val) * f[2];

      if (gpx.projection_type == GR_PROJECTION_PERSPECTIVE)
        {
          /* perspective projection */
          xw = ((cos(fov / 2) / sin(fov / 2)) / xaspect * xw);
          yw = ((cos(fov / 2) / sin(fov / 2)) / yaspect * yw);
          xw /= -zw;
          yw /= -zw;
        }
      else if (gpx.projection_type == GR_PROJECTION_ORTHOGRAPHIC)
        {
          /* orthographic projection */
          xw = (xw * 2 / (gpx.right - gpx.left) / xaspect - (gpx.left + gpx.right) / (gpx.right - gpx.left));
          yw = (yw * 2 / (gpx.top - gpx.bottom) / yaspect - (gpx.bottom + gpx.top) / (gpx.top - gpx.bottom));
          zw = (zw * -2 / (gpx.far_plane - gpx.near_plane) -
                (gpx.far_plane + gpx.near_plane) / (gpx.far_plane - gpx.near_plane));
        }
    }

  *x = xw;
  *y = yw;
}

static void foreach_openws(void (*routine)(int, void *), void *arg)
{
  int state, count, n = 1, errind, ol, wkid;

  gks_inq_operating_state(&state);
  if (state >= GKS_K_WSOP)
    {
      gks_inq_open_ws(n, &errind, &ol, &wkid);
      for (count = ol; count >= 1; count--)
        {
          n = count;
          gks_inq_open_ws(n, &errind, &ol, &wkid);

          routine(wkid, arg);
        }
    }
}

static void foreach_activews(void (*routine)(int, void *), void *arg)
{
  int state, count, n = 1, errind, ol, wkid;

  gks_inq_operating_state(&state);
  if (state >= GKS_K_WSAC)
    {
      gks_inq_active_ws(n, &errind, &ol, &wkid);
      for (count = ol; count >= 1; count--)
        {
          n = count;
          gks_inq_active_ws(n, &errind, &ol, &wkid);

          routine(wkid, arg);
        }
    }
}

static void setspace(double zmin, double zmax, int rotation, int tilt)
{
  int errind, tnr;
  double wn[4], vp[4];
  double xmin = 0, xmax = 0, ymin = 0, ymax = 0, r, t, a, c;

  gks_inq_current_xformno(&errind, &tnr);
  gks_inq_xform(tnr, &errind, wn, vp);

  if (gpx.projection_type == GR_PROJECTION_PERSPECTIVE || gpx.projection_type == GR_PROJECTION_ORTHOGRAPHIC)
    {
      xmin = ix.xmin;
      xmax = ix.xmax;
      ymin = ix.ymin;
      ymax = ix.ymax;
    }
  else
    {
      xmin = wn[0];
      xmax = wn[1];
      ymin = wn[2];
      ymax = wn[3];
    }

  wx.zmin = zmin;
  wx.zmax = zmax;
  wx.phi = rotation;
  wx.delta = tilt;

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
}

static int setscale(int options)
{
  int errind, tnr;
  double wn[4], vp[4];
  double x_min, x_max, y_min, y_max, z_min, z_max;
  int result = 0;

  gks_inq_current_xformno(&errind, &tnr);
  gks_inq_xform(tnr, &errind, wn, vp);

  x_min = wn[0];
  x_max = wn[1];
  y_min = wn[2];
  y_max = wn[3];
  z_min = wx.zmin;
  z_max = wx.zmax;

  nx.a = (vp[1] - vp[0]) / (x_max - x_min);
  nx.b = vp[0] - x_min * nx.a;
  nx.c = (vp[3] - vp[2]) / (y_max - y_min);
  nx.d = vp[2] - y_min * nx.c;

  lx.scale_options = 0;

  lx.xmin = x_min;
  lx.xmax = x_max;

  if (OPTION_X_LOG & options)
    {
      if (x_min > 0)
        {
          lx.a = (x_max - x_min) / log10(x_max / x_min);
          lx.b = x_min - lx.a * log10(x_min);
          lx.scale_options |= OPTION_X_LOG;
        }
      else
        result = -1;
    }

  lx.ymin = y_min;
  lx.ymax = y_max;

  if (OPTION_Y_LOG & options)
    {
      if (y_min > 0)
        {
          lx.c = (y_max - y_min) / log10(y_max / y_min);
          lx.d = y_min - lx.c * log10(y_min);
          lx.scale_options |= OPTION_Y_LOG;
        }
      else
        result = -1;
    }

  setspace(z_min, z_max, wx.phi, wx.delta);

  lx.zmin = z_min;
  lx.zmax = z_max;

  if (OPTION_Z_LOG & options)
    {
      if (lx.zmin > 0)
        {
          lx.e = (lx.zmax - lx.zmin) / log10(lx.zmax / lx.zmin);
          lx.f = lx.zmin - lx.e * log10(lx.zmin);
          lx.scale_options |= OPTION_Z_LOG;
        }
      else
        result = -1;
    }

  if (OPTION_FLIP_X & options) lx.scale_options |= OPTION_FLIP_X;

  if (OPTION_FLIP_Y & options) lx.scale_options |= OPTION_FLIP_Y;

  if (OPTION_FLIP_Z & options) lx.scale_options |= OPTION_FLIP_Z;

  return result;
}

static void initialize(int state)
{
  int tnr = WC, font = 3, options = 0;
  double xmin = 0.2, xmax = 0.9, ymin = 0.2, ymax = 0.9;
  int asf[13] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
  double size = 2, height = 0.027;

  if (state == GKS_K_GKCL)
    {
      gks_select_xform(tnr);
      gks_set_viewport(tnr, xmin, xmax, ymin, ymax);

      gks_set_asf(asf);
      gks_set_pmark_size(size);
      gks_set_pmark_type(GKS_K_MARKERTYPE_ASTERISK);
      gks_set_text_fontprec(font, GKS_K_TEXT_PRECISION_STRING);
      gks_set_text_height(height);
      gks_set_text_align(GKS_K_TEXT_HALIGN_LEFT, GKS_K_TEXT_VALIGN_BASE);
    }

  autoinit = 0;
  double_buf = gks_getenv("GKS_DOUBLE_BUF") != NULL;
  display = gks_getenv("GR_DISPLAY");
  if (display)
    if (*display == '\0') display = NULL;

  setscale(options);
}

#ifdef SIGUSR1

static void resetgks(int sig)
{
  static int exiting = 0;

  if (sig == SIGUSR1)
    {
      if (!exiting)
        {
          exiting = 1;
          gr_emergencyclosegks();
          exiting = 0;
        }

      signal(SIGUSR1, previous_handler);
      if (previous_handler != SIG_DFL) raise(SIGUSR1);
    }
}

#else

static void resetgks(void)
{
  static int exiting = 0;

  if (!exiting)
    {
      exiting = 1;
      gr_emergencyclosegks();
      exiting = 0;
    }
}

#endif


static void initgks(void)
{
  int state, errfil = 0, wkid = 1, errind, conid, wtype, color;
  double r, g, b;

  gks_inq_operating_state(&state);
  if (state == GKS_K_GKCL) gks_open_gks(errfil);

  initialize(state);

  if (state == GKS_K_GKCL || state == GKS_K_GKOP)
    {
      gks_open_ws(wkid, GKS_K_CONID_DEFAULT, GKS_K_WSTYPE_DEFAULT);
      gks_activate_ws(wkid);
    }

  gks_inq_ws_conntype(wkid, &errind, &conid, &wtype);
  if (!double_buf)
    {
      double_buf = wtype == 380 || wtype == 381 || wtype == 400 || wtype == 410 || wtype == 411;
    }

  if (display)
    {
      if (gr_openstream(display) == 0)
        {
          gr_writestream(XML_HEADER);
          gr_writestream(GR_HEADER);
          flag_graphics = 1;
        }
      else
        fprintf(stderr, "%s: open failed\n", display);
    }

  for (color = 0; color < MAX_COLOR; color++)
    {
      gks_inq_color_rep(wkid, color, GKS_K_VALUE_SET, &errind, &r, &g, &b);
      rgb[color] = ((nint(r * 255) & 0xff)) | ((nint(g * 255) & 0xff) << 8) | ((nint(b * 255) & 0xff) << 16);
      used[color] = 0;
    }

#ifdef SIGUSR1
  previous_handler = signal(SIGUSR1, resetgks);
#else
  atexit(resetgks);
#endif
}

void gr_initgr(void)
{
  if (!autoinit)
    {
      gks_init_gks();

      initialize(GKS_K_GKCL);
    }
}

void gr_opengks(void)
{
  int errfil = 0;

  gks_open_gks(errfil);

  initialize(GKS_K_GKCL);
}

void gr_closegks(void)
{
  gks_close_gks();
  autoinit = 1;
}

/*!
 * Get the current display size.
 *
 * \param[out] mwidth Display width in meters
 * \param[out] mheight Display height in meters
 * \param[out] width Display width in pixels
 * \param[out] height Display height in pixels
 *
 * Depending on the current workstation type, the current display might be
 * the primary screen (e.g. when using gksqt or GKSTerm) or a purely virtual
 * display (e.g. when using Cairo). When a high DPI screen is used as the
 * current display, width and height will be in logical pixels.
 */
void gr_inqdspsize(double *mwidth, double *mheight, int *width, int *height)
{
  int n = 1, errind, wkid, ol, conid, wtype, dcunit;

  check_autoinit;

  gks_inq_open_ws(n, &errind, &ol, &wkid);
  gks_inq_ws_conntype(wkid, &errind, &conid, &wtype);
  gks_inq_max_ds_size(wtype, &errind, &dcunit, mwidth, mheight, width, height);
}

/*!
 * Open a graphical workstation.
 *
 * \param[in] workstation_id a workstation identifier
 * \param[in] connection a connection identifier
 * \param[in] type the desired workstation type
 *
 *
 * \verbatim embed:rst:leading-asterisk
 *
 * Available workstation types:
 *
 * +-------------+------------------------------------------------------+
 * |     **Type**|**Description**                                       |
 * +-------------+------------------------------------------------------+
 * |            5|Workstation Independent Segment Storage               |
 * +-------------+------------------------------------------------------+
 * |           41|Windows GDI                                           |
 * +-------------+------------------------------------------------------+
 * |      61 - 64|PostScript (b/w, color)                               |
 * +-------------+------------------------------------------------------+
 * |     101, 102|Portable Document Format (plain, compressed)          |
 * +-------------+------------------------------------------------------+
 * |    210 - 213|X Windows                                             |
 * +-------------+------------------------------------------------------+
 * |          214|Sun Raster file (RF)                                  |
 * +-------------+------------------------------------------------------+
 * |     215, 218|Graphics Interchange Format (GIF87, GIF89)            |
 * +-------------+------------------------------------------------------+
 * |          216|Motif User Interface Language (UIL)                   |
 * +-------------+------------------------------------------------------+
 * |          320|Windows Bitmap (BMP)                                  |
 * +-------------+------------------------------------------------------+
 * |          321|JPEG image file                                       |
 * +-------------+------------------------------------------------------+
 * |          322|Portable Network Graphics file (PNG)                  |
 * +-------------+------------------------------------------------------+
 * |          323|Tagged Image File Format (TIFF)                       |
 * +-------------+------------------------------------------------------+
 * |          371|Gtk                                                   |
 * +-------------+------------------------------------------------------+
 * |          380|wxWidgets                                             |
 * +-------------+------------------------------------------------------+
 * |          381|Qt4                                                   |
 * +-------------+------------------------------------------------------+
 * |          382|Scaleable Vector Graphics (SVG)                       |
 * +-------------+------------------------------------------------------+
 * |          390|Windows Metafile                                      |
 * +-------------+------------------------------------------------------+
 * |          400|Quartz                                                |
 * +-------------+------------------------------------------------------+
 * |          410|Socket driver                                         |
 * +-------------+------------------------------------------------------+
 * |          415|0MQ driver                                            |
 * +-------------+------------------------------------------------------+
 * |          420|OpenGL                                                |
 * +-------------+------------------------------------------------------+
 *
 * \endverbatim
 */
void gr_openws(int workstation_id, char *connection, int type)
{
  if (connection)
    {
      if (!*connection) connection = NULL;
    }
  gks_open_ws(workstation_id, connection, type);
}

/*!
 * Close the specified workstation.
 *
 * \param[in] workstation_id a workstation identifier
 */
void gr_closews(int workstation_id)
{
  gks_close_ws(workstation_id);
}

/*!
 * Activate the specified workstation.
 *
 * \param[in] workstation_id a workstation identifier
 */
void gr_activatews(int workstation_id)
{
  gks_activate_ws(workstation_id);
}

/*!
 * Deactivate the specified workstation.
 *
 * \param[in] workstation_id a workstation identifier
 */
void gr_deactivatews(int workstation_id)
{
  gks_deactivate_ws(workstation_id);
}

static void configure(int workstation_id, void *dummy)
{
  gks_configure_ws(workstation_id);
}

/*!
 * Configure the specified workstation.
 */
void gr_configurews(void)
{
  foreach_activews((void (*)(int, void *))configure, (void *)NULL);
}

static void clear(int workstation_id, int *clearflag)
{
  int wkid = workstation_id, state, errind, conid, wtype, wkcat;

  gks_inq_operating_state(&state);
  if (state == GKS_K_SGOP) gks_close_seg();

  gks_inq_ws_conntype(wkid, &errind, &conid, &wtype);
  gks_inq_ws_category(wtype, &errind, &wkcat);

  if (wkcat == GKS_K_WSCAT_OUTPUT || wkcat == GKS_K_WSCAT_OUTIN || wkcat == GKS_K_WSCAT_MO)
    {
      gks_clear_ws(wkid, *clearflag);
      gks_update_ws(wkid, GKS_K_POSTPONE_FLAG);
    }
}

void gr_clearws(void)
{
  int clearflag = double_buf ? GKS_K_CLEAR_CONDITIONALLY : GKS_K_CLEAR_ALWAYS;

  check_autoinit;

  foreach_activews((void (*)(int, void *))clear, (void *)&clearflag);

  if (flag_graphics)
    {
      gr_writestream(GR_TRAILER);
      gr_flushstream(1);
      gr_writestream(XML_HEADER);
      gr_writestream(GR_HEADER);
    }

  def_color = 0;
}

static void update(int workstation_id, int *regenflag)
{
  int wkid = workstation_id, errind, conid, wtype, wkcat;

  gks_inq_ws_conntype(wkid, &errind, &conid, &wtype);
  gks_inq_ws_category(wtype, &errind, &wkcat);

  if (wkcat == GKS_K_WSCAT_OUTPUT || wkcat == GKS_K_WSCAT_OUTIN) gks_update_ws(wkid, *regenflag);
}

void gr_updatews(void)
{
  int regenflag = double_buf ? GKS_K_PERFORM_FLAG : GKS_K_POSTPONE_FLAG;
  regenflag |= GKS_K_WRITE_PAGE_FLAG;

  check_autoinit;

  foreach_openws((void (*)(int, void *))update, (void *)&regenflag);

  if (flag_graphics)
    if (display)
      {
        gr_writestream(GR_TRAILER);
        gr_flushstream(0);
        gr_writestream(GR_HEADER);
      }
}

static void fillarea(int n, double *x, double *y)
{
  int errind, style;
  int npoints = n;
  double *px = x, *py = y;
  int i;

  check_autoinit;

  if (lx.scale_options)
    {
      if (npoints >= maxpath) reallocate(npoints);

      px = xpoint;
      py = ypoint;
      for (i = 0; i < npoints; i++)
        {
          px[i] = x_lin(x[i]);
          py[i] = y_lin(y[i]);
        }
    }

  gks_inq_fill_int_style(&errind, &style);

  if (style == GKS_K_INTSTYLE_SOLID_WITH_BORDER)
    {
      if (npoints + 1 >= maxpath) reallocate(npoints + 1);

      code[0] = 'M';
      for (i = 1; i < npoints; i++) code[i] = 'L';
      code[npoints] = 'F';

      gks_gdp(npoints, px, py, GKS_K_GDP_DRAW_PATH, npoints + 1, code);
    }
  else
    {
      gks_fillarea(npoints, px, py);
    }
}

static void print_int_array(char *name, int n, int *data)
{
  int i;

  gr_writestream(" %s=\"", name);
  for (i = 0; i < n; i++)
    {
      if (i > 0) gr_writestream(" ");
      gr_writestream("%d", data[i]);
    }
  gr_writestream("\"");
}

static void print_float_array(char *name, int n, double *data)
{
  int i;

  gr_writestream(" %s=\"", name);
  for (i = 0; i < n; i++)
    {
      if (i > 0) gr_writestream(" ");
      gr_writestream("%g", data[i]);
    }
  gr_writestream("\"");
}

static void print_vertex_array(char *name, int n, vertex_t *vertices)
{
  int i;

  gr_writestream(" %s=\"", name);
  for (i = 0; i < n; i++)
    {
      if (i > 0) gr_writestream(" ");
      gr_writestream("%g %g", vertices[i].x, vertices[i].y);
    }
  gr_writestream("\"");
}

static void print_byte_array(char *name, int n, unsigned char *data)
{
  int i;

  gr_writestream(" %s=\"", name);
  for (i = 0; i < n; i++)
    {
      if (i > 0) gr_writestream(" ");
      gr_writestream("%d", data[i]);
    }
  gr_writestream("\"");
}

static void primitive(char *name, int n, double *x, double *y)
{
  gr_writestream("<%s len=\"%d\"", name, n);
  print_float_array("x", n, x);
  print_float_array("y", n, y);
  gr_writestream("/>\n");
}

static void polyline(int n, double *x, double *y)
{
  int npoints = n;
  double *px = x, *py = y;
  int i;

  if (lx.scale_options)
    {
      if (npoints >= maxpath) reallocate(npoints);

      px = xpoint;
      py = ypoint;
      npoints = 0;
      for (i = 0; i < n; i++)
        {
          px[npoints] = x_lin(x[i]);
          py[npoints] = y_lin(y[i]);
          if (is_nan(px[npoints]) || is_nan(py[npoints]))
            {
              if (npoints >= 2) gks_polyline(npoints, px, py);

              npoints = 0;
            }
          else
            npoints++;
        }
    }

  if (npoints != 0) gks_polyline(npoints, px, py);
}

/*!
 * Draw a polyline using the current line attributes, starting from the
 * first data point and ending at the last data point.
 *
 * \param[in] n The number of points
 * \param[in] x A pointer to the X coordinates
 * \param[in] y A pointer to the Y coordinates
 *
 * The values for `x` and `y` are in world coordinates. The attributes that
 * control the appearance of a polyline are linetype, linewidth and color
 * index.
 */
void gr_polyline(int n, double *x, double *y)
{
  check_autoinit;

  polyline(n, x, y);

  if (flag_graphics) primitive("polyline", n, x, y);
}

static void polymarker(int n, double *x, double *y)
{
  int npoints = n;
  double *px = x, *py = y;
  int i;

  if (lx.scale_options)
    {
      if (npoints >= maxpath) reallocate(npoints);

      px = xpoint;
      py = ypoint;
      npoints = 0;
      for (i = 0; i < n; i++)
        {
          px[npoints] = x_lin(x[i]);
          py[npoints] = y_lin(y[i]);
          if (is_nan(px[npoints]) || is_nan(py[npoints]))
            {
              if (npoints >= 1) gks_polymarker(npoints, px, py);

              npoints = 0;
            }
          else
            npoints++;
        }
    }

  if (npoints != 0) gks_polymarker(npoints, px, py);
}

/*!
 * Draw marker symbols centered at the given data points.
 *
 * \param[in] n The number of points
 * \param[in] x A pointer to the X coordinates
 * \param[in] y A pointer to the Y coordinates
 *
 * The values for `x` and `y` are in world coordinates. The attributes that
 * control the appearance of a polymarker are marker type, marker size
 * scale factor and color index.
 */
void gr_polymarker(int n, double *x, double *y)
{
  check_autoinit;

  polymarker(n, x, y);

  if (flag_graphics) primitive("polymarker", n, x, y);
}

/*!
 * Draw a text at position `x`, `y` using the current text attributes.
 *
 * \param[in] x The X coordinate of the starting position of the text string
 * \param[in] y The Y coordinate of the starting position of the text string
 * \param[in] string The text to be drawn
 *
 * The values for `x` and `y` are in normalized device coordinates.
 * The attributes that control the appearance of text are text font and
 * precision, character expansion factor, character spacing, text color index,
 * character height, character up vector, text path and text alignment.
 */
void gr_text(double x, double y, char *string)
{
  int errind, tnr, halign, valign, n;
  char *s, *t;
  double ux, uy, angle, height;
  double rx, ry, sx, sy;

  check_autoinit;

  gks_inq_current_xformno(&errind, &tnr);
  if (tnr != NDC) gks_select_xform(NDC);

  if (strchr(string, '\n') != NULL)
    {
      gks_inq_text_align(&errind, &halign, &valign);
      gks_inq_text_upvec(&errind, &ux, &uy);
      angle = -atan2(ux, uy);
      gks_inq_text_height(&errind, &height);
      height *= 1.5;

      n = 0;
      s = string;
      while (*s)
        if (*s++ == '\n') n++;

      rx = x;
      ry = y;
      switch (valign)
        {
        case 3:
          rx = x - sin(angle) * 0.5 * n * height;
          ry = y + cos(angle) * 0.5 * n * height;
          break;
        case 4:
        case 5:
          rx = x - sin(angle) * n * height;
          ry = y + cos(angle) * n * height;
          break;
        }

      t = gks_strdup(string);
      n = 0;
      s = strtok(t, "\n");
      while (s != NULL)
        {
          sx = rx + sin(angle) * n * height;
          sy = ry - cos(angle) * n * height;
          gks_text(sx, sy, s);
          s = strtok(NULL, "\n");
          n++;
        }
      free(t);
    }
  else
    gks_text(x, y, string);

  if (tnr != NDC) gks_select_xform(tnr);

  if (flag_graphics) gr_writestream("<text x=\"%g\" y=\"%g\" text=\"%s\"/>\n", x, y, string);
}

void gr_inqtext(double x, double y, char *string, double *tbx, double *tby)
{
  int errind, tnr, halign, valign;
  char *s, *t;
  double ux, uy, angle, width = 0.0, height, chh;
  int i;
  int n, wkid = 0;
  double rx, ry, xx, yy, cpx, cpy;

  check_autoinit;

  gks_inq_current_xformno(&errind, &tnr);
  if (tnr != NDC) gks_select_xform(NDC);

  gks_inq_open_ws(1, &errind, &n, &wkid);

  if (strchr(string, '\n') != NULL)
    {
      gks_inq_text_align(&errind, &halign, &valign);
      gks_inq_text_upvec(&errind, &ux, &uy);

      gks_set_text_upvec(0.0, 1.0);

      t = gks_strdup(string);
      n = 0;
      s = strtok(t, "\n");
      while (s != NULL)
        {
          gks_inq_text_extent(wkid, x, y, s, &errind, &cpx, &cpy, tbx, tby);
          s = strtok(NULL, "\n");
          width = max(tbx[1] - tbx[0], width);
          n++;
        }
      free(t);

      gks_set_text_upvec(ux, uy);

      angle = -atan2(ux, uy);
      gks_inq_text_height(&errind, &chh);
      height = chh * n * 1.5;

      rx = x;
      switch (halign)
        {
        case 2:
          rx -= 0.5 * width;
          break;
        case 3:
          rx -= width;
          break;
        }
      ry = y;
      switch (valign)
        {
        case 1:
          ry -= height - chh * 0.04;
          break;
        case 2:
          ry -= height;
          break;
        case 3:
          ry -= 0.5 * height;
          break;
        case 5:
          ry -= chh * 0.04;
          break;
        }
      tbx[0] = rx;
      tbx[1] = rx + width;
      tbx[2] = tbx[1];
      tbx[3] = tbx[0];
      tby[0] = ry;
      tby[1] = tby[0];
      tby[2] = ry + height;
      tby[3] = tby[2];

      for (i = 0; i < 4; i++)
        {
          xx = tbx[i] - x;
          yy = tby[i] - y;
          tbx[i] = x + cos(angle) * xx - sin(angle) * yy;
          tby[i] = y + sin(angle) * xx + cos(angle) * yy;
        }

      cpx = tbx[1];
      cpy = tby[1];
    }
  else
    gks_inq_text_extent(wkid, x, y, string, &errind, &cpx, &cpy, tbx, tby);

  if (tnr != NDC)
    {
      gks_select_xform(tnr);

      for (i = 0; i < 4; i++)
        {
          tbx[i] = (tbx[i] - nx.b) / nx.a;
          tby[i] = (tby[i] - nx.d) / nx.c;
          if (lx.scale_options)
            {
              tbx[i] = x_log(tbx[i]);
              tby[i] = y_log(tby[i]);
            }
        }
    }
}

/*!
 * Allows you to specify a polygonal shape of an area to be filled.
 *
 * \param[in] n The number of points
 * \param[in] x A pointer to the X coordinates
 * \param[in] y A pointer to the Y coordinates
 *
 * The attributes that control the appearance of fill areas are fill area
 * interior style, fill area style index and fill area color index.
 */
void gr_fillarea(int n, double *x, double *y)
{
  fillarea(n, x, y);

  if (flag_graphics) primitive("fillarea", n, x, y);
}

/*!
 * Display rasterlike images in a device-independent manner. The cell array
 * function partitions a rectangle given by two corner points into DIMX X DIMY
 * cells, each of them colored individually by the corresponding color index
 * of the given cell array.
 *
 * \param[in] xmin X coordinate of the lower left point of the rectangle
 * \param[in] xmax X coordinate of the upper right point of the rectangle
 * \param[in] ymin Y coordinate of the lower left point of the rectangle
 * \param[in] ymax Y coordinate of the upper right point of the rectangle
 * \param[in] dimx X dimension of the color index array
 * \param[in] dimy Y dimension of the color index array
 * \param[in] scol number of leading columns in the color index array
 * \param[in] srow number of leading rows in the color index array
 * \param[in] ncol total number of columns in the color index array
 * \param[in] nrow total number of rows in the color index array
 * \param[in] color color index array
 *
 * The values for `xmin`, `xmax`, `ymin` and `ymax` are in world coordinates.
 */
void gr_cellarray(double xmin, double xmax, double ymin, double ymax, int dimx, int dimy, int scol, int srow, int ncol,
                  int nrow, int *color)
{
  check_autoinit;

  gks_cellarray(x_lin(xmin), y_lin(ymax), x_lin(xmax), y_lin(ymin), dimx, dimy, scol, srow, ncol, nrow, color);

  if (flag_graphics)
    {
      gr_writestream("<cellarray xmin=\"%g\" xmax=\"%g\" ymin=\"%g\" ymax=\"%g\" "
                     "dimx=\"%d\" dimy=\"%d\" scol=\"%d\" srow=\"%d\" "
                     "ncol=\"%d\" nrow=\"%d\"",
                     xmin, xmax, ymin, ymax, dimx, dimy, scol, srow, ncol, nrow);
      print_int_array("color", dimx * dimy, color);
      gr_writestream("/>\n");
    }
}

/*!
 * Display a two dimensional color index array with nonuniform cell sizes.
 *
 * \param[in] x X coordinates of the cell edges
 * \param[in] y Y coordinates of the cell edges
 * \param[in] dimx actual X dimension of the color index array in the memory
 * \param[in] dimy actual Y dimension of the color index array in the memory
 * \param[in] scol 1-based starting column of color index and x array
 * \param[in] srow 1-based starting row of color index and y array
 * \param[in] ncol number of columns displayed
 * \param[in] nrow number of rows displayed
 * \param[in] color color index array
 *
 * The values for `x` and `y` are in world coordinates.
 *
 * If `dimx` and `dimy` are positive `x` must contain `dimx`+1 elements and `y` must contain `dimy`+1 elements. The
 * elements i and i+1 are respectively the edges of the i-th cell in X and Y direction.
 *
 * If `dimx` and `dimy` are negative `x` must contain `-dimx` elements and `y` must contain `-dimy` elements. The i-th
 * element is the center of the i-th cell, and the first and last elements will also be the edges of the outer cells.
 *
 * To draw all cells of the color index array use:
 *
 *      gr_nonuniformcellarray(x, y, dimx, dimy, 1, 1, dimx, dimy, color)
 *
 * `scol` and `srow` can be used to specify a (1-based) starting column and row
 * in the `color`, `x` and `y` array. `dimx` and `dimy` specify the actual dimension of the
 * arrays in the memory whereof `ncol` and `nrow` values are displayed.
 */
void gr_nonuniformcellarray(double *x, double *y, int dimx, int dimy, int scol, int srow, int ncol, int nrow,
                            int *color)
{
  int img_data_x, img_data_y, color_x_ind, color_y_ind, color_ind, edges_x = 1, edges_y = 1, size = 2000;
  int *img_data;
  double x_pos, y_pos, x_size, y_size, *x_orig = NULL, *y_orig = NULL;

  if (dimx < 0)
    {
      dimx = -dimx;
      edges_x = 0;
      x_orig = x;
    }
  if (dimy < 0)
    {
      dimy = -dimy;
      edges_y = 0;
      y_orig = y;
    }

  if (scol < 1 || srow < 1 || scol + ncol - 1 > dimx || srow + nrow - 1 > dimy || (!edges_x && ncol < 2) ||
      (!edges_y && nrow < 2))
    {
      fprintf(stderr, "Dimensions of color index array are invalid.\n");
      return;
    }

  check_autoinit;

  scol--;
  srow--;
  nrow += srow;
  ncol += scol;

  if (!edges_x)
    {
      x = (double *)gks_malloc(sizeof(double) * (ncol + 1));
      x[scol] = x_orig[scol];
      for (color_x_ind = scol + 1; color_x_ind < ncol; color_x_ind++)
        {
          x[color_x_ind] = 0.5 * (x_orig[color_x_ind] + x_orig[color_x_ind - 1]);
        }
      x[ncol] = x_orig[ncol - 1];
    }

  if (!edges_y)
    {
      y = (double *)gks_malloc(sizeof(double) * (nrow + 1));
      y[srow] = y_orig[srow];
      for (color_y_ind = srow + 1; color_y_ind < nrow; color_y_ind++)
        {
          y[color_y_ind] = 0.5 * (y_orig[color_y_ind] + y_orig[color_y_ind - 1]);
        }
      y[nrow] = y_orig[nrow - 1];
    }

  for (color_x_ind = scol; color_x_ind < ncol; color_x_ind++)
    {
      if (x[color_x_ind] > x[color_x_ind + 1])
        {
          if (!edges_x)
            {
              gks_free(x);
            }
          if (!edges_y)
            {
              gks_free(y);
            }
          fprintf(stderr, "x points not sorted in ascending order\n");
          return;
        }
    }

  for (color_y_ind = srow; color_y_ind < nrow; color_y_ind++)
    {
      if (y[color_y_ind] > y[color_y_ind + 1])
        {
          if (!edges_x)
            {
              gks_free(x);
            }
          if (!edges_y)
            {
              gks_free(y);
            }
          fprintf(stderr, "y points not sorted in ascending order\n");
          return;
        }
    }

  x_size = x[ncol] - x[scol];
  y_size = y[nrow] - y[srow];
  img_data = (int *)xmalloc(size * size * sizeof(int));

  color_y_ind = srow;
  for (img_data_y = 0; img_data_y < size; img_data_y++)
    {
      y_pos = y[srow] + img_data_y * y_size / size;
      while (color_y_ind < nrow && y[color_y_ind + 1] <= y_pos)
        {
          color_y_ind++;
        }
      color_x_ind = scol;
      for (img_data_x = 0; img_data_x < size; img_data_x++)
        {
          x_pos = x[scol] + img_data_x * x_size / size;
          while (color_x_ind < ncol && x[color_x_ind + 1] <= x_pos)
            {
              color_x_ind++;
            }
          color_ind = color[color_y_ind * dimx + color_x_ind];
          if (color_ind >= 0 && color_ind < MAX_COLOR)
            {
              img_data[img_data_y * size + img_data_x] = (255 << 24) + rgb[color_ind];
            }
          else
            {
              /* invalid color indices in input data result in transparent pixel */
              img_data[img_data_y * size + img_data_x] = 0;
            }
        }
    }

  gr_drawimage(x[scol], x[ncol], y[nrow], y[srow], size, size, img_data, 0);
  free(img_data);
  if (!edges_x)
    {
      gks_free(x);
    }
  if (!edges_y)
    {
      gks_free(y);
    }
}

/*!
 * Display a two dimensional color index array mapped to a disk using polar
 * coordinates.
 *
 * \param[in] x_org X coordinate of the disk center in world coordinates
 * \param[in] y_org Y coordinate of the disk center in world coordinates
 * \param[in] phimin start angle of the disk sector in degrees
 * \param[in] phimax end angle of the disk sector in degrees
 * \param[in] rmin inner radius of the punctured disk in world coordinates
 * \param[in] rmax outer radius of the disk in world coordinates
 * \param[in] dimphi Phi (X) dimension of the color index array
 * \param[in] dimr R (Y) dimension of the color index array
 * \param[in] scol number of leading columns in the color index array
 * \param[in] srow number of leading rows in the color index array
 * \param[in] ncol total number of columns in the color index array
 * \param[in] nrow total number of rows in the color index array
 * \param[in] color color index array
 *
 * The two dimensional color index array is mapped to the resulting image by
 * interpreting the X-axis of the array as the angle and the Y-axis as the radius.
 * The center point of the resulting disk is located at `x_org`, `y_org` and the
 * radius of the disk is `rmax`.
 *
 * To draw a contiguous array as a complete disk use:
 *
 *     gr_polarcellarray(x_org, y_org, 0, 360, 0, rmax, dimphi, dimr, 1, 1, dimphi, dimr, color)
 *
 * The additional parameters to the function can be used to further control the
 * mapping from polar to cartesian coordinates.
 *
 * If `rmin` is greater than 0 the input data is mapped to a punctured disk (or
 * annulus) with an inner radius of `rmin` and an outer radius `rmax`. If `rmin`
 * is greater than `rmax` the Y-axis of the array is reversed.
 *
 * The parameter `phimin` and `phimax` can be used to map the data to a sector
 * of the (punctured) disk starting at `phimin` and ending at `phimax`. If
 * `phimin` is greater than `phimax` the X-axis is reversed. The visible sector
 * is the one starting in mathematically positive direction (counterclockwise)
 * at the smaller angle and ending at the larger angle. An example of the four
 * possible options can be found below:
 *
 * \verbatim embed:rst:leading-asterisk
 *
 * +-----------+-----------+---------------------------------------------------+
 * |**phimin** |**phimax** |**Result**                                         |
 * +-----------+-----------+---------------------------------------------------+
 * |90         |270        |Left half visible, mapped counterclockwise         |
 * +-----------+-----------+---------------------------------------------------+
 * |270        |90         |Left half visible, mapped clockwise                |
 * +-----------+-----------+---------------------------------------------------+
 * |-90        |90         |Right half visible, mapped counterclockwise        |
 * +-----------+-----------+---------------------------------------------------+
 * |90         |-90        |Right half visible, mapped clockwise               |
 * +-----------+-----------+---------------------------------------------------+
 *
 * \endverbatim
 *
 * `scol` and `srow` can be used to specify a (1-based) starting column and row
 * in the `color` array. `ncol` and `nrow` specify the actual dimension of the
 * array in the memory whereof `dimphi` and `dimr` values are mapped to the disk.
 *
 */
void gr_polarcellarray(double x_org, double y_org, double phimin, double phimax, double rmin, double rmax, int dimphi,
                       int dimr, int scol, int srow, int ncol, int nrow, int *color)
{
  int x, y, color_ind, r_ind, phi_ind, phi_reverse, phi_wrapped_reverse, r_reverse, size = 2000;
  int *img_data;
  double r, phi, tmp, center = size / 2.;

  phimin = arc(phimin);
  phimax = arc(phimax);

  if (!(scol >= 1 && srow >= 1 && scol + ncol - 1 <= dimphi && srow + nrow - 1 <= dimr))
    {
      fprintf(stderr, "Dimensions of color index array are invalid.\n");
      return;
    }

  if (phimin == phimax)
    {
      fprintf(stderr, "Invalid angles specified.\n");
      return;
    }

  if (rmin == rmax || rmin < 0 || rmax < 0)
    {
      fprintf(stderr, "Invalid radii specified.\n");
      return;
    }

  check_autoinit;

  if ((r_reverse = rmin > rmax))
    {
      tmp = rmin;
      rmin = rmax;
      rmax = tmp;
    }

  phi_reverse = phimin > phimax;

  /* Wrap phimin and phimax to [0, 2Pi] */
  phimin -= floor(phimin / M_PI / 2) * 2 * M_PI;
  phimax -= floor(phimax / M_PI / 2) * 2 * M_PI;

  if (fabs(phimin - phimax) < 1e-8) /* Avoid empty images when drawing a full circle */
    {
      if (!phi_reverse)
        {
          phimax += 2 * M_PI;
        }
      else
        {
          phimin += 2 * M_PI;
        }
    }

  if ((phi_wrapped_reverse = phimin > phimax))
    {
      tmp = phimin;
      phimin = phimax;
      phimax = tmp;
    }

  if (phi_reverse != phi_wrapped_reverse) /* Drawing in negative direction */
    {
      phimin += 2 * M_PI;
    }

  img_data = (int *)xmalloc(size * size * sizeof(int));

  for (y = 0; y < size; y++)
    {
      for (x = 0; x < size; x++)
        {
          double px = (x - center) / center;
          double py = (y - center) / center;
          r = sqrt(px * px + py * py);
          phi = atan2(py, px);
          if (phi < min(phimin, phimax))
            {
              phi += 2 * M_PI;
            }

          /* map phi from [phimin, phimax] to [0, 1] */
          phi = (phi - phimin) / (phimax - phimin);

          if (r * rmax < rmin || r >= 1 || phi < 0 || phi > 1)
            {
              img_data[y * size + x] = 0;
              continue;
            }
          r = (r * rmax - rmin) / (rmax - rmin);
          r_ind = (int)(r * dimr);
          phi_ind = (int)(phi * dimphi) % dimphi;
          if (r_reverse)
            {
              r_ind = dimr - r_ind - 1;
            }
          if (phi_wrapped_reverse)
            {
              phi_ind = dimphi - phi_ind - 1;
            }
          color_ind = color[(r_ind + srow - 1) * ncol + phi_ind + scol - 1];
          if (color_ind >= 0 && color_ind < MAX_COLOR)
            {
              img_data[y * size + x] = (255 << 24) + rgb[color_ind];
            }
          else
            {
              /* invalid color indices in input data result in transparent pixel */
              img_data[y * size + x] = 0;
            }
        }
    }

  gr_drawimage(x_org - rmax, x_org + rmax, y_org + rmax, y_org - rmax, size, size, img_data, 0);
  free(img_data);
}

/*!
 * Display a two dimensional color index array mapped to a disk using polar
 * coordinates with nonuniform cell sizes.
 *
 * \param[in] x_org X coordinate of the disk center in world coordinates
 * \param[in] y_org Y coordinate of the disk center in world coordinates
 * \param[in] phi array with the angles of the disk sector in degrees
 * \param[in] r array with the radii of the disk in world coordinates
 * \param[in] dimphi Phi (X) dimension of the color index array
 * \param[in] dimr R (Y) dimension of the color index array
 * \param[in] scol number of leading columns in the color index array and the angle array
 * \param[in] srow number of leading rows in the color index array and the radii array
 * \param[in] ncol total number of columns in the color index array and the angle array
 * \param[in] nrow total number of rows in the color index array and the radii array
 * \param[in] color color index array
 *
 * The mapping of the polar coordinates and the drawing is performed simialr to `gr_polarcellarray`
 * with the difference that the individual cell sizes are specified allowing nonuniform sized cells.
 *
 * If `dimphi` and `dimr` are positive `phi` must contain `dimphi`+1 elements and `r` must contain `dimr`+1 elements and
 * the elements i and i+1 are respectively the edges of the i-th cell.
 *
 * If `dimphi` and `dimr` are negative `phi` must contain `-dimphi` elements and `r` must contain `-dimr` elements and
 * the i-th element is the center of the i-th cell, and the first and last elements will also be the edges of the outer
 * cells.
 *
 * `scol` and `srow` can be used to specify a (1-based) starting column and row
 * in the `color`, `phi` and `r` array. `dimr` and `dimphi` specify the actual dimension of the
 * arrays in the memory whereof `ncol` and `nrow` values are displayed.
 *
 */
void gr_nonuniformpolarcellarray(double x_org, double y_org, double *phi, double *r, int dimphi, int dimr, int scol,
                                 int srow, int ncol, int nrow, int *color)
{
  int x, y, color_ind, r_ind, phi_ind, phi_reverse, r_reverse, start, end, edges_phi = 1, edges_r = 1, size = 2000;
  int *img_data;
  double cur_r, cur_phi, tmp, phimin, phimax, rmin, rmax, center = size / 2.;
  double *r_sorted, *phi_sorted;
  if (dimphi < 0)
    {
      edges_phi = 0;
      dimphi = -dimphi;
      ncol--;
    }
  if (dimr < 0)
    {
      edges_r = 0;
      dimr = -dimr;
      nrow--;
    }

  if (scol < 1 || srow < 1 || scol + ncol - 1 > dimphi || srow + nrow - 1 > dimr || (!edges_phi && ncol < 1) ||
      (!edges_r && nrow < 1))
    {
      fprintf(stderr, "Dimensions of color index array are invalid.\n");
      return;
    }

  phimin = phi[scol - 1];
  phimax = phi[ncol];

  rmin = r[srow - 1];
  rmax = r[nrow];

  if (phimin == phimax)
    {
      fprintf(stderr, "Invalid angles specified.\n");
      return;
    }

  if (rmin == rmax || rmin < 0 || rmax < 0)
    {
      fprintf(stderr, "Invalid radii specified.\n");
      return;
    }

  check_autoinit;

  if ((r_reverse = rmin > rmax))
    {
      tmp = rmin;
      rmin = rmax;
      rmax = tmp;
    }
  if (!edges_r)
    {
      r_sorted = (double *)gks_malloc(sizeof(double) * (nrow - srow + 3));
    }
  else
    {
      r_sorted = (double *)gks_malloc(sizeof(double) * (nrow - srow + 2));
    }
  for (y = 0; y < nrow - srow + 2; y++)
    {
      if (r_reverse)
        {
          if (!edges_r && y)
            {
              r_sorted[y] = 0.5 * (r[nrow - y] + r[nrow - y + 1]);
            }
          else
            {
              r_sorted[y] = r[nrow - y];
            }
        }
      else
        {
          if (!edges_r && y)
            {
              r_sorted[y] = 0.5 * (r[srow - 1 + y] + r[srow - 2 + y]);
            }
          else
            {
              r_sorted[y] = r[srow - 1 + y];
            }
        }
      if (y && r_sorted[y] < r_sorted[y - 1])
        {
          fprintf(stderr, "radii not sorted in ascending order.\n");
          gks_free(r_sorted);
          return;
        }
    }
  if (!edges_r)
    {
      if (r_reverse)
        {
          r_sorted[nrow - srow + 2] = r[srow - 1];
        }
      else
        {
          r_sorted[nrow - srow + 2] = r[nrow];
        }
      nrow++;
    }

  if ((phi_reverse = phimin > phimax))
    {
      tmp = phimin;
      phimin = phimax;
      phimax = tmp;
    }
  if (!edges_phi)
    {
      phi_sorted = (double *)gks_malloc(sizeof(double) * (ncol - scol + 3));
    }
  else
    {
      phi_sorted = (double *)gks_malloc(sizeof(double) * (ncol - scol + 2));
    }
  for (x = 0; x < ncol - scol + 2; x++)
    {
      if (phi_reverse)
        {
          if (!edges_phi && x)
            {
              phi_sorted[x] = 0.5 * (phi[ncol - x] + phi[ncol - x + 1]);
            }
          else
            {
              phi_sorted[x] = phi[ncol - x];
            }
        }
      else
        {
          if (!edges_phi && x)
            {
              phi_sorted[x] = 0.5 * (phi[scol - 1 + x] + phi[scol - 2 + x]);
            }
          else
            {
              phi_sorted[x] = phi[scol - 1 + x];
            }
        }
      phi_sorted[x] = phi_sorted[x] - phimax + 360;
      if (x && phi_sorted[x] < phi_sorted[x - 1])
        {
          fprintf(stderr, "angles not sorted in ascending order.\n");
          gks_free(r_sorted);
          gks_free(phi_sorted);
          return;
        }
    }
  if (!edges_phi)
    {
      if (phi_reverse)
        {
          phi_sorted[ncol - scol + 2] = phi[scol - 1] - phimax + 360;
        }
      else
        {
          phi_sorted[ncol - scol + 2] = phi[ncol] - phimax + 360;
        }
      ncol++;
    }

  phimin = fmod(phimin, 360);
  phimax = fmod(phimax, 360);

  img_data = (int *)gks_malloc(size * size * sizeof(int));

  for (y = 0; y < size; y++)
    {
      for (x = 0; x < size; x++)
        {
          double px = (x - center) / center * rmax;
          double py = (y - center) / center * rmax;

          cur_r = sqrt(px * px + py * py);
          if (r_reverse)
            {
              cur_r = rmax - cur_r + rmin;
            }

          if (phi_reverse)
            {
              cur_phi = fmod(-fmod(atan2(py, px) * 180 / M_PI + 360, 360) + phimin + 2 * 360, 360);
            }
          else
            {
              cur_phi = fmod(fmod(atan2(py, px) * 180 / M_PI + 360, 360) + 2 * 360 - phimax, 360);
            }

          start = 0;
          end = nrow - srow + 1;
          if (cur_r < r_sorted[start] || cur_r >= r_sorted[end])
            {
              img_data[y * size + x] = 0;
              continue;
            }

          while (start != end)
            {
              int m = ((start + end) / 2);
              if (cur_r >= r_sorted[m + 1])
                {
                  start = m + 1;
                }
              else if (r_sorted[m] > cur_r)
                {
                  end = m;
                }
              else
                {
                  start = end = m;
                }
            }
          r_ind = start;

          start = 0;
          end = ncol - scol + 1;
          if (cur_phi < phi_sorted[start] || cur_phi >= phi_sorted[end])
            {
              img_data[y * size + x] = 0;
              continue;
            }
          while (start != end)
            {
              int m = ((start + end) / 2);
              if (cur_phi >= phi_sorted[m + 1])
                {
                  start = m + 1;
                }
              else if (phi_sorted[m] > cur_phi)
                {
                  end = m;
                }
              else
                {
                  start = end = m;
                }
            }
          phi_ind = start;

          color_ind = color[(r_ind + srow - 1) * ncol + phi_ind + scol - 1];
          if (color_ind >= 0 && color_ind < MAX_COLOR)
            {
              img_data[y * size + x] = (255 << 24) + rgb[color_ind];
            }
          else
            {
              /* invalid color indices in input data result in transparent pixel */
              img_data[y * size + x] = 0;
            }
        }
    }

  gks_free(r_sorted);
  gks_free(phi_sorted);
  gr_drawimage(x_org - rmax, x_org + rmax, y_org + rmax, y_org - rmax, size, size, img_data, 0);
  gks_free(img_data);
}

void gr_gdp(int n, double *x, double *y, int primid, int ldr, int *datrec)
{
  int npoints = n;
  double *px = x, *py = y;
  int i;

  check_autoinit;

  if (lx.scale_options)
    {
      if (npoints >= maxpath) reallocate(npoints);

      px = xpoint;
      py = ypoint;
      for (i = 0; i < npoints; i++)
        {
          px[i] = x_lin(x[i]);
          py[i] = y_lin(y[i]);
        }
    }

  gks_gdp(npoints, px, py, primid, ldr, datrec);

  if (flag_graphics)
    {
      gr_writestream("<gdp len=\"%d\"", n);
      print_float_array("x", n, x);
      print_float_array("y", n, y);
      gr_writestream(" primid=\"%d\" ldr=\"%d\"", primid, ldr);
      print_int_array("datrec", ldr, datrec);
      gr_writestream("/>\n");
    }
}

/*!
 * Generate a cubic spline-fit, starting from the first data point and
 * ending at the last data point.
 *
 * \param[in] n The number of points
 * \param[in] px A pointer to the X coordinates
 * \param[in] py A pointer to the Y coordinates
 * \param[in] m The number of points in the polygon to be drawn (m > n)
 * \param[in] method The smoothing method
 *
 * The values for `x` and `y` are in world coordinates. The attributes that
 * control the appearance of a spline-fit are linetype, linewidth and color
 * index.
 *
 * If `method` is > 0, then a generalized cross-validated smoothing spline is calculated.
 * If `method` is 0, then an interpolating natural cubic spline is calculated.
 * If `method` is < -1, then a cubic B-spline is calculated.
 */
void gr_spline(int n, double *px, double *py, int m, int method)
{
  int err = 0, i, j;
  double *t, *s;
  double *sx, *sy, *x, *f, *df, *y, *c, *wk, *se, var, d;
  int ic, job, ier;

  if (n <= 2)
    {
      fprintf(stderr, "invalid number of points\n");
      return;
    }
  else if (n >= m)
    {
      fprintf(stderr, "invalid number of domain values\n");
      return;
    }

  check_autoinit;

  t = (double *)xmalloc(sizeof(double) * m);
  s = (double *)xmalloc(sizeof(double) * m);
  sx = (double *)xmalloc(sizeof(double) * m);
  sy = (double *)xmalloc(sizeof(double) * m);
  x = (double *)xmalloc(sizeof(double) * n);
  f = (double *)xmalloc(sizeof(double) * n);
  df = (double *)xmalloc(sizeof(double) * n);
  y = (double *)xmalloc(sizeof(double) * n);
  c = (double *)xmalloc(sizeof(double) * 3 * (n - 1));
  se = (double *)xmalloc(sizeof(double) * n);
  wk = (double *)xmalloc(sizeof(double) * 7 * (n + 2));

  for (i = 0; i < n; i++)
    {
      x[i] = (double)((x_lin(px[i]) - lx.xmin) / (lx.xmax - lx.xmin));
      f[i] = (double)((y_lin(py[i]) - lx.ymin) / (lx.ymax - lx.ymin));
      df[i] = 1;
    }

  if (method >= -1)
    {
      for (i = 1; i < n; i++)
        if (px[i - 1] >= px[i])
          {
            fprintf(stderr, "points not sorted in ascending order\n");
            err = 1;
          }

      if (!err)
        {
          sx[0] = x[0];
          for (j = 1; j < m - 1; j++) sx[j] = x[0] + j * (x[n - 1] - x[0]) / (m - 1);
          sx[m - 1] = x[n - 1];

          job = 0;
          ic = n - 1;
          var = (double)method;

          cubgcv(x, f, df, &n, y, c, &ic, &var, &job, se, wk, &ier);

          if (ier == 0)
            {
              for (j = 0; j < m; j++)
                {
                  i = 0;
                  while ((i < ic) && (x[i] <= sx[j])) i++;
                  if (x[i] > sx[j]) i--;
                  if (i < 0)
                    i = 0;
                  else if (i >= ic)
                    i = ic - 1;
                  d = sx[j] - x[i];

                  s[j] = (double)(((c[i + 2 * ic] * d + c[i + ic]) * d + c[i]) * d + y[i]);
                }
            }
          else
            {
              fprintf(stderr, "invalid argument to math library\n");
              err = 1;
            }
        }
    }
  else
    {
      b_spline(n, x, f, m, sx, sy);

      for (j = 0; j < m; j++) s[j] = (double)sy[j];
    }

  if (!err)
    {
      for (j = 0; j < m; j++)
        {
          t[j] = x_log((double)(lx.xmin + sx[j] * (lx.xmax - lx.xmin)));
          s[j] = y_log((double)(lx.ymin + s[j] * (lx.ymax - lx.ymin)));
        }
      polyline(m, t, s);
    }

  free(wk);
  free(se);
  free(c);
  free(y);
  free(df);
  free(f);
  free(x);
  free(sy);
  free(sx);
  free(s);
  free(t);

  if (flag_graphics)
    {
      gr_writestream("<spline len=\"%d\"", n);
      print_float_array("x", n, px);
      print_float_array("y", n, py);
      gr_writestream(" m=\"%d\" method=\"%d\"/>\n", m, method);
    }
}

/*!
 * Interpolate data from arbitrary points at points on a rectangular grid.
 *
 * \param[in] nd The number of input points
 * \param[in] xd A pointer to the X coordinates of the input points
 * \param[in] yd A pointer to the Y coordinates of the input points
 * \param[in] zd A pointer to the values of the points
 * \param[in] nx The number of points in X direction for the output grid
 * \param[in] ny The number of points in Y direction for the output grid
 * \param[out] x A pointer to the points in X direction for the output grid
 * \param[out] y A pointer to the points in Y direction for the output grid
 * \param[out] z A pointer to the interpolated values on the nx x ny grid points
 */
void gr_gridit(int nd, double *xd, double *yd, double *zd, int nx, int ny, double *x, double *y, double *z)
{
  int i, md, ncp;
  double xmin, ymin, xmax, ymax;
  int *iwk;
  double *wk;

  if (nd < 5)
    {
      fprintf(stderr, "invalid number of domain values\n");
      return;
    }
  else if (nx < 1 || ny < 1)
    {
      fprintf(stderr, "invalid number of points\n");
      return;
    }

  check_autoinit;

  xmin = xd[0];
  xmax = xmin;
  ymin = yd[0];
  ymax = ymin;

  /* CALCULATION OF MIN/MAX VALUES */
  for (i = 1; i < nd; ++i)
    {
      xmin = min(xmin, xd[i]);
      xmax = max(xmax, xd[i]);
      ymin = min(ymin, yd[i]);
      ymax = max(ymax, yd[i]);
    }

  /* DETERMINE GRID POINTS INSIDE THE DATA AREA */
  for (i = 0; i < nx; ++i)
    {
      x[i] = xmin + i / (double)(nx - 1) * (xmax - xmin);
    }
  for (i = 0; i < ny; ++i)
    {
      y[i] = ymin + i / (double)(ny - 1) * (ymax - ymin);
    }

  /* CALL THE SMOOTH SURFACE FIT ROUTINE */
  md = 1;
  ncp = 4;
  iwk = (int *)calloc(31 * nd + nx * ny, sizeof(int));
  wk = (double *)calloc(6 * (nd + 1), sizeof(double));

  idsfft(&md, &ncp, &nd, xd, yd, zd, &nx, &ny, x, y, z, iwk, wk);

  free(wk);
  free(iwk);
}

/*!
 * Specify the line style for polylines.
 *
 * \param[in] type The polyline line style
 *
 *
 * \verbatim embed:rst:leading-asterisk
 *
 * The available line types are:
 *
 * +---------------------------+----+---------------------------------------------------+
 * |LINETYPE_SOLID             |   1|Solid line                                         |
 * +---------------------------+----+---------------------------------------------------+
 * |LINETYPE_DASHED            |   2|Dashed line                                        |
 * +---------------------------+----+---------------------------------------------------+
 * |LINETYPE_DOTTED            |   3|Dotted line                                        |
 * +---------------------------+----+---------------------------------------------------+
 * |LINETYPE_DASHED_DOTTED     |   4|Dashed-dotted line                                 |
 * +---------------------------+----+---------------------------------------------------+
 * |LINETYPE_DASH_2_DOT        |  -1|Sequence of one dash followed by two dots          |
 * +---------------------------+----+---------------------------------------------------+
 * |LINETYPE_DASH_3_DOT        |  -2|Sequence of one dash followed by three dots        |
 * +---------------------------+----+---------------------------------------------------+
 * |LINETYPE_LONG_DASH         |  -3|Sequence of long dashes                            |
 * +---------------------------+----+---------------------------------------------------+
 * |LINETYPE_LONG_SHORT_DASH   |  -4|Sequence of a long dash followed by a short dash   |
 * +---------------------------+----+---------------------------------------------------+
 * |LINETYPE_SPACED_DASH       |  -5|Sequence of dashes double spaced                   |
 * +---------------------------+----+---------------------------------------------------+
 * |LINETYPE_SPACED_DOT        |  -6|Sequence of dots double spaced                     |
 * +---------------------------+----+---------------------------------------------------+
 * |LINETYPE_DOUBLE_DOT        |  -7|Sequence of pairs of dots                          |
 * +---------------------------+----+---------------------------------------------------+
 * |LINETYPE_TRIPLE_DOT        |  -8|Sequence of groups of three dots                   |
 * +---------------------------+----+---------------------------------------------------+
 *
 * \endverbatim
 */
void gr_setlinetype(int type)
{
  check_autoinit;

  gks_set_pline_linetype(type);
  if (ctx) ctx->ltype = type;

  if (flag_graphics) gr_writestream("<setlinetype type=\"%d\"/>\n", type);
}

void gr_inqlinetype(int *ltype)
{
  int errind;

  check_autoinit;

  gks_inq_pline_linetype(&errind, ltype);
}

/*!
 * Define the line width of subsequent polyline output primitives.
 *
 * \param[in] width The polyline line width scale factor
 *
 * The line width is calculated as the nominal line width generated
 * on the workstation multiplied by the line width scale factor.
 * This value is mapped by the workstation to the nearest available line width.
 * The default line width is 1.0, or 1 times the line width generated on the
 * graphics device.
 */
void gr_setlinewidth(double width)
{
  check_autoinit;

  gks_set_pline_linewidth(width);
  if (ctx) ctx->lwidth = width;

  if (flag_graphics) gr_writestream("<setlinewidth width=\"%g\"/>\n", width);
}

void gr_inqlinewidth(double *width)
{
  int errind;

  check_autoinit;

  gks_inq_pline_linewidth(&errind, width);
}

/*!
 * Define the color of subsequent polyline output primitives.
 *
 * \param[in] color The polyline color index (COLOR < 1256)
 */
void gr_setlinecolorind(int color)
{
  check_autoinit;

  gks_set_pline_color_index(color);
  if (ctx) ctx->plcoli = color;

  if (flag_graphics) gr_writestream("<setlinecolorind color=\"%d\"/>\n", color);
}

void gr_inqlinecolorind(int *coli)
{
  int errind;

  check_autoinit;

  gks_inq_pline_color_index(&errind, coli);
}

/*!
 * Specifiy the marker type for polymarkers.
 *
 * \param[in] type The polymarker marker type
 *
 * \verbatim embed:rst:leading-asterisk
 *
 * The available marker types are:
 *
 * +-----------------------------+-----+------------------------------------------------+
 * |MARKERTYPE_DOT               |    1|Smallest displayable dot                        |
 * +-----------------------------+-----+------------------------------------------------+
 * |MARKERTYPE_PLUS              |    2|Plus sign                                       |
 * +-----------------------------+-----+------------------------------------------------+
 * |MARKERTYPE_ASTERISK          |    3|Asterisk                                        |
 * +-----------------------------+-----+------------------------------------------------+
 * |MARKERTYPE_CIRCLE            |    4|Hollow circle                                   |
 * +-----------------------------+-----+------------------------------------------------+
 * |MARKERTYPE_DIAGONAL_CROSS    |    5|Diagonal cross                                  |
 * +-----------------------------+-----+------------------------------------------------+
 * |MARKERTYPE_SOLID_CIRCLE      |   -1|Filled circle                                   |
 * +-----------------------------+-----+------------------------------------------------+
 * |MARKERTYPE_TRIANGLE_UP       |   -2|Hollow triangle pointing upward                 |
 * +-----------------------------+-----+------------------------------------------------+
 * |MARKERTYPE_SOLID_TRI_UP      |   -3|Filled triangle pointing upward                 |
 * +-----------------------------+-----+------------------------------------------------+
 * |MARKERTYPE_TRIANGLE_DOWN     |   -4|Hollow triangle pointing downward               |
 * +-----------------------------+-----+------------------------------------------------+
 * |MARKERTYPE_SOLID_TRI_DOWN    |   -5|Filled triangle pointing downward               |
 * +-----------------------------+-----+------------------------------------------------+
 * |MARKERTYPE_SQUARE            |   -6|Hollow square                                   |
 * +-----------------------------+-----+------------------------------------------------+
 * |MARKERTYPE_SOLID_SQUARE      |   -7|Filled square                                   |
 * +-----------------------------+-----+------------------------------------------------+
 * |MARKERTYPE_BOWTIE            |   -8|Hollow bowtie                                   |
 * +-----------------------------+-----+------------------------------------------------+
 * |MARKERTYPE_SOLID_BOWTIE      |   -9|Filled bowtie                                   |
 * +-----------------------------+-----+------------------------------------------------+
 * |MARKERTYPE_HGLASS            |  -10|Hollow hourglass                                |
 * +-----------------------------+-----+------------------------------------------------+
 * |MARKERTYPE_SOLID_HGLASS      |  -11|Filled hourglass                                |
 * +-----------------------------+-----+------------------------------------------------+
 * |MARKERTYPE_DIAMOND           |  -12|Hollow diamond                                  |
 * +-----------------------------+-----+------------------------------------------------+
 * |MARKERTYPE_SOLID_DIAMOND     |  -13|Filled Diamond                                  |
 * +-----------------------------+-----+------------------------------------------------+
 * |MARKERTYPE_STAR              |  -14|Hollow star                                     |
 * +-----------------------------+-----+------------------------------------------------+
 * |MARKERTYPE_SOLID_STAR        |  -15|Filled Star                                     |
 * +-----------------------------+-----+------------------------------------------------+
 * |MARKERTYPE_TRI_UP_DOWN       |  -16|Hollow triangles pointing up and down overlaid  |
 * +-----------------------------+-----+------------------------------------------------+
 * |MARKERTYPE_SOLID_TRI_RIGHT   |  -17|Filled triangle point right                     |
 * +-----------------------------+-----+------------------------------------------------+
 * |MARKERTYPE_SOLID_TRI_LEFT    |  -18|Filled triangle pointing left                   |
 * +-----------------------------+-----+------------------------------------------------+
 * |MARKERTYPE_HOLLOW PLUS       |  -19|Hollow plus sign                                |
 * +-----------------------------+-----+------------------------------------------------+
 * |MARKERTYPE_SOLID PLUS        |  -20|Solid plus sign                                 |
 * +-----------------------------+-----+------------------------------------------------+
 * |MARKERTYPE_PENTAGON          |  -21|Pentagon                                        |
 * +-----------------------------+-----+------------------------------------------------+
 * |MARKERTYPE_HEXAGON           |  -22|Hexagon                                         |
 * +-----------------------------+-----+------------------------------------------------+
 * |MARKERTYPE_HEPTAGON          |  -23|Heptagon                                        |
 * +-----------------------------+-----+------------------------------------------------+
 * |MARKERTYPE_OCTAGON           |  -24|Octagon                                         |
 * +-----------------------------+-----+------------------------------------------------+
 * |MARKERTYPE_STAR_4            |  -25|4-pointed star                                  |
 * +-----------------------------+-----+------------------------------------------------+
 * |MARKERTYPE_STAR_5            |  -26|5-pointed star (pentagram)                      |
 * +-----------------------------+-----+------------------------------------------------+
 * |MARKERTYPE_STAR_6            |  -27|6-pointed star (hexagram)                       |
 * +-----------------------------+-----+------------------------------------------------+
 * |MARKERTYPE_STAR_7            |  -28|7-pointed star (heptagram)                      |
 * +-----------------------------+-----+------------------------------------------------+
 * |MARKERTYPE_STAR_8            |  -29|8-pointed star (octagram)                       |
 * +-----------------------------+-----+------------------------------------------------+
 * |MARKERTYPE_VLINE             |  -30|verical line                                    |
 * +-----------------------------+-----+------------------------------------------------+
 * |MARKERTYPE_HLINE             |  -31|horizontal line                                 |
 * +-----------------------------+-----+------------------------------------------------+
 * |MARKERTYPE_OMARK             |  -32|o-mark                                          |
 * +-----------------------------+-----+------------------------------------------------+
 * \endverbatim
 *
 * Polymarkers appear centered over their specified coordinates.
 */
void gr_setmarkertype(int type)
{
  check_autoinit;

  gks_set_pmark_type(type);
  if (ctx) ctx->mtype = type;

  if (flag_graphics) gr_writestream("<setmarkertype type=\"%d\"/>\n", type);
}

void gr_inqmarkertype(int *mtype)
{
  int errind;

  check_autoinit;

  gks_inq_pmark_type(&errind, mtype);
}

/*!
 * Specify the marker size for polymarkers.
 *
 * \param[in] size Scale factor applied to the nominal marker size
 *
 * The polymarker size is calculated as the nominal size generated on the graphics device
 * multiplied by the marker size scale factor.
 */
void gr_setmarkersize(double size)
{
  check_autoinit;

  gks_set_pmark_size(size);
  if (ctx) ctx->mszsc = size;

  if (flag_graphics) gr_writestream("<setmarkersize size=\"%g\"/>\n", size);
}

/*!
 *
 * Inquire the marker size for polymarkers.
 *
 * \param[out] size Scale factor applied to the nominal marker size
 *
 */
void gr_inqmarkersize(double *size)
{
  int errind;

  check_autoinit;

  gks_inq_pmark_size(&errind, size);
}

/*!
 * Define the color of subsequent polymarker output primitives.
 *
 * \param[in] color The polymarker color index (COLOR < 1256)
 */
void gr_setmarkercolorind(int color)
{
  check_autoinit;

  gks_set_pmark_color_index(color);
  if (ctx) ctx->pmcoli = color;

  if (flag_graphics) gr_writestream("<setmarkercolorind color=\"%d\"/>\n", color);
}

void gr_inqmarkercolorind(int *coli)
{
  int errind;

  check_autoinit;

  gks_inq_pmark_color_index(&errind, coli);
}

/*!
 * Specify the text font and precision for subsequent text output primitives.
 *
 * \param[in] font Text font (see tables)
 * \param[in] precision Text precision (see table below)
 *
 * \verbatim embed:rst:leading-asterisk
 *
 * The available text fonts are:
 *
 * +--------------------------------------+-----+
 * |FONT_TIMES_ROMAN                      |  101|
 * +--------------------------------------+-----+
 * |FONT_TIMES_ITALIC                     |  102|
 * +--------------------------------------+-----+
 * |FONT_TIMES_BOLD                       |  103|
 * +--------------------------------------+-----+
 * |FONT_TIMES_BOLDITALIC                 |  104|
 * +--------------------------------------+-----+
 * |FONT_HELVETICA                        |  105|
 * +--------------------------------------+-----+
 * |FONT_HELVETICA_OBLIQUE                |  106|
 * +--------------------------------------+-----+
 * |FONT_HELVETICA_BOLD                   |  107|
 * +--------------------------------------+-----+
 * |FONT_HELVETICA_BOLDOBLIQUE            |  108|
 * +--------------------------------------+-----+
 * |FONT_COURIER                          |  109|
 * +--------------------------------------+-----+
 * |FONT_COURIER_OBLIQUE                  |  110|
 * +--------------------------------------+-----+
 * |FONT_COURIER_BOLD                     |  111|
 * +--------------------------------------+-----+
 * |FONT_COURIER_BOLDOBLIQUE              |  112|
 * +--------------------------------------+-----+
 * |FONT_SYMBOL                           |  113|
 * +--------------------------------------+-----+
 * |FONT_BOOKMAN_LIGHT                    |  114|
 * +--------------------------------------+-----+
 * |FONT_BOOKMAN_LIGHTITALIC              |  115|
 * +--------------------------------------+-----+
 * |FONT_BOOKMAN_DEMI                     |  116|
 * +--------------------------------------+-----+
 * |FONT_BOOKMAN_DEMIITALIC               |  117|
 * +--------------------------------------+-----+
 * |FONT_NEWCENTURYSCHLBK_ROMAN           |  118|
 * +--------------------------------------+-----+
 * |FONT_NEWCENTURYSCHLBK_ITALIC          |  119|
 * +--------------------------------------+-----+
 * |FONT_NEWCENTURYSCHLBK_BOLD            |  120|
 * +--------------------------------------+-----+
 * |FONT_NEWCENTURYSCHLBK_BOLDITALIC      |  121|
 * +--------------------------------------+-----+
 * |FONT_AVANTGARDE_BOOK                  |  122|
 * +--------------------------------------+-----+
 * |FONT_AVANTGARDE_BOOKOBLIQUE           |  123|
 * +--------------------------------------+-----+
 * |FONT_AVANTGARDE_DEMI                  |  124|
 * +--------------------------------------+-----+
 * |FONT_AVANTGARDE_DEMIOBLIQUE           |  125|
 * +--------------------------------------+-----+
 * |FONT_PALATINO_ROMAN                   |  126|
 * +--------------------------------------+-----+
 * |FONT_PALATINO_ITALIC                  |  127|
 * +--------------------------------------+-----+
 * |FONT_PALATINO_BOLD                    |  128|
 * +--------------------------------------+-----+
 * |FONT_PALATINO_BOLDITALIC              |  129|
 * +--------------------------------------+-----+
 * |FONT_ZAPFCHANCERY_MEDIUMITALIC        |  130|
 * +--------------------------------------+-----+
 * |FONT_ZAPFDINGBATS                     |  131|
 * +--------------------------------------+-----+
 * |FONT_COMPUTERMODERN                   |  232|
 * +--------------------------------------+-----+
 * |FONT_DEJAVUSANS                       |  233|
 * +--------------------------------------+-----+
 *
 * The available text precisions are:
 *
 * +---------------------------+---+--------------------------------------+
 * |TEXT_PRECISION_STRING      |  0|String precision (higher quality)     |
 * +---------------------------+---+--------------------------------------+
 * |TEXT_PRECISION_CHAR        |  1|Character precision (medium quality)  |
 * +---------------------------+---+--------------------------------------+
 * |TEXT_PRECISION_STROKE      |  2|Stroke precision (lower quality)      |
 * +---------------------------+---+--------------------------------------+
 * |TEXT_PRECISION_OUTLINE     |  3|Outline precision (highest quality)   |
 * +---------------------------+---+--------------------------------------+
 *
 * \endverbatim
 *
 * The appearance of a font depends on the text precision value specified.
 * STRING, CHARACTER, STROKE or OUTLINE precision allows for a greater or
 * lesser realization of the text primitives, for efficiency. STRING is the
 * default precision for GR and produces the high quality output using either
 * native font rendering or FreeType. OUTLINE uses the GR path rendering
 * functions to draw individual glyphs and produces the highest quality
 * output.
 */
void gr_settextfontprec(int font, int precision)
{
  check_autoinit;

  gks_set_text_fontprec(font, precision);
  if (ctx)
    {
      ctx->txfont = font;
      ctx->txprec = precision;
    }

  if (flag_graphics) gr_writestream("<settextfontprec font=\"%d\" precision=\"%d\"/>\n", font, precision);
}

/*!
 * Load a font file from a given filename.
 *
 * \param[in] filename The absolute filename of the font
 * \param[out] font The font index to use with gr_settextfontprec
 *
 * This function loads a font from a given absolute filename and assigns a font index to it. To use the loaded
 * font call `gr_settextfontprec` using the resulting font index and precision 3.
 *
 *      int font;
 *      gr_loadfont(filename, &font);
 *      gr_settextfontprec(font, 3);
 *
 * As the font file is internally loaded using FreeType, it is required that FreeType support is compiled
 * in and FreeType has to support the given file type. On error the font index is set to -1.
 *
 */
void gr_loadfont(char *filename, int *font)
{
  check_autoinit;

  *font = gks_ft_load_user_font(filename);
  if (flag_graphics) gr_writestream("<loadfont filename=\"%s\"/>\n", filename);
}

/*!
 * Set the current character expansion factor (width to height ratio).
 *
 * \param[in] factor Text expansion factor applied to the nominal text
 *                   width-to-height ratio
 *
 * This function defines the width of subsequent text output primitives. The
 * expansion factor alters the width of the generated characters, but not their
 * height. The default text expansion factor is 1, or one times the normal
 * width-to-height ratio of the text.
 */
void gr_setcharexpan(double factor)
{
  check_autoinit;

  gks_set_text_expfac(factor);
  if (ctx) ctx->chxp = factor;

  if (flag_graphics) gr_writestream("<setcharexpan factor=\"%g\"/>\n", factor);
}

void gr_setcharspace(double spacing)
{
  check_autoinit;

  gks_set_text_spacing(spacing);
  if (ctx) ctx->chsp = spacing;

  if (flag_graphics) gr_writestream("<setcharspace spacingr=\"%g\"/>\n", spacing);
}

/*!
 * Set the current text color index.
 *
 * \param[in] color The text color index (COLOR < 1256)
 *
 * This function defines the color of subsequent text output primitives. GR uses
 * the default foreground color (black=1) for the default text color index.
 */
void gr_settextcolorind(int color)
{
  check_autoinit;

  gks_set_text_color_index(color);
  if (ctx) ctx->txcoli = color;

  if (flag_graphics) gr_writestream("<settextcolorind color=\"%d\"/>\n", color);
}

/*!
 * Gets the current text color index.
 *
 * \param[out] color The text color index (COLOR < 1256)
 *
 * This function gets the color of text output primitives.
 */
void gr_inqtextcolorind(int *color)
{
  int errind;
  gks_inq_text_color_index(&errind, color);
}

/*!
 * Set the current character height.
 *
 * \param[in] height Text height value
 *
 * This function defines the height of subsequent text output primitives. Text
 * height is defined as a percentage of the default window. GR uses the default
 * text height of 0.027 (2.7% of the height of the default window).
 */
void gr_setcharheight(double height)
{
  check_autoinit;

  gks_set_text_height(height);
  if (ctx) ctx->chh = height;

  if (flag_graphics) gr_writestream("<setcharheight height=\"%g\"/>\n", height);
}

/*!
 * Gets the current character height..
 *
 * \param[out] height Text height value
 *
 * This function gets the height of text output primitives. Text height is
 * defined as a percentage of the default window. GR uses the default text
 * height of 0.027 (2.7% of the height of the default window).
 */
void gr_inqcharheight(double *height)
{
  int errind;
  gks_inq_text_height(&errind, height);
}

/*!
 * Set the current character text angle up vector.
 *
 * \param[in] ux X coordinate of the text up vector
 * \param[in] uy Y coordinate of the text up vector
 *
 * This function defines the vertical rotation of subsequent text output primitives.
 The text up vector is initially set to (0, 1), horizontal to the baseline.
 */
void gr_setcharup(double ux, double uy)
{
  check_autoinit;

  gks_set_text_upvec(ux, uy);
  if (ctx)
    {
      ctx->chup[0] = ux;
      ctx->chup[1] = uy;
    }

  if (flag_graphics) gr_writestream("<setcharup x=\"%g\" y=\"%g\"/>\n", ux, uy);
}

/*!
 * Define the current direction in which subsequent text will be drawn.
 *
 * \param[in] path Text path (see table)
 *
 * \verbatim embed:rst:leading-asterisk
 *
 * +----------------------+---+---------------+
 * |TEXT_PATH_RIGHT       |  0|left-to-right  |
 * +----------------------+---+---------------+
 * |TEXT_PATH_LEFT        |  1|right-to-left  |
 * +----------------------+---+---------------+
 * |TEXT_PATH_UP          |  2|downside-up    |
 * +----------------------+---+---------------+
 * |TEXT_PATH_DOWN        |  3|upside-down    |
 * +----------------------+---+---------------+
 *
 * \endverbatim
 */
void gr_settextpath(int path)
{
  check_autoinit;

  gks_set_text_path(path);
  if (ctx) ctx->txp = path;

  if (flag_graphics) gr_writestream("<settextpath path=\"%d\"/>\n", path);
}

/*!
 * Set the current horizontal and vertical alignment for text.
 *
 * \param[in] horizontal Horizontal text alignment (see the table)
 * \param[in] vertical Vertical text alignment (see the table)
 *
 * \verbatim embed:rst:leading-asterisk
 *
 * +-------------------------+---+----------------+
 * |TEXT_HALIGN_NORMAL       |  0|                |
 * +-------------------------+---+----------------+
 * |TEXT_HALIGN_LEFT         |  1|Left justify    |
 * +-------------------------+---+----------------+
 * |TEXT_HALIGN_CENTER       |  2|Center justify  |
 * +-------------------------+---+----------------+
 * |TEXT_HALIGN_RIGHT        |  3|Right justify   |
 * +-------------------------+---+----------------+
 *
 * +-------------------------+---+------------------------------------------------+
 * |TEXT_VALIGN_NORMAL       |  0|                                                |
 * +-------------------------+---+------------------------------------------------+
 * |TEXT_VALIGN_TOP          |  1|Align with the top of the characters            |
 * +-------------------------+---+------------------------------------------------+
 * |TEXT_VALIGN_CAP          |  2|Aligned with the cap of the characters          |
 * +-------------------------+---+------------------------------------------------+
 * |TEXT_VALIGN_HALF         |  3|Aligned with the half line of the characters    |
 * +-------------------------+---+------------------------------------------------+
 * |TEXT_VALIGN_BASE         |  4|Aligned with the base line of the characters    |
 * +-------------------------+---+------------------------------------------------+
 * |TEXT_VALIGN_BOTTOM       |  5|Aligned with the bottom line of the characters  |
 * +-------------------------+---+------------------------------------------------+
 *
 * \endverbatim
 *
 * This function specifies how the characters in a text primitive will be
 * aligned in horizontal and vertical space. The default text alignment
 * indicates horizontal left alignment and vertical baseline alignment.
 */
void gr_settextalign(int horizontal, int vertical)
{
  check_autoinit;

  gks_set_text_align(horizontal, vertical);
  if (ctx)
    {
      ctx->txal[0] = horizontal;
      ctx->txal[1] = vertical;
    }

  if (flag_graphics) gr_writestream("<settextalign halign=\"%d\" valign=\"%d\"/>\n", horizontal, vertical);
}

/*!
 * Set the fill area interior style to be used for fill areas.
 *
 * \param[in] style The style of fill to be used
 *
 * \verbatim embed:rst:leading-asterisk
 *
 * +------------------+---+-------------------------------------------------------------------------------------------+
 * |HOLLOW            |  0|No filling. Just draw the bounding polyline                                                |
 * +------------------+---+-------------------------------------------------------------------------------------------+
 * |SOLID             |  1|Fill the interior of the polygon using the fill color index                                |
 * +------------------+---+-------------------------------------------------------------------------------------------+
 * |PATTERN           |  2|Fill the interior of the polygon using the style index as a pattern index                  |
 * +------------------+---+-------------------------------------------------------------------------------------------+
 * |HATCH             |  3|Fill the interior of the polygon using the style index as a cross-hatched style            |
 * +------------------+---+-------------------------------------------------------------------------------------------+
 * |SOLID_WITH_BORDER |  4|Fill the interior of the polygon using the fill color index and draw the bounding polyline |
 * +------------------+---+-------------------------------------------------------------------------------------------+
 * \endverbatim
 *
 * This function defines the interior style  for subsequent fill area output
 * primitives. The default interior style is HOLLOW.
 */
void gr_setfillintstyle(int style)
{
  check_autoinit;

  gks_set_fill_int_style(style);
  if (ctx) ctx->ints = style;

  if (flag_graphics) gr_writestream("<setfillintstyle intstyle=\"%d\"/>\n", style);
}

/*!
 * Set the fill area interior style to be used for fill areas.
 *
 * \param[out] style The currently set fill style
 *
 * This function gets the currently set fill style.
 */
void gr_inqfillintstyle(int *style)
{
  int errind;

  check_autoinit;

  gks_inq_fill_int_style(&errind, style);
}

/*!
 * Set the fill area interior style to be used for fill areas.
 *
 * \param[in] index The fill style index to be used
 *
 * This function specifies an index when PATTERN fill or HATCH fill is
 * requested by the gr_setfillintstyle function. If the interior style is set
 * to PATTERN, the fill style index points to a device-independent pattern
 * table. If interior style is set to HATCH the fill style index indicates
 * different hatch styles. If HOLLOW or SOLID is specified for the interior
 * style, the fill style index is unused.
 */
void gr_setfillstyle(int index)
{
  check_autoinit;

  gks_set_fill_style_index(index);
  if (ctx) ctx->styli = index;

  if (flag_graphics) gr_writestream("<setfillstyle style=\"%d\"/>\n", index);
}

/*!
 * Get the fill area interior style to be used for fill areas.
 *
 * \param[out] index The currently set fill style color index
 *
 * This function gets the color index for PATTERN and HATCH fills.
 */
void gr_inqfillstyle(int *index)
{
  int errind;

  check_autoinit;

  gks_inq_fill_style_index(&errind, index);
}

/*!
 * Sets the current fill area color index.
 *
 * \param[in] color The fill area color index (COLOR < 1256)
 *
 * This function defines the color of subsequent fill area output primitives.
 * GR uses the default foreground color (black=1) for the default fill area
 * color index.
 */
void gr_setfillcolorind(int color)
{
  check_autoinit;

  gks_set_fill_color_index(color);
  if (ctx) ctx->facoli = color;

  if (flag_graphics) gr_writestream("<setfillcolorind color=\"%d\"/>\n", color);
}

/*!
 * Gets the current fill area color index.
 *
 * \param[out] color The fill area color index (COLOR < 1256)
 *
 * This function gets the color of fill area output primitives.
 */
void gr_inqfillcolorind(int *color)
{
  int errind;

  check_autoinit;

  gks_inq_fill_color_index(&errind, color);
}

static void setcolor(int workstation_id, color_t *color)
{
  int wkid = workstation_id;

  gks_set_color_rep(wkid, color->index, color->red, color->green, color->blue);
}

static void setcolorrep(int index, double red, double green, double blue)
{
  color_t color;

  color.index = index;
  color.red = red;
  color.green = green;
  color.blue = blue;

  if (index >= 0 && index < MAX_COLOR)
    rgb[index] = ((nint(red * 255) & 0xff)) | ((nint(green * 255) & 0xff) << 8) | ((nint(blue * 255) & 0xff) << 16);

  foreach_activews((void (*)(int, void *))setcolor, (void *)&color);
}

/*!
 * Redefine an existing color index representation by specifying an RGB color
 * triplet.
 *
 * \param[in] index Color index in the range 0 to 1256
 * \param[in] red Red intensity in the range 0.0 to 1.0
 * \param[in] green Green intensity in the range 0.0 to 1.0
 * \param[in] blue Blue intensity in the range 0.0 to 1.0
 */
void gr_setcolorrep(int index, double red, double green, double blue)
{
  check_autoinit;

  setcolorrep(index, red, green, blue);

  if (flag_graphics)
    gr_writestream("<setcolorrep index=\"%d\" red=\"%g\" green=\"%g\" blue=\"%g\"/>\n", index, red, green, blue);
}

/*!
 * Set the type of transformation to be used for subsequent GR output
 * primitives.
 *
 * \param[in] options Scale specification (see table)
 *
 * \verbatim embed:rst:leading-asterisk
 *
 * +---------------+--------------------+
 * |OPTION_X_LOG   |Logarithmic X-axis  |
 * +---------------+--------------------+
 * |OPTION_Y_LOG   |Logarithmic Y-axis  |
 * +---------------+--------------------+
 * |OPTION_Z_LOG   |Logarithmic Z-axis  |
 * +---------------+--------------------+
 * |OPTION_FLIP_X  |Flip X-axis         |
 * +---------------+--------------------+
 * |OPTION_FLIP_Y  |Flip Y-axis         |
 * +---------------+--------------------+
 * |OPTION_FLIP_Z  |Flip Z-axis         |
 * +---------------+--------------------+
 *
 * \endverbatim
 *
 * This function defines the current transformation according to the given scale
 * specification which may be or'ed together using any of the above options. GR
 * uses these options for all subsequent output primitives until another value
 * is provided. The scale options are used to transform points from an abstract
 * logarithmic or semi-logarithmic coordinate system, which may be flipped along
 * each axis, into the world coordinate system.
 *
 * \note When applying a logarithmic transformation to a specific axis, the
 * system assumes that the axes limits are greater than zero.
 */
int gr_setscale(int options)
{
  int result = 0;

  check_autoinit;

  result = setscale(options);
  if (ctx) ctx->scale_options = options;

  if (flag_graphics) gr_writestream("<setscale scale=\"%d\"/>\n", options);

  return result;
}

void gr_inqscale(int *options)
{
  check_autoinit;

  *options = lx.scale_options;
}

/*!
 * Establishes a window, or rectangular subspace, of world coordinates to be
 * plotted. If you desire log scaling or mirror-imaging of axes, use the
 * gr_setscale function.
 *
 * \param[in] xmin The left horizontal coordinate of the window (xmin < xmax).
 * \param[in] xmax The right horizontal coordinate of the window (xmin < xmax).
 * \param[in] ymin The bottom vertical coordinate of the window (ymin < ymax).
 * \param[in] ymax The top vertical coordinate of the window (ymin < ymax).
 *
 * This function defines the rectangular portion of the World Coordinate space
 * (WC) to be associated with the specified normalization transformation. The
 * WC window and the Normalized Device Coordinates (NDC) viewport define the
 * normalization transformation through which all output primitives are mapped.
 * The WC window is mapped onto the rectangular NDC viewport which is, in turn,
 * mapped onto the display surface of the open and active workstation, in device
 * coordinates. By default, GR uses the range [0,1] x [0,1], in world
 * coordinates, as the normalization transformation window.
 */
void gr_setwindow(double xmin, double xmax, double ymin, double ymax)
{
  int tnr = WC;

  check_autoinit;

  gks_set_window(tnr, xmin, xmax, ymin, ymax);
  if (ctx)
    {
      ctx->wn[0] = xmin;
      ctx->wn[1] = xmax;
      ctx->wn[2] = ymin;
      ctx->wn[3] = ymax;
    }
  setscale(lx.scale_options);

  if (flag_graphics)
    gr_writestream("<setwindow xmin=\"%g\" xmax=\"%g\" ymin=\"%g\" ymax=\"%g\"/>\n", xmin, xmax, ymin, ymax);
}

void gr_inqwindow(double *xmin, double *xmax, double *ymin, double *ymax)
{
  check_autoinit;

  *xmin = lx.xmin;
  *xmax = lx.xmax;
  *ymin = lx.ymin;
  *ymax = lx.ymax;
}

/*
 * Return the three dimensional window.
 */
void gr_inqwindow3d(double *xmin, double *xmax, double *ymin, double *ymax, double *zmin, double *zmax)
{

  check_autoinit;

  *xmin = ix.xmin;
  *xmax = ix.xmax;
  *ymin = ix.ymin;
  *ymax = ix.ymax;
  *zmin = ix.zmin;
  *zmax = ix.zmax;
}

/*!
 * Establishes a rectangular subspace of normalized device coordinates.
 *
 * \param[in] xmin The left horizontal coordinate of the viewport
 *                 (0 <= xmin < xmax).
 * \param[in] xmax The right horizontal coordinate of the viewport
 *                 (xmin < xmax <= 1).
 * \param[in] ymin The bottom vertical coordinate of the viewport
 *                 (0 <= ymin < ymax).
 * \param[in] ymax The top vertical coordinate of the viewport
 *                 (ymin < ymax <= 1).
 *
 * This function defines the rectangular portion of the Normalized Device
 * Coordinate (NDC) space to be associated with the specified normalization
 * transformation. The NDC viewport and World Coordinate (WC) window define
 * the normalization transformation through which all output primitives pass.
 * The WC window is mapped onto the rectangular NDC viewport which is, in turn,
 * mapped onto the display surface of the open and active workstation, in device
 * coordinates.
 */
void gr_setviewport(double xmin, double xmax, double ymin, double ymax)
{
  int tnr = WC;

  check_autoinit;

  gks_set_viewport(tnr, xmin, xmax, ymin, ymax);
  if (ctx)
    {
      ctx->vp[0] = xmin;
      ctx->vp[1] = xmax;
      ctx->vp[2] = ymin;
      ctx->vp[3] = ymax;
    }
  setscale(lx.scale_options);

  vxmin = xmin;
  vxmax = xmax;
  vymin = ymin;
  vymax = ymax;

  if (flag_graphics)
    gr_writestream("<setviewport xmin=\"%g\" xmax=\"%g\" ymin=\"%g\" ymax=\"%g\"/>\n", xmin, xmax, ymin, ymax);
}

void gr_inqviewport(double *xmin, double *xmax, double *ymin, double *ymax)
{
  check_autoinit;

  *xmin = vxmin;
  *xmax = vxmax;
  *ymin = vymin;
  *ymax = vymax;
}

/*!
 * Select a predefined transformation from world coordinates to normalized
 * device coordinates.
 *
 * \param[in] transform A normalization transformation number.
 *
 * \verbatim embed:rst:leading-asterisk
 *
 * +------+----------------------------------------------------------------------------------------------------+
 * |     0|Selects the identity transformation in which both the window and viewport have the range of 0 to 1  |
 * +------+----------------------------------------------------------------------------------------------------+
 * |  >= 1|Selects a normalization transformation as defined by `setwindow` and `setviewport`                  |
 * +------+----------------------------------------------------------------------------------------------------+
 *
 * \endverbatim
 */
void gr_selntran(int transform)
{
  check_autoinit;

  gks_select_xform(transform);

  if (flag_graphics) gr_writestream("<selntran transform=\"%d\"/>\n", transform);
}

/*!
 * Set the clipping indicator.
 *
 * \param[in] indicator An indicator specifying whether clipping is on or off.
 *
 * \verbatim embed:rst:leading-asterisk
 *
 * +----+---------------------------------------------------------------+
 * |   0|Clipping is off. Data outside of the window will be drawn.     |
 * +----+---------------------------------------------------------------+
 * |   1|Clipping is on. Data outside of the window will not be drawn.  |
 * +----+---------------------------------------------------------------+
 *
 * \endverbatim
 *
 * This function enables or disables clipping of the image drawn in the current
 * window. Clipping is defined as the removal of those portions of the graph
 * that lie outside of the defined viewport. If clipping is on, GR does not
 * draw generated output primitives past the viewport boundaries. If clipping
 * is off, primitives may exceed the viewport boundaries, and they will be
 * drawn to the edge of the workstation window. By default, clipping is on.
 */
void gr_setclip(int indicator)
{
  check_autoinit;

  gks_set_clipping(indicator);

  if (flag_graphics) gr_writestream("<setclip indicator=\"%d\"/>\n", indicator);
}

static void wswindow(int workstation_id, rect_t *rect)
{
  int wkid = workstation_id;

  gks_set_ws_window(wkid, rect->xmin, rect->xmax, rect->ymin, rect->ymax);
}

/*!
 * Set the area of the NDC viewport that is to be drawn in the workstation
 * window.
 *
 * \param[in] xmin The left horizontal coordinate of the workstation window
 *                 (0 <= xmin < xmax).
 * \param[in] xmax The right horizontal coordinate of the workstation window
 *                 (xmin < xmax <= 1).
 * \param[in] ymin The bottom vertical coordinate of the workstation window
 *                 (0 <= ymin < ymax).
 * \param[in] ymax The top vertical coordinate of the workstation window
 *                 (ymin < ymax <= 1).
 *
 * This function defines the rectangular area of the Normalized Device
 * Coordinate space to be output to the device. By default, the workstation
 * transformation will map the range [0,1] x [0,1] in NDC onto the largest
 * square on the workstation's display surface. The aspect ratio of the
 * workstation window is maintained at 1 to 1.
 */
void gr_setwswindow(double xmin, double xmax, double ymin, double ymax)
{
  rect_t rect;

  rect.xmin = xmin;
  rect.xmax = xmax;
  rect.ymin = ymin;
  rect.ymax = ymax;

  check_autoinit;

  foreach_activews((void (*)(int, void *))wswindow, (void *)&rect);

  if (flag_graphics)
    gr_writestream("<setwswindow xmin=\"%g\" xmax=\"%g\" ymin=\"%g\" ymax=\"%g\"/>\n", xmin, xmax, ymin, ymax);
}

static void wsviewport(int workstation_id, rect_t *rect)
{
  int wkid = workstation_id;

  gks_set_ws_viewport(wkid, rect->xmin, rect->xmax, rect->ymin, rect->ymax);
}

/*!
 * Define the size of the workstation graphics window in meters.
 *
 * \param[in] xmin The left horizontal coordinate of the workstation window.
 * \param[in] xmax The right horizontal coordinate of the workstation window.
 * \param[in] ymin The bottom vertical coordinate of the workstation window.
 * \param[in] ymax The top vertical coordinate of the workstation window.
 *
 * This function places a workstation window on the display of the specified
 * size in meters. This command allows the workstation window to be accurately
 * sized for a display or hardcopy device, and is often useful for sizing
 * graphs for desktop publishing applications.
 */
void gr_setwsviewport(double xmin, double xmax, double ymin, double ymax)
{
  rect_t rect;

  rect.xmin = xmin;
  rect.xmax = xmax;
  rect.ymin = ymin;
  rect.ymax = ymax;

  check_autoinit;

  foreach_activews((void (*)(int, void *))wsviewport, (void *)&rect);

  sizex = xmax - xmin;

  if (flag_graphics)
    gr_writestream("<setwsviewport xmin=\"%g\" xmax=\"%g\" ymin=\"%g\" ymax=\"%g\"/>\n", xmin, xmax, ymin, ymax);
}

void gr_createseg(int segment)
{
  check_autoinit;

  gks_create_seg(segment);
}

static void copyseg(int workstation_id, int *segment)
{
  int wkid = workstation_id, errind, conid, wtype, wkcat;

  gks_inq_ws_conntype(wkid, &errind, &conid, &wtype);
  gks_inq_ws_category(wtype, &errind, &wkcat);

  if (wkcat == GKS_K_WSCAT_OUTPUT || wkcat == GKS_K_WSCAT_OUTIN)
    {
      gks_copy_seg_to_ws(wkid, *segment);
      gks_update_ws(wkid, GKS_K_POSTPONE_FLAG);
    }
}

void gr_copysegws(int segment)
{
  int segn = segment;

  check_autoinit;

  foreach_activews((void (*)(int, void *))copyseg, (void *)&segn);
}

static void redrawseg(int workstation_id, void *foo)
{
  int wkid = workstation_id;

  gks_redraw_seg_on_ws(wkid);
}

void gr_redrawsegws(void)
{
  check_autoinit;

  foreach_activews((void (*)(int, void *))redrawseg, (void *)NULL);
}

void gr_setsegtran(int segment, double fx, double fy, double transx, double transy, double phi, double scalex,
                   double scaley)
{
  int segn = segment;
  double mat[3][2];

  check_autoinit;

  gks_eval_xform_matrix(fx, fy, transx, transy, phi, scalex, scaley, GKS_K_COORDINATES_NDC, mat);
  gks_set_seg_xform(segn, mat);
}

void gr_closeseg(void)
{
  check_autoinit;

  gks_close_seg();
}

void gr_emergencyclosegks(void)
{
  gks_emergency_close();
  autoinit = 1;
}

void gr_updategks(void)
{
  int state, count, n, errind, ol;
  int wkid, conid, wtype, wkcat;

  gks_inq_operating_state(&state);
  if (state >= GKS_K_WSOP)
    {
      n = 1;
      gks_inq_open_ws(n, &errind, &ol, &wkid);

      for (count = 1; count <= ol; count++)
        {
          n = count;
          gks_inq_open_ws(n, &errind, &ol, &wkid);

          gks_inq_ws_conntype(wkid, &errind, &conid, &wtype);
          gks_inq_ws_category(wtype, &errind, &wkcat);

          if (wkcat == GKS_K_WSCAT_OUTPUT || wkcat == GKS_K_WSCAT_OUTIN)
            {
              gks_update_ws(wkid, GKS_K_WRITE_PAGE_FLAG);
            }
        }
    }
}

/*!
 * Set the abstract Z-space used for mapping three-dimensional output primitives
 * into the current world coordinate space.
 *
 * \param[in] zmin Minimum value for the Z-axis.
 * \param[in] zmax Maximum value for the Z-axis.
 * \param[in] rotation Angle for the rotation of the X axis, in degrees.
 * \param[in] tilt Viewing angle of the Z axis, in degrees.
 *
 * This function establishes the limits of an abstract Z-axis and defines the
 * angles for rotation and for the viewing angle (tilt) of a simulated
 * three-dimensional graph, used for mapping corresponding output primitives
 * into the current window. These settings are used for all subsequent
 * three-dimensional output primitives until other values are specified. Angles
 * of rotation and viewing angle must be specified between 0 and 90 degrees.
 */
int gr_setspace(double zmin, double zmax, int rotation, int tilt)
{
  if (zmin < zmax)
    {
      if (rotation < 0 || rotation > 90 || tilt < 0 || tilt > 90) return -1;
    }
  else
    return -1;

  check_autoinit;

  setspace(zmin, zmax, rotation, tilt);

  if (flag_graphics)
    gr_writestream("<setspace zmin=\"%g\" zmax=\"%g\" rotation=\"%d\" tilt=\"%d\"/>\n", zmin, zmax, rotation, tilt);

  return 0;
}

/*!
 * Set the projection type with this flag.
 *
 * \param[in] flag projection type
 *
 * The available options are:
 *
 * \verbatim embed:rst:leading-asterisk
 *
 * +---------------------------+---+--------------+
 * |GR_PROJECTION_DEFAULT      |  0|default       |
 * +---------------------------+---+--------------+
 * |GR_PROJECTION_ORTHOGRAPHIC |  1|orthographic  |
 * +---------------------------+---+--------------+
 * |GR_PROJECTION_PERSPECTIVE  |  2|perspective   |
 * +---------------------------+---+--------------+
 *
 * \endverbatim
 *
 */
void gr_setprojectiontype(int flag)
{
  check_autoinit;

  if (flag == GR_PROJECTION_DEFAULT || flag == GR_PROJECTION_ORTHOGRAPHIC || flag == GR_PROJECTION_PERSPECTIVE)
    {
      gpx.projection_type = flag;

      if (flag_graphics) gr_writestream("<setprojectiontype flag=\"%i\"/>\n", flag);
    }
  else
    {
      fprintf(stderr, "Invalid projection flag. Possible options are GR_PROJECTION_DEFAULT, GR_PROJECTION_ORTHOGRAPHIC "
                      "and GR_PROJECTION_PERSPECTIV\n");
    }
}

/*!
 * Return the projection type.
 */
void gr_inqprojectiontype(int *projection_type)
{
  check_autoinit;

  *projection_type = gpx.projection_type;
}

/*!
 * Method to set the camera position, the upward facing direction and the focus point of the shown
 * volume
 *
 * \param camera_pos_x x component of the cameraposition in world coordinates
 * \param camera_pos_y y component of the cameraposition in world coordinates
 * \param camera_pos_z z component of the cameraposition in world coordinates
 * \param up_x x component of the up vector
 * \param up_y y component of the up vector
 * \param up_z z component of the up vector
 * \param focus_point_x x component of focus-point inside volume
 * \param focus_point_y y component of focus-point inside volume
 * \param focus_point_z z component of focus-point inside volume
 *
 */
void gr_settransformationparameters(double camera_pos_x, double camera_pos_y, double camera_pos_z, double up_x,
                                    double up_y, double up_z, double focus_point_x, double focus_point_y,
                                    double focus_point_z)
{

  check_autoinit;

  int i;

  tx.camera_pos_x = camera_pos_x;
  tx.camera_pos_y = camera_pos_y;
  tx.camera_pos_z = camera_pos_z;

  tx.focus_point_x = focus_point_x;
  tx.focus_point_y = focus_point_y;
  tx.focus_point_z = focus_point_z;

  double F[3] = {tx.focus_point_x - tx.camera_pos_x, tx.focus_point_y - tx.camera_pos_y,
                 tx.focus_point_z - tx.camera_pos_z}; /* direction between camera and focus point */
  double norm_func = sqrt(F[0] * F[0] + F[1] * F[1] + F[2] * F[2]);
  double f[3] = {F[0] / norm_func, F[1] / norm_func, F[2] / norm_func};
  double norm_up = sqrt(up_x * up_x + up_y * up_y + up_z * up_z);
  double up[3] = {up_x / norm_up, up_y / norm_up, up_z / norm_up};
  double s_deri[3];
  for (i = 0; i < 3; i++) /*  f cross up */
    {
      s_deri[i] = f[(i + 1) % 3] * up[(i + 2) % 3] - up[(i + 1) % 3] * f[(i + 2) % 3];
    }
  double s_norm = sqrt(s_deri[0] * s_deri[0] + s_deri[1] * s_deri[1] + s_deri[2] * s_deri[2]);
  double s[3] = {s_deri[0] / s_norm, s_deri[1] / s_norm, s_deri[2] / s_norm};
  double u_deri[3];
  for (i = 0; i < 3; i++) /* s cross f */
    {
      u_deri[i] = s[(i + 1) % 3] * f[(i + 2) % 3] - f[(i + 1) % 3] * s[(i + 2) % 3];
    }
  double norm_u = sqrt(u_deri[0] * u_deri[0] + u_deri[1] * u_deri[1] + u_deri[2] * u_deri[2]);
  double u[3] = {u_deri[0] / norm_u, u_deri[1] / norm_u, u_deri[2] / norm_u};

  tx.up_x = u[0];
  tx.up_y = u[1];
  tx.up_z = u[2];
  tx.s_x = s[0];
  tx.s_y = s[1];
  tx.s_z = s[2];

  /* Undo possible axis scaling */
  tx.x_axis_scale = 1;
  tx.y_axis_scale = 1;
  tx.z_axis_scale = 1;

  if (flag_graphics)
    gr_writestream("<settransformationparameters camera_pos_x=\"%g\" camera_pos_y=\"%g\" camera_pos_z=\"%g\" "
                   "up_x=\"%g\" up_y=\"%g\" "
                   "up_z=\"%g\" focus_point_x=\"%g\" focus_point_y=\"%g\" focus_point_z=\"%g\"/>\n",
                   camera_pos_x, camera_pos_y, camera_pos_z, up_x, up_y, up_z, focus_point_x, focus_point_y,
                   focus_point_z);
}

/*!
 * Set the far and near clipping plane for perspective projection and the vertical field ov view
 *
 * \param near_plane distance to near clipping plane
 * \param far_plane distance to far clipping plane
 * \param fov vertical field of view, input must be between 0 and 180 degrees
 *
 * Switches projection type to perspective
 */
void gr_setperspectiveprojection(double near_plane, double far_plane, double fov)
{

  check_autoinit;

  gpx.near_plane = near_plane;
  gpx.far_plane = far_plane;

  if (0 < fov && fov < 180)
    {
      gpx.fov = fov;
    }
  else
    {
      fprintf(stderr, "The value for the fov parameter is not between 0 and 180 degree\n");
    }

  gpx.projection_type = GR_PROJECTION_PERSPECTIVE;
  if (flag_graphics)
    gr_writestream("<setperspectiveprojection near_plane=\"%g\" far_plane=\"%g\" fov=\"%g\"/>\n", near_plane, far_plane,
                   fov);
}

/*!
 * Set parameters for orthographic transformation
 *
 * \param left xmin of the volume in worldcoordinates
 * \param right xmax of volume in worldcoordinates
 * \param bottom ymin of volume in worldcoordinates
 * \param top ymax of volume in worldcoordinates
 * \param near_plane distance to near clipping plane
 * \param far_plane distance to far clipping plane
 *
 * Switches projection type to orthographic
 */
void gr_setorthographicprojection(double left, double right, double bottom, double top, double near_plane,
                                  double far_plane)
{
  check_autoinit;

  gpx.left = left;
  gpx.right = right;
  gpx.bottom = bottom;
  gpx.top = top;
  gpx.near_plane = near_plane;
  gpx.far_plane = far_plane;

  gpx.projection_type = GR_PROJECTION_ORTHOGRAPHIC;
  if (flag_graphics)
    gr_writestream("<setorthographicprojection left=\"%g\" right=\"%g\" bottom=\"%g\" top=\"%g\" near_plane=\"%g\" "
                   "far_plane=\"%g\"/>\n",
                   left, right, bottom, top, near_plane, far_plane);
}

/*!
 * Return the camera position, up vector and focus point.
 */
void gr_inqtransformationparameters(double *camera_pos_x, double *camera_pos_y, double *camera_pos_z, double *up_x,
                                    double *up_y, double *up_z, double *focus_point_x, double *focus_point_y,
                                    double *focus_point_z)
{

  check_autoinit;

  *camera_pos_x = tx.camera_pos_x;
  *camera_pos_y = tx.camera_pos_y;
  *camera_pos_z = tx.camera_pos_z;

  *up_x = tx.up_x;
  *up_y = tx.up_y;
  *up_z = tx.up_z;

  *focus_point_x = tx.focus_point_x;
  *focus_point_y = tx.focus_point_y;
  *focus_point_z = tx.focus_point_z;
}

/*!
 *  Return the parameters for the orthographic projection.
 */
void gr_inqorthographicprojection(double *left, double *right, double *bottom, double *top, double *near_plane,
                                  double *far_plane)
{

  check_autoinit;

  *left = gpx.left;
  *right = gpx.right;
  *bottom = gpx.bottom;
  *top = gpx.top;
  *near_plane = gpx.near_plane;
  *far_plane = gpx.far_plane;
}

/*!
 *  Return the parameters for the perspective projection.
 */
void gr_inqperspectiveprojection(double *near_plane, double *far_plane, double *fov)
{

  check_autoinit;

  *near_plane = gpx.near_plane;
  *far_plane = gpx.far_plane;
  *fov = gpx.fov;
}

void gr_inqspace(double *zmin, double *zmax, int *rotation, int *tilt)
{
  check_autoinit;

  *zmin = wx.zmin;
  *zmax = wx.zmax;
  *rotation = wx.phi;
  *tilt = wx.delta;
}

double intpart(double x)
{
  double ipart;
  modf(x, &ipart);
  return ipart;
}

static double pred(double x)
{
  double ipart;
  ipart = intpart(x);
  if (x == ipart)
    return ipart - 1;
  else
    return gauss(x);
}

#define ipred(x) ((int64_t)pred(x))

double succ(double x)
{
  double ipart;
  ipart = intpart(x);
  if (x == ipart)
    return ipart;
  else
    return gauss(x) + 1;
}

#define isucc(x) ((int64_t)succ(x))

static double fract(double x)
{
  double _intpart;
  return modf(x, &_intpart);
}

static void end_pline(void)
{
  if (npoints >= 2)
    {
      gks_polyline(npoints, xpoint, ypoint);
      npoints = 0;
    }
}

static void pline(double x, double y)
{
  if (npoints >= maxpath) reallocate(npoints);

  xpoint[npoints] = x_lin(x);
  ypoint[npoints] = y_lin(y);
  npoints++;
}

static void start_pline(double x, double y)
{
  end_pline();

  npoints = 0;
  pline(x, y);
}

static void pline3d(double x, double y, double z)
{
  if (npoints >= maxpath) reallocate(npoints);

  xpoint[npoints] = x_lin(x);
  ypoint[npoints] = y_lin(y);
  zpoint[npoints] = z_lin(z);

  apply_world_xform(xpoint + npoints, ypoint + npoints, zpoint + npoints);

  npoints++;
}

static void start_pline3d(double x, double y, double z)
{
  end_pline();

  npoints = 0;
  pline3d(x, y, z);
}

/*!
 * Draw a text at position `x`, `y` using the current text attributes. Strings
 * can be defined to create basic mathematical expressions and Greek letters.
 *
 * \param[in] x The X coordinate of starting position of the text string
 * \param[in] y The Y coordinate of starting position of the text string
 * \param[in] string The text to be drawn
 *
 * The values for X and Y are in normalized device coordinates.
 * The attributes that control the appearance of text are text font and
 * precision, character expansion factor, character spacing, text color index,
 * character height, character up vector, text path and text alignment.
 *
 * The character string is interpreted to be a simple mathematical formula.
 * The following notations apply:
 *
 * Subscripts and superscripts: These are indicated by carets ('^') and
 * underscores ('_'). If the sub/superscript contains more than one character,
 * it must be enclosed in curly braces ('{}').
 *
 * Fractions are typeset with A '/' B, where A stands for the numerator and B
 * for the denominator.
 *
 * To include a Greek letter you must specify the corresponding keyword after a
 * backslash ('\') character. The text translator produces uppercase or
 * lowercase Greek letters depending on the case of the keyword.

 * \verbatim embed:rst:leading-asterisk
 *
 * +----------------------------------+--------+
 * |Letter                            |Keyword |
 * +----------------------------------+--------+
 * |:math:`A`      :math:`\alpha`     |alpha   |
 * +----------------------------------+--------+
 * |:math:`B` :math:`\beta`           |beta    |
 * +----------------------------------+--------+
 * |:math:`\Gamma` :math:`\gamma`     |gamma   |
 * +----------------------------------+--------+
 * |:math:`\Delta` :math:`\delta`     |delta   |
 * +----------------------------------+--------+
 * |:math:`E` :math:`\epsilon`        |epsilon |
 * +----------------------------------+--------+
 * |:math:`Z` :math:`\zeta`           |zeta    |
 * +----------------------------------+--------+
 * |:math:`H` :math:`\eta`            |eta     |
 * +----------------------------------+--------+
 * |:math:`\Theta` :math:`\theta`     |theta   |
 * +----------------------------------+--------+
 * |:math:`I` :math:`\iota`           |iota    |
 * +----------------------------------+--------+
 * |:math:`K` :math:`\kappa`          |kappa   |
 * +----------------------------------+--------+
 * |:math:`\Lambda` :math:`\lambda`   |lambda  |
 * +----------------------------------+--------+
 * |:math:`M` :math:`\mu`             |mu      |
 * +----------------------------------+--------+
 * |:math:`N` :math:`\nu`             |v       |
 * +----------------------------------+--------+
 * |:math:`\Xi` :math:`\xi`           |xi      |
 * +----------------------------------+--------+
 * |:math:`O` :math:`o`               |omicron |
 * +----------------------------------+--------+
 * |:math:`\Pi` :math:`\pi`           |pi      |
 * +----------------------------------+--------+
 * |:math:`P` :math:`\rho`            |rho     |
 * +----------------------------------+--------+
 * |:math:`\Sigma` :math:`\sigma`     |sigma   |
 * +----------------------------------+--------+
 * |:math:`T` :math:`\tau`            |tau     |
 * +----------------------------------+--------+
 * |:math:`\Upsilon` :math:`\upsilon` |upsilon |
 * +----------------------------------+--------+
 * |:math:`\Phi` :math:`\phi`         |phi     |
 * +----------------------------------+--------+
 * |:math:`X` :math:`\chi`            |chi     |
 * +----------------------------------+--------+
 * |:math:`\Psi` :math:`\psi`         |psi     |
 * +----------------------------------+--------+
 * |:math:`\Omega` :math:`\omega`     |omega   |
 * +----------------------------------+--------+
 *
 * \endverbatim
 *
 * For more sophisticated mathematical formulas, you should use the gr_mathtex
 * function.
 *
 * Note: 'v' is a replacement for 'nu' which would conflict with '\n' (newline)
 */
int gr_textext(double x, double y, char *string)
{
  int errind, tnr, result;

  check_autoinit;

  gks_inq_current_xformno(&errind, &tnr);
  if (tnr != NDC) gks_select_xform(NDC);

  result = gr_textex(x, y, string, 0, NULL, NULL);

  if (tnr != NDC) gks_select_xform(tnr);

  if (flag_graphics) gr_writestream("<textext x=\"%g\" y=\"%g\" text=\"%s\"/>\n", x, y, string);

  return result;
}

void gr_inqtextext(double x, double y, char *string, double *tbx, double *tby)
{
  int errind, tnr;
  int i;

  check_autoinit;

  gks_inq_current_xformno(&errind, &tnr);
  if (tnr != NDC) gks_select_xform(NDC);

  gr_textex(x, y, string, 1, tbx, tby);

  if (tnr != NDC)
    {
      gks_select_xform(tnr);

      for (i = 0; i < 4; i++)
        {
          tbx[i] = (tbx[i] - nx.b) / nx.a;
          tby[i] = (tby[i] - nx.d) / nx.c;
          if (lx.scale_options)
            {
              tbx[i] = x_log(tbx[i]);
              tby[i] = y_log(tby[i]);
            }
        }
    }
}

static void text2dlbl(double x, double y, const char *chars, double value,
                      void (*fp)(double, double, const char *, double))
{
  int errind, tnr;

  if (lx.scale_options)
    {
      x = x_lin(x);
      y = y_lin(y);
    }

  gks_inq_current_xformno(&errind, &tnr);
  if (tnr != NDC)
    {
      x = nx.a * x + nx.b;
      y = nx.c * y + nx.d;
      gks_select_xform(NDC);
    }

  if (fp == NULL)
    gr_textex(x, y, chars, 0, NULL, NULL);
  else
    fp(x, y, chars, value);

  if (tnr != NDC) gks_select_xform(tnr);
}

static void text2d(double x, double y, const char *chars)
{
  /* 42. dummy value will not be interpreted until last argument fp != NULL */
  text2dlbl(x, y, chars, 42., NULL);
}

static char *replace_minus_sign(char *string)
{
  int errind, font, prec, encoding, n = 0;
  char *s = string, *out;

  gks_inq_text_fontprec(&errind, &font, &prec);
  gks_inq_encoding(&encoding);
  if (prec != GKS_K_TEXT_PRECISION_OUTLINE || encoding != ENCODING_UTF8) return s;

  out = xmalloc(256);

  while (*s && n < 255)
    {
      if (*s == '-')
        {
          out[n++] = 0xe2;
          out[n++] = 0x88;
          out[n++] = 0x92;
        }
      else
        out[n++] = *s;
      s++;
    }
  out[n] = '\0';

  strcpy(string, out);
  free(out);

  return string;
}

static char *gr_ftoa(char *string, double value, double reference)
{
  char *s;

  s = str_ftoa(string, value, reference);

  return replace_minus_sign(s);
}

/*!
 * Create axes in the current workspace and supply a custom function for changing
 * the behaviour of the tick labels.
 *
 * Similar to gr_axes() but allows more fine-grained control over tick labels
 * and text positioning by supplying callback functions. Within the callback
 * function you can use normal GR text primitives for performing any manipulations
 * on the label text.
 *
 * \param[in] x_tick The interval between minor tick marks on the X axis.
 * \param[in] y_tick The interval between minor tick marks on the Y axis.
 * \param[in] x_org The world coordinate of the origin (point of intersection)
 *                  of the X axis.
 * \param[in] y_org The world coordinate of the origin (point of intersection)
 *                  of the Y axis.
 * \param[in] major_x Unitless integer value specifying the number of minor tick
 *                    intervals between major tick marks on the X axis. Values
 *                    of 0 or 1 imply no minor ticks. Negative values specify no
 *                    labels will be drawn for the associated axis.
 * \param[in] major_y Unitless integer value specifying the number of minor tick
 *                    intervals between major tick marks on the Y axis. Values
 *                    of 0 or 1 imply no minor ticks. Negative values specify no
 *                    labels will be drawn for the associated axis.
 * \param[in] tick_size The length of minor tick marks specified in a normalized
 *                      device coordinate unit. Major tick marks are twice as
 *                      long as minor tick marks. A negative value reverses the
 *                      tick marks on the axes from inward facing to outward
 *                      facing (or vice versa).
 * \param[in] fpx Function pointer to a function that returns a label for a given
 *                tick on the X axis. The callback function should have the following arguments:
 *                \param[in] x NDC of the label in X direction.
 *                \param[in] y NDC of the label in Y direction.
 *                \param[in] svalue Internal string representation of the text
 *                                  drawn by GR at (x,y).
 *                \param[in] value Floating point representation of the label drawn
 *                                 at (x,y).
 * \param[in] fpy Exactly same as the fpx above, but for the the Y axis.
 *
 * See gr_axes() for more details on drawing axes.
 */
void gr_axeslbl(double x_tick, double y_tick, double x_org, double y_org, int major_x, int major_y, double tick_size,
                void (*fpx)(double, double, const char *, double), void (*fpy)(double, double, const char *, double))
{
  int errind, tnr;
  int ltype, halign, valign, clsw;
  double chux, chuy;

  double clrt[4], wn[4], vp[4];
  double x_min, x_max, y_min, y_max, feps;

  double tick, minor_tick, major_tick, x_label, y_label, x0, y0, xi, yi;
  int64_t i;
  int decade, exponent;
  char string[256];

  if (x_tick < 0 || y_tick < 0)
    {
      fprintf(stderr, "invalid interval length for major tick-marks\n");
      return;
    }
  else if (tick_size == 0)
    {
      fprintf(stderr, "invalid tick-size\n");
      return;
    }

  check_autoinit;

  /* inquire current normalization transformation */

  gks_inq_current_xformno(&errind, &tnr);
  gks_inq_xform(tnr, &errind, wn, vp);

  x_min = wn[0];
  x_max = wn[1];
  y_min = wn[2];
  y_max = wn[3];

  if (x_min > x_org || x_org > x_max || y_min > y_org || y_org > y_max)
    {
      fprintf(stderr, "origin outside current window\n");
      return;
    }

  /* save linetype, text alignment, character-up vector and
     clipping indicator */

  gks_inq_pline_linetype(&errind, &ltype);
  gks_inq_text_align(&errind, &halign, &valign);
  gks_inq_text_upvec(&errind, &chux, &chuy);
  gks_inq_clip(&errind, &clsw, clrt);

  gks_set_pline_linetype(GKS_K_LINETYPE_SOLID);
  gks_set_text_upvec(0, 1);
  gks_set_clipping(GKS_K_NOCLIP);

  if (y_tick != 0)
    {
      tick = tick_size * (x_max - x_min) / (vp[1] - vp[0]);

      minor_tick = x_log(x_lin(x_org) + tick);
      major_tick = x_log(x_lin(x_org) + 2. * tick);
      x_label = x_log(x_lin(x_org) + 3. * tick);

      /* set text alignment */

      if (x_lin(x_org) <= (x_lin(x_min) + x_lin(x_max)) / 2.)
        {
          gks_set_text_align(GKS_K_TEXT_HALIGN_RIGHT, GKS_K_TEXT_VALIGN_HALF);

          if (tick > 0) x_label = x_log(x_lin(x_org) - tick);
        }
      else
        {
          gks_set_text_align(GKS_K_TEXT_HALIGN_LEFT, GKS_K_TEXT_VALIGN_HALF);

          if (tick < 0) x_label = x_log(x_lin(x_org) - tick);
        }

      if (OPTION_Y_LOG & lx.scale_options)
        {
          y0 = pow(10.0, gauss(log10(y_min)));

          i = ipred(y_min / y0);
          yi = y0 + i * y0;
          decade = igauss(log10(y_min / y_org));

          /* draw Y-axis */

          start_pline(x_org, y_min);

          while (yi <= y_max)
            {
              pline(x_org, yi);

              if (i == 0)
                {
                  xi = major_tick;

                  if (major_y > 0)
                    if (decade % major_y == 0)
                      if (yi != y_org || y_org == y_min || y_org == y_max)
                        {
                          if (y_tick > 1)
                            {
                              exponent = iround(log10(yi));
                              sprintf(string, "10^{%d}", exponent);
                              text2dlbl(x_label, yi, replace_minus_sign(string), yi, fpy);
                            }
                          else
                            text2dlbl(x_label, yi, gr_ftoa(string, yi, 0.), yi, fpy);
                        }
                }
              else
                xi = minor_tick;

              if (i == 0 || abs(major_y) == 1)
                {
                  pline(xi, yi);
                  pline(x_org, yi);
                }

              if (i == 9)
                {
                  y0 = y0 * 10.;
                  i = 0;
                  decade++;
                }
              else
                i++;

              yi = y0 + i * y0;
            }

          if (yi > y_max) pline(x_org, y_max);

          end_pline();
        }
      else
        {
          feps = FEPS * (y_max - y_min);

          check_tick_marks(y_min, y_max, y_tick, 'Y');

          i = isucc(y_min / y_tick);
          yi = i * y_tick;

          /* draw Y-axis */

          start_pline(x_org, y_min);

          while (yi <= y_max + feps)
            {
              pline(x_org, yi);

              if (major_y != 0)
                {
                  if (i % major_y == 0)
                    {
                      xi = major_tick;
                      if (yi != y_org || y_org == y_min || y_org == y_max)
                        if (major_y > 0) text2dlbl(x_label, yi, gr_ftoa(string, yi, y_tick * major_y), yi, fpy);
                    }
                  else
                    xi = minor_tick;
                }
              else
                xi = major_tick;

              pline(xi, yi);
              pline(x_org, yi);

              i++;
              yi = i * y_tick;
            }

          if (yi > y_max + feps) pline(x_org, y_max);

          end_pline();
        }
    }

  if (x_tick != 0)
    {
      tick = tick_size * (y_max - y_min) / (vp[3] - vp[2]);

      minor_tick = y_log(y_lin(y_org) + tick);
      major_tick = y_log(y_lin(y_org) + 2. * tick);
      y_label = y_log(y_lin(y_org) + 3. * tick);

      /* set text alignment */

      if (y_lin(y_org) <= (y_lin(y_min) + y_lin(y_max)) / 2.)
        {
          gks_set_text_align(GKS_K_TEXT_HALIGN_CENTER, GKS_K_TEXT_VALIGN_TOP);

          if (tick > 0) y_label = y_log(y_lin(y_org) - tick);
        }
      else
        {
          gks_set_text_align(GKS_K_TEXT_HALIGN_CENTER, GKS_K_TEXT_VALIGN_BOTTOM);

          if (tick < 0) y_label = y_log(y_lin(y_org) - tick);
        }

      if (OPTION_X_LOG & lx.scale_options)
        {
          x0 = pow(10.0, gauss(log10(x_min)));

          i = ipred(x_min / x0);
          xi = x0 + i * x0;
          decade = igauss(log10(x_min / x_org));

          /* draw X-axis */

          start_pline(x_min, y_org);

          while (xi <= x_max)
            {
              pline(xi, y_org);

              if (i == 0)
                {
                  yi = major_tick;

                  if (major_x > 0)
                    if (decade % major_x == 0)
                      if (xi != x_org || x_org == x_min || x_org == x_max)
                        {
                          if (x_tick > 1)
                            {
                              exponent = iround(log10(xi));
                              sprintf(string, "10^{%d}", exponent);
                              text2dlbl(xi, y_label, replace_minus_sign(string), xi, fpx);
                            }
                          else
                            text2dlbl(xi, y_label, gr_ftoa(string, xi, 0.), xi, fpx);
                        }
                }
              else
                yi = minor_tick;

              if (i == 0 || abs(major_x) == 1)
                {
                  pline(xi, yi);
                  pline(xi, y_org);
                }

              if (i == 9)
                {
                  x0 = x0 * 10.;
                  i = 0;
                  decade++;
                }
              else
                i++;

              xi = x0 + i * x0;
            }

          if (xi > x_max) pline(x_max, y_org);

          end_pline();
        }
      else
        {
          feps = FEPS * (x_max - x_min);

          check_tick_marks(x_min, x_max, x_tick, 'X');

          i = isucc(x_min / x_tick);
          xi = i * x_tick;

          /* draw X-axis */

          start_pline(x_min, y_org);

          while (xi <= x_max + feps)
            {
              pline(xi, y_org);

              if (major_x != 0)
                {
                  if (i % major_x == 0)
                    {
                      yi = major_tick;
                      if (xi != x_org || x_org == x_min || x_org == x_max)
                        if (major_x > 0) text2dlbl(xi, y_label, gr_ftoa(string, xi, x_tick * major_x), xi, fpx);
                    }
                  else
                    yi = minor_tick;
                }
              else
                yi = major_tick;

              pline(xi, yi);
              pline(xi, y_org);

              i++;
              xi = i * x_tick;
            }

          if (xi > x_max + feps) pline(x_max, y_org);

          end_pline();
        }
    }

  /* restore linetype, text alignment, character-up vector
     and clipping indicator */

  gks_set_pline_linetype(ltype);
  gks_set_text_align(halign, valign);
  gks_set_text_upvec(chux, chuy);
  gks_set_clipping(clsw);

  if (flag_graphics)
    gr_writestream("<axes xtick=\"%g\" ytick=\"%g\" xorg=\"%g\" yorg=\"%g\" "
                   "majorx=\"%d\" majory=\"%d\" ticksize=\"%g\"/>\n",
                   x_tick, y_tick, x_org, y_org, major_x, major_y, tick_size);
}

/*!
 * Draw X and Y coordinate axes with linearly and/or logarithmically spaced
 * tick marks.
 *
 * \param[in] x_tick The interval between minor tick marks on the X axis.
 * \param[in] y_tick The interval between minor tick marks on the Y axis.
 * \param[in] x_org The world coordinate of the origin (point of intersection)
 *                  of the X axis.
 * \param[in] y_org The world coordinate of the origin (point of intersection)
 *                  of the Y axis.
 * \param[in] major_x Unitless integer value specifying the number of minor tick
 *                    intervals between major tick marks on the X axis. Values
 *                    of 0 or 1 imply no minor ticks. Negative values specify no
 *                    labels will be drawn for the associated axis.
 * \param[in] major_y Unitless integer value specifying the number of minor tick
 *                    intervals between major tick marks on the Y axis. Values
 *                    of 0 or 1 imply no minor ticks. Negative values specify no
 *                    labels will be drawn for the associated axis.
 * \param[in] tick_size The length of minor tick marks specified in a normalized
 *                      device coordinate unit. Major tick marks are twice as
 *                      long as minor tick marks. A negative value reverses the
 *                      tick marks on the axes from inward facing to outward
 *                      facing (or vice versa).
 *
 * Tick marks are positioned along each axis so that major tick marks fall on
 * the axes origin (whether visible or not). Major tick marks are labeled with
 * the corresponding data values. Axes are drawn according to the scale of the
 * window. Axes and tick marks are drawn using solid lines; line color and width
 * can be modified using the gr_setlinetype and gr_setlinewidth functions. Axes
 * are drawn according to the linear or logarithmic transformation established
 * by the gr_setscale function.
 */
void gr_axes(double x_tick, double y_tick, double x_org, double y_org, int major_x, int major_y, double tick_size)
{
  gr_axeslbl(x_tick, y_tick, x_org, y_org, major_x, major_y, tick_size, NULL, NULL);
}

static void grid_line(double x0, double y0, double x1, double y1, int color, int major)
{
  if (color != 0)
    gks_set_pline_color_index(major ? 88 : 90);
  else
    gks_set_pline_linewidth(major ? 2.0 : 1.0);

  start_pline(x0, y0);
  pline(x1, y1);
  end_pline();
}

/*!
 * Draw a linear and/or logarithmic grid.
 *
 * \param[in] x_tick The length in world coordinates of the interval between
 *                   minor grid lines in X direction.
 * \param[in] y_tick The length in world coordinates of the interval between
 *                   minor grid lines in Y direction.
 * \param[in] x_org The world coordinate of the origin (point of intersection)
 *                  of the X axis.
 * \param[in] y_org The world coordinate of the origin (point of intersection)
 *                  of the Y axis.
 * \param[in] major_x Unitless integer value specifying the number of minor
 *                    grid lines between major grid lines on the X axis. Values
 *                    of 0 or 1 imply no grid lines.
 * \param[in] major_y Unitless integer value specifying the number of minor
 *                    grid lines between major grid lines on the Y axis. Values
 *                    of 0 or 1 imply no grid lines.
 *
 * Major grid lines correspond to the axes origin and major tick marks whether
 * visible or not. Minor grid lines are drawn at points equal to minor tick
 * marks. Major grid lines are drawn using black lines and minor grid lines are
 * drawn using gray lines.
 */
void gr_grid(double x_tick, double y_tick, double x_org, double y_org, int major_x, int major_y)
{
  int errind, tnr;
  int ltype, color, clsw, major;
  double width;

  double clrt[4], wn[4], vp[4];
  double x_min, x_max, y_min, y_max, feps;

  double x0, y0, xi, yi;
  int64_t i;

  if (x_tick < 0 || y_tick < 0)
    {
      fprintf(stderr, "invalid interval length for major tick-marks\n");
      return;
    }

  check_autoinit;

  /* inquire current normalization transformation */

  gks_inq_current_xformno(&errind, &tnr);
  gks_inq_xform(tnr, &errind, wn, vp);

  x_min = wn[0];
  x_max = wn[1];
  y_min = wn[2];
  y_max = wn[3];

  /* save linetype, line width, line color and clipping indicator */

  gks_inq_pline_linetype(&errind, &ltype);
  gks_inq_pline_linewidth(&errind, &width);
  gks_inq_pline_color_index(&errind, &color);
  gks_inq_clip(&errind, &clsw, clrt);

  gks_set_pline_linetype(GKS_K_LINETYPE_SOLID);
  gks_set_clipping(GKS_K_NOCLIP);

  if (y_tick != 0)
    {
      if (OPTION_Y_LOG & lx.scale_options)
        {
          y0 = pow(10.0, gauss(log10(y_min)));

          i = ipred(y_min / y0);
          yi = y0 + i * y0;

          /* draw horizontal grid lines */

          while (yi <= y_max)
            {
              if (i == 0 || major_y == 1)
                {
                  major = i == 0;
                  if (yi != y_min) grid_line(x_min, yi, x_max, yi, color, major);
                }

              if (i == 9)
                {
                  y0 = y0 * 10.;
                  i = 0;
                }
              else
                i++;

              yi = y0 + i * y0;
            }
        }
      else
        {
          feps = FEPS * (y_max - y_min);

          check_tick_marks(y_min, y_max, y_tick, 'Y');

          i = isucc((y_min - y_org) / y_tick);
          yi = y_org + i * y_tick;

          /* draw horizontal grid lines */

          while (yi <= y_max + feps)
            {
              if (major_y > 0)
                major = i % major_y == 0 && major_y > 1;
              else
                major = 0;

              grid_line(x_min, yi, x_max, yi, color, major);

              i++;
              yi = y_org + i * y_tick;
            }
        }
    }

  if (x_tick != 0)
    {
      if (OPTION_X_LOG & lx.scale_options)
        {
          x0 = pow(10.0, gauss(log10(x_min)));

          i = ipred(x_min / x0);
          xi = x0 + i * x0;

          /* draw vertical grid lines */

          while (xi <= x_max)
            {
              if (i == 0 || major_x == 1)
                {
                  major = i == 0;
                  if (xi != x_min) grid_line(xi, y_min, xi, y_max, color, major);
                }

              if (i == 9)
                {
                  x0 = x0 * 10.;
                  i = 0;
                }
              else
                i++;

              xi = x0 + i * x0;
            }
        }
      else
        {
          feps = FEPS * (x_max - x_min);

          check_tick_marks(x_min, x_max, x_tick, 'X');

          i = isucc((x_min - x_org) / x_tick);
          xi = x_org + i * x_tick;

          /* draw vertical grid lines */

          while (xi <= x_max + feps)
            {
              if (major_x > 0)
                major = i % major_x == 0 && major_x > 1;
              else
                major = 0;

              grid_line(xi, y_min, xi, y_max, color, major);

              i++;
              xi = x_org + i * x_tick;
            }
        }
    }

  /* restore linetype, line width, line color and clipping indicator */

  gks_set_pline_linetype(ltype);
  gks_set_pline_linewidth(width);
  gks_set_pline_color_index(color);
  gks_set_clipping(clsw);

  if (flag_graphics)
    gr_writestream("<grid xtick=\"%g\" ytick=\"%g\" xorg=\"%g\" yorg=\"%g\" "
                   "majorx=\"%d\" majory=\"%d\"/>\n",
                   x_tick, y_tick, x_org, y_org, major_x, major_y);
}

static void grid_line3d(double x0, double y0, double z0, double x1, double y1, double z1, int color, int major)
{
  if (color != 0)
    gks_set_pline_color_index(major ? 88 : 90);
  else
    gks_set_pline_linewidth(major ? 2.0 : 1.0);

  start_pline3d(x0, y0, z0);
  pline3d(x1, y1, z1);
  end_pline();
}

/*!
 * Draw a linear and/or logarithmic grid.
 *
 * \param[in] x_tick The length in world coordinates of the interval between
 *                   minor grid lines in X direction.
 * \param[in] y_tick The length in world coordinates of the interval between
 *                   minor grid lines in Y direction.
 * \param[in] z_tick The length in world coordinates of the interval between
 *                   minor grid lines in Z direction.
 * \param[in] x_org The world coordinate of the origin (point of intersection)
 *                  of the X axis.
 * \param[in] y_org The world coordinate of the origin (point of intersection)
 *                  of the Y axis.
 * \param[in] z_org The world coordinate of the origin (point of intersection)
 *                  of the Z axis.
 * \param[in] major_x Unitless integer value specifying the number of minor
 *                    grid lines between major grid lines on the X axis. Values
 *                    of 0 or 1 imply no grid lines.
 * \param[in] major_y Unitless integer value specifying the number of minor
 *                    grid lines between major grid lines on the Y axis. Values
 *                    of 0 or 1 imply no grid lines.
 * \param[in] major_z Unitless integer value specifying the number of minor
 *                    grid lines between major grid lines on the Z axis. Values
 *                    of 0 or 1 imply no grid lines.
 *
 * Major grid lines correspond to the axes origin and major tick marks whether
 * visible or not. Minor grid lines are drawn at points equal to minor tick
 * marks. Major grid lines are drawn using black lines and minor grid lines are
 * drawn using gray lines.
 */
void gr_grid3d(double x_tick, double y_tick, double z_tick, double x_org, double y_org, double z_org, int major_x,
               int major_y, int major_z)
{
  int errind, tnr;
  int ltype, color, clsw, major;
  double width;

  double clrt[4], wn[4], vp[4];
  double x_min = 0, x_max = 0, y_min = 0, y_max = 0, z_min = 0, z_max = 0;

  double x0, y0, z0, xi, yi, zi;

  int i;

  if (x_tick < 0 || y_tick < 0 || z_tick < 0)
    {
      fprintf(stderr, "invalid interval length for major tick-marks\n");
      return;
    }

  check_autoinit;

  setscale(lx.scale_options);

  /* inquire current normalization transformation */

  gks_inq_current_xformno(&errind, &tnr);
  gks_inq_xform(tnr, &errind, wn, vp);

  if (gpx.projection_type == GR_PROJECTION_PERSPECTIVE || gpx.projection_type == GR_PROJECTION_ORTHOGRAPHIC)
    {
      gks_set_window(WC, -1, 1, -1, 1);
      setscale(lx.scale_options);

      x_min = ix.xmin;
      x_max = ix.xmax;
      y_min = ix.ymin;
      y_max = ix.ymax;

      z_min = ix.zmin;
      z_max = ix.zmax;
    }
  else
    {
      x_min = wn[0];
      x_max = wn[1];
      y_min = wn[2];
      y_max = wn[3];

      z_min = wx.zmin;
      z_max = wx.zmax;
    }

  /* save linetype, line width, line color and clipping indicator */

  gks_inq_pline_linetype(&errind, &ltype);
  gks_inq_pline_linewidth(&errind, &width);
  gks_inq_pline_color_index(&errind, &color);
  gks_inq_clip(&errind, &clsw, clrt);

  gks_set_pline_linetype(GKS_K_LINETYPE_SOLID);
  gks_set_clipping(GKS_K_NOCLIP);

  if (z_tick != 0)
    {
      if (OPTION_Y_LOG & lx.scale_options)
        {
          z0 = pow(10.0, gauss(log10(z_min)));

          i = ipred(z_min / z0);
          zi = z0 + i * z0;

          /* draw horizontal grid lines */

          while (zi <= z_max)
            {
              if (i == 0 || major_z == 1)
                {
                  major = i == 0;
                  if (fabs(zi - z_min) > FEPS * zi)
                    {
                      grid_line3d(x_org, y_min, zi, x_org, y_max, zi, color, major);
                      grid_line3d(x_min, y_org, zi, x_max, y_org, zi, color, major);
                    }
                }

              if (i == 9)
                {
                  z0 = z0 * 10.;
                  i = 0;
                }
              else
                i++;

              zi = z0 + i * z0;
            }
        }
      else
        {
          check_tick_marks(z_min, z_max, z_tick, 'Z');

          i = 0;
          z0 = succ(z_min / z_tick) * z_tick;
          zi = z0;

          /* draw horizontal grid lines */

          while (zi <= z_max)
            {
              if (major_z > 0)
                major = i % major_z == 0 && major_z > 1;
              else
                major = 0;

              if (fabs(zi - z_min) > FEPS * zi)
                {
                  grid_line3d(x_org, y_min, zi, x_org, y_max, zi, color, major);
                  grid_line3d(x_min, y_org, zi, x_max, y_org, zi, color, major);
                }

              i++;
              zi = z0 + i * z_tick;
            }
        }
    }

  if (y_tick != 0)
    {
      if (OPTION_Y_LOG & lx.scale_options)
        {
          y0 = pow(10.0, gauss(log10(y_min)));

          i = ipred(y_min / y0);
          yi = y0 + i * y0;

          /* draw horizontal grid lines */

          while (yi <= y_max)
            {
              if (i == 0 || major_y == 1)
                {
                  major = i == 0;
                  if (fabs(yi - y_min) > FEPS * yi)
                    {
                      grid_line3d(x_min, yi, z_org, x_max, yi, z_org, color, major);
                      grid_line3d(x_org, yi, z_min, x_org, yi, z_max, color, major);
                    }
                }

              if (i == 9)
                {
                  y0 = y0 * 10.;
                  i = 0;
                }
              else
                i++;

              yi = y0 + i * y0;
            }
        }
      else
        {
          check_tick_marks(y_min, y_max, y_tick, 'Y');

          i = 0;
          y0 = succ(y_min / y_tick) * y_tick;
          yi = y0;

          /* draw horizontal grid lines */

          while (yi <= y_max)
            {
              if (major_y > 0)
                major = i % major_y == 0 && major_y > 1;
              else
                major = 0;

              if (fabs(yi - y_min) > FEPS * yi)
                {
                  grid_line3d(x_min, yi, z_org, x_max, yi, z_org, color, major);
                  grid_line3d(x_org, yi, z_min, x_org, yi, z_max, color, major);
                }

              i++;
              yi = y0 + i * y_tick;
            }
        }
    }

  if (x_tick != 0)
    {
      if (OPTION_X_LOG & lx.scale_options)
        {
          x0 = pow(10.0, gauss(log10(x_min)));

          i = ipred(x_min / x0);
          xi = x0 + i * x0;

          /* draw vertical grid lines */

          while (xi <= x_max)
            {
              if (i == 0 || major_x == 1)
                {
                  major = i == 0;
                  if (fabs(xi - x_min) > FEPS * xi)
                    {
                      grid_line3d(xi, y_min, z_org, xi, y_max, z_org, color, major);
                      grid_line3d(xi, y_org, z_min, xi, y_org, z_max, color, major);
                    }
                }

              if (i == 9)
                {
                  x0 = x0 * 10.;
                  i = 0;
                }
              else
                i++;

              xi = x0 + i * x0;
            }
        }
      else
        {
          check_tick_marks(x_min, x_max, x_tick, 'X');

          i = 0;
          x0 = succ(x_min / x_tick) * x_tick;
          xi = x0;

          /* draw vertical grid lines */

          while (xi <= x_max)
            {
              if (major_x > 0)
                major = i % major_x == 0 && major_x > 1;
              else
                major = 0;

              if (fabs(xi - x_min) > FEPS * xi)
                {
                  grid_line3d(xi, y_min, z_org, xi, y_max, z_org, color, major);
                  grid_line3d(xi, y_org, z_min, xi, y_org, z_max, color, major);
                }

              i++;
              xi = x0 + i * x_tick;
            }
        }
    }

  /* restore linetype, line width, line color and clipping indicator */

  gks_set_pline_linetype(ltype);
  gks_set_pline_linewidth(width);
  gks_set_pline_color_index(color);
  gks_set_clipping(clsw);

  if (flag_graphics)
    gr_writestream("<grid3d xtick=\"%g\" ytick=\"%g\" ztick=\"%g\" "
                   "xorg=\"%g\" yorg=\"%g\" zorg=\"%g\" "
                   "majorx=\"%d\" majory=\"%d\" majorz=\"%d\"/>\n",
                   x_tick, y_tick, z_tick, x_org, y_org, z_org, major_x, major_y, major_z);

  if (gpx.projection_type == GR_PROJECTION_PERSPECTIVE || gpx.projection_type == GR_PROJECTION_ORTHOGRAPHIC)
    {
      gks_set_window(WC, wn[0], wn[1], wn[2], wn[3]);
      setscale(lx.scale_options);
    }
}

/*!
 * Draw a standard vertical error bar graph.
 *
 * \param[in] n The number of points
 * \param[in] px A pointer to the X coordinates
 * \param[in] py A pointer to the Y coordinates
 * \param[in] e1 A pointer to the absolute values of the lower error bar data
 * \param[in] e2 A pointer to the absolute values of the upper error bar data
 */
void gr_verrorbars(int n, double *px, double *py, double *e1, double *e2)
{
  int errind, i;
  double tick, x, x1, x2, y1, y2, marker_size;

  if (n < 1)
    {
      fprintf(stderr, "invalid number of points\n");
      return;
    }

  check_autoinit;

  gks_inq_pmark_size(&errind, &marker_size);

  for (i = 0; i < n; i++)
    {
      tick = marker_size * 0.0075 * (lx.xmax - lx.xmin);

      x = px[i];
      x1 = x_log(x_lin(x) - tick);
      x2 = x_log(x_lin(x) + tick);
      y1 = e1[i];
      y2 = e2[i];

      start_pline(x1, y1);
      pline(x2, y1);
      end_pline();

      start_pline(x, y1);
      pline(x, y2);
      end_pline();

      start_pline(x1, y2);
      pline(x2, y2);
      end_pline();
    }

  polymarker(n, px, py);

  if (flag_graphics)
    {
      gr_writestream("<verrorbars len=\"%d\"", n);
      print_float_array("x", n, px);
      print_float_array("y", n, py);
      print_float_array("e1", n, e1);
      print_float_array("e2", n, e2);
      gr_writestream("/>\n");
    }
}

/*!
 * Draw a standard horizontal error bar graph.
 *
 * \param[in] n The number of points
 * \param[in] px A pointer to the X coordinates
 * \param[in] py A pointer to the Y coordinates
 * \param[in] e1 A pointer to the absolute values of the lower error bar data
 * \param[in] e2 A pointer to the absolute values of the upper error bar data
 */
void gr_herrorbars(int n, double *px, double *py, double *e1, double *e2)
{
  int errind, i;
  double tick, y, x1, x2, y1, y2, marker_size;

  if (n < 1)
    {
      fprintf(stderr, "invalid number of points\n");
      return;
    }

  check_autoinit;

  gks_inq_pmark_size(&errind, &marker_size);

  for (i = 0; i < n; i++)
    {
      tick = marker_size * 0.0075 * (lx.ymax - lx.ymin);

      y = py[i];
      y1 = y_log(y_lin(y) - tick);
      y2 = y_log(y_lin(y) + tick);
      x1 = e1[i];
      x2 = e2[i];

      start_pline(x1, y1);
      pline(x1, y2);
      end_pline();

      start_pline(x1, y);
      pline(x2, y);
      end_pline();

      start_pline(x2, y1);
      pline(x2, y2);
      end_pline();
    }

  polymarker(n, px, py);

  if (flag_graphics)
    {
      gr_writestream("<herrorbars len=\"%d\"", n);
      print_float_array("x", n, px);
      print_float_array("y", n, py);
      print_float_array("e1", n, e1);
      print_float_array("e2", n, e2);
      gr_writestream("/>\n");
    }
}

static void clip_code(double x, double y, double z, int *c)
{
  *c = 0;
  if (x < cxl)
    *c = LEFT;
  else if (x > cxr)
    *c = RIGHT;
  if (y < cyf)
    *c = *c | FRONT;
  else if (y > cyb)
    *c = *c | BACK;
  if (z < czb)
    *c = *c | BOTTOM;
  else if (z > czt)
    *c = *c | TOP;
}

static void clip3d(double *x0, double *x1, double *y0, double *y1, double *z0, double *z1, int *visible)
{
  int c, c0, c1;
  double x = 0, y = 0, z = 0;

  clip_code(*x0, *y0, *z0, &c0);
  clip_code(*x1, *y1, *z1, &c1);

  *visible = 0;

  while (c0 | c1)
    {
      if (c0 & c1) return;
      c = c0 ? c0 : c1;

      if (c & LEFT)
        {
          x = cxl;
          y = *y0 + (*y1 - *y0) * (cxl - *x0) / (*x1 - *x0);
          z = *z0 + (*z1 - *z0) * (cxl - *x0) / (*x1 - *x0);
        }
      else if (c & RIGHT)
        {
          x = cxr;
          y = *y0 + (*y1 - *y0) * (cxr - *x0) / (*x1 - *x0);
          z = *z0 + (*z1 - *z0) * (cxr - *x0) / (*x1 - *x0);
        }
      else if (c & FRONT)
        {
          x = *x0 + (*x1 - *x0) * (cyf - *y0) / (*y1 - *y0);
          y = cyf;
          z = *z0 + (*z1 - *z0) * (cyf - *y0) / (*y1 - *y0);
        }
      else if (c & BACK)
        {
          x = *x0 + (*x1 - *x0) * (cyb - *y0) / (*y1 - *y0);
          y = cyb;
          z = *z0 + (*z1 - *z0) * (cyb - *y0) / (*y1 - *y0);
        }
      else if (c & BOTTOM)
        {
          x = *x0 + (*x1 - *x0) * (czb - *z0) / (*z1 - *z0);
          y = *y0 + (*y1 - *y0) * (czb - *z0) / (*z1 - *z0);
          z = czb;
        }
      else if (c & TOP)
        {
          x = *x0 + (*x1 - *x0) * (czt - *z0) / (*z1 - *z0);
          y = *y0 + (*y1 - *y0) * (czt - *z0) / (*z1 - *z0);
          z = czt;
        }
      if (c == c0)
        {
          *x0 = x;
          *y0 = y;
          *z0 = z;
          clip_code(x, y, z, &c0);
        }
      else
        {
          *x1 = x;
          *y1 = y;
          *z1 = z;
          clip_code(x, y, z, &c1);
        }
    }

  /* if we reach here, the line from (x0, y0, z0) to (x1, y1, z1) is visible */

  *visible = 1;
}

/*!
 * Draw a 3D curve using the current line attributes, starting from the
 * first data point and ending at the last data point.
 *
 * \param[in] n The number of points
 * \param[in] px A pointer to the X coordinates
 * \param[in] py A pointer to the Y coordinates
 * \param[in] pz A pointer to the Z coordinates
 *
 * The values for x, y and z are in world coordinates. The attributes that
 * control the appearance of a polyline are linetype, linewidth and color
 * index.
 */
void gr_polyline3d(int n, double *px, double *py, double *pz)
{
  int errind, clsw, i, tnr;
  double clrt[4], wn[4], vp[4];

  double x, y, z, x0, y0, z0, x1, y1, z1;
  int clip = 1, visible = 1;

  check_autoinit;

  setscale(lx.scale_options);

  gks_inq_current_xformno(&errind, &tnr);
  gks_inq_xform(tnr, &errind, wn, vp);
  gks_inq_clip(&errind, &clsw, clrt);

  if (clsw == GKS_K_CLIP)
    {
      if (gpx.projection_type == GR_PROJECTION_ORTHOGRAPHIC || gpx.projection_type == GR_PROJECTION_PERSPECTIVE)
        {
          gks_set_window(WC, -1, 1, -1, 1);
          setscale(lx.scale_options);

          cxl = ix.xmin;
          cxr = ix.xmax;
          cyf = ix.ymin;
          cyb = ix.ymax;
          czb = ix.zmin;
          czt = ix.zmax;
        }
      else
        {
          cxl = lx.xmin;
          cxr = lx.xmax;
          cyf = lx.ymin;
          cyb = lx.ymax;
          czb = lx.zmin;
          czt = lx.zmax;
        }
    }
  else
    {
      visible = 1;
    }

  x0 = px[0];
  y0 = py[0];
  z0 = pz[0];

  for (i = 1; i < n; i++)
    {
      x1 = px[i];
      y1 = py[i];
      z1 = pz[i];
      if (is_nan(x1) || is_nan(y1) || is_nan(z1)) break;

      x = x1;
      y = y1;
      z = z1;
      if (clsw == GKS_K_CLIP)
        {
          clip3d(&x0, &x1, &y0, &y1, &z0, &z1, &visible);
        }
      visible = 1;
      if (visible)
        {
          if (clip)
            {
              start_pline3d(x0, y0, z0);
              clip = 0;
            }
          pline3d(x1, y1, z1);
        }

      clip = !visible || x != x1 || y != y1 || z != z1;
      x0 = x;
      y0 = y;
      z0 = z;
    }

  end_pline();

  if (flag_graphics)
    {
      gr_writestream("<polyline3d len=\"%d\"", n);
      print_float_array("x", n, px);
      print_float_array("y", n, py);
      print_float_array("z", n, pz);
      gr_writestream("/>\n");
    }

  if (gpx.projection_type == GR_PROJECTION_PERSPECTIVE || gpx.projection_type == GR_PROJECTION_ORTHOGRAPHIC)
    {
      gks_set_window(WC, wn[0], wn[1], wn[2], wn[3]);
      setscale(lx.scale_options);
    }
}

static int cmp(const void *a, const void *b)
{
  const point_3d *pa = (const point_3d *)a;
  const point_3d *pb = (const point_3d *)b;
  double x, y, da, db;

  x = (OPTION_FLIP_X & lx.scale_options) ? lx.xmin : lx.xmax;
  y = (OPTION_FLIP_Y & lx.scale_options) ? lx.ymin : lx.ymax;

  da = sqrt(pow(x - pa->x, 2) + pow(y - pa->y, 2));
  db = sqrt(pow(x - pb->x, 2) + pow(y - pb->y, 2));

  return db - da;
}

/*!
 * Draw marker symbols centered at the given 3D data points.
 *
 * \param[in] n The number of points
 * \param[in] px A pointer to the X coordinates
 * \param[in] py A pointer to the Y coordinates
 * \param[in] pz A pointer to the Z coordinates
 *
 * The values for x, y and z are in world coordinates. The attributes
 * that control the appearance of a polymarker are marker type, marker size
 * scale factor and color index.
 */
void gr_polymarker3d(int n, double *px, double *py, double *pz)
{
  int errind, clsw, i, tnr;
  double clrt[4], wn[4], vp[4];

  double x, y, z;
  point_3d *point;
  int m, visible;

  check_autoinit;

  setscale(lx.scale_options);

  gks_inq_current_xformno(&errind, &tnr);
  gks_inq_xform(tnr, &errind, wn, vp);
  gks_inq_clip(&errind, &clsw, clrt);

  m = 0;
  point = (point_3d *)xmalloc(n * sizeof(point_3d));

  for (i = 0; i < n; i++)
    {
      x = px[i];
      y = py[i];
      z = pz[i];

      if (clsw == GKS_K_CLIP)
        {
          if (gpx.projection_type == GR_PROJECTION_DEFAULT)
            {
              visible = x >= lx.xmin && x <= lx.xmax && y >= lx.ymin && y <= lx.ymax && z >= lx.zmin && z <= lx.zmax;
            }
          else
            {
              gks_set_window(WC, -1, 1, -1, 1);
              setscale(lx.scale_options);

              visible = x >= ix.xmin && x <= ix.xmax && y >= ix.ymin && y <= ix.ymax && z >= ix.zmin && z <= ix.zmax;
            }
        }
      else
        {
          visible = 1;
        }
      if (visible)
        {
          x = x_lin(x);
          y = y_lin(y);
          z = z_lin(z);

          apply_world_xform(&x, &y, &z);

          point[m].x = x;
          point[m].y = y;
          point[m].z = z;
          m++;
        }
    }

  qsort(point, m, sizeof(point_3d), cmp);

  if (m >= maxpath) reallocate(m);

  for (i = 0; i < m; i++)
    {
      xpoint[i] = point[i].x;
      ypoint[i] = point[i].y;
      zpoint[i] = point[i].z;
    }

  if (m > 0) gr_polymarker(m, xpoint, ypoint);

  if (flag_graphics)
    {
      gr_writestream("<polymarker3d len=\"%d\"", n);
      print_float_array("x", n, px);
      print_float_array("y", n, py);
      print_float_array("z", n, pz);
      gr_writestream("/>\n");
    }

  if (gpx.projection_type == GR_PROJECTION_PERSPECTIVE || gpx.projection_type == GR_PROJECTION_ORTHOGRAPHIC)
    {
      gks_set_window(WC, wn[0], wn[1], wn[2], wn[3]);
      setscale(lx.scale_options);
    }
}

static double text3d_get_height()
{
  double focus_point_x, focus_point_y, focus_point_z, focus_up_x, focus_up_y, focus_up_z;
  /* Calculate char height */
  focus_point_x = tx.focus_point_x / tx.x_axis_scale;
  focus_point_y = tx.focus_point_y / tx.y_axis_scale;
  focus_point_z = tx.focus_point_z / tx.z_axis_scale;
  focus_up_x = focus_point_x + tx.up_x / tx.x_axis_scale;
  focus_up_y = focus_point_y + tx.up_y / tx.y_axis_scale;
  focus_up_z = focus_point_z + tx.up_z / tx.z_axis_scale;

  gr_wc3towc(&focus_point_x, &focus_point_y, &focus_point_z);
  gr_wc3towc(&focus_up_x, &focus_up_y, &focus_up_z);

  gr_wctondc(&focus_point_x, &focus_point_y);
  gr_wctondc(&focus_up_x, &focus_up_y);
  return sqrt(pow(focus_point_x - focus_up_x, 2) + pow(focus_point_y - focus_up_y, 2)) /
         (min((vxmax - vxmin), (vymax - vymin)));
}

static void text3d(double x, double y, double z, char *chars, int axis)
{
  double p_x, p_y, p_z;
  int errind, tnr;
  double scaleFactors[3];

  check_autoinit;

  p_x = x_lin(x);
  p_y = y_lin(y);
  p_z = z_lin(z);

  if (axis == 0)
    {
      apply_world_xform(&p_x, &p_y, &p_z);

      gks_inq_current_xformno(&errind, &tnr);
      if (tnr != NDC)
        {
          p_x = nx.a * p_x + nx.b;
          p_y = nx.c * p_y + nx.d;
          gks_select_xform(NDC);
        }

      gr_textex(p_x, p_y, chars, 0, NULL, NULL);

      if (tnr != NDC) gks_select_xform(tnr);
    }
  else
    {
      scaleFactors[0] = tx.x_axis_scale;
      scaleFactors[1] = tx.y_axis_scale;
      scaleFactors[2] = tx.z_axis_scale;
      gks_ft_text3d(p_x, p_y, p_z, chars, axis, gks_state(), text3d_get_height(), scaleFactors, gks_ft_gdp, gr_wc3towc);
    }
}

void gr_text3d(double x, double y, double z, char *chars, int axis)
{
  double scaleFactors[3];

  check_autoinit;

  scaleFactors[0] = tx.x_axis_scale;
  scaleFactors[1] = tx.y_axis_scale;
  scaleFactors[2] = tx.z_axis_scale;
  gks_ft_text3d(x, y, z, chars, axis, gks_state(), text3d_get_height(), scaleFactors, gks_ft_gdp, gr_wc3towc);

  if (flag_graphics)
    gr_writestream("<text3d x=\"%g\" y=\"%g\" z=\"%g\" text=\"%s\" axis=\"%d\"/>\n", x, y, z, chars, axis);
}

/*!
 * Allows you to get the 2d coordinates and transformed coordinates of a text as if displayed by gr_text3d
 *
 * \param[in] x The base X coordinate
 * \param[in] y The base Y coordinate
 * \param[in] z The base z coordinate
 * \param[in] chars The string to draw
 * \param[in] axis In which direction the text is drawn (1: YX-plane, 2: XY plane, 3: YZ plane, 4: XZ plane)
 * \param[in] tbx A double array of 16 elements to write x-coordinates to
 * \param[in] tby A double array of 16 elements to write y-coordinates to
 *
 * The first 8 coordinates are pre-transformation coordinates, while the last 8 coordinates are transformed coordinates
 * The first 4 coordinates each are the corners of the bounding box, including ascender and descender space, while the
 * last 4 coordinates are without ascenders and descenders.
 */
void gr_inqtext3d(double x, double y, double z, char *chars, int axis, double *tbx, double *tby)
{
  double scaleFactors[3];

  check_autoinit;

  scaleFactors[0] = tx.x_axis_scale;
  scaleFactors[1] = tx.y_axis_scale;
  scaleFactors[2] = tx.z_axis_scale;
  gks_ft_inq_text3d_extent(x, y, z, chars, axis, gks_state(), text3d_get_height(), scaleFactors, gks_ft_gdp, gr_wc3towc,
                           tbx, tby);
}

static void axes3d_get_params(int axis, int *tick_axis, double x_org, double y_org, double z_org, int *text_axis)
{
  /* This function calculates (and sets) the axis of ticks, upvec, valign and axis of text for gr_axes3d.
   * axis: when the axis currently drawn is x = 0, y = 1 or z = 2
   * tick_axis: the preferable axis to draw ticks on
   * x_org, y_org, z_org: the origin of the coordinate system. Used to determine where to rotate text.
   * text_axis: same as tick_axis
   */
  int plane, direction, rotate_text, flip_text;
  double fx, fy, fz, xi, yi, zi;
  double x_min, x_max, y_min, y_max, z_min, z_max;
  double cam_x, cam_y, cam_z, up_x, up_y, up_z, nor_x, nor_y, nor_z, angle;
  double bBoxX[16], bBoxY[16];
  double bBoxSpace, bBoxSpace2;

  int axes[3] = {/* plane (xy, xz, yz) */
                 2, 4, 3};
  int upvecs[4][2] = {
      {0, 1},  /* x pos or y pos if plane = xz */
      {-1, 0}, /* y pos if plane = xy or z pos */
      {0, -1}, /* x neg or y neg if plane = xz */
      {1, 0}   /* y neg if plane = xy or z neg */
  };
  if (axis < 0 || axis > 2)
    {
      fprintf(stderr, "Axis should be between 0 and 2\n");
      return;
    }
  /* Reset options for consistent gr_inqtext3d values */
  gks_set_text_upvec(0, 1);
  gks_set_text_align(GKS_K_TEXT_HALIGN_LEFT, GKS_K_TEXT_VALIGN_HALF);
  gr_inqwindow3d(&x_min, &x_max, &y_min, &y_max, &z_min, &z_max);
  fx = tx.camera_pos_x - tx.focus_point_x;
  fy = tx.camera_pos_y - tx.focus_point_y;
  fz = tx.camera_pos_z - tx.focus_point_z;
  cam_x = tx.camera_pos_x / tx.x_axis_scale;
  cam_y = tx.camera_pos_y / tx.y_axis_scale;
  cam_z = tx.camera_pos_z / tx.z_axis_scale;
  xi = (x_max + x_min) / 2;
  yi = (y_max + y_min) / 2;
  zi = (z_max + z_min) / 2;

  if (axis == 0)
    {
      gr_inqtext3d(xi, y_org, z_org, "A", axes[1], bBoxX, bBoxY);
      bBoxSpace =
          fabs((bBoxX[10] - bBoxX[8]) * (bBoxY[11] - bBoxY[9]) + (bBoxX[11] - bBoxX[9]) * (bBoxY[8] - bBoxY[10]));

      gr_inqtext3d(xi, y_org, z_org, "A", axes[0], bBoxX, bBoxY);
      bBoxSpace2 =
          fabs((bBoxX[10] - bBoxX[8]) * (bBoxY[11] - bBoxY[9]) + (bBoxX[11] - bBoxX[9]) * (bBoxY[8] - bBoxY[10]));

      plane = bBoxSpace2 > bBoxSpace ? 0 : 1; /* 0: xy-plane, 1: xz-plane */
      *tick_axis = plane == 0 ? 1 : 2;
      direction = (!plane && fz <= -fabs(fy)) || (plane && fy > fabs(fz)); /* 1: positive, 0: negative */

      if (plane == 1)
        {
          direction = (zi < z_org); /* align the text and ticks to the 'outside' */
        }
      else
        {
          direction = (yi < y_org);
        }
      rotate_text = (fx < 0) ^ !direction;
      direction = 1 + 2 * !direction; /* always in y or z-direction */
    }
  else if (axis == 1)
    {
      gr_inqtext3d(x_org, yi, z_org, "A", axes[2], bBoxX, bBoxY);
      bBoxSpace =
          fabs((bBoxX[10] - bBoxX[8]) * (bBoxY[11] - bBoxY[9]) + (bBoxX[11] - bBoxX[9]) * (bBoxY[8] - bBoxY[10]));

      gr_inqtext3d(x_org, yi, z_org, "A", axes[0], bBoxX, bBoxY);
      bBoxSpace2 =
          fabs((bBoxX[10] - bBoxX[8]) * (bBoxY[11] - bBoxY[9]) + (bBoxX[11] - bBoxX[9]) * (bBoxY[8] - bBoxY[10]));

      plane = bBoxSpace2 > bBoxSpace ? 0 : 2; /* 2: yz-plane, 0: xy-plane */
      *tick_axis = plane == 0 ? 0 : 2;
      direction = (2 == plane && fx <= -fabs(fz)) || (!plane && fz > fabs(fx)); /* 1: positive, 0: negative */
      if (plane == 0)
        {
          direction = (xi < x_org); /* align the text and ticks to the 'outside' */
          rotate_text = (fy < 0) ^ direction;
        }
      else
        {
          direction = (zi < z_org);
          rotate_text = (fy < 0) ^ !direction;
        }
      direction = (plane == 2) + 2 * !direction; /* always in x or z-direction */
    }
  else /* z-axis */
    {
      gr_inqtext3d(x_org, y_org, zi, "A", axes[2], bBoxX, bBoxY);
      bBoxSpace =
          fabs((bBoxX[10] - bBoxX[8]) * (bBoxY[11] - bBoxY[9]) + (bBoxX[11] - bBoxX[9]) * (bBoxY[8] - bBoxY[10]));

      gr_inqtext3d(x_org, y_org, zi, "A", axes[1], bBoxX, bBoxY);
      bBoxSpace2 =
          fabs((bBoxX[10] - bBoxX[8]) * (bBoxY[11] - bBoxY[9]) + (bBoxX[11] - bBoxX[9]) * (bBoxY[8] - bBoxY[10]));

      plane = bBoxSpace2 > bBoxSpace ? 1 : 2; /* 1: xz-plane, 2: yz-plane */
      *tick_axis = plane == 1 ? 0 : 1;
      direction = (1 == plane && fy <= -fabs(fx)) || (2 == plane && fx > fabs(fy)); /* 1: positive, 0: negative */

      if (plane == 1)
        {
          direction = (xi < x_org); /* align the text and ticks to the 'outside' */
        }
      else
        {
          direction = (yi < y_org);
        }
      rotate_text = (tx.up_z > 0) ^ direction;

      direction = 2 * !direction; /* always in x or y-direction */
    }
  /* now: direction is the direction (0 for negative, 1 for positive) of either the ticks, or if the ticks are on the */
  /* other plane, the direction facing outwards. */
  if (rotate_text)
    {
      direction = (direction + 2) % 4; /* rotate text by 180 degrees */
    }
  *text_axis = axes[plane];
  if (gpx.projection_type == GR_PROJECTION_ORTHOGRAPHIC)
    {
      if (plane == 0)
        {
          cam_x = upvecs[direction][1] * tx.x_axis_scale;
          cam_y = -upvecs[direction][0] * tx.y_axis_scale;
          cam_z = 0;
          up_x = upvecs[direction][0] * tx.x_axis_scale;
          up_y = upvecs[direction][1] * tx.y_axis_scale;
          up_z = 0;
        }
      else if (plane == 1)
        {
          cam_x = upvecs[direction][1] * tx.x_axis_scale;
          cam_y = 0;
          cam_z = -upvecs[direction][0] * tx.z_axis_scale;
          up_x = upvecs[direction][0] * tx.x_axis_scale;
          up_y = 0;
          up_z = upvecs[direction][1] * tx.z_axis_scale;
        }
      else
        {
          cam_x = 0;
          cam_y = upvecs[direction][1] * tx.y_axis_scale;
          cam_z = -upvecs[direction][0] * tx.z_axis_scale;
          up_x = 0;
          up_y = upvecs[direction][0] * tx.y_axis_scale;
          up_z = upvecs[direction][1] * tx.z_axis_scale;
        }

      nor_x = cam_y * up_z - cam_z * up_y;
      nor_y = cam_z * up_x - cam_x * up_z;
      nor_z = cam_x * up_y - cam_y * up_x;

      angle = nor_x * fx + nor_y * fy + nor_z * fz;
      flip_text = angle < 0;
    }
  else
    {
      /* if the camera is on the opposite side of the text, flip/mirror it. */
      flip_text = (plane == 0 && cam_z < z_org) || (plane == 1 && cam_y > y_org) || (plane == 2 && cam_x < x_org);
    }

  if (rotate_text ^ flip_text)
    {
      gks_set_text_align(GKS_K_TEXT_HALIGN_RIGHT, GKS_K_TEXT_VALIGN_HALF);
    }
  else
    {
      gks_set_text_align(GKS_K_TEXT_HALIGN_LEFT, GKS_K_TEXT_VALIGN_HALF);
    }
  if (flip_text)
    {
      *text_axis *= -1;
    }

  gks_set_text_upvec(upvecs[direction][0], upvecs[direction][1]);
}

/*!
 * Draw X, Y and Z coordinate axes with linearly and/or logarithmically
 * spaced tick marks.
 *
 * \param[in] x_tick The length in world coordinates of the interval between
 *                   minor grid lines in X direction.
 * \param[in] y_tick The length in world coordinates of the interval between
 *                   minor grid lines in Y direction.
 * \param[in] z_tick The length in world coordinates of the interval between
 *                   minor grid lines in Z direction.
 * \param[in] x_org The world coordinate of the origin (point of intersection)
 *                  of the X axis.
 * \param[in] y_org The world coordinate of the origin (point of intersection)
 *                  of the Y axis.
 * \param[in] z_org The world coordinate of the origin (point of intersection)
 *                  of the Z axis.
 * \param[in] major_x Unitless integer value specifying the number of minor
 *                    grid lines between major grid lines on the X axis. Values
 *                    of 0 or 1 imply no grid lines.
 * \param[in] major_y Unitless integer value specifying the number of minor
 *                    grid lines between major grid lines on the Y axis. Values
 *                    of 0 or 1 imply no grid lines.
 * \param[in] major_z Unitless integer value specifying the number of minor
 *                    grid lines between major grid lines on the Z axis. Values
 *                    of 0 or 1 imply no grid lines.
 * \param[in] tick_size The length of minor tick marks specified in a normalized
 *                      device coordinate unit. Major tick marks are twice as
 *                      long as minor tick marks. A negative value reverses the
 *                      tick marks on the axes from inward facing to outward
 *                      facing (or vice versa).
 *
 * Tick marks are positioned along each axis so that major tick marks fall on
 * the axes origin (whether visible or not). Major tick marks are labeled with
 * the corresponding data values. Axes are drawn according to the scale of the
 * window. Axes and tick marks are drawn using solid lines; line color and width
 * can be modified using the gr_setlinetype and gr_setlinewidth functions. Axes
 * are drawn according to the linear or logarithmic transformation established
 * by the gr_setscale function.
 */
void gr_axes3d(double x_tick, double y_tick, double z_tick, double x_org, double y_org, double z_org, int major_x,
               int major_y, int major_z, double tick_size)
{
  int errind, tnr;
  int ltype, halign, valign, font, prec, clsw, axis, tick_axis;
  double chux, chuy, slant;

  double clrt[4], wn[4], vp[4];
  double x_min = 0, x_max = 0, y_min = 0, y_max = 0, z_min = 0, z_max = 0;

  double r, alpha, beta;
  double a[2], c[2], text_slant[4];
  int *anglep, which_rep, rep;

  double tick;
  double x_minor_tick, x_major_tick, x_label;
  double y_minor_tick, y_major_tick, y_label;
  double z_minor_tick, z_major_tick, z_label;
  double x0, y0, z0, xi, yi, zi;
  int64_t i;
  int decade, exponent;
  char string[256];
  int modern_projection_type;

  if (x_tick < 0 || y_tick < 0 || z_tick < 0)
    {
      fprintf(stderr, "invalid interval length for major tick-marks\n");
      return;
    }
  else if (tick_size == 0)
    {
      fprintf(stderr, "invalid tick-size\n");
      return;
    }

  check_autoinit;

  setscale(lx.scale_options);

  /* inquire current normalization transformation */

  gks_inq_current_xformno(&errind, &tnr);
  gks_inq_xform(tnr, &errind, wn, vp);

  modern_projection_type =
      gpx.projection_type == GR_PROJECTION_PERSPECTIVE || gpx.projection_type == GR_PROJECTION_ORTHOGRAPHIC;

  if (modern_projection_type)
    {
      gks_set_window(WC, -1, 1, -1, 1);
      setscale(lx.scale_options);

      x_min = ix.xmin;
      x_max = ix.xmax;
      y_min = ix.ymin;
      y_max = ix.ymax;

      z_min = ix.zmin;
      z_max = ix.zmax;
    }
  else
    {
      x_min = wn[0];
      x_max = wn[1];
      y_min = wn[2];
      y_max = wn[3];

      z_min = wx.zmin;
      z_max = wx.zmax;
    }

  if (x_min > x_org || x_org > x_max || y_min > y_org || y_org > y_max || z_min > z_org || z_org > z_max)
    {
      fprintf(stderr, "origin outside current window\n");
      return;
    }

  r = (x_max - x_min) / (y_max - y_min) * (vp[3] - vp[2]) / (vp[1] - vp[0]);

  alpha = atan_2(r * wx.c1, wx.a1);
  a[0] = -sin(alpha);
  c[0] = cos(alpha);
  alpha = deg(alpha);

  beta = atan_2(r * wx.c2, wx.a2);
  a[1] = -sin(beta);
  c[1] = cos(beta);
  beta = deg(beta);

  text_slant[0] = alpha;
  text_slant[1] = beta;
  text_slant[2] = -(90.0 + alpha - beta);
  text_slant[3] = 90.0 + alpha - beta;

  /* save linetype, text alignment, text font, text slant,
     character-up vector and clipping indicator */

  gks_inq_pline_linetype(&errind, &ltype);
  gks_inq_text_align(&errind, &halign, &valign);
  gks_inq_text_fontprec(&errind, &font, &prec);
  gks_inq_text_slant(&errind, &slant);
  gks_inq_text_upvec(&errind, &chux, &chuy);
  gks_inq_clip(&errind, &clsw, clrt);

  gks_set_pline_linetype(GKS_K_LINETYPE_SOLID);
  gks_set_text_fontprec(font, GKS_K_TEXT_PRECISION_STROKE);
  gks_set_clipping(GKS_K_NOCLIP);

  which_rep = 0;
  anglep = angle;
  while (wx.delta > *anglep++) which_rep++;
  anglep = angle;
  while (wx.phi > *anglep++) which_rep += 4;
  if (modern_projection_type)
    tick_size /= text3d_get_height(); /* Adjust tick_size to match text units / scale properly with viewports */
  if (z_tick != 0)
    {
      if (modern_projection_type)
        tick = tick_size / tx.y_axis_scale;
      else
        tick = tick_size * (y_max - y_min) / (vp[3] - vp[2]);

      y_minor_tick = y_log(y_lin(y_org) + tick);
      y_major_tick = y_log(y_lin(y_org) + 2. * tick);
      y_label = y_log(y_lin(y_org) + 3. * tick);

      /* set text alignment */

      if (y_lin(y_org) <= (y_lin(y_min) + y_lin(y_max)) / 2.)
        {
          gks_set_text_align(GKS_K_TEXT_HALIGN_RIGHT, GKS_K_TEXT_VALIGN_HALF);
          if (tick > 0) y_label = y_log(y_lin(y_org) - tick);
        }
      else
        {
          gks_set_text_align(GKS_K_TEXT_HALIGN_LEFT, GKS_K_TEXT_VALIGN_HALF);
          if (tick < 0) y_label = y_log(y_lin(y_org) - tick);
        }


      if (!modern_projection_type)
        {
          rep = rep_table[which_rep][2];

          gks_set_text_upvec(a[axes_rep[rep][0]], c[axes_rep[rep][1]]);
          gks_set_text_slant(text_slant[axes_rep[rep][2]]);
          tick_axis = 1;
        }
      else
        {
          tick = tick_size / tx.x_axis_scale;

          x_minor_tick = x_log(x_lin(x_org) + tick);
          x_major_tick = x_log(x_lin(x_org) + 2. * tick);
          x_label = x_log(x_lin(x_org) + 3. * tick);

          if (x_lin(x_org) <= (x_lin(x_min) + x_lin(x_max)) / 2.)
            {
              if (tick > 0) x_label = x_log(x_lin(x_org) - tick);
            }
          else
            {
              if (tick < 0) x_label = x_log(x_lin(x_org) - tick);
            }
          axes3d_get_params(2, &tick_axis, x_org, y_org, z_org, &axis);
        }

      if (tick_axis != 1)
        {
          y_label = y_major_tick = y_minor_tick = y_org;
        }
      if (tick_axis != 0)
        {
          x_label = x_major_tick = x_minor_tick = x_org;
        }

      if (OPTION_Z_LOG & lx.scale_options)
        {
          z0 = pow(10.0, gauss(log10(z_min)));

          i = ipred(z_min / z0);
          zi = z0 + i * z0;
          decade = igauss(log10(z_min / z_org));

          /* draw Z-axis */

          start_pline3d(x_org, y_org, z_min);

          while (zi <= z_max)
            {
              pline3d(x_org, y_org, zi);

              if (i == 0)
                {
                  xi = x_major_tick;
                  yi = y_major_tick;
                  if (major_z > 0 && zi != z_org)
                    if (decade % major_z == 0)
                      {
                        if (z_tick > 1)
                          {
                            exponent = iround(log10(zi));
                            sprintf(string, "10^{%d}", exponent);
                            text3d(x_label, y_label, zi, replace_minus_sign(string), 0);
                          }
                        else
                          text3d(x_label, y_label, zi, gr_ftoa(string, zi, 0.), 0);
                      }
                }
              else
                {
                  xi = x_minor_tick;
                  yi = y_minor_tick;
                }

              if (i == 0 || abs(major_z) == 1)
                {
                  pline3d(xi, yi, zi);
                  pline3d(x_org, y_org, zi);
                }

              if (i == 9)
                {
                  z0 = z0 * 10.;
                  i = 0;
                  decade++;
                }
              else
                i++;

              zi = z0 + i * z0;
            }

          pline3d(x_org, y_org, z_max);

          end_pline();
        }
      else
        {
          check_tick_marks(z_min, z_max, z_tick, 'Z');

          i = isucc(z_min / z_tick);
          zi = i * z_tick;

          /* draw Z-axis */

          start_pline3d(x_org, y_org, z_min);

          while (zi <= z_max)
            {
              pline3d(x_org, y_org, zi);

              if (major_z == 0 || i % major_z == 0)
                {
                  xi = x_major_tick;
                  yi = y_major_tick;
                  if ((zi != z_org) && (major_z > 0))
                    text3d(x_label, y_label, zi, gr_ftoa(string, zi, z_tick * major_z),
                           modern_projection_type ? axis : 0);
                }
              else
                {
                  xi = x_minor_tick;
                  yi = y_minor_tick;
                }

              pline3d(xi, yi, zi);
              pline3d(x_org, y_org, zi);

              i++;
              zi = i * z_tick;
            }

          if (zi > z_max) pline3d(x_org, y_org, z_max);

          end_pline();
        }
    }

  if (y_tick != 0)
    {
      if (modern_projection_type)
        tick = tick_size / tx.x_axis_scale;
      else
        tick = tick_size * (x_max - x_min) / (vp[1] - vp[0]);

      x_minor_tick = x_log(x_lin(x_org) + tick);
      x_major_tick = x_log(x_lin(x_org) + 2. * tick);
      x_label = x_log(x_lin(x_org) + 3. * tick);

      /* set text alignment */

      if (x_lin(x_org) <= (x_lin(x_min) + x_lin(x_max)) / 2.)
        {
          gks_set_text_align(GKS_K_TEXT_HALIGN_RIGHT, GKS_K_TEXT_VALIGN_HALF);
          if (tick > 0) x_label = x_log(x_lin(x_org) - tick);
        }
      else
        {
          gks_set_text_align(GKS_K_TEXT_HALIGN_LEFT, GKS_K_TEXT_VALIGN_HALF);
          if (tick < 0) x_label = x_log(x_lin(x_org) - tick);
        }

      if (!modern_projection_type)
        {
          rep = rep_table[which_rep][1];
          if (rep == 0) gks_set_text_align(GKS_K_TEXT_HALIGN_CENTER, GKS_K_TEXT_VALIGN_TOP);

          gks_set_text_upvec(a[axes_rep[rep][0]], c[axes_rep[rep][1]]);
          gks_set_text_slant(text_slant[axes_rep[rep][2]]);
          tick_axis = 0;
        }
      else
        {
          tick = tick_size / tx.z_axis_scale;

          z_minor_tick = z_log(z_lin(z_org) + tick);
          z_major_tick = z_log(z_lin(z_org) + 2. * tick);
          z_label = z_log(z_lin(z_org) + 3. * tick);

          if (z_lin(z_org) <= (z_lin(z_min) + z_lin(z_max)) / 2.)
            {
              if (tick > 0) z_label = z_log(z_lin(z_org) - tick);
            }
          else
            {
              if (tick < 0) z_label = z_log(z_lin(z_org) - tick);
            }

          axes3d_get_params(1, &tick_axis, x_org, y_org, z_org, &axis);
        }

      if (tick_axis != 0)
        {
          x_label = x_major_tick = x_minor_tick = x_org;
        }
      if (tick_axis != 2)
        {
          z_label = z_major_tick = z_minor_tick = z_org;
        }

      if (OPTION_Y_LOG & lx.scale_options)
        {
          y0 = pow(10.0, gauss(log10(y_min)));

          i = ipred(y_min / y0);
          yi = y0 + i * y0;
          decade = igauss(log10(y_min / y_org));

          /* draw Y-axis */

          start_pline3d(x_org, y_min, z_org);

          while (yi <= y_max)
            {
              pline3d(x_org, yi, z_org);

              if (i == 0)
                {
                  xi = x_major_tick;
                  zi = z_major_tick;
                  if (major_y > 0 && yi != y_org)
                    if (decade % major_y == 0)
                      {
                        if (y_tick > 1)
                          {
                            exponent = iround(log10(yi));
                            sprintf(string, "10^{%d}", exponent);
                            text3d(x_label, yi, z_label, replace_minus_sign(string), 0);
                          }
                        else
                          text3d(x_label, yi, z_label, gr_ftoa(string, yi, 0.), 0);
                      }
                }
              else
                {
                  xi = x_minor_tick;
                  zi = z_minor_tick;
                }

              if (i == 0 || abs(major_y) == 1)
                {
                  pline3d(xi, yi, zi);
                  pline3d(x_org, yi, z_org);
                }

              if (i == 9)
                {
                  y0 = y0 * 10.;
                  i = 0;
                  decade++;
                }
              else
                i++;

              yi = y0 + i * y0;
            }

          pline3d(x_org, y_max, z_org);

          end_pline();
        }
      else
        {
          check_tick_marks(y_min, y_max, y_tick, 'Y');

          i = isucc(y_min / y_tick);
          yi = i * y_tick;

          /* draw Y-axis */

          start_pline3d(x_org, y_min, z_org);

          while (yi <= y_max)
            {
              pline3d(x_org, yi, z_org);

              if (major_y == 0 || i % major_y == 0)
                {
                  xi = x_major_tick;
                  zi = z_major_tick;
                  if ((yi != y_org) && (major_y > 0))
                    text3d(x_label, yi, z_label, gr_ftoa(string, yi, y_tick * major_y),
                           modern_projection_type ? axis : 0);
                }
              else
                {
                  xi = x_minor_tick;
                  zi = z_minor_tick;
                }

              pline3d(xi, yi, zi);
              pline3d(x_org, yi, z_org);

              i++;
              yi = i * y_tick;
            }

          if (yi > y_max) pline3d(x_org, y_max, z_org);

          end_pline();
        }
    }

  if (x_tick != 0)
    {
      if (modern_projection_type)
        tick = tick_size / tx.y_axis_scale;
      else
        tick = tick_size * (y_max - y_min) / (vp[3] - vp[2]);

      y_minor_tick = y_log(y_lin(y_org) + tick);
      y_major_tick = y_log(y_lin(y_org) + 2. * tick);
      y_label = y_log(y_lin(y_org) + 3. * tick);

      /* set text alignment */

      if (y_lin(y_org) <= (y_lin(y_min) + y_lin(y_max)) / 2.)
        {
          gks_set_text_align(GKS_K_TEXT_HALIGN_RIGHT, GKS_K_TEXT_VALIGN_HALF);
          if (tick > 0) y_label = y_log(y_lin(y_org) - tick);
        }
      else
        {
          gks_set_text_align(GKS_K_TEXT_HALIGN_LEFT, GKS_K_TEXT_VALIGN_HALF);
          if (tick < 0) y_label = y_log(y_lin(y_org) - tick);
        }

      if (!modern_projection_type)
        {
          rep = rep_table[which_rep][0];
          if (rep == 2) gks_set_text_align(GKS_K_TEXT_HALIGN_CENTER, GKS_K_TEXT_VALIGN_TOP);

          gks_set_text_upvec(a[axes_rep[rep][0]], c[axes_rep[rep][1]]);
          gks_set_text_slant(text_slant[axes_rep[rep][2]]);
          tick_axis = 1;
        }
      else
        {
          tick = tick_size / tx.z_axis_scale;

          z_minor_tick = z_log(z_lin(z_org) + tick);
          z_major_tick = z_log(z_lin(z_org) + 2. * tick);
          z_label = z_log(z_lin(z_org) + 3. * tick);

          if (z_lin(z_org) <= (z_lin(z_min) + z_lin(z_max)) / 2.)
            {
              if (tick > 0) z_label = z_log(z_lin(z_org) - tick);
            }
          else
            {
              if (tick < 0) z_label = z_log(z_lin(z_org) - tick);
            }

          axes3d_get_params(0, &tick_axis, x_org, y_org, z_org, &axis);
        }

      if (tick_axis != 1)
        {
          y_label = y_major_tick = y_minor_tick = y_org;
        }
      if (tick_axis != 2)
        {
          z_label = z_major_tick = z_minor_tick = z_org;
        }

      if (OPTION_X_LOG & lx.scale_options)
        {
          x0 = pow(10.0, gauss(log10(x_min)));

          i = ipred(x_min / x0);
          xi = x0 + i * x0;
          decade = igauss(log10(x_min / x_org));

          /* draw X-axis */

          start_pline3d(x_min, y_org, z_org);

          while (xi <= x_max)
            {
              pline3d(xi, y_org, z_org);

              if (i == 0)
                {
                  yi = y_major_tick;
                  zi = z_major_tick;
                  if (major_x > 0 && xi != x_org)
                    if (decade % major_x == 0)
                      {
                        if (x_tick > 1)
                          {
                            exponent = iround(log10(xi));
                            sprintf(string, "10^{%d}", exponent);
                            text3d(xi, y_label, z_label, replace_minus_sign(string), 0);
                          }
                        else
                          text3d(xi, y_label, z_label, gr_ftoa(string, xi, 0.), 0);
                      }
                }
              else
                {
                  yi = y_minor_tick;
                  zi = z_minor_tick;
                }

              if (i == 0 || abs(major_x) == 1)
                {
                  pline3d(xi, yi, zi);
                  pline3d(xi, y_org, z_org);
                }

              if (i == 9)
                {
                  x0 = x0 * 10.;
                  i = 0;
                  decade++;
                }
              else
                i++;

              xi = x0 + i * x0;
            }

          pline3d(x_max, y_org, z_org);

          end_pline();
        }
      else
        {
          check_tick_marks(x_min, x_max, x_tick, 'X');

          i = isucc(x_min / x_tick);
          xi = i * x_tick;

          /* draw X-axis */

          start_pline3d(x_min, y_org, z_org);

          while (xi <= x_max)
            {
              pline3d(xi, y_org, z_org);

              if (major_x == 0 || i % major_x == 0)
                {
                  yi = y_major_tick;
                  zi = z_major_tick;
                  if ((xi != x_org) && (major_x > 0))
                    text3d(xi, y_label, z_label, gr_ftoa(string, xi, x_tick * major_x),
                           modern_projection_type ? axis : 0);
                }
              else
                {
                  yi = y_minor_tick;
                  zi = z_minor_tick;
                }

              pline3d(xi, yi, zi);
              pline3d(xi, y_org, z_org);

              i++;
              xi = i * x_tick;
            }

          if (xi > x_max) pline3d(x_max, y_org, z_org);

          end_pline();
        }
    }

  /* restore linetype, text alignment, text font, text slant,
     character-up vector and clipping indicator */

  gks_set_pline_linetype(ltype);
  gks_set_text_align(halign, valign);
  gks_set_text_fontprec(font, prec);
  gks_set_text_slant(slant);
  gks_set_text_upvec(chux, chuy);
  gks_set_clipping(clsw);

  if (flag_graphics)
    gr_writestream("<axes3d xtick=\"%g\" ytick=\"%g\" ztick=\"%g\" "
                   "xorg=\"%g\" yorg=\"%g\" zorg=\"%g\" "
                   "majorx=\"%d\" majory=\"%d\" majorz=\"%d\" ticksize=\"%g\"/>\n",
                   x_tick, y_tick, z_tick, x_org, y_org, z_org, major_x, major_y, major_z, tick_size);

  if (modern_projection_type)
    {
      gks_set_window(WC, wn[0], wn[1], wn[2], wn[3]);
      setscale(lx.scale_options);
    }
}

/*!
 * Display axis titles just outside of their respective axes.
 *
 * \param[in] x_title The text to be displayed on the X axis
 * \param[in] y_title The text to be displayed on the Y axis
 * \param[in] z_title The text to be displayed on the Z axis
 */
void gr_titles3d(char *x_title, char *y_title, char *z_title)
{
  int errind, tnr;
  int halign, valign, clsw, font, prec;
  double chux, chuy;

  double clrt[4], wn[4], vp[4];
  double x_min, x_max, y_min, y_max, z_min, z_max;
  double x_rel, y_rel, z_rel, x, y, z;

  double r, t, alpha, beta;
  double a[2], c[2];

  double slant, text_slant[4];
  int *anglep, which_rep, rep;

  double x_2d, y_2d, x_2d_max, y_2d_max;
  double x_angle, y_angle;
  double x_mid_x, x_mid_y, y_mid_x, y_mid_y;
  double a1, a2, c1, c2, c3, aa, cc;
  double xr, yr, zr;

  int flip_x, flip_y, flip_z;
  int modern_projection_type;

  check_autoinit;

  setscale(lx.scale_options);

  /* inquire current normalization transformation */

  gks_inq_current_xformno(&errind, &tnr);
  gks_inq_xform(tnr, &errind, wn, vp);

  modern_projection_type =
      gpx.projection_type == GR_PROJECTION_PERSPECTIVE || gpx.projection_type == GR_PROJECTION_ORTHOGRAPHIC;

  if (wx.phi != 0 || wx.delta != 90)
    {
      if (modern_projection_type)
        {
          gks_set_window(WC, -1, 1, -1, 1);
          setscale(lx.scale_options);

          x_min = ix.xmin;
          x_max = ix.xmax;
          y_min = ix.ymin;
          y_max = ix.ymax;
          z_min = ix.zmin;
          z_max = ix.zmin;
        }
      else
        {
          x_min = wn[0];
          x_max = wn[1];
          y_min = wn[2];
          y_max = wn[3];
          z_min = wx.zmin;
          z_max = wx.zmax;
        }
      r = (x_max - x_min) / (y_max - y_min) * (vp[3] - vp[2]) / (vp[1] - vp[0]);

      alpha = atan_2(r * wx.c1, wx.a1);
      a[0] = -sin(alpha);
      c[0] = cos(alpha);
      alpha = deg(alpha);

      beta = atan_2(r * wx.c2, wx.a2);
      a[1] = -sin(beta);
      c[1] = cos(beta);
      beta = deg(beta);

      text_slant[0] = alpha;
      text_slant[1] = beta;
      text_slant[2] = -(90.0 + alpha - beta);
      text_slant[3] = 90.0 + alpha - beta;

      /* save text alignment, text font, text slant,
         character-up vector and clipping indicator */

      gks_inq_text_align(&errind, &halign, &valign);
      gks_inq_text_fontprec(&errind, &font, &prec);
      gks_inq_text_slant(&errind, &slant);
      gks_inq_text_upvec(&errind, &chux, &chuy);
      gks_inq_clip(&errind, &clsw, clrt);

      gks_set_pline_linetype(GKS_K_LINETYPE_SOLID);
      gks_set_text_fontprec(font, GKS_K_TEXT_PRECISION_STROKE);
      gks_set_clipping(GKS_K_NOCLIP);

      which_rep = 0;
      anglep = angle;
      while (wx.delta > *anglep++) which_rep++;
      anglep = angle;
      while (wx.phi > *anglep++) which_rep += 4;

      r = arc(wx.phi);
      a1 = cos(r);
      a2 = sin(r);
      aa = a1 + a2;
      a1 = a1 / aa;
      a2 = a2 / aa;

      t = arc(wx.delta);
      c1 = (pow(cos(r), 2.) - 1) * tan(t / 2.);
      c2 = -(pow(sin(r), 2.) - 1) * tan(t / 2.);
      c3 = cos(t);
      cc = (c2 + c3 - c1);
      c1 = c1 / cc;
      c2 = c2 / cc;
      c3 = c3 / cc;

      a1 = fabs(a1) * (vp[1] - vp[0]);
      a2 = fabs(a2) * (vp[1] - vp[0]);
      c1 = fabs(c1) * (vp[3] - vp[2]);
      c2 = fabs(c2) * (vp[3] - vp[2]);
      c3 = fabs(c3) * (vp[3] - vp[2]);

      x_mid_x = vp[0] + a1 / 2.;
      x_mid_y = vp[2] + c1 / 2.;
      y_mid_x = 1 - vp[1] + a2 / 2.;
      y_mid_y = vp[2] + c2 / 2.;

      x_2d_max = sqrt(y_mid_x * y_mid_x + y_mid_y * y_mid_y);
      y_2d_max = sqrt(x_mid_x * x_mid_x + x_mid_y * x_mid_y);

      x_angle = atan_2(a1, c1);
      x_2d = y_mid_y / cos(x_angle);

      if (x_2d > x_2d_max)
        {
          x_angle = M_PI / 2. - x_angle;
          x_2d = y_mid_x / cos(x_angle);
        }

      xr = x_2d / (2 * sqrt(pow(c1, 2.) + pow(a1, 2.)));

      y_angle = atan_2(c2, a2);
      y_2d = x_mid_x / cos(y_angle);

      if (y_2d > y_2d_max)
        {
          y_angle = M_PI / 2. - y_angle;
          y_2d = x_mid_y / cos(y_angle);
        }

      if (wx.phi + wx.delta != 0)
        yr = y_2d / (2 * sqrt(pow(c2, 2.) + pow(a2, 2.)));
      else
        yr = 0;

      x_rel = xr * (x_lin(x_max) - x_lin(x_min));
      y_rel = yr * (y_lin(y_max) - y_lin(y_min));

      flip_x = OPTION_FLIP_X & lx.scale_options;
      flip_y = OPTION_FLIP_Y & lx.scale_options;
      flip_z = OPTION_FLIP_Z & lx.scale_options;

      if (*x_title)
        {
          rep = rep_table[which_rep][1];

          x = x_log(0.5 * (x_lin(x_min) + x_lin(x_max)));
          if (rep == 0)
            {
              gks_set_text_align(GKS_K_TEXT_HALIGN_CENTER, GKS_K_TEXT_VALIGN_TOP);

              rep = rep_table[which_rep][0];

              if (flip_y)
                y = y_max;
              else
                y = y_min;

              zr = x_mid_y / (2 * c3);
              z_rel = zr * (z_lin(z_max) - z_lin(z_min));

              if (flip_z)
                z = z_log(z_lin(z_max) + z_rel);
              else
                z = z_log(z_lin(z_min) - z_rel);
            }
          else
            {
              if (flip_y)
                y = y_log(y_lin(y_max) + y_rel);
              else
                y = y_log(y_lin(y_min) - y_rel);

              if (flip_z)
                z = z_max;
              else
                z = z_min;

              gks_set_text_align(GKS_K_TEXT_HALIGN_CENTER, GKS_K_TEXT_VALIGN_HALF);
            }

          gks_set_text_upvec(a[axes_rep[rep][0]], c[axes_rep[rep][1]]);
          gks_set_text_slant(text_slant[axes_rep[rep][2]]);

          text3d(x, y, z, x_title, 0);
        }

      if (*y_title)
        {
          rep = rep_table[which_rep][0];

          y = y_log(0.5 * (y_lin(y_min) + y_lin(y_max)));
          if (rep == 2)
            {
              gks_set_text_align(GKS_K_TEXT_HALIGN_CENTER, GKS_K_TEXT_VALIGN_TOP);

              rep = rep_table[which_rep][1];

              if (flip_x)
                x = x_min;
              else
                x = x_max;

              zr = y_mid_y / (2 * c3);
              z_rel = zr * (z_lin(z_max) - z_lin(z_min));

              if (flip_z)
                z = z_log(z_lin(z_max) + z_rel);
              else
                z = z_log(z_lin(z_min) - z_rel);
            }
          else
            {
              if (flip_x)
                x = x_log(x_lin(x_min) - x_rel);
              else
                x = x_log(x_lin(x_max) + x_rel);

              if (flip_z)
                z = z_max;
              else
                z = z_min;

              gks_set_text_align(GKS_K_TEXT_HALIGN_CENTER, GKS_K_TEXT_VALIGN_HALF);
            }

          gks_set_text_upvec(a[axes_rep[rep][0]], c[axes_rep[rep][1]]);
          gks_set_text_slant(text_slant[axes_rep[rep][2]]);

          text3d(x, y, z, y_title, 0);
        }

      if (*z_title)
        {
          rep = rep_table[which_rep][2];

          gks_set_text_upvec(a[axes_rep[rep][0]], c[axes_rep[rep][1]]);
          gks_set_text_slant(text_slant[axes_rep[rep][2]]);

          if (flip_x)
            x = x_max;
          else
            x = x_min;

          if (flip_y)
            y = y_max;
          else
            y = y_min;

          zr = (1 - (c1 + c3 + vp[2])) / (2 * c3);
          z_rel = zr * (z_lin(z_max) - z_lin(z_min));

          if (flip_z)
            z = z_log(z_lin(z_min) - 0.5 * z_rel);
          else
            z = z_log(z_lin(z_max) + 0.5 * z_rel);

          gks_set_text_align(GKS_K_TEXT_HALIGN_CENTER, GKS_K_TEXT_VALIGN_BASE);

          text3d(x, y, z, z_title, 0);
        }

      /* restore text alignment, text font, text slant,
         character-up vector and clipping indicator */

      gks_set_text_align(halign, valign);
      gks_set_text_fontprec(font, prec);
      gks_set_text_slant(slant);
      gks_set_text_upvec(chux, chuy);
      gks_set_clipping(clsw);
    }
  else
    {
      /* save text alignment and character-up vector */

      gks_inq_text_align(&errind, &halign, &valign);
      gks_inq_text_upvec(&errind, &chux, &chuy);

      if (*x_title)
        {
          gks_set_text_align(GKS_K_TEXT_HALIGN_CENTER, GKS_K_TEXT_VALIGN_BASE);
          gks_set_text_upvec(0.0, 1.0);

          x = 0.5 * (vp[0] + vp[1]);
          y = vp[2] * 0.2 * (vp[3] - vp[2]);
          gr_ndctowc(&x, &y);

          text2d(x, y, x_title);
        }

      if (*y_title)
        {
          gks_set_text_align(GKS_K_TEXT_HALIGN_CENTER, GKS_K_TEXT_VALIGN_TOP);
          gks_set_text_upvec(-1.0, 0.0);

          x = vp[0] * 0.1 * (vp[1] - vp[0]);
          y = 0.5 * (vp[2] + vp[3]);
          gr_ndctowc(&x, &y);

          text2d(x, y, y_title);
        }

      if (*z_title)
        {
          gks_set_text_align(GKS_K_TEXT_HALIGN_CENTER, GKS_K_TEXT_VALIGN_BASE);
          gks_set_text_upvec(0.0, 1.0);

          x = 0.5 * (vp[0] + vp[1]);
          y = vp[3] + 0.05 * (vp[3] - vp[2]);
          gr_ndctowc(&x, &y);

          text2d(x, y, z_title);
        }

      /* restore text alignment and character-up vector */

      gks_set_text_align(halign, valign);
      gks_set_text_upvec(chux, chuy);
    }

  if (flag_graphics)
    gr_writestream("<titles3d xtitle=\"%s\" ytitle=\"%s\" ztitle=\"%s\"/>\n", x_title, y_title, z_title);

  if (modern_projection_type)
    {
      gks_set_window(WC, wn[0], wn[1], wn[2], wn[3]);
      setscale(lx.scale_options);
    }
}

static void init_hlr(void)
{
  int sign, i, j, x1, x2;
  double *hide, a, b, m = 0;
  double x[3], y[3], z[3], yj;
  double eps;

  eps = (lx.ymax - lx.ymin) * 1E-5;

  for (i = 0; i <= RESOLUTION_X; i++)
    {
      hlr.ymin[i] = -FLT_MAX;
      hlr.ymax[i] = FLT_MAX;
    }

  for (sign = -1; sign <= 1; sign += 2)
    {
      if (sign == 1)
        {
          hide = hlr.ymin;
          x[0] = hlr.x0;
          y[0] = hlr.y0;
          z[0] = hlr.z0;
          x[1] = hlr.x1;
          y[1] = hlr.y0;
          z[1] = hlr.z0;
          x[2] = hlr.x1;
          y[2] = hlr.y1;
          z[2] = hlr.z0;
        }
      else
        {
          hide = hlr.ymax;
          x[0] = hlr.x0;
          y[0] = hlr.y0;
          z[0] = hlr.z1;
          x[1] = hlr.x0;
          y[1] = hlr.y1;
          z[1] = hlr.z1;
          x[2] = hlr.x1;
          y[2] = hlr.y1;
          z[2] = hlr.z1;
        }

      for (i = 0; i < 3; i++) apply_world_xform(x + i, y + i, z + i);

      if (hlr.xmax > hlr.xmin)
        {
          a = RESOLUTION_X / (hlr.xmax - hlr.xmin);
          b = -(hlr.xmin * a);
        }
      else
        {
          a = 1;
          b = 0;
        }

      x1 = nint(a * x[0] + b);
      if (x1 < 0) x1 = 0;
      x2 = x1;

      for (i = 1; i < 3; i++)
        {
          x1 = x2;
          x2 = nint(a * x[i] + b);

          if (x1 <= x2)
            {
              if (x1 != x2) m = (y[i] - y[i - 1]) / (x2 - x1);

              for (j = x1; j <= x2; j++)
                {
                  if (x1 != x2)
                    yj = y[i - 1] + m * (j - x1);
                  else
                    yj = y[i];

                  hide[j] = yj - sign * eps;
                }
            }
        }
    }
}

static void pline_hlr(int n, double *x, double *y, double *z)
{
  int i, j, x1, x2;
  int visible, draw;
  double *hide, a, b, c, m = 0;

  int saved_scale_options;
  double xj, yj;

  if (hlr.buf == NULL)
    {
      hlr.buf = (double *)xmalloc(sizeof(double) * (RESOLUTION_X + 1) * 2);
      hlr.ymin = hlr.buf;
      hlr.ymax = hlr.buf + RESOLUTION_X + 1;
    }

  if (hlr.sign == 1)
    hide = hlr.ymin;
  else
    hide = hlr.ymax;

  for (i = 0; i < n; i++) apply_world_xform(x + i, y + i, z + i);

  draw = !hlr.initialize || hlr.sign > 0;
  visible = 0;

  saved_scale_options = lx.scale_options;
  lx.scale_options = 0;

  if (hlr.xmax > hlr.xmin)
    {
      a = RESOLUTION_X / (hlr.xmax - hlr.xmin);
      b = -(hlr.xmin * a);
    }
  else
    {
      a = 1;
      b = 0;
    }
  c = 1.0 / a;

  x1 = nint(a * x[0] + b);
  if (x1 < 0) x1 = 0;
  x2 = x1;

  if (hlr.initialize)
    {
      init_hlr();

      if (hlr.ymin[x1] <= y[0] && y[0] <= hlr.ymax[x1])
        {
          hide[x1] = y[0];

          if (draw) start_pline(x[0], y[0]);

          visible = 1;
        }
    }

  for (i = 1; i < n; i++)
    {
      x1 = x2;
      x2 = nint(a * x[i] + b);

      if (x1 < x2)
        {
          if (x1 != x2) m = (y[i] - y[i - 1]) / (x2 - x1);

          for (j = x1; j <= x2; j++)
            {
              if (x1 != x2)
                yj = y[i - 1] + m * (j - x1);
              else
                yj = y[i];

              if (hlr.ymin[j] <= yj && yj <= hlr.ymax[j])
                {
                  if (!visible)
                    {
                      if (draw)
                        {
                          xj = c * j + hlr.xmin;

                          start_pline(xj, yj);
                        }

                      visible = 1;
                    }
                }
              else
                {
                  if (visible)
                    {
                      if (draw)
                        {
                          xj = c * j + hlr.xmin;

                          pline(xj, yj);
                          end_pline();
                        }

                      visible = 0;
                    }
                }

              if ((yj - hide[j]) * hlr.sign > 0) hide[j] = yj;
            }

          if (visible && draw) pline(x[i], y[i]);
        }

      else if (x1 == x2)
        {
          if (draw)
            {
              j = x1;
              xj = c * j + hlr.xmin;
              yj = y[i];

              if ((yj - hide[j]) * hlr.sign > 0)
                {
                  start_pline(xj, hide[j]);
                  pline(xj, yj);
                  end_pline();

                  visible = 1;
                  hide[j] = yj;
                }
              else
                visible = 0;
            }
        }
    }

  if (visible && draw) end_pline();

  lx.scale_options = saved_scale_options;
}

static void glint(int dinp, int *inp, int doutp, int *outp)
{
  int i, j, k, n;
  double ratio, delta;

  n = (doutp + 1) / dinp;
  ratio = 1.0 / n;

  j = (n + 1) / 2;
  for (k = 0; k < j; k++) outp[k] = inp[0];

  for (i = 0; i < dinp - 1; i++)
    {
      delta = ratio * (inp[i + 1] - inp[i]);
      for (k = 1; k <= n; k++) outp[j++] = inp[i] + (int)(k * delta + 0.5);
    }

  for (k = j; k < doutp; k++) outp[k] = inp[dinp - 1];
}

static void pixel(double xmin, double xmax, double ymin, double ymax, int dx, int dy, int *colia, int w, int h,
                  int *pixmap, int dwk, int *wk1, int *wk2)
{
  int i, j, ix, nx;
  int sx = 1, sy = 1;

  if ((w + 1) % dx != 0 || (h + 1) % dy != 0)
    {
      fprintf(stderr, "improper input parameter values\n");
      return;
    }

  ix = 0;
  nx = (w + 1) / dx;

  for (i = 0; i < dx; i++)
    {
      for (j = 0; j < dy; j++) wk1[j] = colia[i + j * dx];

      glint(dy, wk1, h, wk2);
      for (j = 0; j < h; j++) pixmap[ix + j * w] = wk2[j];

      ix += nx;
    }

  for (j = 0; j < h; j++)
    {
      ix = 0;
      for (i = 0; i < dx; i++)
        {
          wk1[i] = pixmap[ix + j * w];
          ix += nx;
        }

      glint(dx, wk1, w, wk2);
      for (i = 0; i < w; i++) pixmap[i + j * w] = wk2[i];
    }

  gks_cellarray(xmin, ymin, xmax, ymax, w, h, sx, sy, w, h, pixmap);
}

static void get_intensity(double *fx, double *fy, double *fz, double *light_source, double *intensity)
{
  int k;
  double max_x, max_y, max_z, min_x, min_y, min_z, norm_1, norm_2;
  double center[4], normal[4], negated[4], oddnormal[4], negated_norm[4];

  min_x = max_x = fx[0];
  min_y = max_y = fy[0];
  min_z = max_z = fz[0];

  for (k = 1; k < 4; k++)
    {
      if (fx[k] > max_x)
        max_x = fx[k];
      else if (fx[k] < min_x)
        min_x = fx[k];
      if (fy[k] > max_y)
        max_y = fy[k];
      else if (fy[k] < min_y)
        min_y = fy[k];
      if (fz[k] > max_z)
        max_z = fz[k];
      else if (fz[k] < min_z)
        min_z = fz[k];
    }

  center[0] = (max_x + min_x) / 2;
  center[1] = (max_y + min_y) / 2;
  center[2] = (max_z + min_z) / 2;

  for (k = 0; k < 3; k++) negated[k] = light_source[k] - center[k];

  norm_1 = (double)sqrt(negated[0] * negated[0] + negated[1] * negated[1] + negated[2] * negated[2]);

  for (k = 0; k < 3; k++) negated_norm[k] = negated[k] / norm_1;

  normal[0] = ((fy[1] - fy[0]) * (fz[2] - fz[0]) - (fz[1] - fz[0]) * (fy[2] - fy[0])) +
              ((fy[2] - fy[1]) * (fz[3] - fz[1]) - (fz[2] - fz[1]) * (fy[3] - fy[1]));
  normal[1] = ((fz[1] - fz[0]) * (fx[2] - fx[0]) - (fx[1] - fx[0]) * (fz[2] - fz[0])) +
              ((fz[2] - fz[1]) * (fx[3] - fx[1]) - (fx[2] - fx[1]) * (fz[3] - fz[1]));
  normal[2] = ((fx[1] - fx[0]) * (fy[2] - fy[0]) - (fy[1] - fy[0]) * (fx[2] - fx[0])) +
              ((fx[2] - fx[1]) * (fy[3] - fy[1]) - (fy[2] - fy[1]) * (fx[3] - fx[1]));
  normal[3] = 1;

  norm_2 = (double)sqrt(normal[0] * normal[0] + normal[1] * normal[1] + normal[2] * normal[2]);

  for (k = 0; k < 3; k++) oddnormal[k] = normal[k] / norm_2;

  *intensity =
      (oddnormal[0] * negated_norm[0] + oddnormal[1] * negated_norm[1] + oddnormal[2] * negated_norm[2]) * 0.8 + 0.2;
}

/*!
 * Draw a three-dimensional surface plot for the given data points.
 *
 * \param[in] nx The number of points along the X axis
 * \param[in] ny The number of points along the Y axis
 * \param[in] px A pointer to the X coordinates
 * \param[in] py A pointer to the Y coordinates
 * \param[in] pz A pointer to the Z coordinates
 * \param[in] option Surface display option (see table)
 *
 * x and y define a grid. z is a singly dimensioned array containing at least
 * nx * ny data points. z describes the surface height at each point on the
 * grid. Data is ordered as shown in the following table:
 *
 * \verbatim embed:rst:leading-asterisk
 *
 * +------------------+--+--------------------------------------------------------------+
 * |LINES             | 0|Use X Y polylines to denote the surface                       |
 * +------------------+--+--------------------------------------------------------------+
 * |MESH              | 1|Use a wire grid to denote the surface                         |
 * +------------------+--+--------------------------------------------------------------+
 * |FILLED_MESH       | 2|Applies an opaque grid to the surface                         |
 * +------------------+--+--------------------------------------------------------------+
 * |Z_SHADED_MESH     | 3|Applies Z-value shading to the surface                        |
 * +------------------+--+--------------------------------------------------------------+
 * |COLORED_MESH      | 4|Applies a colored grid to the surface                         |
 * +------------------+--+--------------------------------------------------------------+
 * |CELL_ARRAY        | 5|Applies a grid of individually-colored cells to the surface   |
 * +------------------+--+--------------------------------------------------------------+
 * |SHADED_MESH       | 6|Applies light source shading to the 3-D surface               |
 * +------------------+--+--------------------------------------------------------------+
 *
 * To see option 2 correctly change linecolorind or fillcolorind. Both default values are black.
 * \endverbatim
 */
void gr_surface(int nx, int ny, double *px, double *py, double *pz, int option)
{
  int errind, ltype, coli, int_style, tnr;

  int i, ii, j, jj, k;
  int color;

  double *xn, *yn, *zn, *x, *y, *z;
  double facex[4], facey[4], facez[4], vp[4], wn[4], intensity = 0, meanz;
  double a, b, c, d, e, f;

  double ymin, ymax, zmin, zmax;

  int flip_x, flip_y, flip_z;
  int np;

  int *colia, w, h, *ca, dwk, *wk1, *wk2;

  static double light_source[3] = {0.5, -1, 2};

  if ((nx <= 0) || (ny <= 0))
    {
      fprintf(stderr, "invalid number of points\n");
      return;
    }

  /* be sure that points ordinates are sorted in ascending order */

  for (i = 1; i < nx; i++)
    if (px[i - 1] >= px[i])
      {
        fprintf(stderr, "points not sorted in ascending order\n");
        return;
      }

  for (j = 1; j < ny; j++)
    if (py[j - 1] >= py[j])
      {
        fprintf(stderr, "points not sorted in ascending order\n");
        return;
      }

  check_autoinit;

  setscale(lx.scale_options);

  gks_inq_current_xformno(&errind, &tnr);
  gks_inq_xform(tnr, &errind, wn, vp);

  if (gpx.projection_type == GR_PROJECTION_PERSPECTIVE || gpx.projection_type == GR_PROJECTION_ORTHOGRAPHIC)
    {
      gks_set_window(WC, -1, 1, -1, 1);
      setscale(lx.scale_options);
    }

#define Z(x, y) pz[(x) + nx * (y)]

  a = 1.0 / (lx.xmax - lx.xmin);
  b = -(lx.xmin * a);
  c = 1.0 / (lx.ymax - lx.ymin);
  d = -(lx.ymin * c);
  e = 1.0 / (wx.zmax - wx.zmin);
  f = -(wx.zmin * e);

  /* save linetype, fill area interior style and color
     index */

  gks_inq_pline_linetype(&errind, &ltype);
  gks_inq_fill_int_style(&errind, &int_style);
  gks_inq_fill_color_index(&errind, &coli);

  k = sizeof(double) * (nx + ny) * 3;
  xn = (double *)xmalloc(k);
  yn = (double *)xmalloc(k);
  zn = (double *)xmalloc(k);
  x = (double *)xmalloc(nx * sizeof(double));
  y = (double *)xmalloc(ny * sizeof(double));
  z = (double *)xmalloc(nx * ny * sizeof(double));

  flip_x = OPTION_FLIP_X & lx.scale_options;
  if ((gpx.projection_type == GR_PROJECTION_PERSPECTIVE || gpx.projection_type == GR_PROJECTION_ORTHOGRAPHIC) &&
      tx.camera_pos_x < tx.focus_point_x)
    {
      flip_x = 1;
    }

  for (i = 0; i < nx; i++) x[i] = x_lin(px[flip_x ? nx - 1 - i : i]);

  flip_y = OPTION_FLIP_Y & lx.scale_options;
  if ((gpx.projection_type == GR_PROJECTION_PERSPECTIVE || gpx.projection_type == GR_PROJECTION_ORTHOGRAPHIC) &&
      tx.camera_pos_y > tx.focus_point_y)
    {
      flip_y = 1;
    }
  for (j = 0; j < ny; j++) y[j] = y_lin(py[flip_y ? ny - 1 - j : j]);

  k = 0;
  for (j = 0; j < ny; j++)
    {
      jj = flip_y ? ny - j - 1 : j;
      for (i = 0; i < nx; i++)
        {
          ii = flip_x ? nx - i - 1 : i;
          z[k++] = z_lin(Z(ii, jj));
        }
    }

#undef Z

  hlr.x0 = x[0];
  hlr.x1 = x[nx - 1];
  hlr.y0 = y[0];
  hlr.y1 = y[ny - 1];
  hlr.z0 = wx.zmin;
  hlr.z1 = wx.zmax;

  hlr.xmin = x[0];
  hlr.xmax = x[nx - 1];
  ymin = y[0];
  ymax = y[ny - 1];
  zmin = wx.zmin;
  zmax = wx.zmax;

  apply_world_xform(&hlr.xmin, &ymin, &zmin);
  apply_world_xform(&hlr.xmax, &ymax, &zmax);

  flip_z = OPTION_FLIP_Z & lx.scale_options;
  gks_set_pline_linetype(flip_z ? GKS_K_LINETYPE_DOTTED : GKS_K_LINETYPE_SOLID);
  if ((gpx.projection_type == GR_PROJECTION_PERSPECTIVE || gpx.projection_type == GR_PROJECTION_ORTHOGRAPHIC) &&
      tx.camera_pos_z < tx.focus_point_z)
    {
      flip_z = 1;
    }

#define Z(x, y) z[(x) + nx * (y)]

  hlr.sign = -1;

  do
    {
      hlr.sign = -hlr.sign;

      switch (option)
        {

        case OPTION_LINES:
          {
            j = 0;
            hlr.initialize = 1;

            while (j < ny)
              {
                k = 0;

                if (j > 0)
                  {
                    xn[k] = x[0];
                    yn[k] = y[j - 1];
                    zn[k] = Z(0, j - 1);
                    k++;
                  }

                for (i = 0; i < nx; i++)
                  {
                    xn[k] = x[i];
                    yn[k] = y[j];
                    zn[k] = Z(i, j);
                    k++;
                  }

                if (j == 0)

                  for (i = 1; i < ny; i++)
                    {
                      xn[k] = x[nx - 1];
                      yn[k] = y[i];
                      zn[k] = Z(nx - 1, i);
                      k++;
                    }

                pline_hlr(k, xn, yn, zn);

                hlr.initialize = 0;
                j++;
              }

            break;
          }

        case OPTION_MESH:
          {
            k = 0;

            for (i = 0; i < nx; i++)
              {
                xn[k] = x[i];
                yn[k] = y[0];
                zn[k] = Z(i, 0);
                k++;
              }

            for (j = 1; j < ny; j++)
              {
                xn[k] = x[nx - 1];
                yn[k] = y[j];
                zn[k] = Z(nx - 1, j);
                k++;
              }

            hlr.initialize = 1;

            pline_hlr(k, xn, yn, zn);

            i = nx - 1;

            while (i > 0)
              {
                hlr.initialize = 0;

                for (j = 1; j < ny; j++)
                  {
                    xn[0] = x[i - 1];
                    yn[0] = y[j - 1];
                    zn[0] = Z(i - 1, j - 1);

                    xn[1] = x[i - 1];
                    yn[1] = y[j];
                    zn[1] = Z(i - 1, j);

                    xn[2] = x[i];
                    yn[2] = y[j];
                    zn[2] = Z(i, j);

                    pline_hlr(3, xn, yn, zn);
                  }

                i--;
              }

            break;
          }

        case OPTION_FILLED_MESH:
        case OPTION_Z_SHADED_MESH:
        case OPTION_COLORED_MESH:
        case OPTION_SHADED_MESH:
          {
            j = ny - 1;

            gks_set_fill_int_style(GKS_K_INTSTYLE_SOLID);

            while (j > 0)
              {
                for (i = 1; i < nx; i++)
                  {
                    xn[0] = x[i - 1];
                    yn[0] = y[j];
                    zn[0] = Z(i - 1, j);

                    xn[1] = x[i - 1];
                    yn[1] = y[j - 1];
                    zn[1] = Z(i - 1, j - 1);

                    xn[2] = x[i];
                    yn[2] = y[j - 1];
                    zn[2] = Z(i, j - 1);

                    xn[3] = x[i];
                    yn[3] = y[j];
                    zn[3] = Z(i, j);

                    xn[4] = xn[0];
                    yn[4] = yn[0];
                    zn[4] = zn[0];

                    if (option == OPTION_SHADED_MESH)
                      {
                        for (k = 0; k < 4; k++)
                          {
                            facex[k] = a * xn[k] + b;
                            facey[k] = c * yn[k] + d;
                            facez[k] = e * zn[k] + f;
                          }
                        get_intensity(facex, facey, facez, light_source, &intensity);
                      }

                    for (k = 0; k <= 4; k++) apply_world_xform(xn + k, yn + k, zn + k);

                    meanz = 0.25 * (Z(i - 1, j - 1) + Z(i, j - 1) + Z(i, j) + Z(i - 1, j));

                    if (option == OPTION_Z_SHADED_MESH)
                      {
                        color = iround(meanz) + first_color;

                        if (color < first_color)
                          color = first_color;
                        else if (color > last_color)
                          color = last_color;

                        gks_set_fill_color_index(color);
                      }

                    else if (option == OPTION_COLORED_MESH)
                      {
                        color =
                            iround((meanz - wx.zmin) / (wx.zmax - wx.zmin) * (last_color - first_color)) + first_color;

                        if (color < first_color)
                          color = first_color;
                        else if (color > last_color)
                          color = last_color;

                        gks_set_fill_color_index(color);
                      }

                    else if (option == OPTION_SHADED_MESH)
                      {
                        color = iround(intensity * (last_color - first_color)) + first_color;

                        if (color < first_color)
                          color = first_color;
                        else if (color > last_color)
                          color = last_color;

                        gks_set_fill_color_index(color);
                      }

                    np = 4;
                    gks_fillarea(np, xn, yn);

                    if (option == OPTION_FILLED_MESH)
                      {
                        np = 5;
                        gks_polyline(np, xn, yn);
                      }
                  }

                j--;
              }

            break;
          }

        case OPTION_CELL_ARRAY:

          colia = (int *)xmalloc(nx * ny * sizeof(int));
          k = 0;
          for (j = 0; j < ny; j++)
            for (i = 0; i < nx; i++)
              {
                if (!is_nan(Z(i, j)))
                  {
                    color = first_color + (int)((Z(i, j) - wx.zmin) / (wx.zmax - wx.zmin) * (last_color - first_color));

                    if (color < first_color)
                      color = first_color;
                    else if (color > last_color)
                      color = last_color;
                  }
                else
                  color = BACKGROUND;

                colia[k++] = color;
              }

          w = (nx < 256) ? nx * (255 / nx + 1) - 1 : nx - 1;
          h = (ny < 256) ? ny * (255 / ny + 1) - 1 : ny - 1;
          ca = (int *)xmalloc((w + 1) * (h + 1) * sizeof(int));

          dwk = w;
          if (h > dwk) dwk = h;
          if (nx > dwk) dwk = nx;
          if (ny > dwk) dwk = ny;
          dwk += 1;
          wk1 = (int *)xmalloc(dwk * sizeof(int));
          wk2 = (int *)xmalloc(dwk * sizeof(int));

          pixel(hlr.xmin, hlr.xmax, ymin, ymax, nx, ny, colia, w, h, ca, dwk, wk1, wk2);

          free(wk2);
          free(wk1);
          free(ca);
          free(colia);

          break;
        }

      gks_set_pline_linetype(flip_z ? GKS_K_LINETYPE_SOLID : GKS_K_LINETYPE_DOTTED);
    }
  while ((hlr.sign >= 0) && ((int)option <= (int)OPTION_MESH));

#undef Z

  free(z);
  free(y);
  free(x);
  free(zn);
  free(yn);
  free(xn);

  /* restore linetype, fill area interior style and color index */

  gks_set_pline_linetype(ltype);
  gks_set_fill_int_style(int_style);
  gks_set_fill_color_index(coli);

  if (flag_graphics)
    {
      gr_writestream("<surface nx=\"%d\" ny=\"%d\"", nx, ny);
      print_float_array("x", nx, px);
      print_float_array("y", ny, py);
      print_float_array("z", nx * ny, pz);
      gr_writestream(" option=\"%d\"/>\n", option);
    }

  if (gpx.projection_type == GR_PROJECTION_PERSPECTIVE || gpx.projection_type == GR_PROJECTION_ORTHOGRAPHIC)
    {
      gks_set_window(WC, wn[0], wn[1], wn[2], wn[3]);
      setscale(lx.scale_options);
    }
}

static const double *xp, *yp;

static int compar(const void *a, const void *b)
{
  /* TODO: Implement a version which works for every case. At the moment the GR_PROJECTION_DEFAULT has the problem, that
  the current camera position is not considered. For the other projection types the algorithm has some mistakes at the
  edge of the surface. */
  int ret = -1;
  if (GR_PROJECTION_DEFAULT == gpx.projection_type)
    {
      if (xp[*(int *)a] > xp[*(int *)b]) ret = 1;
      if (yp[*(int *)a] < yp[*(int *)b]) ret = 1;
    }
  else
    {
      if (((triangle_with_distance *)b)->sp > ((triangle_with_distance *)a)->sp)
        {
          ret = 1;
        }
    }
  return ret;
}

/*!
 * Draw a triangular surface plot for the given data points.
 *
 * \param[in] n The number of points
 * \param[in] px A pointer to the X coordinates
 * \param[in] py A pointer to the Y coordinates
 * \param[in] pz A pointer to the Z coordinates
 */
void gr_trisurface(int n, double *px, double *py, double *pz)
{
  int errind, coli, int_style;
  int ntri, *triangles = NULL;
  double x[4], y[4], z[4], meanz;
  int i, j, color;

  if (n < 3)
    {
      fprintf(stderr, "invalid number of points\n");
      return;
    }

  check_autoinit;

  setscale(lx.scale_options);

  /* save fill area interior style and color index */

  gks_inq_fill_int_style(&errind, &int_style);
  gks_inq_fill_color_index(&errind, &coli);

  gks_set_fill_int_style(GKS_K_INTSTYLE_SOLID);

  gr_delaunay(n, px, py, &ntri, &triangles);

  if (gpx.projection_type == GR_PROJECTION_ORTHOGRAPHIC || gpx.projection_type == GR_PROJECTION_PERSPECTIVE)
    {
      triangle_with_distance *ps = (triangle_with_distance *)gks_malloc(ntri * (sizeof(triangle_with_distance)));
      double f[3] = {tx.focus_point_x - tx.camera_pos_x, tx.focus_point_y - tx.camera_pos_y,
                     tx.focus_point_z - tx.camera_pos_z};

      for (i = 0; i < ntri; i++)
        {
          double x_cord, y_cord, z_cord;

          /* simplify the access for each triangle point */
          double xa[3] = {px[triangles[3 * i + 0]], px[triangles[3 * i + 1]], px[triangles[3 * i + 2]]};
          double ya[3] = {py[triangles[3 * i + 0]], py[triangles[3 * i + 1]], py[triangles[3 * i + 2]]};
          double za[3] = {pz[triangles[3 * i + 0]], pz[triangles[3 * i + 1]], pz[triangles[3 * i + 2]]};

          /* calculate the distance of each edge midpoint */
          x_cord = (xa[1] + xa[0]) / 2;
          y_cord = (ya[1] + ya[0]) / 2;
          z_cord = (za[1] + za[0]) / 2;
          double edge_01_distance[3] = {x_cord - tx.camera_pos_x, y_cord - tx.camera_pos_y, z_cord - tx.camera_pos_z};
          double edge_01_scalar = edge_01_distance[0] * f[0] + edge_01_distance[1] * f[1] + edge_01_distance[2] * f[2];

          x_cord = (xa[1] + xa[2]) / 2;
          y_cord = (ya[1] + ya[2]) / 2;
          z_cord = (za[1] + za[2]) / 2;
          double edge_12_distance[3] = {x_cord - tx.camera_pos_x, y_cord - tx.camera_pos_y, z_cord - tx.camera_pos_z};
          double edge_12_scalar = edge_12_distance[0] * f[0] + edge_12_distance[1] * f[1] + edge_12_distance[2] * f[2];

          x_cord = (xa[0] + xa[2]) / 2;
          y_cord = (ya[0] + ya[2]) / 2;
          z_cord = (za[0] + za[2]) / 2;
          double edge_20_distance[3] = {x_cord - tx.camera_pos_x, y_cord - tx.camera_pos_y, z_cord - tx.camera_pos_z};
          double edge_20_scalar = edge_20_distance[0] * f[0] + edge_20_distance[1] * f[1] + edge_20_distance[2] * f[2];

          double nearest_edge_distance = edge_01_scalar;
          if (nearest_edge_distance > edge_12_scalar)
            {
              nearest_edge_distance = edge_12_scalar;
            }
          if (nearest_edge_distance > edge_20_scalar)
            {
              nearest_edge_distance = edge_20_scalar;
            }

          /* create now ntri triangles with the nearest distance attribut */
          ps[i].a = triangles[3 * i + 0];
          ps[i].b = triangles[3 * i + 1];
          ps[i].c = triangles[3 * i + 2];
          ps[i].sp = nearest_edge_distance;
        }

      qsort(ps, ntri, sizeof(triangle_with_distance), compar);
      /* uses the sorted ps to get the sorted triangle which is need for the next steps */
      for (i = 0; i < ntri; i++)
        {
          triangles[3 * i + 0] = ps[i].a;
          triangles[3 * i + 1] = ps[i].b;
          triangles[3 * i + 2] = ps[i].c;
        }
      gks_free(ps);
    }
  else
    {
      xp = px;
      yp = py;

      qsort(triangles, ntri, 3 * sizeof(int), compar);
    }

  for (i = 0; i < ntri; i++)
    {
      meanz = 0.0;
      for (j = 0; j < 3; j++)
        {
          x[j] = x_lin(px[triangles[3 * i + j]]);
          y[j] = y_lin(py[triangles[3 * i + j]]);
          z[j] = z_lin(pz[triangles[3 * i + j]]);
          meanz += z[j];

          apply_world_xform(x + j, y + j, z + j);
        }
      meanz /= 3.0;

      color = iround((meanz - wx.zmin) / (wx.zmax - wx.zmin) * (last_color - first_color)) + first_color;

      if (color < first_color)
        color = first_color;
      else if (color > last_color)
        color = last_color;

      gks_set_fill_color_index(color);
      gks_fillarea(3, x, y);

      x[3] = x[0];
      y[3] = y[0];
      gks_polyline(4, x, y);
    }

  /* restore fill area interior style and color index */
  gks_set_fill_int_style(int_style);
  gks_set_fill_color_index(coli);

  free(triangles);

  if (flag_graphics)
    {
      gr_writestream("<trisurface len=\"%d\"", n);
      print_float_array("x", n, px);
      print_float_array("y", n, py);
      print_float_array("z", n, pz);
      gr_writestream("/>\n");
    }
}

void gr_gradient(int nx, int ny, double *x, double *y, double *z, double *u, double *v)
{
  int im1, i, ip1, jm1, j, jp1;
  double dx, dy, sx, sy;

  if ((nx <= 0) || (ny <= 0))
    {
      fprintf(stderr, "invalid number of points\n");
      return;
    }

  /* be sure that points ordinates are sorted in ascending order */

  for (i = 1; i < nx; i++)
    if (x[i - 1] >= x[i])
      {
        fprintf(stderr, "points not sorted in ascending order\n");
        return;
      }

  for (j = 1; j < ny; j++)
    if (y[j - 1] >= y[j])
      {
        fprintf(stderr, "points not sorted in ascending order\n");
        return;
      }

  check_autoinit;

#define Z(i, j) (z[(nx * (j) + (i))])
#define U(i, j) (u[(nx * (j) + (i))])
#define V(i, j) (v[(nx * (j) + (i))])

  dx = (x[nx - 1] - x[0]) / (nx - 1);
  dy = (y[ny - 1] - y[0]) / (ny - 1);

  for (j = 0; j < ny; j++)
    {
      jm1 = j > 0 ? j - 1 : 0;
      jp1 = j < ny - 1 ? j + 1 : ny - 1;
      sy = j > 0 && j < ny - 1 ? 2 * dy : dy;
      for (i = 0; i < nx; i++)
        {
          im1 = i > 0 ? i - 1 : 0;
          ip1 = i < nx - 1 ? i + 1 : nx - 1;
          sx = i > 0 && i < nx - 1 ? 2 * dx : dx;
          U(i, j) = (Z(ip1, j) - Z(im1, j)) / sx;
          V(i, j) = (Z(i, jp1) - Z(i, jm1)) / sy;
        }
    }

#undef V
#undef U
#undef Z
}

void gr_quiver(int nx, int ny, double *x, double *y, double *u, double *v, int color)
{
  int i, j, ci;
  double gnorm, gmax = 0;
  double dx, dy;
  int errind, linecolor, fillcolor;

  if ((nx <= 0) || (ny <= 0))
    {
      fprintf(stderr, "invalid number of points\n");
      return;
    }

  /* be sure that points ordinates are sorted in ascending order */

  for (i = 1; i < nx; i++)
    if (x[i - 1] >= x[i])
      {
        fprintf(stderr, "points not sorted in ascending order\n");
        return;
      }

  for (j = 1; j < ny; j++)
    if (y[j - 1] >= y[j])
      {
        fprintf(stderr, "points not sorted in ascending order\n");
        return;
      }

  check_autoinit;

  setscale(lx.scale_options);

  /* save line color and fill color */

  gks_inq_pline_color_index(&errind, &linecolor);
  gks_inq_fill_color_index(&errind, &fillcolor);

#define Z(i, j) (z[(nx * (j) + (i))])
#define U(i, j) (u[(nx * (j) + (i))])
#define V(i, j) (v[(nx * (j) + (i))])

  for (j = 0; j < ny; j++)
    for (i = 0; i < nx; i++)
      {
        gmax = max(U(i, j) * U(i, j) + V(i, j) * V(i, j), gmax);
      }
  gmax = sqrt(gmax);

  dx = (x[nx - 1] - x[0]) / (nx - 1);
  dy = (y[ny - 1] - y[0]) / (ny - 1);

  for (j = 0; j < ny; j++)
    for (i = 0; i < nx; i++)
      {
        gnorm = sqrt(U(i, j) * U(i, j) + V(i, j) * V(i, j)) / gmax;
        if (color)
          {
            ci = first_color + (int)((last_color - first_color) * gnorm);
            gr_setlinecolorind(ci);
            gr_setfillcolorind(ci);
          }
        gr_setarrowsize(gnorm);
        gr_drawarrow(x[i], y[j], x[i] + dx * U(i, j) / gmax, y[j] + dy * V(i, j) / gmax);
      }

#undef V
#undef U
#undef Z

  /* restore line color and fill color */

  gks_set_pline_color_index(linecolor);
  gks_set_fill_color_index(fillcolor);

  if (flag_graphics)
    {
      gr_writestream("<quiver nx=\"%d\" ny=\"%d\"", nx, ny);
      print_float_array("x", nx, x);
      print_float_array("y", ny, y);
      print_float_array("u", nx * ny, u);
      print_float_array("v", nx * ny, v);
      gr_writestream(" color=\"%d\"/>\n", color);
    }
}

static int islinspace(int n, double *a)
{
  double step, feps;
  int i;

  if (n < 2) return 0;
  step = (a[n - 1] - a[0]) / (n - 1);
  feps = step * FEPS;
  for (i = 1; i < n; i++)
    if (fabs(a[i] - a[i - 1] - step) > feps) return 0;

  return 1;
}

static void rebin(int nx, int ny, double *px, double *py, double *pz, int *nxq, int *nyq, double **xq, double **yq,
                  double **zq)
{
  double step, *x, *y, *z;
  int i;

  *nxq = 500;
  *nyq = 500;

  x = *xq = (double *)xmalloc(sizeof(double) * *nxq);
  y = *yq = (double *)xmalloc(sizeof(double) * *nyq);
  z = *zq = (double *)xmalloc(sizeof(double) * *nxq * *nyq);

  step = (px[nx - 1] - px[0]) / (*nxq - 1);
  for (i = 0; i < *nxq; i++) x[i] = px[0] + i * step;

  step = (py[ny - 1] - py[0]) / (*nyq - 1);
  for (i = 0; i < *nyq; i++) y[i] = py[0] + i * step;

  gr_interp2(nx, ny, px, py, pz, *nxq, *nyq, x, y, z, 1, 0.0);
}

/*!
 * Draw contours of a three-dimensional data set whose values are specified over a
 rectangular mesh. Contour lines may optionally be labeled.
 *
 * \param[in] nx The number of points along the X axis
 * \param[in] ny The number of points along the Y axis
 * \param[in] nh The number of height values
 * \param[in] px A pointer to the X coordinates
 * \param[in] py A pointer to the Y coordinates
 * \param[in] h A pointer to the height values
 * \param[in] pz A pointer to the Z coordinates
 * \param[in] major_h Directs GR to label contour lines. For example, a value of
 *                    3 would label every third line. A value of 1 will label
 *                    every line. A value of 0 produces no labels. To produce
 *                    colored contour lines, add an offset of 1000 to major_h
 */
void gr_contour(int nx, int ny, int nh, double *px, double *py, double *h, double *pz, int major_h)
{
  int i, j;
  int errind, ltype, color, halign, valign;
  double chux, chuy;
  int nxq, nyq;
  double *xq = NULL, *yq = NULL, *zq = NULL;

  if ((nx <= 0) || (ny <= 0))
    {
      fprintf(stderr, "invalid number of points\n");
      return;
    }

  /* be sure that points ordinates are sorted in ascending order */

  for (i = 1; i < nx; i++)
    if (px[i - 1] >= px[i])
      {
        fprintf(stderr, "points not sorted in ascending order\n");
        return;
      }

  for (j = 1; j < ny; j++)
    if (py[j - 1] >= py[j])
      {
        fprintf(stderr, "points not sorted in ascending order\n");
        return;
      }

  check_autoinit;

  setscale(lx.scale_options);

  /* save linetype, line color, text alignment and character-up vector */

  gks_inq_pline_linetype(&errind, &ltype);
  gks_inq_pline_color_index(&errind, &color);
  gks_inq_text_align(&errind, &halign, &valign);
  gks_inq_text_upvec(&errind, &chux, &chuy);

  gks_set_text_align(GKS_K_TEXT_HALIGN_CENTER, GKS_K_TEXT_VALIGN_HALF);

  if (!islinspace(nx, px) || !islinspace(ny, py))
    {
      rebin(nx, ny, px, py, pz, &nxq, &nyq, &xq, &yq, &zq);

      gr_draw_contours(nxq, nyq, nh, xq, yq, h, zq, major_h);

      free(zq);
      free(yq);
      free(xq);
    }
  else
    gr_draw_contours(nx, ny, nh, px, py, h, pz, major_h);

  /* restore linetype, line color, character-up vector and text alignment */

  gks_set_pline_linetype(ltype);
  gks_set_pline_color_index(color);
  gks_set_text_align(halign, valign);
  gks_set_text_upvec(chux, chuy);

  if (flag_graphics)
    {
      gr_writestream("<contour nx=\"%d\" ny=\"%d\" nh=\"%d\"", nx, ny, nh);
      print_float_array("x", nx, px);
      print_float_array("y", ny, py);
      print_float_array("h", nh, h);
      print_float_array("z", nx * ny, pz);
      gr_writestream(" majorh=\"%d\"/>\n", major_h);
    }
}

/*!
 * Draw filled contour plot of a three-dimensional data set whose values are
 * specified over a rectangular mesh.
 *
 * \param[in] nx The number of points along the X axis
 * \param[in] ny The number of points along the Y axis
 * \param[in] nh The number of height values. 0 for default contours.
 * \param[in] px A pointer to the X coordinates
 * \param[in] py A pointer to the Y coordinates
 * \param[in] h A pointer to the height values. If NULL, use nh evenly distributed height values between
 *                      minimum and maximum Z value.
 * \param[in] pz A pointer to the Z coordinates
 * \param[in] major_h Directs GR to label contour lines. For example, a value of
 *                    3 would label every third line. A value of 1 will label
 *                    every line. A value of 0 produces no labels. To produce
 *                    colored contour lines, add an offset of 1000 to major_h
 */
void gr_contourf(int nx, int ny, int nh, double *px, double *py, double *h, double *pz, int major_h)
{
  int i, j;
  int errind;
  int nxq, nyq;
  int fillintstyle, fillcolorind;
  double *xq = NULL, *yq = NULL, *zq = NULL;

  if ((nx <= 0) || (ny <= 0))
    {
      fprintf(stderr, "invalid number of points\n");
      return;
    }

  /* be sure that points ordinates are sorted in ascending order */

  for (i = 1; i < nx; i++)
    if (px[i - 1] >= px[i])
      {
        fprintf(stderr, "points not sorted in ascending order\n");
        return;
      }

  for (j = 1; j < ny; j++)
    if (py[j - 1] >= py[j])
      {
        fprintf(stderr, "points not sorted in ascending order\n");
        return;
      }

  if (nh > 0 && h)
    {
      for (i = 1; i < nh; i++)
        if (h[i - 1] >= h[i])
          {
            fprintf(stderr, "contours not sorted in ascending order\n");
            return;
          }
    }

  check_autoinit;

  setscale(lx.scale_options);

  /* save fill style and color */

  gks_inq_fill_style_index(&errind, &fillintstyle);
  gks_inq_fill_color_index(&errind, &fillcolorind);

  if (!islinspace(nx, px) || !islinspace(ny, py))
    {
      rebin(nx, ny, px, py, pz, &nxq, &nyq, &xq, &yq, &zq);

      gr_draw_contourf(nxq, nyq, nh, xq, yq, h, zq, first_color, last_color, major_h);

      free(zq);
      free(yq);
      free(xq);
    }
  else
    gr_draw_contourf(nx, ny, nh, px, py, h, pz, first_color, last_color, major_h);

  /* restore fill style and color */

  gks_set_fill_style_index(fillintstyle);
  gks_set_fill_color_index(fillcolorind);

  if (flag_graphics)
    {
      gr_writestream("<contourf nx=\"%d\" ny=\"%d\" nh=\"%d\"", nx, ny, nh);
      print_float_array("x", nx, px);
      print_float_array("y", ny, py);
      print_float_array("h", nh, h);
      print_float_array("z", nx * ny, pz);
      gr_writestream(" majorh=\"%d\"/>\n", major_h);
    }
}

/*!
 * Draw a contour plot for the given triangle mesh.
 *
 * \param[in] npoints The number of points
 * \param[in] x A pointer to the X coordinates
 * \param[in] y A pointer to the Y coordinates
 * \param[in] z A pointer to the Z coordinates
 * \param[in] nlevels The number of contour levels
 * \param[in] levels A pointer to the contour levels
 */
void gr_tricontour(int npoints, double *x, double *y, double *z, int nlevels, double *levels)
{
  int i, *colors;

  if (npoints < 3)
    {
      fprintf(stderr, "invalid number of points\n");
      return;
    }

  if (nlevels < 1)
    {
      fprintf(stderr, "invalid number of iso levels\n");
      return;
    }

  check_autoinit;

  setscale(lx.scale_options);

  colors = (int *)xmalloc(nlevels * sizeof(int));
  for (i = 0; i < nlevels; i++) colors[i] = (int)((double)i / (nlevels - 1) * (last_color - first_color)) + first_color;

  gr_draw_tricont(npoints, x, y, z, nlevels, levels, colors);

  free(colors);

  if (flag_graphics)
    {
      gr_writestream("<tricont npoints=\"%d\"", npoints);
      print_float_array("x", npoints, x);
      print_float_array("y", npoints, y);
      print_float_array("z", npoints, z);
      print_float_array("levels", nlevels, levels);
      gr_writestream("/>\n");
    }
}

static int binning(double x[], double y[], int *cell, int *cnt, double size, double shape, double rx[2], double ry[2],
                   int bnd[2], int n, double ycorr)
{
  int nc;
  double xi, yi;
  int i, i1, i2, iinc;
  int j1, j2, jinc;
  int L, lmax, lat;
  double c1, c2, con1, con2, dist1;
  double sx, sy, xmin, ymin, xr, yr;

  xmin = rx[0];
  ymin = ry[0] + ycorr;
  xr = rx[1] - xmin;
  yr = ry[1] + ycorr - ymin;
  c1 = size / xr;
  c2 = size * shape / (yr * sqrt(3.));

  jinc = bnd[1];
  lat = jinc + 1;
  iinc = 2 * jinc;
  lmax = bnd[0] * bnd[1];
  con1 = 0.25;
  con2 = 1. / 3.;

  for (i = 0; i < n; i++)
    {
      xi = x[i];
      yi = y[i];
      gr_wctondc(&xi, &yi);
      if (xi < vxmin || xi > vxmax || yi < vymin || yi > vymax)
        {
          continue;
        }
      sx = c1 * (xi - xmin);
      sy = c2 * (yi - ymin);
      j1 = sx + 0.5;
      i1 = sy + 0.5;
      dist1 = pow((sx - j1), 2) + 3.0 * pow((sy - i1), 2);
      if (dist1 < con1)
        L = i1 * iinc + j1 + 1;
      else if (dist1 > con2)
        L = (int)sy * iinc + (int)sx + lat;
      else
        {
          j2 = sx;
          i2 = sy;
          if (dist1 <= pow((sx - j2 - 0.5), 2) + 3.0 * pow((sy - i2 - 0.5), 2))
            L = i1 * iinc + j1 + 1;
          else
            L = i2 * iinc + j2 + lat;
        }

      cnt[L] = cnt[L] + 1;
    }

  nc = 0;
  for (L = 1; L <= lmax; L++)
    {
      if (cnt[L] > 0)
        {
          nc++;
          cell[nc] = L;
          cnt[nc] = cnt[L];
        }
    }
  bnd[0] = (cell[nc] - 1) / bnd[1] + 1;
  return nc;
}

static int hcell2xy(int nbins, double rx[2], double ry[2], double shape, int bnd[2], int *cell, double *x, double *y,
                    int *cnt, double ycorr)
{
  double c3, c4, tmp;
  int jmax, lmax, L;
  int cntmax;

  c3 = (rx[1] - rx[0]) / nbins;
  c4 = ((ry[1] - ry[0]) * sqrt(3)) / (2 * shape * nbins);
  jmax = bnd[1];
  lmax = bnd[0] * bnd[1];

  cntmax = 0;
  for (L = 0; L <= lmax; L++)
    {
      y[L] = c4 * ((cell[L] - 1) / jmax) + ry[0] + ycorr;
      tmp = ((cell[L] - 1) / jmax) % 2 == 0 ? ((cell[L] - 1) % jmax) : ((cell[L] - 1) % jmax + 0.5);
      x[L] = c3 * tmp + rx[0];
      if (cnt[L] > cntmax) cntmax = cnt[L];
    }
  return cntmax;
}

int gr_hexbin(int n, double *x, double *y, int nbins)
{
  int errind, int_style, coli;
  int jmax, c1, imax, lmax;
  double size, shape;
  double d, R;
  double ycorr;
  int *cell, *cnt;
  double *xcm, *ycm;
  double rx[2], ry[2];
  int bnd[2];
  int nc, cntmax;
  int i, j;
  double xlist[7], ylist[7], xdelta[6], ydelta[6];

  if (n <= 2)
    {
      fprintf(stderr, "invalid number of points\n");
      return -1;
    }
  else if (nbins <= 2)
    {
      fprintf(stderr, "invalid number of bins\n");
      return -1;
    }

  check_autoinit;

  setscale(lx.scale_options);

  /* save fill area interior style and color index */

  gks_inq_fill_int_style(&errind, &int_style);
  gks_inq_fill_color_index(&errind, &coli);

  size = nbins;
  shape = (vymax - vymin) / (vxmax - vxmin);

  jmax = floor(nbins + 1.5001);
  c1 = 2 * floor((nbins * shape) / sqrt(3) + 1.5001);
  imax = floor((jmax * c1 - 1) / jmax + 1);
  lmax = jmax * imax;

  d = (vxmax - vxmin) / nbins;
  R = 1. / sqrt(3) * d;

  ycorr = (vymax - vymin) - ((imax - 2) * 1.5 * R + (imax % 2) * R);
  ycorr = ycorr / 2;

  cell = (int *)calloc(lmax + 1, sizeof(int));
  cnt = (int *)calloc(lmax + 1, sizeof(int));
  xcm = (double *)calloc(lmax + 1, sizeof(double));
  ycm = (double *)calloc(lmax + 1, sizeof(double));

  rx[0] = vxmin;
  rx[1] = vxmax;
  ry[0] = vymin;
  ry[1] = vymax;

  bnd[0] = imax;
  bnd[1] = jmax;

  nc = binning(x, y, cell, cnt, size, shape, rx, ry, bnd, n, ycorr);

  cntmax = hcell2xy(nbins, rx, ry, shape, bnd, cell, xcm, ycm, cnt, ycorr);

  for (j = 0; j < 6; j++)
    {
      xdelta[j] = sin(M_PI / 3 * j) * R;
      ydelta[j] = cos(M_PI / 3 * j) * R;
    }

  gks_set_fill_int_style(GKS_K_INTSTYLE_SOLID);

  for (i = 1; i <= nc; i++)
    {
      for (j = 0; j < 6; j++)
        {
          xlist[j] = xcm[i] + xdelta[j];
          ylist[j] = ycm[i] + ydelta[j];
          gr_ndctowc(xlist + j, ylist + j);
        }
      xlist[6] = xlist[0];
      ylist[6] = ylist[0];

      gks_set_fill_color_index(first_color + (last_color - first_color) * ((double)cnt[i] / cntmax));
      gks_fillarea(6, xlist, ylist);
      gks_polyline(7, xlist, ylist);
    }

  free(ycm);
  free(xcm);
  free(cnt);
  free(cell);

  /* restore fill area interior style and color index */

  gks_set_fill_int_style(int_style);
  gks_set_fill_color_index(coli);

  if (flag_graphics)
    {
      gr_writestream("<hexbin len=\"%d\"", n);
      print_float_array("x", n, x);
      print_float_array("y", n, y);
      gr_writestream(" nbins=\"%d\"/>\n", nbins);
    }

  return cntmax;
}

/*!
 * Set the currently used colormap.
 *
 * \param[in] index Colormap index
 *
 * A list of colormaps can be found at: https://gr-framework.org/colormaps.html
 *
 * Using a negative index will use the reverse of the selected colormap.
 */
void gr_setcolormap(int index)
{
  int ind = index, reverse, i, j;
  double r, g, b;

  colormap = index;

  check_autoinit;

  reverse = 0;
  r = g = b = 0;
  if (ind < 0)
    {
      ind = -ind;
      reverse = 1;
    }

  if (ind >= 100)
    {
      ind %= 100;
      first_color = 1000;
      last_color = 1255;
    }
  else
    {
      first_color = DEFAULT_FIRST_COLOR;
      last_color = DEFAULT_LAST_COLOR;
    }

  if (ind >= sizeof(cmap) / sizeof(cmap[0])) ind = 0;

  for (i = 0; i <= DEFAULT_LAST_COLOR - DEFAULT_FIRST_COLOR; i++)
    {
      j = reverse ? DEFAULT_LAST_COLOR - DEFAULT_FIRST_COLOR - 1 - i : i;
      r = ((cmap[ind][j] >> 16) & 0xff) / 255.0;
      g = ((cmap[ind][j] >> 8) & 0xff) / 255.0;
      b = (cmap[ind][j] & 0xff) / 255.0;

      setcolorrep(DEFAULT_FIRST_COLOR + i, r, g, b);
    }

  for (i = 0; i < 256; i++)
    {
      j = reverse ? 255 - i : i;
      r = ((cmap_h[ind][j] >> 16) & 0xff) / 255.0;
      g = ((cmap_h[ind][j] >> 8) & 0xff) / 255.0;
      b = (cmap_h[ind][j] & 0xff) / 255.0;

      setcolorrep(1000 + i, r, g, b);
    }

  if (flag_graphics) gr_writestream("<setcolormap index=\"%d\"/>\n", index);
}

void gr_inqcolormap(int *index)
{
  check_autoinit;

  *index = colormap;
}

/*!
 * Define a colormap by a list of RGB colors.
 *
 * \param[in] n The number of colors
 * \param[in] r A pointer to the red intensities in range 0.0 to 1.0
 * \param[in] g A pointer to the green intensities in range 0.0 to 1.0
 * \param[in] b A pointer to the blue intensities in range 0.0 to 1.0
 * \param[in] x A pointer to the positions of the corresponding color in the resulting colormap or NULL. The values of
 * x must increase monotonically from 0.0 to 1.0. If x is NULL the given colors are evenly distributed in the colormap.
 *
 * This function defines a colormap using the n given color intensities. If less than 256 colors are provided the
 * colors intensities are linear interpolated. If x is NULL the given color values are evenly distributed in the
 * colormap. Otherwise the normalized value of x defines the position of the color in the colormap.
 */
void gr_setcolormapfromrgb(int n, double *r, double *g, double *b, double *x)
{
  int i, j;
  int start_index, end_index;

  check_autoinit;

  if (n < 2)
    {
      fprintf(stderr, "Not enough colors provided.\n");
      return;
    }

  /* make sure x is in ascending order starting at 0 and ending at 1 */
  if (x != NULL)
    {
      if (x[0] != 0.0)
        {
          fprintf(stderr, "x must start at 0.0\n");
          return;
        }
      if (x[n - 1] != 1.0)
        {
          fprintf(stderr, "x must end at 1.0\n");
          return;
        }
      for (i = 0; i < n - 1; i++)
        {
          if (x[i] >= x[i + 1])
            {
              fprintf(stderr, "x not sorted in ascending order\n");
              return;
            }
        }
    }
  for (i = 0; i < n - 1; i++)
    {
      if (x == NULL)
        {
          start_index = nint(i * 256. / (n - 1));
          end_index = nint((i + 1) * 256. / (n - 1));
        }
      else
        {
          start_index = nint(x[i] * 256);
          end_index = nint(x[i + 1] * 256);
        }
      for (j = start_index; j < end_index; j++)
        {
          double a = ((double)j - start_index) / (end_index - start_index);
          double rj = r[i] * (1 - a) + r[i + 1] * a;
          double gj = g[i] * (1 - a) + g[i + 1] * a;
          double bj = b[i] * (1 - a) + b[i + 1] * a;
          gr_setcolorrep(1000 + j, rj, gj, bj);
        }
    }
}

void gr_colorbar(void)
{
  int errind, halign, valign, clsw, tnr;
  double clrt[4], wn[4], vp[4];
  double xmin, xmax, ymin, ymax, zmin, zmax;
  int w, h, sx, sy, nx, ny, colia[256];
  int i, nz, ci, cells;
  double x, y, z, dy, dz;
  char text[256];

  check_autoinit;

  setscale(lx.scale_options);

  /* save text alignment and clipping indicator */

  gks_inq_text_align(&errind, &halign, &valign);
  gks_inq_clip(&errind, &clsw, clrt);

  /* inquire current normalization transformation */

  gks_inq_current_xformno(&errind, &tnr);
  gks_inq_xform(tnr, &errind, wn, vp);

  cells = last_color - first_color + 1;
  for (ci = first_color; ci <= last_color; ci++) colia[ci - first_color] = ci;

  xmin = lx.xmin;
  xmax = lx.xmax;
  ymin = lx.ymin;
  ymax = lx.ymax;
  zmin = wx.zmin;
  zmax = wx.zmax;

  w = nx = 1;
  h = ny = cells;
  sx = sy = 1;

  gks_cellarray(xmin, ymin, xmax, ymax, w, h, sx, sy, nx, ny, colia);

  dz = 0.5 * gr_tick(zmin, zmax);
  nz = (int)((zmax - zmin) / dz + 0.5);
  dy = (ymax - ymin) / nz;

  gks_set_text_align(GKS_K_TEXT_HALIGN_LEFT, GKS_K_TEXT_VALIGN_HALF);
  gks_set_clipping(GKS_K_NOCLIP);

  x = xmax + 0.01 * (xmax - xmin) / (vp[1] - vp[0]);
  for (i = 0; i <= nz; i++)
    {
      y = ymin + i * dy;
      z = zmin + i * dz;
      text2d(x, y, gr_ftoa(text, z, dz));
    }

  /* restore text alignment and clipping indicator */

  gks_set_text_align(halign, valign);
  gks_set_clipping(clsw);

  if (flag_graphics) gr_writestream("<colorbar/>\n");
}

void gr_inqcolor(int color, int *rgb)
{
  int wkid = 1, errind;
  double r, g, b;

  check_autoinit;

  gks_inq_color_rep(wkid, color, GKS_K_VALUE_SET, &errind, &r, &g, &b);
  *rgb = ((nint(r * 255) & 0xff)) | ((nint(g * 255) & 0xff) << 8) | ((nint(b * 255) & 0xff) << 16);
}

int gr_inqcolorfromrgb(double red, double green, double blue)
{
  int wkid = 1, color, errind, ind = 0;
  unsigned int rgbmask;
  double r, g, b, dmin = FLT_MAX, d, dr, dg, db;

  check_autoinit;

  rgbmask = ((nint(red * 255) & 0xff)) | ((nint(green * 255) & 0xff) << 8) | ((nint(blue * 255) & 0xff) << 16);

  for (color = 80; color < 980; color++)
    if (rgb[color] == rgbmask)
      {
        setcolorrep(color, red, green, blue);
        used[color] = 1;
        return color;
      }

  for (color = 80; color < 980; color++)
    {
      if (!used[color])
        {
          setcolorrep(color, red, green, blue);
          used[color] = 1;
          return color;
        }
    }

  for (color = 80; color < 980; color++)
    {
      gks_inq_color_rep(wkid, color, GKS_K_VALUE_SET, &errind, &r, &g, &b);
      dr = 0.30 * (r - red);
      dg = 0.59 * (g - green);
      db = 0.11 * (b - blue);
      d = dr * dr + dg * dg + db * db;
      if (d < dmin)
        {
          ind = color;
          dmin = d;
          if (d < FEPS) break;
        }
    }

  return ind;
}

void gr_hsvtorgb(double h, double s, double v, double *r, double *g, double *b)
{
  int i;
  double f, p, q, t;

  if (s == 0)
    {
      *r = *g = *b = v;
    }
  else
    {
      h *= 6;
      i = floor(h);
      f = h - i;
      p = v * (1 - s);
      q = v * (1 - s * f);
      t = v * (1 - s * (1 - f));

      switch (i)
        {
        case 0:
          *r = v;
          *g = t;
          *b = p;
          break;
        case 1:
          *r = q;
          *g = v;
          *b = p;
          break;
        case 2:
          *r = p;
          *g = v;
          *b = t;
          break;
        case 3:
          *r = p;
          *g = q;
          *b = v;
          break;
        case 4:
          *r = t;
          *g = p;
          *b = v;
          break;
        case 5:
          *r = v;
          *g = p;
          *b = q;
          break;
        }
    }
}

double gr_tick(double amin, double amax)
{
  double exponent, fractpart, intpart, factor, tick_unit;
  int n;

  if (amax > amin)
    {
      exponent = log10(amax - amin);
      fractpart = modf(exponent, &intpart);
      GR_UNUSED(fractpart); /* silence compiler warning */
      n = (int)intpart;

      factor = pow(10.0, exponent - n);

      if (factor > 5)
        tick_unit = 2;
      else if (factor > 2.5)
        tick_unit = 1;
      else if (factor > 1)
        tick_unit = 0.5;
      else if (factor > 0.5)
        tick_unit = 0.2;
      else if (factor > 0.25)
        tick_unit = 0.1;
      else
        tick_unit = 0.05;

      tick_unit = tick_unit * pow(10.0, (double)n);
    }
  else
    {
      fprintf(stderr, "invalid range\n");
      tick_unit = 0;
    }

  return (tick_unit); /* return a tick unit that evenly divides into the
                         difference between the minimum and maximum value */
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

int gr_validaterange(double amin, double amax)
{
  /* Check whether the given coordinate range does not lead
     to loss of precision in subsequent GR functions, e.g.
     when applying normalization or device transformations. */
  return check_range(amin, amax);
}

#ifdef _MSC_VER
#if _MSC_VER < 1700
static double trunc(double d)
{
  return (d > 0) ? floor(d) : ceil(d);
}
#endif
#endif

void gr_adjustrange(double *amin, double *amax)
{
  double tick;

  if (*amin == *amax)
    {
      if (*amin != 0)
        tick = pow(10.0, trunc(log10(fabs(*amin))));
      else
        tick = 0.1;

      *amin = *amin - tick;
      *amax = *amax + tick;
    }

  tick = gr_tick(*amin, *amax);

  if (fract(*amin / tick) != 0) *amin = tick * gauss(*amin / tick);

  if (fract(*amax / tick) != 0) *amax = tick * (gauss(*amax / tick) + 1);
}

static int gks_wstype(char *type)
{
  int wstype;

  if (!str_casecmp(type, "ps") || !str_casecmp(type, "eps"))
    wstype = 62;
  else if (!str_casecmp(type, "pdf"))
    wstype = 102;
  else if (!str_casecmp(type, "mov"))
    wstype = 120;
  else if (!str_casecmp(type, "gif"))
    wstype = 130;
  else if (!str_casecmp(type, "bmp"))
    wstype = 145;
  else if (!str_casecmp(type, "jpeg") || !str_casecmp(type, "jpg"))
    wstype = 144;
  else if (!str_casecmp(type, "png"))
    wstype = 140;
  else if (!str_casecmp(type, "mem"))
    wstype = 143;
  else if (!str_casecmp(type, "mp4"))
    wstype = 160;
  else if (!str_casecmp(type, "webm"))
    wstype = 161;
  else if (!str_casecmp(type, "ogg"))
    wstype = 162;
  else if (!str_casecmp(type, "tiff") || !str_casecmp(type, "tif"))
    wstype = 146;
  else if (!str_casecmp(type, "svg"))
    wstype = 382;
  else if (!str_casecmp(type, "wmf"))
    wstype = 390;
  else if (!str_casecmp(type, "pgf"))
    wstype = 314;
  else
    {
      fprintf(stderr, "%s: unrecognized file type\nAvailable formats: \
bmp, eps, jpeg, mov, mp4, webm, ogg, pdf, pgf, png, ps, svg, tiff or wmf\n",
              type);
      wstype = -1;
    }

  if (wstype == 145 && gks_getenv("GKS_USE_GS_BMP") != NULL) wstype = 320;

  if (wstype == 144 && gks_getenv("GKS_USE_GS_JPG") != NULL) wstype = 321;

  if (wstype == 140 && gks_getenv("GKS_USE_GS_PNG") != NULL) wstype = 322;

  if (wstype == 146 && gks_getenv("GKS_USE_GS_TIF") != NULL) wstype = 323;

  return wstype;
}

/*!
 * Open and activate a print device.
 *
 * \param[in] pathname Filename for the print device.
 *
 * This function opens an additional graphics output device. The device type is
 * obtained from the given file extension. The following file types are
 * supported:
 *
 * \verbatim embed:rst:leading-asterisk
 *
 * The following file types are supported:
 *
 * +-------------+---------------------------------------+
 * |.ps, .eps    |PostScript                             |
 * +-------------+---------------------------------------+
 * |.pdf         |Portable Document Format               |
 * +-------------+---------------------------------------+
 * |.bmp         |Windows Bitmap (BMP)                   |
 * +-------------+---------------------------------------+
 * |.jpeg, .jpg  |JPEG image file                        |
 * +-------------+---------------------------------------+
 * |.png         |Portable Network Graphics file (PNG)   |
 * +-------------+---------------------------------------+
 * |.tiff, .tif  |Tagged Image File Format (TIFF)        |
 * +-------------+---------------------------------------+
 * |.svg         |Scalable Vector Graphics               |
 * +-------------+---------------------------------------+
 * |.wmf         |Windows Metafile                       |
 * +-------------+---------------------------------------+
 * |.mp4         |MPEG-4 video file                      |
 * +-------------+---------------------------------------+
 * |.webm        |WebM video file                        |
 * +-------------+---------------------------------------+
 * |.ogg         |Ogg video file                         |
 * +-------------+---------------------------------------+
 *
 * \endverbatim
 */
void gr_beginprint(char *pathname)
{
  int wkid = 6, wstype = 62;
  char *type;

  check_autoinit;

  if (!flag_printing)
    {
      if ((type = strrchr(pathname, '.')) != NULL) wstype = gks_wstype(type + 1);

      if (wstype >= 0)
        {
          gks_open_ws(wkid, pathname, wstype);
          gks_activate_ws(wkid);
          flag_printing = 1;
        }
    }
  else
    fprintf(stderr, "print device already activated\n");
}

/*!
 * Open and activate a print device with the given layout attributes.
 *
 * \param[in] pathname Filename for the print device.
 * \param[in] mode Output mode (Color, GrayScale)
 * \param[in] format Output format (see table)
 * \param[in] orientation Page orientation (Landscape, Portait)
 *
 * \verbatim embed:rst:leading-asterisk
 *
 * The available formats are:
 *
 * +-----------+---------------+
 * |A4         |0.210 x 0.297  |
 * +-----------+---------------+
 * |B5         |0.176 x 0.250  |
 * +-----------+---------------+
 * |Letter     |0.216 x 0.279  |
 * +-----------+---------------+
 * |Legal      |0.216 x 0.356  |
 * +-----------+---------------+
 * |Executive  |0.191 x 0.254  |
 * +-----------+---------------+
 * |A0         |0.841 x 1.189  |
 * +-----------+---------------+
 * |A1         |0.594 x 0.841  |
 * +-----------+---------------+
 * |A2         |0.420 x 0.594  |
 * +-----------+---------------+
 * |A3         |0.297 x 0.420  |
 * +-----------+---------------+
 * |A5         |0.148 x 0.210  |
 * +-----------+---------------+
 * |A6         |0.105 x 0.148  |
 * +-----------+---------------+
 * |A7         |0.074 x 0.105  |
 * +-----------+---------------+
 * |A8         |0.052 x 0.074  |
 * +-----------+---------------+
 * |A9         |0.037 x 0.052  |
 * +-----------+---------------+
 * |B0         |1.000 x 1.414  |
 * +-----------+---------------+
 * |B1         |0.500 x 0.707  |
 * +-----------+---------------+
 * |B10        |0.031 x 0.044  |
 * +-----------+---------------+
 * |B2         |0.500 x 0.707  |
 * +-----------+---------------+
 * |B3         |0.353 x 0.500  |
 * +-----------+---------------+
 * |B4         |0.250 x 0.353  |
 * +-----------+---------------+
 * |B6         |0.125 x 0.176  |
 * +-----------+---------------+
 * |B7         |0.088 x 0.125  |
 * +-----------+---------------+
 * |B8         |0.062 x 0.088  |
 * +-----------+---------------+
 * |B9         |0.044 x 0.062  |
 * +-----------+---------------+
 * |C5E        |0.163 x 0.229  |
 * +-----------+---------------+
 * |Comm10E    |0.105 x 0.241  |
 * +-----------+---------------+
 * |DLE        |0.110 x 0.220  |
 * +-----------+---------------+
 * |Folio      |0.210 x 0.330  |
 * +-----------+---------------+
 * |Ledger     |0.432 x 0.279  |
 * +-----------+---------------+
 * |Tabloid    |0.279 x 0.432  |
 * +-----------+---------------+
 *
 * \endverbatim
 */
void gr_beginprintext(char *pathname, char *mode, char *format, char *orientation)
{
  int wkid = 6, wstype = 62;
  char *type;
  double width = 0.210, height = 0.297;
  format_t *p = formats;
  int color = 0, landscape = 0;

  check_autoinit;

  if (!flag_printing)
    {
      if ((type = strrchr(pathname, '.')) != NULL) wstype = gks_wstype(type + 1);

      if (wstype >= 0)
        {
          if (str_casecmp(mode, "Color") == 0)
            color = 1;
          else if (str_casecmp(mode, "GrayScale") != 0)
            fprintf(stderr, "%s: invalid color mode\n", mode);

          while (p->format != NULL)
            {
              if (str_casecmp(p->format, format) == 0)
                {
                  width = p->width * 0.9;
                  height = p->height * 0.9;
                  break;
                }
              p++;
            }
          if (p->format == NULL) fprintf(stderr, "%s: invalid page size\n", format);

          if (str_casecmp(orientation, "Landscape") == 0)
            landscape = 1;
          else if (str_casecmp(orientation, "Portrait") != 0)
            fprintf(stderr, "%s: invalid page orientation\n", orientation);

          if (wstype == 62)
            {
              if (!color) wstype -= 1;
              if (landscape) wstype += 2;
            }

          gks_open_ws(wkid, pathname, wstype);
          gks_activate_ws(wkid);

          if (!landscape)
            {
              gks_set_ws_viewport(wkid, 0.0, width, 0.0, height);
              if (width < height)
                gks_set_ws_window(wkid, 0.0, width / height, 0.0, 1.0);
              else
                gks_set_ws_window(wkid, 0.0, 1.0, 0.0, height / width);
            }
          else
            {
              gks_set_ws_viewport(wkid, 0.0, height, 0.0, width);
              if (width < height)
                gks_set_ws_window(wkid, 0.0, 1.0, 0.0, width / height);
              else
                gks_set_ws_window(wkid, 0.0, height / width, 0.0, 1.0);
            }

          flag_printing = 1;
        }
    }
  else
    fprintf(stderr, "print device already activated\n");
}

void gr_endprint(void)
{
  int wkid = 6;

  if (flag_printing)
    {
      if (!autoinit) /* GKS may have been closed by gr_emergencyclosegks() */
        {
          gks_deactivate_ws(wkid);
          gks_close_ws(wkid);
        }
      flag_printing = 0;
    }
  else
    fprintf(stderr, "no print device activated\n");
}

void gr_ndctowc(double *x, double *y)
{
  check_autoinit;

  *x = x_log((*x - nx.b) / nx.a);
  *y = y_log((*y - nx.d) / nx.c);
}

void gr_wctondc(double *x, double *y)
{
  check_autoinit;

  *x = nx.a * x_lin(*x) + nx.b;
  *y = nx.c * y_lin(*y) + nx.d;
}

void gr_wc3towc(double *x, double *y, double *z)
{
  check_autoinit;

  apply_world_xform(x, y, z);
}

/*!
 * Draw a rectangle using the current line attributes.
 *
 * \param[in] xmin Left edge of the rectangle
 * \param[in] xmax Right edge of the rectangle
 * \param[in] ymin Bottom edge of the rectangle
 * \param[in] ymax Upper edge of the rectangle
 */
void gr_drawrect(double xmin, double xmax, double ymin, double ymax)
{
  int errind, style;
  double x[5], y[5];
  int codes[] = {'M', 'L', 'L', 'L', 'S'};

  check_autoinit;

  gks_inq_fill_int_style(&errind, &style);

  if (style != GKS_K_INTSTYLE_SOLID_WITH_BORDER)
    {
      x[0] = x[3] = min(xmin, xmax);
      x[1] = x[2] = max(xmin, xmax);
      x[4] = x[0];
      y[0] = y[1] = min(ymin, ymax);
      y[2] = y[3] = max(ymin, ymax);
      y[4] = y[0];

      polyline(5, x, y);
    }
  else
    {
      x[1] = x[2] = x_lin(max(xmin, xmax));
      x[0] = x[3] = x_lin(min(xmin, xmax));
      y[2] = y[3] = y_lin(max(ymin, ymax));
      y[0] = y[1] = y_lin(min(ymin, ymax));

      gks_gdp(4, x, y, GKS_K_GDP_DRAW_PATH, 5, codes);
    }

  if (flag_graphics)
    gr_writestream("<drawrect xmin=\"%g\" xmax=\"%g\" ymin=\"%g\" ymax=\"%g\"/>\n", xmin, xmax, ymin, ymax);
}

/*!
 * Draw a filled rectangle using the current fill attributes.
 *
 * \param[in] xmin Left edge of the rectangle
 * \param[in] xmax Right edge of the rectangle
 * \param[in] ymin Bottom edge of the rectangle
 * \param[in] ymax Upper edge of the rectangle
 */
void gr_fillrect(double xmin, double xmax, double ymin, double ymax)
{
  int errind, style;
  double bwidth, x[4], y[4];
  int codes[] = {'M', 'L', 'L', 'L', 'f'};

  check_autoinit;

  gks_inq_fill_int_style(&errind, &style);

  if (style != GKS_K_INTSTYLE_SOLID_WITH_BORDER)
    {
      x[0] = x[3] = min(xmin, xmax);
      x[1] = x[2] = max(xmin, xmax);
      y[0] = y[1] = min(ymin, ymax);
      y[2] = y[3] = max(ymin, ymax);

      fillarea(4, x, y);
    }
  else
    {
      x[0] = min(x_lin(xmin), x_lin(xmax));
      y[0] = min(y_lin(ymin), y_lin(ymax));
      x[1] = max(x_lin(xmin), x_lin(xmax));
      y[1] = max(y_lin(ymin), y_lin(ymax));

      x[1] = x[2] = x_lin(max(xmin, xmax));
      x[0] = x[3] = x_lin(min(xmin, xmax));
      y[2] = y[3] = y_lin(max(ymin, ymax));
      y[0] = y[1] = y_lin(min(ymin, ymax));

      gr_inqborderwidth(&bwidth);
      if (bwidth != 0) codes[4] = 'F';

      gks_gdp(4, x, y, GKS_K_GDP_DRAW_PATH, 5, codes);
    }

  if (flag_graphics)
    gr_writestream("<fillrect xmin=\"%g\" xmax=\"%g\" ymin=\"%g\" ymax=\"%g\"/>\n", xmin, xmax, ymin, ymax);
}

/*!
 * Draw a circular or elliptical arc covering the specified rectangle.
 *
 * \param[in] xmin Left edge of the rectangle
 * \param[in] xmax Right edge of the rectangle
 * \param[in] ymin Bottom edge of the rectangle
 * \param[in] ymax Upper edge of the rectangle
 * \param[in] a1 The start angle
 * \param[in] a2 The end angle
 *
 * The resulting arc begins at a1 and ends at a2 degrees. Angles are
 * interpreted such that 0 degrees is at the 3 o'clock position. The center of
 * the arc is the center of the given rectangle.
 */
void gr_drawarc(double xmin, double xmax, double ymin, double ymax, double a1, double a2)
{
  int errind, style;
  double xcenter, ycenter, width, height, start, end, a;
  int n;
  double x[361], y[361];
  int codes[3] = {'M', 'A', 'S'};

  check_autoinit;

  gks_inq_fill_int_style(&errind, &style);

  xcenter = (x_lin(xmin) + x_lin(xmax)) / 2.0;
  ycenter = (y_lin(ymin) + y_lin(ymax)) / 2.0;
  width = fabs(x_lin(xmax) - x_lin(xmin)) / 2.0;
  height = fabs(y_lin(ymax) - y_lin(ymin)) / 2.0;

  if (style != GKS_K_INTSTYLE_SOLID_WITH_BORDER)
    {
      start = min(a1, a2);
      end = max(a1, a2);
      start += ((int)(end - start)) / 360 * 360;
      /* Ensure that two equivalent but unequal angles result in a full arc. */
      if (fabs(end - start) < FEPS && fabs(a1 - a2) > FEPS)
        {
          end += 360;
        }

      n = 0;
      for (a = start; a <= end; a++)
        {
          x[n] = x_log(xcenter + width * cos(a * M_PI / 180));
          y[n] = y_log(ycenter + height * sin(a * M_PI / 180));
          n++;
        }
      if (fabs((a - 1) - end) > FEPS)
        {
          x[n] = x_log(xcenter + width * cos(end * M_PI / 180));
          y[n] = y_log(ycenter + height * sin(end * M_PI / 180));
          n++;
        }

      if (n > 1)
        {
          polyline(n, x, y);
        }
    }
  else
    {
      x[0] = xcenter + width * cos(a1);
      y[0] = ycenter + height * sin(a1);
      x[1] = width;
      y[1] = height;
      x[2] = a1 * M_PI / 180;
      y[2] = a2 * M_PI / 180;
      x[3] = y[3] = 0;

      gks_gdp(4, x, y, GKS_K_GDP_DRAW_PATH, 3, codes);
    }

  if (flag_graphics)
    {
      gr_writestream("<drawarc xmin=\"%g\" xmax=\"%g\" ymin=\"%g\" ymax=\"%g\" "
                     "a1=\"%g\" a2=\"%g\"/>\n",
                     xmin, xmax, ymin, ymax, a1, a2);
    }
}

/*!
 * Fill a circular or elliptical arc covering the specified rectangle.
 *
 * \param[in] xmin Left edge of the rectangle
 * \param[in] xmax Right edge of the rectangle
 * \param[in] ymin Bottom edge of the rectangle
 * \param[in] ymax Upper edge of the rectangle
 * \param[in] a1 The start angle
 * \param[in] a2 The end angle
 *
 * The resulting arc begins at a1 and ends at a2 degrees. Angles are
 * interpreted such that 0 degrees is at the 3 o'clock position. The center of
 * the arc is the center of the given rectangle.
 */
void gr_fillarc(double xmin, double xmax, double ymin, double ymax, double a1, double a2)
{
  int errind, style;
  double xcenter, ycenter, width, height, start, end, a;
  int n;
  double bwidth, x[362], y[362];
  int codes[3] = {'M', 'A', 'f'};

  check_autoinit;

  gks_inq_fill_int_style(&errind, &style);

  xcenter = (x_lin(xmin) + x_lin(xmax)) / 2.0;
  ycenter = (y_lin(ymin) + y_lin(ymax)) / 2.0;
  width = fabs(x_lin(xmax) - x_lin(xmin)) / 2.0;
  height = fabs(y_lin(ymax) - y_lin(ymin)) / 2.0;

  if (style != GKS_K_INTSTYLE_SOLID_WITH_BORDER)
    {
      start = min(a1, a2);
      end = max(a1, a2);
      start += ((int)(end - start)) / 360 * 360;
      /* Ensure that two equivalent but unequal angles result in a full arc. */
      if (fabs(end - start) < FEPS && fabs(a1 - a2) > FEPS)
        {
          end += 360;
        }

      x[0] = x_log(xcenter);
      y[0] = x_log(ycenter);
      n = 1;
      for (a = start; a <= end; a++)
        {
          x[n] = x_log(xcenter + width * cos(a * M_PI / 180));
          y[n] = y_log(ycenter + height * sin(a * M_PI / 180));
          n++;
        }
      if (fabs((a - 1) - end) > FEPS)
        {
          x[n] = x_log(xcenter + width * cos(end * M_PI / 180));
          y[n] = y_log(ycenter + height * sin(end * M_PI / 180));
          n++;
        }

      if (n > 2)
        {
          fillarea(n, x, y);
        }
    }
  else
    {
      x[0] = xcenter + width * cos(a1);
      y[0] = ycenter + height * sin(a1);
      x[1] = width;
      y[1] = height;
      x[2] = a1 * M_PI / 180;
      y[2] = a2 * M_PI / 180;
      x[3] = y[3] = 0;

      gr_inqborderwidth(&bwidth);
      if (bwidth != 0) codes[2] = 'F';

      gks_gdp(4, x, y, GKS_K_GDP_DRAW_PATH, 3, codes);
    }

  if (flag_graphics)
    {
      gr_writestream("<fillarc xmin=\"%g\" xmax=\"%g\" ymin=\"%g\" ymax=\"%g\" "
                     "a1=\"%g\" a2=\"%g\"/>\n",
                     xmin, xmax, ymin, ymax, a1, a2);
    }
}

static void addpath(double x, double y)
{
  xpath[npath] = x;
  ypath[npath] = y;
  npath += 1;
}

static void closepath(int fill)
{
  if (fill)
    {
      if (npath > 2) gks_fillarea(npath, xpath, ypath);
    }
  else if (npath > 1)
    gks_polyline(npath, xpath, ypath);
  npath = 0;
}

static void quad_bezier(double x[3], double y[3], int n)
{
  int i;
  double t, a, b, c;

  if (npath + n >= maxpath) reallocate(npath + n);

  for (i = 0; i < n; i++)
    {
      t = (double)i / (n - 1);
      a = pow((1.0 - t), 2.0);
      b = 2.0 * t * (1.0 - t);
      c = pow(t, 2.0);
      addpath(a * x[0] + b * x[1] + c * x[2], a * y[0] + b * y[1] + c * y[2]);
    }
}

static void cubic_bezier(double x[4], double y[4], int n)
{
  int i;
  double t, a, b, c, d;

  if (npath + n >= maxpath) reallocate(npath + n);

  for (i = 0; i < n; i++)
    {
      t = (double)i / (n - 1);
      a = pow((1.0 - t), 3.0);
      b = 3.0 * t * pow((1.0 - t), 2.0);
      c = 3.0 * pow(t, 2.0) * (1.0 - t);
      d = pow(t, 3.0);
      addpath(a * x[0] + b * x[1] + c * x[2] + d * x[3], a * y[0] + b * y[1] + c * y[2] + d * y[3]);
    }
}

/*!
 * Draw simple and compound outlines consisting of line segments and bezier
 * curves.
 *
 * \param[in] n The number of vertices
 * \param[in] vertices A pointer to the vertices
 * \param[in] codes A pointer to the path codes
 * \param[in] fill A flag indication whether resulting path is to be filled or
 *                 not
 *
 * \verbatim embed:rst:leading-asterisk
 *
 * The following path codes are recognized:
 *
 * +----------+-----------------------------------------------------------+
 * |      STOP|end the entire path                                        |
 * +----------+-----------------------------------------------------------+
 * |    MOVETO|move to the given vertex                                   |
 * +----------+-----------------------------------------------------------+
 * |    LINETO|draw a line from the current position to the given vertex  |
 * +----------+-----------------------------------------------------------+
 * |    CURVE3|draw a quadratic Bezier curve                              |
 * +----------+-----------------------------------------------------------+
 * |    CURVE4|draw a cubic Bezier curve                                  |
 * +----------+-----------------------------------------------------------+
 * | CLOSEPOLY|draw a line segment to the start point of the current path |
 * +----------+-----------------------------------------------------------+
 *
 * \endverbatim
 */
void gr_drawpath(int n, vertex_t *vertices, unsigned char *codes, int fill)
{
  int i, j = 0, code, nan = 0;

  check_autoinit;

  if (n >= maxpath) reallocate(n);

  if (codes == NULL)
    {
      memset(opcode, LINETO, n);
      opcode[0] = MOVETO;
    }
  else
    memmove(opcode, codes, n);

  for (i = 0; i < n; i++)
    {
      if (is_nan(vertices[i].x) || is_nan(vertices[i].y))
        {
          nan = 1;
          continue;
        }
      else
        {
          opcode[j] = nan ? MOVETO : opcode[i];
          nan = 0;
        }
      xpoint[j] = vertices[i].x;
      ypoint[j] = vertices[i].y;
      j++;
    }

  for (i = 0; i < j; i++)
    {
      code = opcode[i];
      if (code == STOP)
        break;
      else if (code == MOVETO)
        {
          closepath(fill);
          addpath(xpoint[i], ypoint[i]);
        }
      else if (code == LINETO)
        addpath(xpoint[i], ypoint[i]);
      else if (code == CURVE3)
        {
          quad_bezier(xpoint + i - 1, ypoint + i - 1, 20);
          i += 1;
        }
      else if (code == CURVE4)
        {
          cubic_bezier(xpoint + i - 1, ypoint + i - 1, 20);
          i += 2;
        }
      else if (code == CLOSEPOLY)
        {
          addpath(xpoint[i], ypoint[i]);
          closepath(fill);
        }
    }
  closepath(fill);

  if (flag_graphics)
    {
      gr_writestream("<drawpath len=\"%d\"", n);
      print_vertex_array("vertices", n, vertices);
      print_byte_array("codes", codes != NULL ? n : 0, codes);
      gr_writestream(" fill=\"%d\"/>\n", fill);
    }
}

/*!
 * Set the arrow style to be used for subsequent arrow commands.
 *
 * \param[in] style The arrow style to be used
 *
 * This function defines the arrow style for subsequent arrow primitives.
 * The default arrow style is 1.
 *
 * \verbatim embed:rst:leading-asterisk
 *
 * +---+----------------------------------+
 * |  1|simple, single-ended              |
 * +---+----------------------------------+
 * |  2|simple, single-ended, acute head  |
 * +---+----------------------------------+
 * |  3|hollow, single-ended              |
 * +---+----------------------------------+
 * |  4|filled, single-ended              |
 * +---+----------------------------------+
 * |  5|triangle, single-ended            |
 * +---+----------------------------------+
 * |  6|filled triangle, single-ended     |
 * +---+----------------------------------+
 * |  7|kite, single-ended                |
 * +---+----------------------------------+
 * |  8|filled kite, single-ended         |
 * +---+----------------------------------+
 * |  9|simple, double-ended              |
 * +---+----------------------------------+
 * | 10|simple, double-ended, acute head  |
 * +---+----------------------------------+
 * | 11|hollow, double-ended              |
 * +---+----------------------------------+
 * | 12|filled, double-ended              |
 * +---+----------------------------------+
 * | 13|triangle, double-ended            |
 * +---+----------------------------------+
 * | 14|filled triangle, double-ended     |
 * +---+----------------------------------+
 * | 15|kite, double-ended                |
 * +---+----------------------------------+
 * | 16|filled kite, double-ended         |
 * +---+----------------------------------+
 * | 17|double line, single-ended         |
 * +---+----------------------------------+
 * | 18|double line, double-ended         |
 * +---+----------------------------------+
 *
 * \endverbatim
 */
void gr_setarrowstyle(int style)
{
  check_autoinit;

  if (style >= 1 && style <= 18) arrow_style = style - 1;

  if (flag_graphics) gr_writestream("<setarrowstyle style=\"%d\"/>\n", style);
}

/*!
 * Set the arrow size to be used for subsequent arrow commands.
 *
 * \param[in] size The arrow size to be used
 *
 * This function defines the arrow size for subsequent arrow primitives.
 * The default arrow size is 1.
 */
void gr_setarrowsize(double size)
{
  check_autoinit;

  if (arrow_size > 0) arrow_size = size;

  if (flag_graphics) gr_writestream("<setarrowsize size=\"%g\"/>\n", size);
}

/*!
 * Draw an arrow between two points.
 *
 * \param[in] x1 The X coordinate of the arrow start point (tail)
 * \param[in] y1 The Y coordinate of the arrow start point (tail)
 * \param[in] x2 The X coordinate of the arrow end point (head)
 * \param[in] y2 The Y coordinate of the arrow end point (head)
 *
 * Different arrow styles (angles between arrow tail and wing, optionally filled
 * heads, double headed arrows) are available and can be set with the
 * gr_setarrowstyle function.
 */
void gr_drawarrow(double x1, double y1, double x2, double y2)
{
  double xs, ys, xe, ye;
  int errind, ltype, intstyle, tnr;
  double a, c, xc, yc, f, fh;
  int fill, i, j, n;
  double xi, yi, x[10], y[10];

  check_autoinit;

  gks_inq_pline_linetype(&errind, &ltype);
  gks_inq_fill_int_style(&errind, &intstyle);
  gks_inq_current_xformno(&errind, &tnr);

  if (tnr != NDC)
    {
      xs = nx.a * x_lin(x1) + nx.b;
      ys = nx.c * y_lin(y1) + nx.d;
      xe = nx.a * x_lin(x2) + nx.b;
      ye = nx.c * y_lin(y2) + nx.d;
    }
  else
    {
      xs = x1;
      ys = y1;
      xe = x2;
      ye = y2;
    }

  gks_set_fill_int_style(GKS_K_INTSTYLE_SOLID);

  c = sqrt((xe - xs) * (xe - xs) + (ye - ys) * (ye - ys));
  if (ys != ye)
    a = acos(fabs(xe - xs) / c);
  else
    a = 0;
  if (ye < ys) a = 2 * M_PI - a;
  if (xe < xs) a = M_PI - a;
  a -= M_PI / 2;

  xc = (xs + xe) / 2;
  yc = (ys + ye) / 2;
  f = 0.01 * c / 2;
  fh = 0.15 / c * arrow_size;

  j = 0;
  while ((n = vertex_list[arrow_style][j++]) != 0)
    {
      fill = n < 0;
      n = abs(n);
      gks_set_pline_linetype(n > 2 ? GKS_K_LINETYPE_SOLID : ltype);
      for (i = 0; i < n; i++)
        {
          xi = vertex_list[arrow_style][j++];
          yi = vertex_list[arrow_style][j++];
          xi *= fh;
          if (yi < 0)
            yi = (yi + 100) * fh - 100;
          else
            yi = (yi - 100) * fh + 100;
          xi *= f;
          yi *= f;
          x[i] = xc + cos(a) * xi - sin(a) * yi;
          y[i] = yc + sin(a) * xi + cos(a) * yi;
          if (tnr != NDC)
            {
              x[i] = (x[i] - nx.b) / nx.a;
              y[i] = (y[i] - nx.d) / nx.c;
              if (lx.scale_options)
                {
                  x[i] = x_log(x[i]);
                  y[i] = y_log(y[i]);
                }
            }
        }
      if (fill)
        gks_fillarea(n, x, y);
      else
        gks_polyline(n, x, y);
    }

  gks_set_fill_int_style(intstyle);
  gks_set_pline_linetype(ltype);

  if (flag_graphics) gr_writestream("<drawarrow x1=\"%g\" y1=\"%g\" x2=\"%g\" y2=\"%g\"/>\n", x1, y1, x2, y2);
}

/*!
 * Draw an image into a given rectangular area.
 *
 * \param[in] xmin X coordinate of the lower left point of the rectangle
 * \param[in] ymin Y coordinate of the lower left point of the rectangle
 * \param[in] xmax X coordinate of the upper right point of the rectangle
 * \param[in] ymax Y coordinate of the upper right point of the rectangle
 * \param[in] width X dimension of the color index array
 * \param[in] height Y dimension of the color index array
 * \param[in] data color array
 * \param[in] model color model
 *
 * The points (xmin, ymin) and (xmax, ymax) are world coordinates defining
 * diagonally opposite corner points of a rectangle. This rectangle is divided
 * into width by height cells. The two-dimensional array data specifies colors
 * for each cell.
 *
 * \verbatim embed:rst:leading-asterisk
 *
 * The available color models are:
 *
 * +-----------------------+---+-----------+
 * |MODEL_RGB              |  0|   AABBGGRR|
 * +-----------------------+---+-----------+
 * |MODEL_HSV              |  1|   AAVVSSHH|
 * +-----------------------+---+-----------+
 *
 * \endverbatim
 */
void gr_drawimage(double xmin, double xmax, double ymin, double ymax, int width, int height, int *data, int model)
{
  int *img = data, *imgT;
  int n, i, j, w, h;
  double hue, saturation, value, red, green, blue, x, y;

  check_autoinit;

  if (model == MODEL_HSV)
    {
      n = width * height;
      img = (int *)xmalloc(n * sizeof(int));
      for (i = 0; i < n; i++)
        {
          hue = (data[i] & 0xff) / 255.0;
          saturation = ((data[i] & 0xff00) >> 8) / 255.0;
          value = ((data[i] & 0xff0000) >> 16) / 255.0;
          gr_hsvtorgb(hue, saturation, value, &red, &green, &blue);
          img[i] = (data[i] & 0xff000000) | ((int)(red * 255) << 16) | ((int)(green * 255) << 8) | ((int)(blue * 255));
        }
    }

  if (lx.scale_options != 0)
    {
      w = max(width, 500);
      h = max(height, 500);
      linear_xform lx_original = lx;
      lx.xmin = xmin;
      lx.xmax = xmax;
      lx.a = (xmax - xmin) / log10(xmax / xmin);
      lx.b = xmin - lx.a * log10(xmin);
      lx.ymin = ymin;
      lx.ymax = ymax;
      lx.c = (ymax - ymin) / log10(ymax / ymin);
      lx.d = ymin - lx.c * log10(ymin);
      imgT = (int *)xmalloc(w * h * sizeof(int));
      for (i = 0; i < w; i++)
        {
          if (w > 1)
            {
              x = (x_log(xmin + i * (xmax - xmin) / (w - 1)) - xmin) / (xmax - xmin);
              if (x < 0)
                x = 0;
              else if (x > 1)
                x = 1;
            }
          else
            x = 0;
          for (j = 0; j < h; j++)
            {
              if (h > 1)
                {
                  y = (y_log(ymin + (h - 1 - j) * (ymax - ymin) / (h - 1)) - ymin) / (ymax - ymin);
                  if (y < 0)
                    y = 0;
                  else if (y > 1)
                    y = 1;
                }
              else
                y = 0;
              imgT[i + j * w] = img[(int)min(x * width, width - 1) + (int)min((1 - y) * height, height - 1) * width];
            }
        }
      lx = lx_original;
      if (lx.scale_options & OPTION_FLIP_X)
        {
          double t = xmin;
          xmin = xmax;
          xmax = t;
        }
      if (lx.scale_options & OPTION_FLIP_Y)
        {
          double t = ymin;
          ymin = ymax;
          ymax = t;
        }
      gks_draw_image(x_lin(xmin), y_lin(ymax), x_lin(xmax), y_lin(ymin), w, h, imgT);
      free(imgT);
    }
  else
    gks_draw_image(xmin, ymax, xmax, ymin, width, height, img);

  if (flag_graphics)
    {
      n = width * height;
      gr_writestream("<drawimage xmin=\"%g\" xmax=\"%g\" ymin=\"%g\" ymax=\"%g\" "
                     "width=\"%d\" height=\"%d\"",
                     xmin, xmax, ymin, ymax, width, height);
      print_int_array("data", n, data);
      gr_writestream("model=\"%d\"/>\n", model);
    }

  if (model == MODEL_HSV) free(img);
}

/*!
 * Allows drawing of shadows, realized by images painted underneath,
 * and offset from, graphics objects such that the shadow mimics the effect of
 * a light source cast on the graphics objects.
 *
 * \param[in] offsetx An x-offset, which specifies how far in the horizontal
 *                    direction the shadow is offset from the object
 * \param[in] offsety A y-offset, which specifies how far in the vertical
 *                    direction the shadow is offset from the object
 * \param[in] blur A blur value, which specifies whether the object has a
 *                    hard or a diffuse edge
 */
void gr_setshadow(double offsetx, double offsety, double blur)
{
  check_autoinit;

  gks_set_shadow(offsetx, offsety, blur);
}

/*!
 * Set the value of the alpha component associated with GR colors.
 *
 * \param[in] alpha An alpha value (0.0 - 1.0)
 */
void gr_settransparency(double alpha)
{
  check_autoinit;

  gks_set_transparency(alpha);
}

/*!
 * Change the coordinate transformation according to the given matrix.
 *
 * \param[in] mat 2D transformation matrix
 */
void gr_setcoordxform(double mat[3][2])
{
  check_autoinit;

  gks_set_coord_xform(mat);
}

/*!
 * Open a file for graphics output.
 *
 * \param[in] path Filename for the graphics file.
 *
 * gr_begingraphics allows to write all graphics output into a XML-formatted
 * file until the gr_endgraphics functions is called. The resulting file may
 * later be imported with the gr_importgraphics function.
 */
void gr_begingraphics(char *path)
{
  if (!flag_graphics)
    {
      if (gr_openstream(path) == 0)
        {
          gr_writestream(XML_HEADER);
          gr_writestream(GR_HEADER);
          flag_graphics = 1;
        }
      else
        fprintf(stderr, "%s: open failed\n", path);
    }
}

void gr_endgraphics(void)
{
  if (flag_graphics)
    {
      gr_writestream(GR_TRAILER);
      gr_closestream();
      flag_graphics = 0;
    }
}

static void latex2image(char *string, int pointSize, double *rgb, int *width, int *height, int **data)
{
  int color;
  char s[FILENAME_MAX], path[FILENAME_MAX], cache[33];
  char *tmp, *temp, *null, cmd[1024];
  static char *preamble = NULL;
  char tex[FILENAME_MAX], dvi[FILENAME_MAX], png[FILENAME_MAX];
  FILE *stream;
  int math, ret;
#ifdef _WIN32
  wchar_t w_path[MAX_PATH];
#endif

  color = ((int)(rgb[0] * 255)) + ((int)(rgb[1] * 255) << 8) + ((int)(rgb[2] * 255) << 16) + (255 << 24);
  sprintf(s, "%d%x%s", pointSize, color, string);
  md5(s, cache);
#ifdef _WIN32
  temp = (char *)gks_getenv("TEMP");
#else
  temp = TMPDIR;
#endif
  sprintf(path, "%s%sgr-cache-%s.png", temp, DIRDELIM, cache);

#ifdef _WIN32
  MultiByteToWideChar(CP_UTF8, 0, path, strlen(path) + 1, w_path, MAX_PATH);
  if (_waccess(w_path, R_OK) != 0)
#else
  if (access(path, R_OK) != 0)
#endif
    {
      math = strstr(string, "\\(") == NULL;
#ifdef _WIN32
      tmp = cache;
      temp = ".";
#else
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif
      tmp = tempnam(temp, NULL);
#ifdef __clang__
#pragma clang diagnostic pop
#endif
#endif
      sprintf(tex, "%s.tex", tmp);
      sprintf(dvi, "%s.dvi", tmp);
      sprintf(png, "%s.png", tmp);
#ifdef _WIN32
      null = "NUL";
      MultiByteToWideChar(CP_UTF8, 0, tex, strlen(tex) + 1, w_path, MAX_PATH);
      stream = _wfopen(w_path, L"w");
#else
      null = "/dev/null";
      stream = fopen(tex, "w");
#endif
      if (preamble == NULL)
        {
          preamble = (char *)gks_getenv("GR_LATEX_PREAMBLE");
        }
      if (preamble != NULL)
        {
          if (strcmp(preamble, "AMS") == 0)
            {
              preamble = "\
\\documentclass{article}\n\
\\pagestyle{empty}\n\
\\usepackage{amssymb}\n\
\\usepackage{amsmath}\n\
\\usepackage[dvips]{color}\n\
\\begin{document}\n";
            }
        }
      else
        {
          preamble = "\
\\documentclass{article}\n\
\\pagestyle{empty}\n\
\\usepackage[dvips]{color}\n\
\\begin{document}\n";
        }
      fprintf(stream, "%s", preamble);
      if (math) fprintf(stream, "\\[\n");
      fprintf(stream, "\\color[rgb]{%.3f,%.3f,%.3f} {\n", rgb[0], rgb[1], rgb[2]);
      fwrite(string, strlen(string), 1, stream);
      fprintf(stream, "}\n");
      if (math) fprintf(stream, "\\]\n");
      fprintf(stream, "\\end{document}");
      fclose(stream);

      sprintf(cmd, "latex -interaction=batchmode -halt-on-error -output-directory=%s %s >%s", temp, tex, null);
      ret = system(cmd);

#ifdef _WIN32
      MultiByteToWideChar(CP_UTF8, 0, dvi, strlen(dvi) + 1, w_path, MAX_PATH);
      if (ret == 0 && _waccess(w_path, R_OK) == 0)
#else
      if (ret == 0 && access(dvi, R_OK) == 0)
#endif
        {
          sprintf(cmd, "dvipng -bg transparent -q -T tight -x %d %s -o %s >%s", pointSize * 100, dvi, png, null);
          ret = system(cmd);
          if (ret == 0)
            {
              rename(png, path);
#ifdef _WIN32
              sprintf(cmd, "DEL %s.*", tmp);
#else
              sprintf(cmd, "rm -f %s.*", tmp);
#endif
              ret = system(cmd);
              if (ret != 0) fprintf(stderr, "error deleting temprorary files\n");
            }
          else
            fprintf(stderr, "dvipng: PNG conversion failed\n");
        }
      else
        fprintf(stderr, "latex: failed to create a dvi file\n");
    }

#ifdef _WIN32
  MultiByteToWideChar(CP_UTF8, 0, path, strlen(path) + 1, w_path, MAX_PATH);
  if (_waccess(w_path, R_OK) == 0)
#else
  if (access(path, R_OK) == 0)
#endif
    {
      gr_readimage(path, width, height, data);
    }
}

int *rotl90(int m, int n, int *mat)
{
  int *trans = (int *)xcalloc(m * n, sizeof(int));
  int i, j;

  for (j = 0; j < n; j++)
    for (i = 0; i < m; i++) trans[(m - 1 - i) * n + j] = mat[j * m + i];

  return trans;
}

static int *rot180(int m, int n, int *mat)
{
  int *trans = (int *)xcalloc(m * n, sizeof(int));
  int i, j;

  for (j = 0; j < n; j++)
    for (i = 0; i < m; i++) trans[(n - 1 - j) * m + m - 1 - i] = mat[j * m + i];

  return trans;
}

static int *rotr90(int m, int n, int *mat)
{
  int *trans = (int *)xcalloc(m * n, sizeof(int));
  int i, j;

  for (j = 0; j < n; j++)
    for (i = 0; i < m; i++) trans[i * n + n - 1 - j] = mat[j * m + i];

  return trans;
}

static void mathtex(double x, double y, char *string, int inquire, double *tbx, double *tby)
{
  int wkid = 1, errind, conid, wtype, dcunit;
  int pointSize, pixels, color;
  double chh, rgb[3], ux, uy;
  int width, height, *data = NULL, w, h, *trans = NULL;
  double rad, rw, rh, rx, ry, xx, yy, bbx[4], bby[4];
  double x1, x2, y1, y2, midx, midy, sinf, cosf;
  int i, j, ii, jj, angle, path, halign, valign, tnr;

  gks_inq_ws_conntype(wkid, &errind, &conid, &wtype);
  gks_inq_max_ds_size(wtype, &errind, &dcunit, &rw, &rh, &width, &height);
  if (sizex > 0)
    pixels = sizex / rh * height;
  else
    pixels = 500;
  if (wtype == 101 || wtype == 102 || wtype == 120 || wtype == 382) pixels *= 8;

  gks_inq_text_height(&errind, &chh);
  gks_inq_text_color_index(&errind, &color);
  gks_inq_color_rep(wkid, color, GKS_K_VALUE_SET, &errind, &rgb[0], &rgb[1], &rgb[2]);

  pointSize = chh * pixels;
  latex2image(string, pointSize, rgb, &width, &height, &data);

  gks_inq_text_upvec(&errind, &ux, &uy);
  rad = -atan2(ux, uy);
  angle = (int)(rad * 180 / M_PI + 0.5);
  if (angle < 0) angle += 360;
  path = ((angle + 45) / 90) % 4;

  if (data != NULL)
    {
      rw = width / (double)pixels;
      rh = height / (double)pixels;

      gks_inq_text_align(&errind, &halign, &valign);

      rx = x;
      switch (halign)
        {
        case 2:
          rx -= 0.5 * rw;
          break;
        case 3:
          rx -= rw;
          break;
        }
      ry = y;
      switch (valign)
        {
        case 1:
          ry -= rh - chh * 0.04;
          break;
        case 2:
          ry -= rh;
          break;
        case 3:
          ry -= 0.5 * rh;
          break;
        case 5:
          ry -= chh * 0.04;
          break;
        }
      bbx[0] = rx;
      bbx[1] = rx + rw;
      bbx[2] = bbx[1];
      bbx[3] = bbx[0];
      bby[0] = ry;
      bby[1] = bby[0];
      bby[2] = ry + rh;
      bby[3] = bby[2];

      x1 = y1 = FLT_MAX;
      x2 = y2 = -FLT_MAX;

      for (i = 0; i < 4; i++)
        {
          xx = bbx[i] - x;
          yy = bby[i] - y;
          bbx[i] = x + cos(rad) * xx - sin(rad) * yy;
          bby[i] = y + sin(rad) * xx + cos(rad) * yy;

          x1 = min(x1, bbx[i]);
          x2 = max(x2, bbx[i]);
          y1 = min(y1, bby[i]);
          y2 = max(y2, bby[i]);
        }

      if (inquire)
        {
          for (i = 0; i < 4; i++)
            {
              tbx[i] = bbx[i];
              tby[i] = bby[i];
            }
        }
      else
        {
          gks_inq_current_xformno(&errind, &tnr);
          if (tnr != NDC) gks_select_xform(NDC);

          if (angle % 90 == 0)
            {
              switch (path)
                {
                case 0:
                  gks_draw_image(x1, y2, x2, y1, width, height, data);
                  break;
                case 1:
                  trans = rotl90(width, height, data);
                  gks_draw_image(x1, y2, x2, y1, height, width, trans);
                  free(trans);
                  break;
                case 2:
                  trans = rot180(width, height, data);
                  gks_draw_image(x1, y2, x2, y1, width, height, trans);
                  free(trans);
                  break;
                case 3:
                  trans = rotr90(width, height, data);
                  gks_draw_image(x1, y2, x2, y1, height, width, trans);
                  free(trans);
                  break;
                }
            }
          else
            {
              w = (int)((x2 - x1) * pixels + 0.5), h = (int)((y2 - y1) * pixels + 0.5);
              trans = (int *)xcalloc(w * h, sizeof(int));
              midx = ceil(0.5 * w);
              midy = ceil(0.5 * h);
              sinf = sin(rad);
              cosf = cos(rad);
              for (j = 0; j < h; j++)
                for (i = 0; i < w; i++)
                  {
                    xx = (i - midx) * cosf - (j - midy) * sinf;
                    yy = (i - midx) * sinf + (j - midy) * cosf;
                    ii = round(xx) + ceil(0.5 * width);
                    jj = round(yy) + ceil(0.5 * height);
                    if (ii >= 0 && jj >= 0 && ii < width && jj < height) trans[j * w + i] = data[jj * width + ii];
                  }
              gks_draw_image(x1, y2, x2, y1, w, h, trans);
              free(trans);
            }

          if (tnr != NDC) gks_select_xform(tnr);
        }

      free(data);
    }
}

void mathtex2(double x, double y, const char *formula, int inquire, double *tbx, double *tby);

/*!
 * Generate a character string starting at the given location. Strings can be
 * defined to create mathematical symbols and Greek letters using LaTeX syntax.
 *
 * \param[in] x The X coordinate of the starting position of the text string
 * \param[in] y The Y coordinate of the starting position of the text string
 * \param[in] string The text string to be drawn
 */
void gr_mathtex(double x, double y, char *string)
{
  int unused;
  int prec;

  check_autoinit;

  gks_inq_text_fontprec(&unused, &unused, &prec);
  if (prec == 3)
    {
      mathtex2(x, y, string, 0, NULL, NULL);
    }
  else
    {
      mathtex(x, y, string, 0, NULL, NULL);
    }

  if (flag_graphics) gr_writestream("<mathtex x=\"%g\" y=\"%g\" text=\"%s\"/>\n", x, y, string);
}

void gr_inqmathtex(double x, double y, char *string, double *tbx, double *tby)
{
  int unused;
  int prec;

  check_autoinit;

  gks_inq_text_fontprec(&unused, &unused, &prec);
  if (prec == 3)
    {
      mathtex2(x, y, string, 1, tbx, tby);
    }
  else
    {
      mathtex(x, y, string, 1, tbx, tby);
    }
}

void gr_beginselection(int index, int type)
{
  check_autoinit;

  gks_begin_selection(index, type);
}

void gr_endselection(void)
{
  check_autoinit;

  gks_end_selection();
}

void gr_moveselection(double x, double y)
{
  check_autoinit;

  gks_move_selection(x, y);
}

void gr_resizeselection(int type, double x, double y)
{
  check_autoinit;

  gks_resize_selection(type, x, y);
}

void gr_inqbbox(double *xmin, double *xmax, double *ymin, double *ymax)
{
  int errind;

  check_autoinit;

  gks_inq_bbox(&errind, xmin, xmax, ymin, ymax);
}

double gr_precision(void)
{
  /* check_autoinit is intentionally not called here, because
   *  wrappers might call gr_precision on top-level. */

  return gks_precision();
}

void gr_setregenflags(int flags)
{
  check_autoinit;

  regeneration_flags = flags;
}

int gr_inqregenflags(void)
{
  check_autoinit;

  return regeneration_flags;
}

void gr_savestate(void)
{
  int errind;
  state_list *s = NULL;

  check_autoinit;

  if (state_saved < MAX_SAVESTATE)
    {
      if (state == NULL) state = (state_list *)xmalloc(sizeof(state_list) * MAX_SAVESTATE);

      s = state + state_saved;
      state_saved += 1;

      gks_inq_pline_linetype(&errind, &s->ltype);
      gks_inq_pline_linewidth(&errind, &s->lwidth);
      gks_inq_pline_color_index(&errind, &s->plcoli);
      gks_inq_pmark_type(&errind, &s->mtype);
      gks_inq_pmark_size(&errind, &s->mszsc);
      gks_inq_pmark_color_index(&errind, &s->pmcoli);
      gks_inq_text_fontprec(&errind, &s->txfont, &s->txprec);
      gks_inq_text_expfac(&errind, &s->chxp);
      gks_inq_text_spacing(&errind, &s->chsp);
      gks_inq_text_color_index(&errind, &s->txcoli);
      gks_inq_text_height(&errind, &s->chh);
      gks_inq_text_upvec(&errind, &s->chup[0], &s->chup[1]);
      gks_inq_text_path(&errind, &s->txp);
      gks_inq_text_align(&errind, &s->txal[0], &s->txal[1]);
      gks_inq_fill_int_style(&errind, &s->ints);
      gks_inq_fill_style_index(&errind, &s->styli);
      gks_inq_fill_color_index(&errind, &s->facoli);

      gks_inq_current_xformno(&errind, &s->tnr);
      gks_inq_xform(WC, &errind, s->wn, s->vp);

      s->scale_options = lx.scale_options;

      gks_inq_border_width(&errind, &s->bwidth);
      gks_inq_border_color_index(&errind, &s->bcoli);
      gks_inq_clip_xform(&errind, &s->clip_tnr);
    }
  else
    fprintf(stderr, "attempt to save state beyond implementation limit\n");

  if (flag_graphics) gr_writestream("<savestate/>\n");
}

void gr_restorestate(void)
{
  state_list *s = NULL;

  check_autoinit;

  if (state_saved > 0)
    {
      state_saved -= 1;
      s = state + state_saved;

      gks_set_pline_linetype(s->ltype);
      gks_set_pline_linewidth(s->lwidth);
      gks_set_pline_color_index(s->plcoli);
      gks_set_pmark_type(s->mtype);
      gks_set_pmark_size(s->mszsc);
      gks_set_pmark_color_index(s->pmcoli);
      gks_set_text_fontprec(s->txfont, s->txprec);
      gks_set_text_expfac(s->chxp);
      gks_set_text_spacing(s->chsp);
      gks_set_text_color_index(s->txcoli);
      gks_set_text_height(s->chh);
      gks_set_text_upvec(s->chup[0], s->chup[1]);
      gks_set_text_path(s->txp);
      gks_set_text_align(s->txal[0], s->txal[1]);
      gks_set_fill_int_style(s->ints);
      gks_set_fill_style_index(s->styli);
      gks_set_fill_color_index(s->facoli);

      gks_select_xform(s->tnr);
      gks_set_window(WC, s->wn[0], s->wn[1], s->wn[2], s->wn[3]);
      gks_set_viewport(WC, s->vp[0], s->vp[1], s->vp[2], s->vp[3]);

      setscale(s->scale_options);

      gks_set_border_width(s->bwidth);
      gks_set_border_color_index(s->bcoli);
      gks_select_clip_xform(s->clip_tnr);
    }
  else
    fprintf(stderr, "attempt to restore unsaved state\n");

  if (flag_graphics) gr_writestream("<restorestate/>\n");
}

void gr_selectcontext(int context)
{
  int id;

  check_autoinit;

  if (context >= 1 && context <= MAX_CONTEXT)
    {
      id = context - 1;
      if (app_context[id] == NULL)
        {
          app_context[id] = (state_list *)xmalloc(sizeof(state_list));
          ctx = app_context[id];

          ctx->ltype = GKS_K_LINETYPE_SOLID;
          ctx->lwidth = 1;
          ctx->plcoli = 1;
          ctx->mtype = GKS_K_MARKERTYPE_ASTERISK;
          ctx->mszsc = 2;
          ctx->pmcoli = 1;
          ctx->txfont = 3;
          ctx->txprec = GKS_K_TEXT_PRECISION_STRING;
          ctx->chxp = 1;
          ctx->chsp = 0;
          ctx->txcoli = 1;
          ctx->chh = 0.027;
          ctx->chup[0] = 0;
          ctx->chup[1] = 1;
          ctx->txp = GKS_K_TEXT_PATH_RIGHT;
          ctx->txal[0] = GKS_K_TEXT_HALIGN_LEFT;
          ctx->txal[1] = GKS_K_TEXT_VALIGN_BASE;
          ctx->ints = GKS_K_INTSTYLE_HOLLOW;
          ctx->styli = 1;
          ctx->facoli = 1;

          ctx->tnr = WC;
          ctx->wn[0] = ctx->wn[2] = 0;
          ctx->wn[1] = ctx->wn[3] = 1;
          ctx->vp[0] = ctx->vp[2] = 0.2;
          ctx->vp[1] = ctx->vp[3] = 0.9;

          ctx->scale_options = 0;

          ctx->bwidth = 1;
          ctx->bcoli = 1;
          ctx->clip_tnr = 0;
        }
      else
        {
          ctx = app_context[id];
        }

      gks_set_pline_linetype(ctx->ltype);
      gks_set_pline_linewidth(ctx->lwidth);
      gks_set_pline_color_index(ctx->plcoli);
      gks_set_pmark_type(ctx->mtype);
      gks_set_pmark_size(ctx->mszsc);
      gks_set_pmark_color_index(ctx->pmcoli);
      gks_set_text_fontprec(ctx->txfont, ctx->txprec);
      gks_set_text_expfac(ctx->chxp);
      gks_set_text_spacing(ctx->chsp);
      gks_set_text_color_index(ctx->txcoli);
      gks_set_text_height(ctx->chh);
      gks_set_text_upvec(ctx->chup[0], ctx->chup[1]);
      gks_set_text_path(ctx->txp);
      gks_set_text_align(ctx->txal[0], ctx->txal[1]);
      gks_set_fill_int_style(ctx->ints);
      gks_set_fill_style_index(ctx->styli);
      gks_set_fill_color_index(ctx->facoli);

      gks_select_xform(ctx->tnr);
      gks_set_window(WC, ctx->wn[0], ctx->wn[1], ctx->wn[2], ctx->wn[3]);
      gks_set_viewport(WC, ctx->vp[0], ctx->vp[1], ctx->vp[2], ctx->vp[3]);

      setscale(ctx->scale_options);

      gks_set_border_width(ctx->bwidth);
      gks_set_border_color_index(ctx->bcoli);
      gks_select_clip_xform(ctx->clip_tnr);
    }
  else
    {
      fprintf(stderr, "invalid context id\n");
      ctx = NULL;
    }
}

void gr_destroycontext(int context)
{
  int id;

  check_autoinit;

  if (context >= 1 && context <= MAX_CONTEXT)
    {
      id = context - 1;
      if (app_context[id] != NULL) free(app_context[id]);

      app_context[id] = NULL;
    }
  else
    {
      fprintf(stderr, "invalid context id\n");
      ctx = NULL;
    }
}

int gr_uselinespec(char *linespec)
{
  char *spec = linespec, lastspec = ' ';
  int result, linetype = 0, markertype = 0, color = -1;

  while (*spec)
    {
      switch (*spec)
        {
        case ' ':
          def_color = 0;
          break;
        case '-':
          if (lastspec == '-')
            linetype = GKS_K_LINETYPE_DASHED;
          else
            linetype = GKS_K_LINETYPE_SOLID;
          break;
        case ':':
          linetype = GKS_K_LINETYPE_DOTTED;
          break;
        case '.':
          if (lastspec == '-')
            linetype = GKS_K_LINETYPE_DASHED_DOTTED;
          else
            markertype = GKS_K_MARKERTYPE_DOT;
          break;

        case '+':
          markertype = GKS_K_MARKERTYPE_PLUS;
          break;
        case 'o':
          markertype = GKS_K_MARKERTYPE_CIRCLE;
          break;
        case '*':
          markertype = GKS_K_MARKERTYPE_ASTERISK;
          break;
        case 'x':
          markertype = GKS_K_MARKERTYPE_DIAGONAL_CROSS;
          break;
        case 's':
          markertype = GKS_K_MARKERTYPE_SOLID_SQUARE;
          break;
        case 'd':
          markertype = GKS_K_MARKERTYPE_SOLID_DIAMOND;
          break;
        case '^':
          markertype = GKS_K_MARKERTYPE_SOLID_TRI_UP;
          break;
        case 'v':
          markertype = GKS_K_MARKERTYPE_SOLID_TRI_DOWN;
          break;
        case '>':
          markertype = GKS_K_MARKERTYPE_SOLID_TRI_RIGHT;
          break;
        case '<':
          markertype = GKS_K_MARKERTYPE_SOLID_TRI_LEFT;
          break;
        case 'p':
          markertype = GKS_K_MARKERTYPE_SOLID_STAR;
          break;
        case 'h':
          markertype = GKS_K_MARKERTYPE_TRI_UP_DOWN;
          break;

        case 'r':
          color = 984;
          break;
        case 'g':
          color = 987;
          break;
        case 'b':
          color = 989;
          break;
        case 'c':
          color = 983;
          break;
        case 'm':
          color = 988;
          break;
        case 'y':
          color = 994;
          break;
        case 'k':
          color = 1;
          break;
        case 'w':
          color = 0;
          break;
        default:
          break;
        }
      lastspec = *spec++;
    }

  result = 0;
  if (linetype != 0)
    {
      result |= SPEC_LINE;
      gr_setlinetype(linetype);
    }
  if (markertype != 0)
    {
      result |= SPEC_MARKER;
      gr_setmarkertype(markertype);
    }
  if (color == -1)
    {
      color = 980 + predef_colors[def_color];
      if (strcmp(linespec, " ")) def_color = (def_color + 1) % 20;
    }
  else
    {
      if (result == 0) result = SPEC_LINE;
      result |= SPEC_COLOR;
    }

  gr_setlinecolorind(color);
  gr_setmarkercolorind(color);

  if (flag_graphics) gr_writestream("<uselinespec linespec=\"%s\"/>\n", linespec);

  return result;
}

void gr_adjustlimits(double *amin, double *amax)
{
  double a = log10(*amax - *amin), b = 1;
  double quotient, remainder, exponent, scale;

  if (*amin == *amax)
    {
      *amin -= 1;
      *amax += 1;
    }

  remainder = fmod(a, b);
  quotient = (a - remainder) / b;
  if (remainder)
    {
      if ((b < 0) != (remainder < 0))
        {
          remainder += b;
          quotient -= 1;
        }
    }
  else
    {
      remainder *= remainder;
      if (b < 0) remainder = -remainder;
    }
  if (quotient)
    {
      exponent = floor(quotient);
      if (quotient - exponent > 0.5) exponent += 1;
    }
  else
    {
      quotient *= quotient;
      exponent = quotient * a / b;
    }
  if (remainder < 0.5) exponent -= 1;

  scale = pow(10.0, -exponent);
  *amin = floor(*amin * scale + FEPS) / scale;
  *amax = ceil(*amax * scale - FEPS) / scale;
}

/*!
 * Returns the version string of the GR runtime.
 *
 * \returns A pointer to the GR runtime version string.
 */
const char *gr_version(void)
{
  static const char *gr_version_str = GR_VERSION;
  return gr_version_str;
}

/*!
 * Finds the index of the minimum and maximum value in array.
 *
 * \param[out] min_index the index of the minimum value in array
 * \param[out] max_index the index of the maximum value in array
 * \param[in] n the number of values in array
 * \param[in] array the value array
 */
static void arg_min_max(int *min_index, int *max_index, int n, const double *array)
{
  int i;
  *min_index = 0;
  *max_index = 0;
  for (i = 1; i < n; i++)
    {
      if (array[i] < array[*min_index]) *min_index = i;
      if (array[i] > array[*max_index]) *max_index = i;
    }
}

/*!
 * Reduces the number of points of the x and y array.
 *
 * @param[in] n the number of points of the x and y arrays
 * @param[in] x the x value array
 * @param[in] y the y value array
 * @param[in] points the requested number of points
 * @param[out] x_array the return array for the x values
 * @param[out] y_array the return array for the y values
 */
void gr_reducepoints(int n, const double *x, const double *y, int points, double *x_array, double *y_array)
{
  int append_index = 0;
  int num_intervals = points / 2;
  double exact_interval_width = (double)n / num_intervals;
  int interval_width = n / num_intervals;
  int interval;
  if (n < points)
    {
      /* Copy the original array */
      memcpy(x_array, x, sizeof(double) * n);
      memcpy(y_array, y, sizeof(double) * n);
      fprintf(stderr, "Not enough points provided.\n");
      return;
    }
  for (interval = 0; interval < num_intervals; interval++)
    {
      int index = interval * exact_interval_width;
      int min_index, max_index;
      arg_min_max(&min_index, &max_index, min(interval_width, n - index - 1), y + index);
      x_array[append_index] = x[min_index + index];
      y_array[append_index] = y[min_index + index];
      append_index++;
      x_array[append_index] = x[max_index + index];
      y_array[append_index] = y[max_index + index];
      append_index++;
    }
}

/*!
 * Display a point set as a aggregated and rasterized image.
 *
 * \param[in] n The number of points
 * \param[in] x A pointer to the X coordinates
 * \param[in] y A pointer to the Y coordinates
 * \param[in] xform The transformation type used for color mapping
 * \param[in] w The width of the grid used for rasterization
 * \param[in] h The height of the grid used for rasterization
 *
 * The values for `x` and `y` are in world coordinates.
 *
 * \verbatim embed:rst:leading-asterisk
 *
 * The available transformation types are:
 *
 * +----------------+---+--------------------+
 * |XFORM_BOOLEAN   |  0|boolean             |
 * +----------------+---+--------------------+
 * |XFORM_LINEAR    |  1|linear              |
 * +----------------+---+--------------------+
 * |XFORM_LOG       |  2|logarithmic         |
 * +----------------+---+--------------------+
 * |XFORM_LOGLOG    |  3|double logarithmic  |
 * +----------------+---+--------------------+
 * |XFORM_CUBIC     |  4|cubic               |
 * +----------------+---+--------------------+
 * |XFORM_EQUALIZED |  5|histogram equalized |
 * +----------------+---+--------------------+
 *
 * \endverbatim
 */
void gr_shadepoints(int n, double *x, double *y, int xform, int w, int h)
{
  int *bins;
  double roi[4];

  if (n <= 2)
    {
      fprintf(stderr, "invalid number of points\n");
      return;
    }

  if (xform < 0 || xform > 5)
    {
      fprintf(stderr, "invalid transfer function\n");
      return;
    }

  if (w < 1 || h < 1)
    {
      fprintf(stderr, "invalid dimensions\n");
      return;
    }

  check_autoinit;

  roi[0] = lx.xmin;
  roi[1] = lx.xmax;
  roi[2] = lx.ymin;
  roi[3] = lx.ymax;
  bins = (int *)xcalloc(w * h, sizeof(int));

  gr_shade(n, x, y, 0, xform, roi, w, h, bins);

  gks_cellarray(lx.xmin, lx.ymax, lx.xmax, lx.ymin, w, h, 1, 1, w, h, bins);

  free(bins);

  if (flag_graphics)
    {
      gr_writestream("<shadepoints len=\"%d\"", n);
      print_float_array("x", n, x);
      print_float_array("y", n, y);
      gr_writestream(" xform=\"%d\" w=\"%d\" h=\"%d\"/>\n", xform, w, h);
    }
}

/*!
 * Display a line set as a aggregated and rasterized image.
 *
 * \param[in] n The number of points
 * \param[in] x A pointer to the X coordinates
 * \param[in] y A pointer to the Y coordinates
 * \param[in] xform The transformation type used for color mapping
 * \param[in] w The width of the grid used for rasterization
 * \param[in] h The height of the grid used for rasterization
 *
 * The values for `x` and `y` are in world coordinates.
 * NaN values can be used to separate the point set into line segments.
 *
 * \verbatim embed:rst:leading-asterisk
 *
 * The available transformation types are:
 *
 * +----------------+---+--------------------+
 * |XFORM_BOOLEAN   |  0|boolean             |
 * +----------------+---+--------------------+
 * |XFORM_LINEAR    |  1|linear              |
 * +----------------+---+--------------------+
 * |XFORM_LOG       |  2|logarithmic         |
 * +----------------+---+--------------------+
 * |XFORM_LOGLOG    |  3|double logarithmic  |
 * +----------------+---+--------------------+
 * |XFORM_CUBIC     |  4|cubic               |
 * +----------------+---+--------------------+
 * |XFORM_EQUALIZED |  5|histogram equalized |
 * +----------------+---+--------------------+
 *
 * \endverbatim
 */
void gr_shadelines(int n, double *x, double *y, int xform, int w, int h)
{
  int *bins;
  double roi[4];

  if (n <= 2)
    {
      fprintf(stderr, "invalid number of points\n");
      return;
    }

  if (xform < 0 || xform > 5)
    {
      fprintf(stderr, "invalid transfer function\n");
      return;
    }

  if (w < 1 || h < 1)
    {
      fprintf(stderr, "invalid dimensions\n");
      return;
    }

  check_autoinit;

  roi[0] = lx.xmin;
  roi[1] = lx.xmax;
  roi[2] = lx.ymin;
  roi[3] = lx.ymax;
  bins = (int *)xcalloc(w * h, sizeof(int));

  gr_shade(n, x, y, 1, xform, roi, w, h, bins);

  gks_cellarray(lx.xmin, lx.ymax, lx.xmax, lx.ymin, w, h, 1, 1, w, h, bins);

  free(bins);

  if (flag_graphics)
    {
      gr_writestream("<shadelines len=\"%d\"", n);
      print_float_array("x", n, x);
      print_float_array("y", n, y);
      gr_writestream(" xform=\"%d\" w=\"%d\" h=\"%d\"/>\n", xform, w, h);
    }
}

void gr_panzoom(double x, double y, double xzoom, double yzoom, double *xmin, double *xmax, double *ymin, double *ymax)
{
  int errind, tnr;
  double wn[4], vp[4];
  double x0, x1, y0, y1, tmp;

  check_autoinit;

  gks_inq_current_xformno(&errind, &tnr);
  gks_inq_xform(tnr, &errind, wn, vp);

  xzoom = fabs(xzoom);
  yzoom = fabs(yzoom);

  if (yzoom < FEPS)
    {
      yzoom = xzoom;
    }

  if (xzoom < FEPS)
    {
      x0 = vp[0] + x;
      x1 = vp[1] + x;
      y0 = vp[2] + y;
      y1 = vp[3] + y;
    }
  else
    {
      x0 = x1 = 0.5 * (vp[0] + vp[1]) + x;
      y0 = y1 = 0.5 * (vp[2] + vp[3]) + y;
    }

  gr_ndctowc(&x0, &y0);
  gr_ndctowc(&x1, &y1);

  if (OPTION_FLIP_X & lx.scale_options)
    {
      tmp = x0;
      x0 = x1;
      x1 = tmp;
    }
  if (OPTION_FLIP_Y & lx.scale_options)
    {
      tmp = y0;
      y0 = y1;
      y1 = tmp;
    }

  if (xzoom < FEPS)
    {
      *xmin = x0;
      *xmax = x1;
      *ymin = y0;
      *ymax = y1;
    }
  else
    {
      gr_inqwindow(xmin, xmax, ymin, ymax);
      gr_wctondc(xmin, ymin);
      gr_wctondc(xmax, ymax);
      gr_wctondc(&x0, &y0);
      *xmin = x0 - (x0 - *xmin) * xzoom;
      *xmax = x0 + (*xmax - x0) * xzoom;
      *ymin = y0 - (y0 - *ymin) * yzoom;
      *ymax = y0 + (*ymax - y0) * yzoom;
      gr_ndctowc(xmin, ymin);
      gr_ndctowc(xmax, ymax);
    }
}

/*!
 * Find a boundary around a given point set.
 *
 * The boundary is calculated using a 2 dimensional ball pivoting algorithm.
 *
 * \param[in] n The number of points
 * \param[in] x A pointer to the X coordinates
 * \param[in] y A pointer to the Y coordinates
 * \param[in] r A constant ball radius
 * \param[in] r_function A ball radius callback function
 * \param[in] n_contour The amount of memory allocated for the boundary points
 * \param[out] contour A pointer to allocated memory to store the indices of the boundary points
 *
 * \returns Number of points in the boundary contour
 *
 * The ball radius used in the algorithm can be either constant or position dependent. In the first case
 * `r` should contain the radius and `r_function` is NULL. Otherwise `r_function` should be a pointer to
 * a function returning the appropriate radius for a given position.
 *
 * If `r_function` is NULL and `r` has a value less or equal 0 the algorithm calculates a ball radius
 * based on the largest distance from a point to its nearest neighbor.
 *
 * The calculated boundary is represented as a list of indices in the given `x` and `y` arrays and is
 * stored in the `contour` array. `contour` must be a pointer to allocated memory for at least `n_contour`
 * integer indices. Normally less than `n` indices are needed for the boundary, in the worst case 2*`n`
 * indices are needed.
 *
 */
int gr_findboundary(int n, double *x, double *y, double r, double (*r_function)(double x, double y), int n_contour,
                    int *contour)
{
  int result;
  if (n < 2)
    {
      fprintf(stderr, "Not enough points provided.\n");
      return 0;
    }
  result = find_boundary(n, x, y, r, r_function, n_contour, contour);
  if (result < 0)
    {
      if (result == -1)
        {
          fprintf(stderr, "Ball radius is too small.\n");
        }
      else if (result == -2)
        {
          fprintf(stderr, "Ball radius is too large.\n");
        }
      else if (result == -3)
        {
          fprintf(stderr, "Not enough memory provided in contour array.\n");
        }
      else
        {
          fprintf(stderr, "An error occurred finding the boundary.\n");
        }
      result = 0;
    }
  return result;
}

/*!
 * Set the resample method for resampling.
 *
 * \param[in] flag Resample method
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
void gr_setresamplemethod(unsigned int flag)
{
  check_autoinit;

  gks_set_resample_method(flag);
}

/*!
 * Inquire the resample flag status.
 *
 * \returns Resample flag
 */
void gr_inqresamplemethod(unsigned int *flag)
{
  check_autoinit;

  gks_inq_resample_method(flag);
}

/*!
 * Draw paths using given vertices and path codes.
 *
 * \param[in] n The number of points
 * \param[in] x A pointer to the X coordinates
 * \param[in] y A pointer to the Y coordinates
 * \param[in] codes Path codes as a null-terminated string
 *
 * The values for `x` and `y` are in world coordinates. `n` is the number of vertices to use.
 * The `codes` describe several patch primitives that can be used to create compound paths.
 *
 * The following path codes are recognized:
 *
 * \verbatim embed:rst:leading-asterisk
 *
 * +----------+---------------------------------+-------------------+-------------------+
 * | **Code** | **Description**                 | **x**             | **y**             |
 * +----------+---------------------------------+-------------------+-------------------+
 * |     M, m | move                            | x                 | y                 |
 * +----------+---------------------------------+-------------------+-------------------+
 * |     L, l | line                            | x                 | y                 |
 * +----------+---------------------------------+-------------------+-------------------+
 * |     Q, q | quadratic Bezier                | x1, x2            | y1, y2            |
 * +----------+---------------------------------+-------------------+-------------------+
 * |     C, c | cubic Bezier                    | x1, x2, x3        | y1, y2, y3        |
 * +----------+---------------------------------+-------------------+-------------------+
 * |     A, a | arc                             | rx, a1, reserved  | ry, a2, reserved  |
 * +----------+---------------------------------+-------------------+-------------------+
 * |        Z | close path                      |                   |                   |
 * +----------+---------------------------------+-------------------+-------------------+
 * |        S | stroke                          |                   |                   |
 * +----------+---------------------------------+-------------------+-------------------+
 * |        s | close path and stroke           |                   |                   |
 * +----------+---------------------------------+-------------------+-------------------+
 * |        f | close path and fill             |                   |                   |
 * +----------+---------------------------------+-------------------+-------------------+
 * |        F | close path, fill and stroke     |                   |                   |
 * +----------+---------------------------------+-------------------+-------------------+
 *
 * \endverbatim
 *
 *  - M, m
 *
 *    Moves the current position to (`x`, `y`). The new position is either absolute (`M`) or relative to the current
 *    position (`m`). The initial position of `gr_path` is (0, 0).
 *
 *    Example:
 *
 *        double x[2] = {0.5, -0.1};
 *        double y[2] = {0.2, 0.1};
 *        gr_path(2, x, y, "Mm");
 *
 *    The first move command in this example moves the current position to the absolute coordinates (0.5, 0.2). The
 *    second move to performs a movement by (-0.1, 0.1) relative to the current position resulting in the point
 *    (0.4, 0.3).
 *
 *
 *  - L, l
 *
 *    Draws a line from the current position to the given position (`x`, `y`). The end point of the line is either
 *    absolute (`L`) or relative to the current position (`l`). The current position is set to the end point of the
 *    line.
 *
 *    Example:
 *
 *        double x[3] = {0.1, 0.5, 0.0};
 *        double y[3] = {0.1, 0.1, 0.2};
 *        gr_path(3, x, y, "MLlS");
 *
 *    The first line to command draws a straight line from the current position (0.1, 0.1) to the absolute position
 *    (0.5, 0.1) resulting in a horizontal line. The second line to command draws a vertical line relative to the
 *    current position resulting in the end point (0.5, 0.3).
 *
 *
 *  - Q, q
 *
 *    Draws a quadratic bezier curve from the current position to the end point (`x2`, `y2`) using (`x1`, `y1`) as the
 *    control point. Both points are either absolute (`Q`) or relative to the current position (`q`). The current
 *    position is set to the end point of the bezier curve.
 *
 *    Example:
 *
 *        double x[5] = {0.1, 0.3, 0.5, 0.2, 0.4};
 *        double y[5] = {0.1, 0.2, 0.1, 0.1, 0.0};
 *        gr_path(5, x, y, "MQqS");
 *
 *    This example will generate two bezier curves whose start and end points are each located at y=0.1. As the control
 *    points are horizontally in the middle of each bezier curve with a higher y value both curves are symmetrical
 *    and bend slightly upwards in the middle. The current position is set to (0.9, 0.1) at the end.
 *
 *  - C, c
 *
 *    Draws a cubic bezier curve from the current position to the end point (`x3`, `y3`) using (`x1`, `y1`) and
 *    (`x2`, `y2`) as the control points. All three points are either absolute (`C`) or relative to the current position
 *    (`c`). The current position is set to the end point of the bezier curve.
 *
 *    Example:
 *
 *        double x[7] = {0.1, 0.2, 0.3, 0.4, 0.1, 0.2, 0.3};
 *        double y[7] = {0.1, 0.2, 0.0, 0.1, 0.1, -0.1, 0.0};
 *        gr_path(7, x, y, "MCcS");
 *
 *    This example will generate two bezier curves whose start and end points are each located at y=0.1. As the control
 *    points are equally spaced along the x-axis and the first is above and the second is below the start and end
 *    points this creates a wave-like shape for both bezier curves. The current position is set to (0.8, 0.1) at the
 *    end.
 *
 *
 *  - A, a
 *
 *    Draws an elliptical arc starting at the current position. The major axis of the ellipse is aligned with the x-axis
 *    and the minor axis is aligned with the y-axis of the plot. `rx` and `ry` are the ellipses radii along the major
 *    and minor axis. `a1` and `a2` define the start and end angle of the arc in radians. The current position is set
 *    to the end point of the arc. If `a2` is greater than `a1` the arc is drawn counter-clockwise, otherwise it is
 *    drawn clockwise. The `a` and `A` commands draw the same arc. The third coordinates of the `x` and `y` array are
 *    ignored and reserved for future use.
 *
 *    Examples:
 *
 *        double x[4] = {0.1, 0.2, -3.14159 / 2, 0.0};
 *        double y[4] = {0.1, 0.4, 3.14159 / 2, 0.0};
 *        gr_path(4, x, y, "MAS");
 *
 *    This example draws an arc starting at (0.1, 0.1). As the start angle -pi/2 is smaller than the end angle pi/2 the
 *    arc is drawn counter-clockwise. In this case the right half of an ellipse with an x radius of 0.2 and a y radius
 *    of 0.4 is shown. Therefore the current position is set to (0.1, 0.9) at the end.
 *
 *        double x[4] = {0.1, 0.2, 3.14159 / 2, 0.0};
 *        double y[4] = {0.9, 0.4, -3.14159 / 2, 0.0};
 *        gr_path(4, x, y, "MAS");
 *
 *    This examples draws the same arc as the previous one. The only difference is that the starting point is now at
 *    (0.1, 0.9) and the start angle pi/2 is greater than the end angle -pi/2 so that the ellipse arc is drawn
 * clockwise. Therefore the current position is set to (0.1, 0.1) at the end.
 *
 *  - Z
 *
 *    Closes the current path by connecting the current position to the target position of the last move command
 *    (`m` or `M`) with a straight line. If no move to was performed in this path it connects the current position to
 *    (0, 0). When the path is stroked this line will also be drawn.
 *
 *
 *  - S, s
 *
 *    Strokes the path with the current border width and border color (set with `gr_setborderwidth` and
 *    `gr_setbordercolorind`). In case of `s` the path is closed beforehand, which is equivalent to `ZS`.
 *
 *
 *  - F, f
 *
 *    Fills the current path using the even-odd-rule using the current fill color. Filling a path implicitly closes the
 *    path. The fill color can be set using `gr_setfillcolorind`. In case of `F` the path is also stroked using the
 *    current border width and color afterwards.
 *
 */
void gr_path(int n, double *x, double *y, const char *codes)
{
  int i, len;

  check_autoinit;

  len = strlen(codes);
  if (len >= maxpath) reallocate(len);

  for (i = 0; i < len; i++) code[i] = (unsigned int)codes[i];

  gks_gdp(n, x, y, GKS_K_GDP_DRAW_PATH, len, code);
}

/*!
 * Return the position on the hyperboloid or sphere.
 */
static void gr_trackballposition(const double *mouse, double r, double *erg)
{
  double x, y, z;
  double fx, fy, fz, f_length;

  x = mouse[0];
  y = mouse[1];

  /* reverse projection */
  if (gpx.projection_type == GR_PROJECTION_ORTHOGRAPHIC)
    {
      x = (gpx.right - gpx.left) * (x + 1) * 0.5 + gpx.left;
      y = (gpx.bottom - gpx.top) * (-y + 1) * 0.5 + gpx.top;

      if (x * x + y * y <= r * r / sqrt(2))
        {
          /* sphere */
          z = sqrt(r * r - (x * x + y * y));
        }
      else
        {
          /* hyperboloid */
          z = r * r / (2 * sqrt(x * x + y * y));
        }
    }
  else
    {
      /* point on z_near */
      double opposite_catheter = tan(gpx.fov / 2);
      double factor_x = x * opposite_catheter * (ix.xmax - ix.xmin) / (ix.ymax - ix.ymin);
      double factor_y = y * opposite_catheter;
      double d[3] = {tx.focus_point_x - tx.camera_pos_x, tx.focus_point_y - tx.camera_pos_y,
                     tx.focus_point_z - tx.camera_pos_z};
      /* distance to the focuspoint as offset for the calculated z */
      double distance = sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]);

      z = (distance - sqrt(-distance * distance * (factor_x * factor_x + factor_y * factor_y) +
                           r * r * (factor_x * factor_x + factor_y * factor_y + 1))) /
          (factor_x * factor_x + factor_y * factor_y + 1);
      x = factor_x * z;
      y = factor_y * z;

      if (x * x + y * y > r * r / sqrt(2))
        {
          /* position wasnt onside the trackball */
          z = distance / 2 -
              sqrt(distance * distance / 4 + r * r / (2 * sqrt(factor_x * factor_x + factor_y * factor_y)));
          x = factor_x * z;
          y = factor_y * z;
        }
    }

  /* calculate and normalize the forward direction */
  fx = tx.focus_point_x - tx.camera_pos_x;
  fy = tx.focus_point_y - tx.camera_pos_y;
  fz = tx.focus_point_z - tx.camera_pos_z;
  f_length = sqrt(fx * fx + fy * fy + fz * fz);
  fx /= f_length;
  fy /= f_length;
  fz /= f_length;

  /* transform the point into the camera system */
  erg[0] = x * tx.s_x + y * tx.up_x + z * fx;
  erg[1] = x * tx.s_y + y * tx.up_y + z * fy;
  erg[2] = x * tx.s_z + y * tx.up_z + z * fz;
}

/*!
 * Return the radius of the minimum bounding sphere for the current 3D window
 */
static void gr_calculateradius(double *radius)
{
  int i;
  double max = 0;
  double left = pow(ix.xmin - tx.focus_point_x, 2);
  double right = pow(ix.xmax - tx.focus_point_x, 2);
  double bottom = pow(ix.ymin - tx.focus_point_y, 2);
  double top = pow(ix.ymax - tx.focus_point_y, 2);
  double z_far = pow(ix.zmin - tx.focus_point_z, 2);
  double z_near = pow(ix.zmax - tx.focus_point_z, 2);

  /* all eight coordinates quadrants */
  double rtf_radius = sqrt(right + top + z_far);
  double rtn_radius = sqrt(right + top + z_near);
  double ltf_radius = sqrt(left + top + z_far);
  double ltn_radius = sqrt(left + top + z_near);
  double rbf_radius = sqrt(right + bottom + z_far);
  double rbn_radius = sqrt(right + bottom + z_near);
  double lbf_radius = sqrt(left + bottom + z_far);
  double lbn_radius = sqrt(left + bottom + z_near);
  double radii[8] = {rtf_radius, rtn_radius, ltf_radius, ltn_radius, rbf_radius, rbn_radius, lbf_radius, lbn_radius};

  /* find the max radius*/
  for (i = 0; i < 8; i++)
    {
      if (max < radii[i])
        {
          max = radii[i];
        }
    }

  *radius = max;
}

/*!
 * Rotate the current scene according to a virtual arcball.
 *
 * \param start_mouse_pos_x x component of the start mouse position
 * \param start_mouse_pos_y y component of the start mouse position
 * \param end_mouse_pos_x x component of the end mouse position
 * \param end_mouse_pos_y y component of the end mouse position
 *
 * This function requires values between 0 (left side or bottom of the drawing
 * area) and 1 (right side or top of the drawing area).
 */
void gr_camerainteraction(double start_mouse_pos_x, double start_mouse_pos_y, double end_mouse_pos_x,
                          double end_mouse_pos_y)
{
  check_autoinit;

  /* check if there is any mouse movement */
  if (start_mouse_pos_x != end_mouse_pos_x || start_mouse_pos_y != end_mouse_pos_y)
    {
      double start[3], end[3];
      double radius, camera_distance;
      double start_length, end_length;
      double cos_angle, c, s;
      double axis_x, axis_y, axis_z, axis_length;
      double fx, fy, fz, f_length;
      double axis_projection_forward;
      double cam_x, cam_y, cam_z, up_x, up_y, up_z, sx, sy, sz;

      gr_calculateradius(&radius);

      camera_distance = radius;
      /* parameter for better overview */
      if (gpx.projection_type == GR_PROJECTION_PERSPECTIVE)
        {
          camera_distance = fabs(radius / sin((gpx.fov * M_PI / 180) / 2));
        }

      /* transform mouseposition onto [-1, 1] */
      double mouse_start[3] = {(start_mouse_pos_x * 2 - 1), (2 * start_mouse_pos_y - 1), 0};
      double mouse_end[3] = {(end_mouse_pos_x * 2 - 1), (2 * end_mouse_pos_y - 1), 0};

      /* get the trackball positions of the start and end mouseposition */
      gr_trackballposition(mouse_start, camera_distance, start);
      gr_trackballposition(mouse_end, camera_distance, end);

      /* calculate the rotation axis and the angle of the rotation */
      start_length = sqrt(start[0] * start[0] + start[1] * start[1] + start[2] * start[2]);
      end_length = sqrt(end[0] * end[0] + end[1] * end[1] + end[2] * end[2]);
      cos_angle = (start[0] * end[0] + start[1] * end[1] + start[2] * end[2]) / start_length / end_length;
      axis_x = start[1] * end[2] - end[1] * start[2];
      axis_y = start[2] * end[0] - end[2] * start[0];
      axis_z = start[0] * end[1] - end[0] * start[1];

      /* calculate and normalize the forward direction */
      fx = tx.focus_point_x - tx.camera_pos_x;
      fy = tx.focus_point_y - tx.camera_pos_y;
      fz = tx.focus_point_z - tx.camera_pos_z;
      f_length = sqrt(fx * fx + fy * fy + fz * fz);
      fx /= f_length;
      fy /= f_length;
      fz /= f_length;

      /* reverse the rotation axis around the forward axis */
      axis_projection_forward = axis_x * fx + axis_y * fy + axis_z * fz;
      axis_x -= fx * axis_projection_forward * 2;
      axis_y -= fy * axis_projection_forward * 2;
      axis_z -= fz * axis_projection_forward * 2;
      axis_length = sqrt(axis_x * axis_x + axis_y * axis_y + axis_z * axis_z);
      axis_x /= axis_length;
      axis_y /= axis_length;
      axis_z /= axis_length;

      /* shorter names for sin and cos to reduce the size of the matrix */
      c = cos_angle;
      s = sqrt(1 - c * c);

      double rotation_mat[9] = {axis_x * axis_x * (1 - c) + c,          axis_x * axis_y * (1 - c) - axis_z * s,
                                axis_x * axis_z * (1 - c) + axis_y * s, axis_y * axis_x * (1 - c) + axis_z * s,
                                axis_y * axis_y * (1 - c) + c,          axis_y * axis_z * (1 - c) - axis_x * s,
                                axis_z * axis_x * (1 - c) - axis_y * s, axis_z * axis_y * (1 - c) + axis_x * s,
                                axis_z * axis_z * (1 - c) + c};

      /* rotate camera position */
      cam_x = (tx.camera_pos_x - tx.focus_point_x) * rotation_mat[0] +
              (tx.camera_pos_y - tx.focus_point_y) * rotation_mat[1] +
              (tx.camera_pos_z - tx.focus_point_z) * rotation_mat[2] + tx.focus_point_x;
      cam_y = (tx.camera_pos_x - tx.focus_point_x) * rotation_mat[3] +
              (tx.camera_pos_y - tx.focus_point_y) * rotation_mat[4] +
              (tx.camera_pos_z - tx.focus_point_z) * rotation_mat[5] + tx.focus_point_y;
      cam_z = (tx.camera_pos_x - tx.focus_point_x) * rotation_mat[6] +
              (tx.camera_pos_y - tx.focus_point_y) * rotation_mat[7] +
              (tx.camera_pos_z - tx.focus_point_z) * rotation_mat[8] + tx.focus_point_z;

      /* rotate up and right camera axes */
      up_x = tx.up_x * rotation_mat[0] + tx.up_y * rotation_mat[1] + tx.up_z * rotation_mat[2];
      up_y = tx.up_x * rotation_mat[3] + tx.up_y * rotation_mat[4] + tx.up_z * rotation_mat[5];
      up_z = tx.up_x * rotation_mat[6] + tx.up_y * rotation_mat[7] + tx.up_z * rotation_mat[8];
      sx = tx.s_x * rotation_mat[0] + tx.s_y * rotation_mat[1] + tx.s_z * rotation_mat[2];
      sy = tx.s_x * rotation_mat[3] + tx.s_y * rotation_mat[4] + tx.s_z * rotation_mat[5];
      sz = tx.s_x * rotation_mat[6] + tx.s_y * rotation_mat[7] + tx.s_z * rotation_mat[8];

      /* save the new calculated values for the transformation */
      tx.camera_pos_x = cam_x;
      tx.camera_pos_y = cam_y;
      tx.camera_pos_z = cam_z;
      tx.up_x = up_x;
      tx.up_y = up_y;
      tx.up_z = up_z;
      tx.s_x = sx;
      tx.s_y = sy;
      tx.s_z = sz;
    }

  if (flag_graphics)
    gr_writestream("<camerainteraction start_mouse_pos_x=\"%g\" start_mouse_pos_y=\"%g\" end_mouse_pos_x=\"%g\" "
                   "end_mouse_pos_y=\"%g\"/>\n",
                   start_mouse_pos_x, start_mouse_pos_y, end_mouse_pos_x, end_mouse_pos_y);
}

/*!
 * Set the three dimensional window. Only used for perspective and orthographic projection.
 *
 * \param xmin min x-value
 * \param xmax max x-value
 * \param ymin min y-value
 * \param ymax max y-value
 * \param zmin min z-value
 * \param zmax max z-value
 */
void gr_setwindow3d(double xmin, double xmax, double ymin, double ymax, double zmin, double zmax)
{
  check_autoinit;

  ix.xmin = xmin;
  ix.xmax = xmax;
  ix.ymin = ymin;
  ix.ymax = ymax;
  ix.zmin = zmin;
  ix.zmax = zmax;

  if (flag_graphics)
    gr_writestream("<setwindow3d xmin=\"%g\" xmax=\"%g\" ymin=\"%g\" ymax=\"%g\" zmin=\"%g\" zmax=\"%g\"/>\n", xmin,
                   xmax, ymin, ymax, zmin, zmax);
}

/*!
 * Set the scale factor for each axis.
 *
 * \param x_axis_scale factor for scaling the x-axis
 * \param y_axis_scale factor for scaling the y-axis
 * \param z_axis_scale factor for scaling the z-axis
 *
 * The scaling factors must not be zero.
 */
void gr_setscalefactors3d(double x_axis_scale, double y_axis_scale, double z_axis_scale)
{
  check_autoinit;

  if (x_axis_scale == 0 || y_axis_scale == 0 || z_axis_scale == 0)
    {
      fprintf(stderr, "Invalid scale factor. Please check your parameters again.\n");
      return;
    }

  tx.x_axis_scale = x_axis_scale;
  tx.y_axis_scale = y_axis_scale;
  tx.z_axis_scale = z_axis_scale;

  if (flag_graphics)
    gr_writestream("<setscalefactors3d x_axis_scale=\"%g\" y_axis_scale=\"%g\" z_axis_scale=\"%g\"/>\n", x_axis_scale,
                   y_axis_scale, z_axis_scale);
}

/*!
 * Returns the scaling factor for each axis.
 */
void gr_inqscalefactors3d(double *x_axis_scale, double *y_axis_scale, double *z_axis_scale)
{
  *x_axis_scale = tx.x_axis_scale;
  *y_axis_scale = tx.y_axis_scale;
  *z_axis_scale = tx.z_axis_scale;
}

/*!
 * Define the border width of subsequent path output primitives.
 *
 * \param[in] width The border width scale factor
 */
void gr_setborderwidth(double width)
{
  check_autoinit;

  gks_set_border_width(width);
  if (ctx) ctx->bwidth = width;

  if (flag_graphics) gr_writestream("<setborderwidth width=\"%g\"/>\n", width);
}

void gr_inqborderwidth(double *width)
{
  int errind;

  check_autoinit;

  gks_inq_border_width(&errind, width);
}

/*!
 * Define the border color of subsequent path output primitives.
 *
 * \param[in] color The border color index (COLOR < 1256)
 */
void gr_setbordercolorind(int color)
{
  check_autoinit;

  gks_set_border_color_index(color);
  if (ctx) ctx->bcoli = color;

  if (flag_graphics) gr_writestream("<setbordercolorind color=\"%d\"/>\n", color);
}

void gr_inqbordercolorind(int *coli)
{
  int errind;

  check_autoinit;

  gks_inq_border_color_index(&errind, coli);
}

void gr_selectclipxform(int tnr)
{
  check_autoinit;

  gks_select_clip_xform(tnr);
  if (ctx) ctx->clip_tnr = tnr;

  if (flag_graphics) gr_writestream("<selectclipxform tnr=\"%d\"/>\n", tnr);
}

void gr_inqclipxform(int *tnr)
{
  int errind;

  check_autoinit;

  gks_inq_clip_xform(&errind, tnr);
}

/*!
 * Set the camera for orthographic or perspective projection.
 *
 * The center of the 3d window is used as the focus point and the camera is
 * positioned relative to it, using camera distance, azimuthal angle and polar
 * angle similar to gr_setspace.  This function can be used if the user
 * prefers spherical coordinates to setting the camera position directly, but
 * has reduced functionality in comparison to gr_settransformationparameters,
 * gr_setscalefactors3d, gr_setperspectiveprojection and
 * gr_setorthographicprojection.
 *
 * \param phi azimuthal angle of the spherical coordinates
 * \param theta polar angle of the spherical coordinates
 * \param fov vertical field of view(0 or NaN for orthographic projection)
 * \param camera_distance distance between the camera and the focus point (in
 * arbitrary units, 0 or NaN for the radius of the object's smallest bounding
 * sphere)
 */
void gr_setspace3d(double phi, double theta, double fov, double camera_distance)
{
  double scale_factor_x, scale_factor_y, scale_factor_z;
  double bounding_sphere_radius;

  tx.focus_point_x = (ix.xmax + ix.xmin) / 2;
  tx.focus_point_y = (ix.ymin + ix.ymax) / 2;
  tx.focus_point_z = (ix.zmax + ix.zmin) / 2;

  bounding_sphere_radius = sqrt(3);
  if (fov != fov || fov == 0)
    {
      if (camera_distance == 0 || camera_distance != camera_distance)
        {
          camera_distance = bounding_sphere_radius;
        }
      gr_setorthographicprojection(-camera_distance, camera_distance, -camera_distance, camera_distance,
                                   -camera_distance * 2, camera_distance * 2);
    }
  else
    {
      if (camera_distance == 0 || camera_distance != camera_distance)
        {
          camera_distance = fabs(bounding_sphere_radius / sin((fov * M_PI / 180) / 2));
        }
      gr_setperspectiveprojection(max(0.01, camera_distance - bounding_sphere_radius * 1.01),
                                  camera_distance + bounding_sphere_radius * 2, fov);
    }

  scale_factor_x = 2.0 / (ix.xmax - ix.xmin);
  scale_factor_y = 2.0 / (ix.ymax - ix.ymin);
  scale_factor_z = 2.0 / (ix.zmax - ix.zmin);


  gr_settransformationparameters(
      camera_distance * sin(theta * M_PI / 180) * cos(phi * M_PI / 180) + tx.focus_point_x * scale_factor_x,
      camera_distance * sin(theta * M_PI / 180) * sin(phi * M_PI / 180) + tx.focus_point_y * scale_factor_y,
      camera_distance * cos(theta * M_PI / 180) + tx.focus_point_z * scale_factor_z,
      -cos(phi * M_PI / 180) * cos(theta * M_PI / 180), -sin(phi * M_PI / 180) * cos(theta * M_PI / 180),
      sin(theta * M_PI / 180), tx.focus_point_x * scale_factor_x, tx.focus_point_y * scale_factor_y,
      tx.focus_point_z * scale_factor_z);

  gr_setscalefactors3d(scale_factor_x, scale_factor_y, scale_factor_z);
}

void gr_settextencoding(int encoding)
{
  check_autoinit;

  gks_set_encoding(encoding);

  if (flag_graphics) gr_writestream("<settextencoding encoding=\"%d\"/>\n", encoding);
}

void gr_inqtextencoding(int *encoding)
{
  check_autoinit;

  gks_inq_encoding(encoding);
}

void gr_setcallback(char *(*callback)(const char *arg))
{
  check_autoinit;

  gks_set_callback(callback);
}
