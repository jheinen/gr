#ifndef _GKS_H_
#define _GKS_H_

#include <stdio.h>

#ifdef _WIN32

#include <windows.h> /* required for all Windows applications */
#define DLLEXPORT __declspec(dllexport)
#else
#define DLLEXPORT
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* default connection identifier */

#define GKS_K_CONID_DEFAULT NULL

/* default workstation type */

#define GKS_K_WSTYPE_DEFAULT 0

/* standard workstation types */

#define GKS_K_WSTYPE_MO 2
#define GKS_K_WSTYPE_MI 3
#define GKS_K_WSTYPE_WISS 5

/* aspect source flag */

#define GKS_K_ASF_BUNDLED 0
#define GKS_K_ASF_INDIVIDUAL 1

/* clear control flag */

#define GKS_K_CLEAR_CONDITIONALLY 0
#define GKS_K_CLEAR_ALWAYS 1

/* clipping indicator */

#define GKS_K_NOCLIP 0
#define GKS_K_CLIP 1

/* coordinate switch */

#define GKS_K_COORDINATES_WC 0
#define GKS_K_COORDINATES_NDC 1

/* device coordinate units */

#define GKS_K_METERS 0
#define GKS_K_OTHER_UNITS 1

/* fill area interior style */

#define GKS_K_INTSTYLE_HOLLOW 0
#define GKS_K_INTSTYLE_SOLID 1
#define GKS_K_INTSTYLE_PATTERN 2
#define GKS_K_INTSTYLE_HATCH 3
#define GKS_K_INTSTYLE_SOLID_WITH_BORDER 4

/* input device status */

#define GKS_K_STATUS_NONE 0
#define GKS_K_STATUS_OK 1

/* level of GKS */

#define GKS_K_LEVEL_0A 0
#define GKS_K_LEVEL_0B 1
#define GKS_K_LEVEL_0C 2
#define GKS_K_LEVEL_1A 3
#define GKS_K_LEVEL_1B 4
#define GKS_K_LEVEL_1C 5
#define GKS_K_LEVEL_2A 6
#define GKS_K_LEVEL_2B 7
#define GKS_K_LEVEL_2C 8

/* operating state value */

#define GKS_K_GKCL 0
#define GKS_K_GKOP 1
#define GKS_K_WSOP 2
#define GKS_K_WSAC 3
#define GKS_K_SGOP 4

/* regeneration flag */

#define GKS_K_POSTPONE_FLAG 0
#define GKS_K_PERFORM_FLAG 1
#define GKS_K_WRITE_PAGE_FLAG 2

/* text alignment horizontal */

#define GKS_K_TEXT_HALIGN_NORMAL 0
#define GKS_K_TEXT_HALIGN_LEFT 1
#define GKS_K_TEXT_HALIGN_CENTER 2
#define GKS_K_TEXT_HALIGN_RIGHT 3

/* text alignment vertical */

#define GKS_K_TEXT_VALIGN_NORMAL 0
#define GKS_K_TEXT_VALIGN_TOP 1
#define GKS_K_TEXT_VALIGN_CAP 2
#define GKS_K_TEXT_VALIGN_HALF 3
#define GKS_K_TEXT_VALIGN_BASE 4
#define GKS_K_TEXT_VALIGN_BOTTOM 5

/* text path */

#define GKS_K_TEXT_PATH_RIGHT 0
#define GKS_K_TEXT_PATH_LEFT 1
#define GKS_K_TEXT_PATH_UP 2
#define GKS_K_TEXT_PATH_DOWN 3

/* text precision */

#define GKS_K_TEXT_PRECISION_STRING 0
#define GKS_K_TEXT_PRECISION_CHAR 1
#define GKS_K_TEXT_PRECISION_STROKE 2
#define GKS_K_TEXT_PRECISION_OUTLINE 3

/* workstation category */

#define GKS_K_WSCAT_OUTPUT 0
#define GKS_K_WSCAT_INPUT 1
#define GKS_K_WSCAT_OUTIN 2
#define GKS_K_WSCAT_WISS 3
#define GKS_K_WSCAT_MO 4
#define GKS_K_WSCAT_MI 5

/* workstation state */

#define GKS_K_WS_INACTIVE 0
#define GKS_K_WS_ACTIVE 1

/* standard linetypes */

#define GKS_K_LINETYPE_SOLID 1
#define GKS_K_LINETYPE_DASHED 2
#define GKS_K_LINETYPE_DOTTED 3
#define GKS_K_LINETYPE_DASHED_DOTTED 4

/* GKS specific linetypes */

#define GKS_K_LINETYPE_DASH_2_DOT -1
#define GKS_K_LINETYPE_DASH_3_DOT -2
#define GKS_K_LINETYPE_LONG_DASH -3
#define GKS_K_LINETYPE_LONG_SHORT_DASH -4
#define GKS_K_LINETYPE_SPACED_DASH -5
#define GKS_K_LINETYPE_SPACED_DOT -6
#define GKS_K_LINETYPE_DOUBLE_DOT -7
#define GKS_K_LINETYPE_TRIPLE_DOT -8

/* standard markertypes */

#define GKS_K_MARKERTYPE_DOT 1
#define GKS_K_MARKERTYPE_PLUS 2
#define GKS_K_MARKERTYPE_ASTERISK 3
#define GKS_K_MARKERTYPE_CIRCLE 4
#define GKS_K_MARKERTYPE_DIAGONAL_CROSS 5

/* GKS specific markertypes */

#define GKS_K_MARKERTYPE_SOLID_CIRCLE -1
#define GKS_K_MARKERTYPE_TRIANGLE_UP -2
#define GKS_K_MARKERTYPE_SOLID_TRI_UP -3
#define GKS_K_MARKERTYPE_TRIANGLE_DOWN -4
#define GKS_K_MARKERTYPE_SOLID_TRI_DOWN -5
#define GKS_K_MARKERTYPE_SQUARE -6
#define GKS_K_MARKERTYPE_SOLID_SQUARE -7
#define GKS_K_MARKERTYPE_BOWTIE -8
#define GKS_K_MARKERTYPE_SOLID_BOWTIE -9
#define GKS_K_MARKERTYPE_HOURGLASS -10
#define GKS_K_MARKERTYPE_SOLID_HGLASS -11
#define GKS_K_MARKERTYPE_DIAMOND -12
#define GKS_K_MARKERTYPE_SOLID_DIAMOND -13
#define GKS_K_MARKERTYPE_STAR -14
#define GKS_K_MARKERTYPE_SOLID_STAR -15
#define GKS_K_MARKERTYPE_TRI_UP_DOWN -16
#define GKS_K_MARKERTYPE_SOLID_TRI_RIGHT -17
#define GKS_K_MARKERTYPE_SOLID_TRI_LEFT -18
#define GKS_K_MARKERTYPE_HOLLOW_PLUS -19
#define GKS_K_MARKERTYPE_SOLID_PLUS -20
#define GKS_K_MARKERTYPE_PENTAGON -21
#define GKS_K_MARKERTYPE_HEXAGON -22
#define GKS_K_MARKERTYPE_HEPTAGON -23
#define GKS_K_MARKERTYPE_OCTAGON -24
#define GKS_K_MARKERTYPE_STAR_4 -25
#define GKS_K_MARKERTYPE_STAR_5 -26
#define GKS_K_MARKERTYPE_STAR_6 -27
#define GKS_K_MARKERTYPE_STAR_7 -28
#define GKS_K_MARKERTYPE_STAR_8 -29
#define GKS_K_MARKERTYPE_VLINE -30
#define GKS_K_MARKERTYPE_HLINE -31
#define GKS_K_MARKERTYPE_OMARK -32

/* Type of output values set */

#define GKS_K_VALUE_SET 0
#define GKS_K_VALUE_REALIZED 1

/* Resample flags */

#define GKS_K_UPSAMPLE_VERTICAL_DEFAULT 0x00000000u
#define GKS_K_UPSAMPLE_HORIZONTAL_DEFAULT 0x00000000u
#define GKS_K_DOWNSAMPLE_VERTICAL_DEFAULT 0x00000000u
#define GKS_K_DOWNSAMPLE_HORIZONTAL_DEFAULT 0x00000000u
#define GKS_K_UPSAMPLE_VERTICAL_NEAREST 0x00000001u
#define GKS_K_UPSAMPLE_HORIZONTAL_NEAREST 0x00000100u
#define GKS_K_DOWNSAMPLE_VERTICAL_NEAREST 0x00010000u
#define GKS_K_DOWNSAMPLE_HORIZONTAL_NEAREST 0x01000000u
#define GKS_K_UPSAMPLE_VERTICAL_LINEAR 0x00000002u
#define GKS_K_UPSAMPLE_HORIZONTAL_LINEAR 0x00000200u
#define GKS_K_DOWNSAMPLE_VERTICAL_LINEAR 0x00020000u
#define GKS_K_DOWNSAMPLE_HORIZONTAL_LINEAR 0x02000000u
#define GKS_K_UPSAMPLE_VERTICAL_LANCZOS 0x00000003u
#define GKS_K_UPSAMPLE_HORIZONTAL_LANCZOS 0x00000300u
#define GKS_K_DOWNSAMPLE_VERTICAL_LANCZOS 0x00030000u
#define GKS_K_DOWNSAMPLE_HORIZONTAL_LANCZOS 0x03000000u

#define GKS_K_RESAMPLE_DEFAULT                                                                               \
  (GKS_K_UPSAMPLE_VERTICAL_DEFAULT | GKS_K_UPSAMPLE_HORIZONTAL_DEFAULT | GKS_K_DOWNSAMPLE_VERTICAL_DEFAULT | \
   GKS_K_DOWNSAMPLE_HORIZONTAL_DEFAULT)
#define GKS_K_RESAMPLE_NEAREST                                                                               \
  (GKS_K_UPSAMPLE_VERTICAL_NEAREST | GKS_K_UPSAMPLE_HORIZONTAL_NEAREST | GKS_K_DOWNSAMPLE_VERTICAL_NEAREST | \
   GKS_K_DOWNSAMPLE_HORIZONTAL_NEAREST)
#define GKS_K_RESAMPLE_LINEAR                                                                             \
  (GKS_K_UPSAMPLE_VERTICAL_LINEAR | GKS_K_UPSAMPLE_HORIZONTAL_LINEAR | GKS_K_DOWNSAMPLE_VERTICAL_LINEAR | \
   GKS_K_DOWNSAMPLE_HORIZONTAL_LINEAR)
#define GKS_K_RESAMPLE_LANCZOS                                                                               \
  (GKS_K_UPSAMPLE_VERTICAL_LANCZOS | GKS_K_UPSAMPLE_HORIZONTAL_LANCZOS | GKS_K_DOWNSAMPLE_VERTICAL_LANCZOS | \
   GKS_K_DOWNSAMPLE_HORIZONTAL_LANCZOS)

/* GKS generalized drawing primitive (GDP) function IDs */

#define GKS_K_GDP_DRAW_PATH 1
#define GKS_K_GDP_DRAW_LINES 2
#define GKS_K_GDP_DRAW_MARKERS 3
#define GKS_K_GDP_DRAW_TRIANGLES 4

/* GKS error codes */

#define GKS_K_NO_ERROR 0
#define GKS_K_ERROR 1

/* Simple types */

typedef FILE Gfile;
typedef char Gchar;
typedef char Gconn;
typedef double Gfloat;
typedef int Gwstype;
typedef int Gint;
typedef unsigned int Guint;
typedef long Glong;

#define GWC_DEF NULL
#define GCONID_DEFAULT NULL

#define GWS_DEFAULT NULL /* default Workstation type */
#define GWS_DEF NULL

/* Enumerations */

typedef enum
{ /* aspect control flag */
  GBUNDLED,
  GINDIVIDUAL
} Gasf;

typedef enum
{ /* clipping indicator */
  GCLIP,
  GNOCLIP
} Gclip;

typedef enum
{ /* clear control flag */
  GCONDITIONALLY,
  GALWAYS
} Gclrflag;

typedef enum
{ /* co-ord switch */
  GWC,
  GNDC
} Gcsw;

typedef enum
{ /* device co-ord units */
  GDC_METRES,
  GDC_OTHER
} Gdevunits;

typedef enum
{ /* fill area interior style */
  GHOLLOW,
  GSOLID,
  GPATTERN,
  GHATCH
} Gflinter;

typedef enum
{ /* request status */
  GOK,
  GNONE
} Gistat;

typedef enum
{ /* line type - not standard */
  GLN_SOLID = 1,
  GLN_DASHED,
  GLN_DOTTED,
  GLN_DASHDOT,
  GLN_TRIPLE_DOT = -8,
  GLN_DOUBLE_DOT,
  GLN_SPACED_DOT,
  GLN_SPACED_DASH,
  GLN_LONG_SHORT_DASH,
  GLN_LONG_DASH,
  GLN_DASH_3_DOT,
  GLN_DASH_2_DOT
} Glntype;

typedef enum
{ /* marker type - not standard */
  GMK_POINT = 1,
  GMK_PLUS,
  GMK_STAR,
  GMK_O,
  GMK_X,
  GMK_SOLID_DIAMOND = -13,
  GMK_DIAMOND,
  GMK_SOLID_HGLASS,
  GMK_HOURGLASS,
  GMK_SOLID_BOWTIE,
  GMK_BOWTIE,
  GMK_SOLID_SQUARE,
  GMK_SQUARE,
  GMK_SOLID_TRI_DOWN,
  GMK_TRIANGLE_DOWN,
  GMK_SOLID_TRI_UP,
  GMK_TRIANGLE_UP,
  GMK_SOLID_CIRCLE
} Gmktype;

typedef enum
{        /* GKS operating state */
  GGKCL, /* closed */
  GGKOP, /* open */
  GWSOP, /* workstation open */
  GWSAC, /* workstation active */
  GSGOP  /* segment open */
} Gopst;

typedef enum
{ /* regeneration flag */
  GPERFORM,
  GPOSTPONE
} Gregen;

typedef enum
{ /* horiz text alignment component */
  GAH_NORMAL,
  GAH_LEFT,
  GAH_CENTRE,
  GAH_RIGHT
} Gtxhor;

typedef enum
{ /* text path */
  GTP_RIGHT,
  GTP_LEFT,
  GTP_UP,
  GTP_DOWN
} Gtxpath;

typedef enum
{ /* text precision */
  GP_STRING,
  GP_CHAR,
  GP_STROKE
} Gtxprec;

typedef enum
{ /* vert text alignment component */
  GAV_NORMAL,
  GAV_TOP,
  GAV_CAP,
  GAV_HALF,
  GAV_BASE,
  GAV_BOTTOM
} Gtxver;

typedef enum
{ /* WS category */
  GOUTPUT,
  GINPUT,
  GOUTIN,
  GWISS,
  GMO,
  GMI
} Gwscat;

/* Forward type definitions */

typedef struct
{         /* integer point */
  Gint x; /* x coordinate */
  Gint y; /* y coordinate */
} Gipoint;

typedef struct
{           /* coordinate point */
  Gfloat x; /* X coordinate */
  Gfloat y; /* Y coordinate */
} Gpoint;

typedef struct
{              /* coordinate limits */
  Gfloat xmin; /* x minimum limit */
  Gfloat xmax; /* x maximum limit */
  Gfloat ymin; /* y minimum limit */
  Gfloat ymax; /* y maximum limit */
} Glimit;

typedef struct
{               /* text facilities */
  Gint font;    /* text font */
  Gtxprec prec; /* text precision */
} Gtxfp;

typedef struct
{             /* text alignment */
  Gtxhor hor; /* horizontal component */
  Gtxver ver; /* vertical component */
} Gtxalign;

typedef struct
{            /* coordinate rectangle pointer */
  Gpoint ul; /* upper left point */
  Gpoint lr; /* lower right point */
} Grect;

typedef struct
{              /* dimension in integer values */
  Guint x_dim; /* X dimension */
  Guint y_dim; /* Y dimension */
} Gidim;

/* Structs */

typedef struct
{                 /* aspect source flags */
  Gasf ln_type;   /* line type */
  Gasf ln_width;  /* line width */
  Gasf ln_colour; /* line colour */
  Gasf mk_type;   /* marker type */
  Gasf mk_size;   /* marker size */
  Gasf mk_colour; /* marker colour */
  Gasf tx_fp;     /* text font and precision */
  Gasf tx_exp;    /* text expansion */
  Gasf tx_space;  /* text character spacing */
  Gasf tx_colour; /* text colour */
  Gasf fl_inter;  /* fill area interior style */
  Gasf fl_style;  /* fill area style index */
  Gasf fl_colour; /* fill area colour */
} Gasfs;

typedef struct
{               /* colour bundle */
  Gfloat red;   /* red intensity */
  Gfloat green; /* green intensity */
  Gfloat blue;  /* blue intensity */
} Gcobundl;

typedef struct
{             /* clipping rectangle */
  Gclip ind;  /* clipping indicator */
  Glimit rec; /* clipping rectangle */
} Gcliprect;

typedef struct
{                  /* display size */
  Gdevunits units; /* device coordinate units */
  Gpoint device;   /* device coordinate unit size */
  Gipoint raster;  /* raster unit size */
} Gdspsize;

typedef struct
{                  /* text extent */
  Gpoint concat;   /* concatenation point */
  Gpoint corner_1; /* corner 1 */
  Gpoint corner_2; /* corner 2 */
  Gpoint corner_3; /* corner 3 */
  Gpoint corner_4; /* corner 4 */
} Gextent;

typedef struct
{                  /* locator data */
  Gint transform;  /* normalization transformation number */
  Gpoint position; /* locator position */
} Gloc;

typedef struct
{                /* request locator */
  Gistat status; /* request status */
  Gloc loc;      /* locator data */
} Gqloc;

typedef struct
{                /* request string */
  Gistat status; /* request status */
  Gchar *string; /* string data */
} Gqstring;

typedef struct
{ /* scale vector */
  Gfloat x_scale;
  Gfloat y_scale;
} Gscale;

typedef struct
{           /* transformation */
  Glimit w; /* window */
  Glimit v; /* viewport */
} Gtran;

typedef struct
{              /* metafile item information */
  Gint type;   /* item type */
  Gint length; /* item length */
} Ggksmit;

/* GKS function prototypes */

DLLEXPORT void gks_init_gks(void);
DLLEXPORT void gks_open_gks(int errfil);
DLLEXPORT void gks_close_gks(void);
DLLEXPORT void gks_open_ws(int wkid, char *conid, int wtype);
DLLEXPORT void gks_close_ws(int wkid);
DLLEXPORT void gks_activate_ws(int wkid);
DLLEXPORT void gks_deactivate_ws(int wkid);
DLLEXPORT void gks_configure_ws(int wkid);
DLLEXPORT void gks_clear_ws(int wkid, int cofl);
DLLEXPORT void gks_redraw_seg_on_ws(int wkid);
DLLEXPORT void gks_update_ws(int wkid, int regfl);
DLLEXPORT void gks_set_deferral_state(int wkid, int defmo, int regmo);
DLLEXPORT void gks_escape(int funid, int dimidr, int *idr, int maxodr, int *lenodr, int *odr);
DLLEXPORT void gks_message(int wkid, char *message);

DLLEXPORT void gks_polyline(int n, double *pxa, double *pya);
DLLEXPORT void gks_polymarker(int n, double *pxa, double *pya);
DLLEXPORT void gks_text(double px, double py, char *str);
DLLEXPORT void gks_fillarea(int n, double *pxa, double *pya);
DLLEXPORT void gks_cellarray(double qx, double qy, double rx, double ry, int dimx, int dimy, int scol, int srow,
                             int ncol, int nrow, int *colia);
DLLEXPORT void gks_gdp(int n, double *px, double *py, int primid, int ldr, int *datrec);

DLLEXPORT void gks_set_pline_index(int index);
DLLEXPORT void gks_set_pline_linetype(int ltype);
DLLEXPORT void gks_set_pline_linewidth(double lwidth);
DLLEXPORT void gks_set_pline_color_index(int coli);
DLLEXPORT void gks_set_pmark_index(int index);
DLLEXPORT void gks_set_pmark_type(int mtype);
DLLEXPORT void gks_set_pmark_size(double mszsc);
DLLEXPORT void gks_set_pmark_color_index(int coli);
DLLEXPORT void gks_set_text_index(int index);
DLLEXPORT void gks_set_text_fontprec(int font, int prec);
DLLEXPORT void gks_set_text_expfac(double chxp);
DLLEXPORT void gks_set_text_spacing(double chsp);
DLLEXPORT void gks_set_text_color_index(int coli);
DLLEXPORT void gks_set_text_height(double chh);
DLLEXPORT void gks_set_text_upvec(double chux, double chuy);
DLLEXPORT void gks_set_text_path(int txp);
DLLEXPORT void gks_set_text_align(int txalh, int txalv);
DLLEXPORT void gks_set_fill_index(int index);
DLLEXPORT void gks_set_fill_int_style(int ints);
DLLEXPORT void gks_set_fill_style_index(int styli);
DLLEXPORT void gks_set_fill_color_index(int coli);

DLLEXPORT void gks_set_asf(int *flag);
DLLEXPORT void gks_set_color_rep(int wkid, int index, double red, double green, double blue);

DLLEXPORT void gks_set_window(int tnr, double xmin, double xmax, double ymin, double ymax);
DLLEXPORT void gks_set_viewport(int tnr, double xmin, double xmax, double ymin, double ymax);
DLLEXPORT void gks_select_xform(int tnr);
DLLEXPORT void gks_set_clipping(int clsw);
DLLEXPORT void gks_set_ws_window(int wkid, double xmin, double xmax, double ymin, double ymax);
DLLEXPORT void gks_set_ws_viewport(int wkid, double xmin, double xmax, double ymin, double ymax);

DLLEXPORT void gks_create_seg(int segn);
DLLEXPORT void gks_close_seg(void);
DLLEXPORT void gks_delete_seg(int segn);
DLLEXPORT void gks_assoc_seg_with_ws(int wkid, int segn);
DLLEXPORT void gks_copy_seg_to_ws(int wkid, int segn);
DLLEXPORT void gks_set_seg_xform(int segn, double mat[3][2]);

DLLEXPORT void gks_initialize_locator(int wkid, int lcdnr, int tnr, double px, double py, int pet, double xmin,
                                      double xmax, double ymin, double ymax, int ldr, char *datrec);
DLLEXPORT void gks_request_locator(int wkid, int lcdnr, int *stat, int *tnr, double *px, double *py);
DLLEXPORT void gks_request_stroke(int wkid, int skdnr, int n, int *stat, int *tnr, int *np, double *pxa, double *pya);
DLLEXPORT void gks_request_choice(int wkid, int chdnr, int *stat, int *chnr);
DLLEXPORT void gks_request_string(int wkid, int stdnr, int *stat, int *lostr, char *str);

DLLEXPORT void gks_read_item(int wkid, int lenidr, int maxodr, char *odr);
DLLEXPORT void gks_get_item(int wkid, int *type, int *lenodr);
DLLEXPORT void gks_interpret_item(int type, int lenidr, int dimidr, char *idr);
DLLEXPORT void gks_eval_xform_matrix(double fx, double fy, double transx, double transy, double phi, double scalex,
                                     double scaley, int coord, double tran[3][2]);

DLLEXPORT void gks_inq_operating_state(int *opsta);
DLLEXPORT void gks_inq_level(int *errind, int *lev);
DLLEXPORT void gks_inq_wstype(int n, int *errind, int *number, int *wtype);
DLLEXPORT void gks_inq_max_xform(int *errind, int *maxtnr);
DLLEXPORT void gks_inq_open_ws(int n, int *errind, int *ol, int *wkid);
DLLEXPORT void gks_inq_active_ws(int n, int *errind, int *ol, int *wkid);
DLLEXPORT void gks_inq_segn_ws(int wkid, int n, int *errind, int *ol, int *segn);
DLLEXPORT void gks_inq_color_rep(int wkid, int index, int type, int *errind, double *red, double *green, double *blue);
DLLEXPORT void gks_inq_pline_linetype(int *errind, int *ltype);
DLLEXPORT void gks_inq_pline_linewidth(int *errind, double *lwidth);
DLLEXPORT void gks_inq_pline_color_index(int *errind, int *coli);
DLLEXPORT void gks_inq_pmark_type(int *errind, int *mtype);
DLLEXPORT void gks_inq_pmark_size(int *errind, double *mszsc);
DLLEXPORT void gks_inq_pmark_color_index(int *errind, int *coli);
DLLEXPORT void gks_inq_text_fontprec(int *errind, int *font, int *prec);
DLLEXPORT void gks_inq_text_expfac(int *errind, double *chxp);
DLLEXPORT void gks_inq_text_spacing(int *errind, double *chsp);
DLLEXPORT void gks_inq_text_color_index(int *errind, int *coli);
DLLEXPORT void gks_inq_text_height(int *errind, double *chh);
DLLEXPORT void gks_inq_text_upvec(int *errind, double *chux, double *chuy);
DLLEXPORT void gks_inq_text_path(int *errind, int *txp);
DLLEXPORT void gks_inq_text_align(int *errind, int *txalh, int *txalv);
DLLEXPORT void gks_inq_fill_int_style(int *errind, int *ints);
DLLEXPORT void gks_inq_fill_style_index(int *errind, int *styli);
DLLEXPORT void gks_inq_fill_color_index(int *errind, int *coli);
DLLEXPORT void gks_inq_open_segn(int *errind, int *segn);
DLLEXPORT void gks_inq_current_xformno(int *errind, int *tnr);
DLLEXPORT void gks_inq_xform(int tnr, int *errind, double *wn, double *vp);
DLLEXPORT void gks_inq_clip(int *errind, int *clsw, double *clrt);
DLLEXPORT void gks_inq_ws_conntype(int wkid, int *errind, int *conid, int *wtype);
DLLEXPORT void gks_inq_ws_category(int wkid, int *errind, int *wscat);
DLLEXPORT void gks_inq_text_extent(int wkid, double px, double py, char *str, int *errind, double *cpx, double *cpy,
                                   double *tx, double *ty);
DLLEXPORT void gks_inq_max_ds_size(int wtype, int *errind, int *dcunit, double *rx, double *ry, int *lx, int *ly);

DLLEXPORT void gks_emergency_close(void);

DLLEXPORT void gks_set_text_slant(double slant);
DLLEXPORT void gks_draw_image(double x, double y, double scalex, double scaley, int width, int height, int *data);
DLLEXPORT void gks_set_shadow(double offsetx, double offsety, double blur);
DLLEXPORT void gks_set_transparency(double alpha);
DLLEXPORT void gks_set_coord_xform(double mat[3][2]);

DLLEXPORT void gks_begin_selection(int index, int kind);
DLLEXPORT void gks_end_selection(void);
DLLEXPORT void gks_move_selection(double x, double y);
DLLEXPORT void gks_resize_selection(int kind, double x, double y);

DLLEXPORT void gks_inq_bbox(int *errind, double *xmin, double *xmax, double *ymin, double *ymax);
DLLEXPORT void gks_inq_text_slant(int *errind, double *slant);

DLLEXPORT double gks_precision(void);

DLLEXPORT void gks_set_border_color_index(int coli);
DLLEXPORT void gks_inq_border_color_index(int *errind, int *coli);
DLLEXPORT void gks_set_border_width(double bwidth);
DLLEXPORT void gks_inq_border_width(int *errind, double *bwidth);

DLLEXPORT void gks_select_clip_xform(int tnr);
DLLEXPORT void gks_inq_clip_xform(int *errind, int *tnr);

DLLEXPORT void gks_set_resample_method(unsigned int flag);
DLLEXPORT void gks_inq_resample_method(unsigned int *flag);

DLLEXPORT void gks_ft_gdp(int n, double *px, double *py, int primid, int ldr, int *datrec);
DLLEXPORT void *gks_state(void);

/* Entry point definitions */

#define gsetlinecolorind gsetlinecolourind
#define gsetmarkercolorind gsetmarkercolourind
#define gsettextcolorind gsettextcolourind
#define gsetfillcolorind gsetfillcolourind
#define gsetcolorrep gsetcolourrep
#define ginqlinecolorind ginqlinecolourind
#define ginqmarkercolorind ginqmarkercolourind
#define ginqtextcolorind ginqtextcolourind
#define ginqfillcolorind ginqfillcolourind

DLLEXPORT int gopengks(Gfile *, Glong);
DLLEXPORT int gclosegks(void);
DLLEXPORT int gopenws(Gint, Gconn *, Gwstype *);
DLLEXPORT int gclosews(Gint);
DLLEXPORT int gactivatews(Gint);
DLLEXPORT int gdeactivatews(Gint);
DLLEXPORT int gconfigurews(Gint);
DLLEXPORT int gclearws(Gint, Gclrflag);
DLLEXPORT int gupdatews(Gint, Gregen);
DLLEXPORT int gmessage(Gint, Gchar *);
DLLEXPORT int gpolyline(Gint, Gpoint *);
DLLEXPORT int gpolymarker(Gint, Gpoint *);
DLLEXPORT int gtext(Gpoint *, Gchar *);
DLLEXPORT int gfillarea(Gint, Gpoint *);
DLLEXPORT int gcellarray(Grect *, Gidim *, Gint *);
DLLEXPORT int gsetasf(Gasfs *);
DLLEXPORT int gsetlineind(Gint);
DLLEXPORT int gsetlinetype(Gint);
DLLEXPORT int gsetlinewidth(Gfloat);
DLLEXPORT int gsetlinecolourind(Gint);
DLLEXPORT int gsetmarkerind(Gint);
DLLEXPORT int gsetmarkertype(Gint);
DLLEXPORT int gsetmarkersize(Gfloat);
DLLEXPORT int gsetmarkercolourind(Gint);
DLLEXPORT int gsettextind(Gint);
DLLEXPORT int gsettextfontprec(Gtxfp *);
DLLEXPORT int gsetcharexpan(Gfloat);
DLLEXPORT int gsetcharspace(Gfloat);
DLLEXPORT int gsettextcolourind(Gint);
DLLEXPORT int gsetcharheight(Gfloat);
DLLEXPORT int gsetcharup(Gpoint *);
DLLEXPORT int gsettextpath(Gtxpath);
DLLEXPORT int gsettextalign(Gtxalign *);
DLLEXPORT int gsetfillind(Gint);
DLLEXPORT int gsetfillintstyle(Gflinter);
DLLEXPORT int gsetfillstyle(Gint);
DLLEXPORT int gsetfillcolourind(Gint);
DLLEXPORT int gsetcolourrep(Gint, Gint, Gcobundl *);
DLLEXPORT int gsetwindow(Gint, Glimit *);
DLLEXPORT int gsetviewport(Gint, Glimit *);
DLLEXPORT int gselntran(Gint);
DLLEXPORT int gsetclip(Gclip);
DLLEXPORT int gsetwswindow(Gint, Glimit *);
DLLEXPORT int gsetwsviewport(Gint, Glimit *);
DLLEXPORT int greqloc(Gint, Gint, Gqloc *);
DLLEXPORT int greqstring(Gint, Gint, Gqstring *);
DLLEXPORT int gcreateseg(Gint);
DLLEXPORT int gcopysegws(Gint, Gint);
DLLEXPORT int gredrawsegws(Gint);
DLLEXPORT int gcloseseg(void);
DLLEXPORT int gevaltran(Gpoint *, Gpoint *, Gfloat, Gscale *, Gcsw, Gfloat[2][3]);
DLLEXPORT int gsetsegtran(Gint, Gfloat[2][3]);
DLLEXPORT int ginqopst(Gint *);
DLLEXPORT int ginqlevelgks(Gint *, Gint *);
DLLEXPORT int ginqmaxntrannum(Gint *, Gint *);
DLLEXPORT int ginqcharheight(Gfloat *, Gint *);
DLLEXPORT int ginqcharup(Gpoint *, Gint *);
DLLEXPORT int ginqtextpath(Gtxpath *, Gint *);
DLLEXPORT int ginqtextalign(Gtxalign *, Gint *);
DLLEXPORT int ginqlinetype(Gint *, Gint *);
DLLEXPORT int ginqlinewidth(Gfloat *, Gint *);
DLLEXPORT int ginqlinecolourind(Gint *, Gint *);
DLLEXPORT int ginqmarkertype(Gint *, Gint *);
DLLEXPORT int ginqmarkersize(Gfloat *, Gint *);
DLLEXPORT int ginqmarkercolourind(Gint *, Gint *);
DLLEXPORT int ginqtextfontprec(Gtxfp *, Gint *);
DLLEXPORT int ginqcharexpan(Gfloat *, Gint *);
DLLEXPORT int ginqcharspace(Gfloat *, Gint *);
DLLEXPORT int ginqtextcolourind(Gint *, Gint *);
DLLEXPORT int ginqfillintstyle(Gint *, Gint *);
DLLEXPORT int ginqfillstyle(Gint *, Gint *);
DLLEXPORT int ginqfillcolourind(Gint *, Gint *);
DLLEXPORT int ginqcurntrannum(Gint *, Gint *);
DLLEXPORT int ginqntran(Gint, Gtran *, Gint *);
DLLEXPORT int ginqclip(Gcliprect *, Gint *);
DLLEXPORT int ginqwscategory(Gwstype *, Gint *, Gint *);
DLLEXPORT int ginqdisplaysize(Gwstype *, Gdspsize *, Gint *);
DLLEXPORT int ginqtextextent(Gint, Gpoint *, Gchar *, Gextent *, Gint *);
DLLEXPORT int ginqnameopenseg(Gint *, Gint *);
DLLEXPORT int gemergencyclosegks(void);

#ifdef __cplusplus
}
#endif

#endif
