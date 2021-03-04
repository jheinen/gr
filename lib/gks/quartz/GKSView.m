
#include "gks.h"
#include "gkscore.h"

#import "GKSView.h"

#define MEMORY_INCREMENT 262144
#define PATTERNS 120
#define HATCH_STYLE 108
#define NUM_POINTS 10000

#define RESOLVE(arg, type, nbytes) \
  arg = (type *)(s + sp);          \
  sp += nbytes

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif

#ifndef max
#define max(a, b) (((a) > (b)) ? (a) : (b))
#endif

#define nint(a) (int)((a) + 0.5)

#define is_NaN(x) ((x) != (x))

#define WC_to_NDC(xw, yw, tnr, xn, yn) \
  xn = a[tnr] * (xw) + b[tnr];         \
  yn = c[tnr] * (yw) + d[tnr]

#define WC_to_NDC_rel(xw, yw, tnr, xn, yn) \
  xn = a[tnr] * (xw);                      \
  yn = c[tnr] * (yw)

#define NDC_to_DC(xn, yn, xd, yd) \
  xd = p->a * (xn) + p->b;        \
  yd = p->c * (yn) + p->d

#define DC_to_NDC(xd, yd, xn, yn) \
  xn = ((xd)-p->b) / p->a;        \
  yn = ((yd)-p->d) / p->c

#define CharXform(xrel, yrel, x, y)                      \
  x = cos(p->angle) * (xrel) - (sin(p->angle)) * (yrel); \
  y = sin(p->angle) * (xrel) + (cos(p->angle)) * (yrel)

static gks_state_list_t gkss_, *gkss;

static double a[MAX_TNR], b[MAX_TNR], c[MAX_TNR], d[MAX_TNR];

static int patArray[33];

static int predef_font[] = {1, 1, 1, -2, -3, -4};

static int predef_prec[] = {0, 1, 2, 2, 2, 2};

static int predef_ints[] = {0, 1, 3, 3, 3};

static int predef_styli[] = {1, 1, 1, 2, 3};

static char *fonts[] = {"Times-Roman",
                        "Times-Italic",
                        "Times-Bold",
                        "Times-BoldItalic",
                        "Helvetica",
                        "Helvetica-Oblique",
                        "Helvetica-Bold",
                        "Helvetica-BoldOblique",
                        "Courier",
                        "Courier-Oblique",
                        "Courier-Bold",
                        "Courier-BoldOblique",
                        "Symbol",
                        "Cochin",
                        "Cochin-Italic",
                        "Cochin-Bold",
                        "Cochin-BoldItalic",
                        "Baskerville",
                        "Baskerville-Italic",
                        "Baskerville-Bold",
                        "Baskerville-BoldItalic",
                        "Optima",
                        "Optima-Italic",
                        "Optima-Bold",
                        "Optima-BoldItalic",
                        "Palatino-Roman",
                        "Palatino-Italic",
                        "Palatino-Bold",
                        "Palatino-BoldItalic",
                        "Monotype Corsiva",
                        "ZapfDingbatsITC"};

static CGFontRef cgfontrefs[] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                                 NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                                 NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};

static double capheights[29] = {0.662, 0.660, 0.681, 0.662, 0.729, 0.729, 0.729, 0.729, 0.583, 0.583,
                                0.583, 0.583, 0.667, 0.681, 0.681, 0.681, 0.681, 0.722, 0.722, 0.722,
                                0.722, 0.739, 0.739, 0.739, 0.739, 0.694, 0.693, 0.683, 0.683};

static int map[32] = {22, 9,  5, 14, 18, 26, 13, 1, 24, 11, 7, 16, 20, 28, 13, 3,
                      23, 10, 6, 15, 19, 27, 13, 2, 25, 12, 8, 17, 21, 29, 13, 4};

static double xfac[4] = {0, 0, -0.5, -1};

static double yfac[6] = {0, -1.2, -1, -0.5, 0, 0.2};

static int dingbats[256] = {
    0,     1,     2,     3,     4,     5,     6,     7,     8,     9,     10,    11,    12,    13,    14,    15,
    16,    17,    18,    19,    20,    21,    22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
    32,    9985,  9986,  9987,  9988,  9742,  9990,  9991,  9992,  9993,  9755,  9758,  9996,  9997,  9998,  9999,
    10000, 10001, 10002, 10003, 10004, 10005, 10006, 10007, 10008, 10009, 10010, 10011, 10012, 10013, 10014, 10015,
    10016, 10017, 10018, 10019, 10020, 10021, 10022, 10023, 9733,  10025, 10026, 10027, 10028, 10029, 10030, 10031,
    10032, 10033, 10034, 10035, 10036, 10037, 10038, 10039, 10040, 10041, 10042, 10043, 10044, 10045, 10046, 10047,
    10048, 10049, 10050, 10051, 10052, 10053, 10054, 10055, 10056, 10057, 10058, 10059, 9679,  10061, 9632,  10063,
    10064, 10065, 10066, 9650,  9660,  9670,  10070, 9687,  10072, 10073, 10074, 10075, 10076, 10077, 10078, 127,
    128,   129,   130,   131,   132,   133,   134,   135,   136,   137,   138,   139,   140,   141,   142,   143,
    144,   145,   146,   147,   148,   149,   150,   151,   152,   153,   154,   155,   156,   157,   158,   159,
    160,   10081, 10082, 10083, 10084, 10085, 10086, 10087, 9827,  9830,  9829,  9824,  9312,  9313,  9314,  9315,
    9316,  9317,  9318,  9319,  9320,  9321,  10102, 10103, 10104, 10105, 10106, 10107, 10107, 10109, 10110, 10111,
    10112, 10113, 10114, 10115, 10116, 10117, 10118, 10119, 10120, 10121, 10122, 10123, 10124, 10125, 10126, 10127,
    10128, 10129, 10130, 10131, 10132, 8594,  8596,  8597,  10136, 10137, 10138, 10139, 10140, 10141, 10142, 10143,
    10144, 10145, 10146, 10147, 10148, 10149, 10150, 10151, 10152, 10153, 10154, 10155, 10156, 10157, 10158, 10159,
    32,    10161, 10162, 10163, 10164, 10165, 10166, 10167, 10168, 10169, 10170, 10171, 10172, 10173, 10174, 32};

static ws_state_list p_, *p;

static int fontfile = 0;

static CGPoint *points = NULL;

static int num_points = 0;

static CGLayerRef patternLayers[PATTERNS];

static int pattern_ = -1;

static int pcolor[PATTERNS];

static int color_ = -1;

static CGColorSpaceRef colorSpace = NULL;

static CGContextRef context = NULL;

static CGLayerRef layer;

static NSMutableArray *contextStack = NULL, *layerStack = NULL;

static CGRect clipRect;

static int have_colors = 0;

static void set_norm_xform(int tnr, double *wn, double *vp)
{
  CGRect *rect = &p->rect[tnr];

  a[tnr] = (vp[1] - vp[0]) / (wn[1] - wn[0]);
  b[tnr] = vp[0] - wn[0] * a[tnr];
  c[tnr] = (vp[3] - vp[2]) / (wn[3] - wn[2]);
  d[tnr] = vp[2] - wn[2] * c[tnr];

  NDC_to_DC(vp[0], vp[2], rect->origin.x, rect->origin.y);
  NDC_to_DC(vp[1] - vp[0], vp[3] - vp[2], rect->size.width, rect->size.height);

  rect->size.width += 1;
  rect->size.height += 1;
}

static void init_norm_xform(void)
{
  int tnr;

  for (tnr = 0; tnr < MAX_TNR; tnr++) set_norm_xform(tnr, gkss->window[tnr], gkss->viewport[tnr]);
}

static void set_color_rep(int color, double red, double green, double blue)
{
  int i;

  if (color >= 0 && color < MAX_COLOR)
    {
      if (p->rgb[color] != 0)
        {
          CGColorRelease(p->rgb[color]);
        }
      p->rgb[color] = CGColorCreateGenericRGB(red, green, blue, gkss->alpha);
      for (i = 0; i < PATTERNS; i++)
        {
          if (pcolor[i] == color) pcolor[i] = -1;
        }
    }
}

static void init_colors(void)
{
  int color;
  double red, green, blue;

  for (color = 0; color < MAX_COLOR; color++)
    {
      gks_inq_rgb(color, &red, &green, &blue);
      set_color_rep(color, red, green, blue);
    }
}

static void update_color(int color)
{
  if (CGColorGetAlpha(p->rgb[color]) != gkss->alpha)
    {
      p->rgb[color] = CGColorCreateCopyWithAlpha(p->rgb[color], gkss->alpha);
    }
}

static void set_xform(void)
{
  double aspect_ratio, w, h, x, y;

  aspect_ratio = (p->window[1] - p->window[0]) / (p->window[3] - p->window[2]);

  if (p->width > p->height * aspect_ratio)
    {
      w = p->height * aspect_ratio;
      h = p->height;
      x = 0.5 * (p->width - w);
      y = 0;
    }
  else
    {
      w = p->width;
      h = p->width / aspect_ratio;
      x = 0;
      y = 0.5 * (p->height - h);
    }

  p->a = w / (p->window[1] - p->window[0]);
  p->b = x - p->window[0] * p->a;
  p->c = h / (p->window[3] - p->window[2]);
  p->d = y + p->window[2] * p->c;
}

static void seg_xform(double *x, double *y) {}

static void seg_xform_rel(double *x, double *y) {}

@implementation GKSView

- (void)interp:(char *)str
{
  char *s;
  gks_state_list_t *sl = NULL, saved_gkss;
  int sp = 0, *len, *f;
  int *i_arr = NULL, *dx = NULL, *dy = NULL, *dimx = NULL, *len_c_arr;
  int *n, *primid, *ldr;
  double *f_arr_1 = NULL, *f_arr_2 = NULL;
  char *c_arr = NULL;
  double mat[3][2];
  int i;

  s = str;

  RESOLVE(len, int, sizeof(int));
  while (*len > 0)
    {
      RESOLVE(f, int, sizeof(int));

      switch (*f)
        {
        case 2:
          RESOLVE(sl, gks_state_list_t, sizeof(gks_state_list_t));
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

        case 16: /* cell array */
        case 201:
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

        default:
          gks_perror("display list corrupted (len=%d, fctid=%d)", *len, *f);
          exit(1);
        }

      switch (*f)
        {
        case 2:
          gkss = &gkss_;
          p = &p_;

          memmove(&saved_gkss, gkss, sizeof(gks_state_list_t));
          memmove(gkss, sl, sizeof(gks_state_list_t));

          CGSize screen_size = CGDisplayScreenSize(CGMainDisplayID());
          double mwidth = 0.001 * screen_size.width;

          p->width = [self bounds].size.width;
          p->height = [self bounds].size.height;
          p->nominal_size = min(p->width, p->height) / 500.0;
          p->swidth = NSMaxX([[[NSScreen screens] objectAtIndex:0] frame]);
          p->sheight = NSMaxY([[[NSScreen screens] objectAtIndex:0] frame]);

          p->window[0] = p->window[2] = 0.0;
          p->window[1] = p->window[3] = 1.0;

          p->viewport[0] = p->viewport[2] = 0.0;
          p->viewport[1] = p->width * mwidth / p->swidth;
          p->viewport[3] = p->height * mwidth / p->sheight;

          set_xform();
          init_norm_xform();

          if (!have_colors)
            {
              init_colors();
              have_colors = 1;
            }

          gkss->fontfile = fontfile;
          gks_init_core(gkss);

          [self set_clip_rect:gkss->cntnr];
          break;

        case 12:
          [self polyline:i_arr[0]:f_arr_1:f_arr_2];
          break;

        case 13:
          [self polymarker:i_arr[0]:f_arr_1:f_arr_2];
          break;

        case 14:
          [self text:f_arr_1[0]:f_arr_2[0]:c_arr];
          break;

        case 15:
          [self fillarea:i_arr[0]:f_arr_1:f_arr_2];
          break;

        case 16:
          [self cellarray:f_arr_1[0]:f_arr_1[1]:f_arr_2[0]:f_arr_2[1]:*dx:*dy:*dimx:i_arr:0];
          break;

        case 17:
          [self gdp:*n:f_arr_1:f_arr_2:*primid:*ldr:i_arr];
          break;

        case 19:
          gkss->ltype = i_arr[0];
          break;

        case 20:
          gkss->lwidth = f_arr_1[0];
          break;

        case 21:
          gkss->plcoli = i_arr[0];
          break;

        case 23:
          gkss->mtype = i_arr[0];
          break;

        case 24:
          gkss->mszsc = f_arr_1[0];
          break;

        case 25:
          gkss->pmcoli = i_arr[0];
          break;

        case 27:
          gkss->txfont = i_arr[0];
          gkss->txprec = i_arr[1];
          break;

        case 28:
          gkss->chxp = f_arr_1[0];
          break;

        case 29:
          gkss->chsp = f_arr_1[0];
          break;

        case 30:
          gkss->txcoli = i_arr[0];
          break;

        case 31:
          gkss->chh = f_arr_1[0];
          break;

        case 32:
          gkss->chup[0] = f_arr_1[0];
          gkss->chup[1] = f_arr_2[0];
          break;

        case 33:
          gkss->txp = i_arr[0];
          break;

        case 34:
          gkss->txal[0] = i_arr[0];
          gkss->txal[1] = i_arr[1];
          break;

        case 36:
          gkss->ints = i_arr[0];
          break;

        case 37:
          gkss->styli = i_arr[0];
          break;

        case 38:
          gkss->facoli = i_arr[0];
          break;

        case 41:
          for (i = 0; i < 13; i++) gkss->asf[i] = i_arr[i];
          break;

        case 48:
          set_color_rep(i_arr[0], f_arr_1[0], f_arr_1[1], f_arr_1[2]);
          break;

        case 49:
          gkss->window[*i_arr][0] = f_arr_1[0];
          gkss->window[*i_arr][1] = f_arr_1[1];
          gkss->window[*i_arr][2] = f_arr_2[0];
          gkss->window[*i_arr][3] = f_arr_2[1];
          set_xform();
          set_norm_xform(*i_arr, gkss->window[*i_arr], gkss->viewport[*i_arr]);
          break;

        case 50:
          gkss->viewport[*i_arr][0] = f_arr_1[0];
          gkss->viewport[*i_arr][1] = f_arr_1[1];
          gkss->viewport[*i_arr][2] = f_arr_2[0];
          gkss->viewport[*i_arr][3] = f_arr_2[1];
          set_norm_xform(*i_arr, gkss->window[*i_arr], gkss->viewport[*i_arr]);

          if (*i_arr == gkss->cntnr) [self set_clip_rect:*i_arr];
          break;

        case 52:
          gkss->cntnr = i_arr[0];
          [self set_clip_rect:gkss->cntnr];
          break;

        case 53:
          gkss->clip = i_arr[0];
          [self set_clip_rect:gkss->cntnr];
          break;

        case 54:
          p->window[0] = f_arr_1[0];
          p->window[1] = f_arr_1[1];
          p->window[2] = f_arr_2[0];
          p->window[3] = f_arr_2[1];

          set_xform();
          init_norm_xform();
          break;

        case 55:
          if (!has_been_resized)
            {
              p->viewport[0] = f_arr_1[0];
              p->viewport[1] = f_arr_1[1];
              p->viewport[2] = f_arr_2[0];
              p->viewport[3] = f_arr_2[1];
            }

          [self resize_window];

          set_xform();
          init_norm_xform();
          break;

        case 108:
          gkss->resample_method = i_arr[0];
          break;

        case 200:
          gkss->txslant = f_arr_1[0];
          break;

        case 201:
          [self cellarray:f_arr_1[0]:f_arr_1[1]:f_arr_2[0]:f_arr_2[1]:*dx:*dy:*dimx:i_arr:1];
          break;

        case 202:
          gkss->shoff[0] = f_arr_1[0];
          gkss->shoff[1] = f_arr_1[1];
          gkss->blur = f_arr_1[2];
          [self gks_set_shadow];
          break;

        case 203:
          gkss->alpha = f_arr_1[0];
          break;

        case 204:
          mat[0][0] = f_arr_1[0];
          mat[0][1] = f_arr_1[1];
          mat[1][0] = f_arr_1[2];
          mat[1][1] = f_arr_1[3];
          mat[2][0] = f_arr_1[4];
          mat[2][1] = f_arr_1[5];
          break;

        case 206:
          gkss->bwidth = f_arr_1[0];
          break;

        case 207:
          gkss->bcoli = i_arr[0];
          break;

        case 208:
          gkss->clip_tnr = i_arr[0];
          break;
        }

      RESOLVE(len, int, sizeof(int));
    }

  if (gkss != NULL) memmove(gkss, &saved_gkss, sizeof(gks_state_list_t));
}

- (id)initWithFrame:(NSRect)frame
{
  self = [super initWithFrame:frame];

  if (self)
    {
      int i;
      for (i = 0; i < PATTERNS; i++)
        {
          patternLayers[i] = nil;
          pcolor[i] = -1;
        }
      buffer = NULL;
      size = 0;
      angle = 0;
      has_been_resized = 0;
      fontfile = gks_open_font();
    }
  return self;
}

- (void)drawRect:(NSRect)rect
{
  CGContextRef c;
  CGFloat centerx, centery;

  if ([self inLiveResize]) has_been_resized = 1;

  if (contextStack == NULL)
    {
      contextStack = [[NSMutableArray alloc] initWithCapacity:5];
      layerStack = [[NSMutableArray alloc] initWithCapacity:5];
    }

  if (buffer)
    {
      double scale = [self.window backingScaleFactor];
      c = (CGContextRef)[[NSGraphicsContext currentContext] graphicsPort];

      layer = CGLayerCreateWithContext(c, CGSizeMake(self.bounds.size.width * scale, self.bounds.size.height * scale),
                                       NULL);
      context = CGLayerGetContext(layer);
      CGContextScaleCTM(context, scale, scale);

      // Clear the layer with white before drawing
      CGContextSetFillColorWithColor(context, CGColorGetConstantColor(kCGColorWhite));
      CGContextFillRect(context, CGRectMake(0, 0, self.bounds.size.width, self.bounds.size.height));

      [contextStack addObject:(id)context];
      [layerStack addObject:(id)layer];

      if (angle != 0)
        {
          centerx = self.bounds.size.width / 2;
          centery = self.bounds.size.height / 2;
          CGContextTranslateCTM(context, centerx, centery);
          CGContextRotateCTM(context, (angle)*M_PI / 180);
          CGContextTranslateCTM(context, -centerx, -centery);
        }

      [self interp:buffer];
      CGContextDrawLayerInRect(c, CGRectMake(0, 0, self.bounds.size.width, self.bounds.size.height), layer);

      CGContextFlush(context);
      CGLayerRelease(layer);

      [contextStack removeLastObject];
      [layerStack removeLastObject];

      context = (CGContextRef)[contextStack lastObject];
      layer = (CGLayerRef)[layerStack lastObject];
    }
}

- (void)setDisplayList:(id)display_list
{
  int len = [display_list length];
  if (len + sizeof(int) > size)
    {
      while (len + sizeof(int) > size) size += MEMORY_INCREMENT;
      buffer = (char *)gks_realloc(buffer, size);
    }

  memmove(buffer, (char *)[display_list bytes], len);
  memset(buffer + len, 0, sizeof(int));

  [self setNeedsDisplay:YES];
}

- (void)setWinID:(int)winid
{
  win_id = winid;
}

- (int)getWinID
{
  return win_id;
}

- (IBAction)keep_on_display:(id)sender
{
  [[NSNotificationQueue defaultQueue]
      enqueueNotification:[NSNotification notificationWithName:@"GKSViewKeepOnDisplayNotification" object:self]
             postingStyle:NSPostWhenIdle];
}

- (IBAction)rotate:(id)sender
{
  NSRect rect = [[self window] frame];

  angle = (int)(angle + 90) % 360;
  if (angle == 90 || angle == 270)
    {
      double tmp = rect.size.width;
      rect.size.width = rect.size.height;
      rect.size.height = tmp;
    }
  has_been_resized = 1;

  [[self window] setFrame:rect display:YES];
  [self setNeedsDisplay:YES];
}

- (void)clear
{
  if (buffer)
    {
      if (context != NULL)
        {
          CGContextSetFillColorWithColor(context, p->rgb[0]);
          CGContextFillRect(context, CGRectMake(0, 0, self.bounds.size.width, self.bounds.size.height));
        }
      buffer[0] = 0;
    }
}

- (void)close
{
  int i;
  for (i = 0; i < PATTERNS; i++)
    {
      if (patternLayers[i])
        {
          CGLayerRelease(patternLayers[i]);
          patternLayers[i] = nil;
        }
    }
  if (colorSpace != NULL)
    {
      CGColorSpaceRelease(colorSpace);
      colorSpace = NULL;
    }
  gks_close_font(fontfile);
  if (buffer)
    {
      free(buffer);
      buffer = NULL;
      size = 0;
    }
  if (points)
    {
      free(points);
      points = NULL;
      num_points = 0;
    }
  [self release];
}

/* SaveAs Dialog */

- (IBAction)saveDocumentAs:(id)sender
{
  NSSavePanel *savePanel = [NSSavePanel savePanel];

#if __MAC_OS_X_VERSION_MAX_ALLOWED > 1070
  if (![[NSBundle mainBundle] loadNibNamed:@"ExtendSavePanel" owner:self topLevelObjects:nil])
#else
  if (![NSBundle loadNibNamed:@"ExtendSavePanel" owner:self])
#endif
    {
      NSLog(@"Failed to load ExtendSavePanel.nib");
      return;
    }
  NSString *saveFormat = [[NSUserDefaults standardUserDefaults] objectForKey:@"CurrentSaveFormat"];
  if (!saveFormat)
    {
      saveFormat = @"PNG";
    }
  [saveFormatPopUp selectItemWithTitle:saveFormat];

  [savePanel setAccessoryView:extendSavePanelView];
  [savePanel setNameFieldStringValue:[[self window] title]];
  NSString *saveFolder = [[NSUserDefaults standardUserDefaults] objectForKey:@"CurrentSaveFolder"];
  if (saveFolder)
    {
      [savePanel setDirectoryURL:[NSURL fileURLWithPath:saveFolder]];
    }
#if __MAC_OS_X_VERSION_MAX_ALLOWED < 101500
  [savePanel beginSheetModalForWindow:_window
                    completionHandler:^(NSInteger result) {
                      [self savePanelDidEnd:savePanel returnCode:result contextInfo:saveFormatPopUp];
                    }];
#else
  [savePanel beginSheetModalForWindow:cWindow
                    completionHandler:^(NSInteger result) {
                      [self savePanelDidEnd:savePanel returnCode:result contextInfo:saveFormatPopUp];
                    }];
#endif
}

- (void)savePanelDidEnd:(NSSavePanel *)theSheet returnCode:(int)returnCode contextInfo:(NSPopUpButton *)formatPopUp
{
  NSString *filename;
  NSData *data;
  NSBitmapImageRep *bitmap;

  if (NSFileHandlingPanelOKButton == returnCode && [[theSheet URL] isFileURL])
    {
      filename = [[[theSheet URL] path] stringByDeletingPathExtension];
      if ([[formatPopUp titleOfSelectedItem] isEqualToString:@"PDF"])
        {
          data = [self dataWithPDFInsideRect:[self bounds]];
          [data writeToFile:[filename stringByAppendingPathExtension:@"pdf"] atomically:NO];
        }
      else if ([[formatPopUp titleOfSelectedItem] isEqualToString:@"EPS"])
        {
          data = [self dataWithEPSInsideRect:[self bounds]];
          [data writeToFile:[filename stringByAppendingPathExtension:@"eps"] atomically:NO];
        }
      else if ([[formatPopUp titleOfSelectedItem] isEqualToString:@"TIFF"])
        {
          int compression = NSTIFFCompressionLZW; // non-lossy LZW compression

          filename = [filename stringByAppendingPathExtension:@"tiff"];

          [self lockFocus];
          bitmap = [self bitmapImageRepForCachingDisplayInRect:[self bounds]];
          [self cacheDisplayInRect:[self bounds] toBitmapImageRep:bitmap];
          [self unlockFocus];

          [[bitmap TIFFRepresentationUsingCompression:compression factor:1.0] writeToFile:filename atomically:YES];

          [bitmap release];
        }
      else if ([[formatPopUp titleOfSelectedItem] isEqualToString:@"PNG"])
        {
          filename = [filename stringByAppendingPathExtension:@"png"];

          [self lockFocus];
          bitmap = [self bitmapImageRepForCachingDisplayInRect:[self bounds]];
          [self cacheDisplayInRect:[self bounds] toBitmapImageRep:bitmap];
          [self unlockFocus];

          CGImageRef image = [bitmap CGImage];

          CFURLRef url =
              CFURLCreateWithFileSystemPath(kCFAllocatorDefault, (CFStringRef)filename, kCFURLPOSIXPathStyle, false);
          CGImageDestinationRef dr = CGImageDestinationCreateWithURL(url, (CFStringRef) @"public.png", 1, NULL);

          CGImageDestinationAddImage(dr, image, NULL);

          CGImageDestinationFinalize(dr);

          CFRelease(dr);

          CFRelease(url);
        }
      else if ([[formatPopUp titleOfSelectedItem] isEqualToString:@"JPEG"])
        {
          filename = [filename stringByAppendingPathExtension:@"jpg"];

          [self lockFocus];
          bitmap = [self bitmapImageRepForCachingDisplayInRect:[self bounds]];
          [self cacheDisplayInRect:[self bounds] toBitmapImageRep:bitmap];
          [self unlockFocus];

          CGImageRef image = [bitmap CGImage];

          CFMutableDictionaryRef mSaveMetaAndOpts =
              CFDictionaryCreateMutable(nil, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);

          float compression = [compressionSlider floatValue];

          CFDictionarySetValue(mSaveMetaAndOpts, kCGImageDestinationLossyCompressionQuality,
                               [NSNumber numberWithFloat:compression]);

          CFURLRef url =
              CFURLCreateWithFileSystemPath(kCFAllocatorDefault, (CFStringRef)filename, kCFURLPOSIXPathStyle, false);
          CGImageDestinationRef dr = CGImageDestinationCreateWithURL(url, (CFStringRef) @"public.jpeg", 1, NULL);

          CGImageDestinationAddImage(dr, image, mSaveMetaAndOpts);

          CGImageDestinationFinalize(dr);

          CFRelease(mSaveMetaAndOpts);

          CFRelease(dr);

          CFRelease(url);
        }
      else if ([[formatPopUp titleOfSelectedItem] isEqualToString:@"JPEG-2000"])
        {
          filename = [filename stringByAppendingPathExtension:@"jp2"];

          [self lockFocus];
          bitmap = [self bitmapImageRepForCachingDisplayInRect:[self bounds]];
          [self cacheDisplayInRect:[self bounds] toBitmapImageRep:bitmap];
          [self unlockFocus];

          CGImageRef image = [bitmap CGImage];

          CFMutableDictionaryRef mSaveMetaAndOpts =
              CFDictionaryCreateMutable(nil, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);

          float compression = [compressionSlider floatValue];

          CFDictionarySetValue(mSaveMetaAndOpts, kCGImageDestinationLossyCompressionQuality,
                               [NSNumber numberWithFloat:compression]);

          CFURLRef url =
              CFURLCreateWithFileSystemPath(kCFAllocatorDefault, (CFStringRef)filename, kCFURLPOSIXPathStyle, false);
          CGImageDestinationRef dr = CGImageDestinationCreateWithURL(url, (CFStringRef) @"public.jpeg-2000", 1, NULL);

          CGImageDestinationAddImage(dr, image, mSaveMetaAndOpts);

          CGImageDestinationFinalize(dr);

          CFRelease(mSaveMetaAndOpts);

          CFRelease(dr);

          CFRelease(url);
        }
      else if ([[formatPopUp titleOfSelectedItem] isEqualToString:@"GIF"])
        {
          filename = [filename stringByAppendingPathExtension:@"gif"];

          [self lockFocus];
          bitmap = [self bitmapImageRepForCachingDisplayInRect:[self bounds]];
          [self cacheDisplayInRect:[self bounds] toBitmapImageRep:bitmap];
          [self unlockFocus];

          CGImageRef image = [bitmap CGImage];

          CFURLRef url =
              CFURLCreateWithFileSystemPath(kCFAllocatorDefault, (CFStringRef)filename, kCFURLPOSIXPathStyle, false);
          CGImageDestinationRef dr = CGImageDestinationCreateWithURL(url, (CFStringRef) @"com.compuserve.gif", 1, NULL);

          CGImageDestinationAddImage(dr, image, NULL);

          CGImageDestinationFinalize(dr);

          CFRelease(dr);

          CFRelease(url);
        }
      else if ([[formatPopUp titleOfSelectedItem] isEqualToString:@"Windows BMP"])
        {
          filename = [filename stringByAppendingPathExtension:@"bmp"];

          [self lockFocus];
          bitmap = [self bitmapImageRepForCachingDisplayInRect:[self bounds]];
          [self cacheDisplayInRect:[self bounds] toBitmapImageRep:bitmap];
          [self unlockFocus];

          CGImageRef image = [bitmap CGImage];

          CFURLRef url =
              CFURLCreateWithFileSystemPath(kCFAllocatorDefault, (CFStringRef)filename, kCFURLPOSIXPathStyle, false);
          CGImageDestinationRef dr = CGImageDestinationCreateWithURL(url, (CFStringRef) @"com.microsoft.bmp", 1, NULL);

          CGImageDestinationAddImage(dr, image, NULL);

          CGImageDestinationFinalize(dr);

          CFRelease(dr);

          CFRelease(url);
        }
      else if ([[formatPopUp titleOfSelectedItem] isEqualToString:@"PICT"])
        {
          filename = [filename stringByAppendingPathExtension:@"pic"];

          [self lockFocus];
          bitmap = [self bitmapImageRepForCachingDisplayInRect:[self bounds]];
          [self cacheDisplayInRect:[self bounds] toBitmapImageRep:bitmap];
          [self unlockFocus];

          CGImageRef image = [bitmap CGImage];

          CFURLRef url =
              CFURLCreateWithFileSystemPath(kCFAllocatorDefault, (CFStringRef)filename, kCFURLPOSIXPathStyle, false);
          CGImageDestinationRef dr = CGImageDestinationCreateWithURL(url, (CFStringRef) @"com.apple.pict", 1, NULL);

          CGImageDestinationAddImage(dr, image, NULL);

          CGImageDestinationFinalize(dr);

          CFRelease(dr);

          CFRelease(url);
        }

      [[NSUserDefaults standardUserDefaults] setObject:[filename stringByDeletingLastPathComponent]
                                                forKey:@"CurrentSaveFolder"];
      [[NSUserDefaults standardUserDefaults] setObject:[formatPopUp titleOfSelectedItem] forKey:@"CurrentSaveFormat"];
    }
}

- (void)set_fill_color:(int)color:(CGContextRef)context
{
  update_color(color);
  CGContextSetFillColorWithColor(context, p->rgb[color]);
}

- (void)set_stroke_color:(int)color:(CGContextRef)context
{
  update_color(color);
  CGContextSetStrokeColorWithColor(context, p->rgb[color]);
}

- (void)resize_window
{
  double max_width, max_height, width, height;
  NSRect rect = [[self window] frame];
  CGSize screen_size;

  screen_size = CGDisplayScreenSize(CGMainDisplayID());
  max_width = 0.001 * screen_size.width;
  max_height = max_width * p->sheight / p->swidth;

  if (!has_been_resized)
    {
      gks_fit_ws_viewport(p->viewport, max_width, max_height, 0.0075);
      width = (p->viewport[1] - p->viewport[0]) / max_width * p->swidth;
      height = (p->viewport[3] - p->viewport[2]) / max_height * p->sheight;
    }
  else
    {
      width = [self bounds].size.width;
      height = [self bounds].size.height;
      p->viewport[0] = p->viewport[2] = 0;
      p->viewport[1] = width * max_width / p->swidth;
      p->viewport[3] = height * max_height / p->sheight;
    }

  if (fabs(p->width - width) > 0.5 || fabs(p->height - height) > 0.5)
    {
      rect.origin.y += rect.size.height - height;
      rect.size.width = width;
      rect.size.height = height;

      NSSize contentSize = [[self window] contentRectForFrameRect:rect].size;
      rect.origin.y -= height - contentSize.height;
      rect.size.width += width - contentSize.width;
      rect.size.height += height - contentSize.height;

      p->width = width;
      p->height = height;
      p->nominal_size = min(p->width, p->height) / 500.0;

      [self setNeedsDisplay:YES];
      [[self window] setFrame:rect display:YES];
    }
}

- (void)set_clip_rect:(int)tnr
{
  if (gkss->clip_tnr != 0)
    clipRect = p->rect[gkss->clip_tnr];
  else if (gkss->clip == GKS_K_CLIP)
    clipRect = p->rect[tnr];
  else
    clipRect = p->rect[0];
}

static void begin_context(CGContextRef context)
{
  CGContextSaveGState(context);
  CGContextClipToRect(context, clipRect);
}

static void end_context(CGContextRef context)
{
  CGContextRestoreGState(context);
}

- (void)gks_set_shadow
{
  CGSize offset;

  offset.width = gkss->shoff[0];
  offset.height = gkss->shoff[1];

  CGContextSetShadow(context, offset, gkss->blur);
}

static void line_routine(int n, double *px, double *py, int linetype, int tnr)
{
  double x, y;
  int i;

  if (n > num_points)
    {
      while (n > num_points) num_points += NUM_POINTS;
      points = (CGPoint *)gks_realloc(points, num_points * sizeof(CGPoint));
    }

  for (i = 0; i < n; ++i)
    {
      WC_to_NDC(px[i], py[i], tnr, x, y);
      seg_xform(&x, &y);
      NDC_to_DC(x, y, points[i].x, points[i].y);
    }

  CGContextBeginPath(context);
  CGContextAddLines(context, points, n);
  CGContextDrawPath(context, kCGPathStroke);

  if (linetype == 0) CGContextClosePath(context);
}

- (void)polyline:(int)n:(double *)px:(double *)py
{
  int ln_type, ln_color, i;
  double ln_width;
  int dashlist[10];
  CGFloat lengths[10] = {0., 0., 0., 0., 0., 0., 0., 0., 0., 0.};
  double x, y;

  if (n > num_points)
    {
      while (n > num_points) num_points += NUM_POINTS;
      points = (CGPoint *)gks_realloc(points, num_points * sizeof(CGPoint));
    }

  for (i = 0; i < n; ++i)
    {
      WC_to_NDC(px[i], py[i], gkss->cntnr, x, y);
      seg_xform(&x, &y);
      NDC_to_DC(x, y, points[i].x, points[i].y);
    }

  ln_type = gkss->asf[0] ? gkss->ltype : gkss->lindex;
  ln_width = gkss->asf[1] ? gkss->lwidth : 1;
  ln_color = gkss->asf[2] ? gkss->plcoli : 1;

  [self set_stroke_color:ln_color:context];

  begin_context(context);

  CGContextBeginPath(context);
  CGContextSetLineWidth(context, ln_width * p->nominal_size);

  if (ln_type != 1)
    {
      gks_get_dash_list(ln_type, ln_width, dashlist);
      for (i = 1; i <= dashlist[0]; ++i) lengths[i - 1] = (float)dashlist[i];

      CGContextSetLineDash(context, 0.0, lengths, dashlist[0]);
    }

  // Disable antialiasing for polylines with many points
  CGContextSetShouldAntialias(context, n > 100000 ? NO : YES);

  CGContextAddLines(context, points, n);
  CGContextDrawPath(context, kCGPathStroke);

  end_context(context);
}

- (void)draw_marker:(double)
                 xn:(double)yn
                   :(int)mtype
                   :(double)mscale
                   :(int)mcolor
                   :(CGContextRef)context
                   :(const CGFloat *)color
{
  double x, y;
  int r, i;
  double scale, xr, yr;
  int pc, op;
  const CGFloat *marker_color, *border_color, *background_color;

#include "marker.h"

  marker_color = color != NULL ? color : CGColorGetComponents(p->rgb[mcolor]);
  border_color = CGColorGetComponents(p->rgb[gkss->bcoli]);
  background_color = CGColorGetComponents(p->rgb[0]);

  mscale *= p->nominal_size;
  r = (int)(3 * mscale);
  scale = 0.01 * mscale / 3.0;

  xr = r;
  yr = 0;
  seg_xform_rel(&xr, &yr);
  r = nint(sqrt(xr * xr + yr * yr));

  NDC_to_DC(xn, yn, x, y);

  pc = 0;
  mtype = (r > 0) ? mtype + marker_off : marker_off + 1;

  do
    {
      op = marker[mtype][pc];
      switch (op)
        {
        case 1: // point
          CGContextSetFillColor(context, marker_color);
          CGContextFillRect(context, CGRectMake(x, y, 1, 1));
          break;

        case 2: // line
          CGContextBeginPath(context);
          CGContextSetLineWidth(context, p->nominal_size);
          CGContextSetStrokeColor(context, marker_color);
          for (i = 0; i < 2; i++)
            {
              xr = scale * marker[mtype][pc + 2 * i + 1];
              yr = scale * marker[mtype][pc + 2 * i + 2];
              seg_xform_rel(&xr, &yr);
              if (i == 0)
                CGContextMoveToPoint(context, x - xr, y + yr);
              else
                CGContextAddLineToPoint(context, x - xr, y + yr);
            }
          CGContextDrawPath(context, kCGPathStroke);
          pc += 4;
          break;

        case 3: // polyline
          CGContextBeginPath(context);
          CGContextSetLineWidth(context, p->nominal_size);
          CGContextSetStrokeColor(context, marker_color);
          for (i = 0; i < marker[mtype][pc + 1]; i++)
            {
              xr = scale * marker[mtype][pc + 2 + 2 * i];
              yr = scale * marker[mtype][pc + 3 + 2 * i];
              seg_xform_rel(&xr, &yr);
              if (i == 0)
                CGContextMoveToPoint(context, x - xr, y + yr);
              else
                CGContextAddLineToPoint(context, x - xr, y + yr);
            }
          CGContextClosePath(context);
          CGContextDrawPath(context, kCGPathStroke);
          pc += 1 + 2 * marker[mtype][pc + 1];
          break;

        case 4: // filled polygon
        case 5: // hollow polygon
          CGContextBeginPath(context);
          if (op == 4)
            {
              CGContextSetFillColor(context, marker_color);
              if (gkss->bcoli != gkss->pmcoli)
                {
                  CGContextSetLineWidth(context, gkss->bwidth * p->nominal_size);
                  CGContextSetStrokeColor(context, border_color);
                }
            }
          else
            CGContextSetFillColor(context, background_color);
          for (i = 0; i < marker[mtype][pc + 1]; i++)
            {
              xr = scale * marker[mtype][pc + 2 + 2 * i];
              yr = scale * marker[mtype][pc + 3 + 2 * i];
              seg_xform_rel(&xr, &yr);
              if (i == 0)
                CGContextMoveToPoint(context, x - xr, y + yr);
              else
                CGContextAddLineToPoint(context, x - xr, y + yr);
            }
          CGContextClosePath(context);
          if (op == 4 && gkss->bcoli != gkss->pmcoli)
            CGContextDrawPath(context, kCGPathFillStroke);
          else
            CGContextDrawPath(context, kCGPathFill);

          pc += 1 + 2 * marker[mtype][pc + 1];
          break;

        case 6: // arc
          CGContextBeginPath(context);
          CGContextSetLineWidth(context, p->nominal_size);
          CGContextSetStrokeColor(context, marker_color);
          CGContextAddArc(context, x, y, r, 0.0, 2 * M_PI, 0);
          CGContextDrawPath(context, kCGPathStroke);
          break;

        case 7: // filled arc
        case 8: // hollow arc
          CGContextBeginPath(context);
          if (op == 7)
            {
              CGContextSetFillColor(context, marker_color);
              if (gkss->bcoli != gkss->pmcoli)
                {
                  CGContextSetLineWidth(context, gkss->bwidth * p->nominal_size);
                  CGContextSetStrokeColor(context, border_color);
                }
            }
          else
            CGContextSetStrokeColor(context, background_color);
          CGContextAddArc(context, x, y, r, 0.0, 2 * M_PI, 0);
          if (op == 7 && gkss->bcoli != gkss->pmcoli)
            CGContextDrawPath(context, kCGPathFillStroke);
          else
            CGContextDrawPath(context, kCGPathFill);
          break;
        }
      pc++;
    }
  while (op != 0);
}

- (void)polymarker:(int)n:(double *)px:(double *)py
{
  int mk_type, mk_color;
  double mk_size;
  double x, y;
  double *clrt = gkss->viewport[gkss->cntnr];
  int i, draw;

  mk_type = gkss->asf[3] ? gkss->mtype : gkss->mindex;
  mk_size = gkss->asf[4] ? gkss->mszsc : 1;
  mk_color = gkss->asf[5] ? gkss->pmcoli : 1;

  begin_context(context);

  if (colorSpace == NULL)
    {
      colorSpace = CGColorSpaceCreateDeviceRGB();
    }
  CGContextSetStrokeColorSpace(context, colorSpace);
  CGContextSetFillColorSpace(context, colorSpace);

  for (i = 0; i < n; i++)
    {
      WC_to_NDC(px[i], py[i], gkss->cntnr, x, y);
      seg_xform(&x, &y);

      if (gkss->clip == GKS_K_CLIP)
        draw = (x >= clrt[0] && x <= clrt[1] && y >= clrt[2] && y <= clrt[3]);
      else
        draw = 1;

      if (draw) [self draw_marker:x:y:mk_type:mk_size:mk_color:context:NULL];
    }
  end_context(context);
}

static void drawPatternCell(void *info, CGContextRef context)
{
  CGLayerRef patternLayer = *(CGLayerRef *)info;
  CGColorSpaceRef patternSpace;
  patternSpace = CGColorSpaceCreatePattern(NULL);
  CGContextSetFillColorSpace(context, patternSpace);
  CGColorSpaceRelease(patternSpace);

  CGContextSetRGBFillColor(context, 0, 0, 0, 1);

  CGContextDrawLayerAtPoint(context, CGPointMake(0, 0), patternLayer);
}

static void draw_pattern(int index, int coli, CGPathRef shape, CGContextRef context)
{
  double scale = 0.125 * (int)(p->c + p->a) / 125;

  gks_inq_pattern_array(index, patArray);
  double patHeight = patArray[0] * scale;
  double patWidth = patHeight;
  double i, l;
  int k = 1, n;

  begin_context(context);
  if (!patternLayers[index] || pcolor[index] != coli)
    {
      patternLayers[index] = CGLayerCreateWithContext(context, CGSizeMake(patWidth, patHeight), NULL);
      pcolor[index] = coli;
      CGContextRef layerContext = CGLayerGetContext(patternLayers[index]);
      CGContextSetShouldAntialias(layerContext, NO);
      CGContextSetFillColorWithColor(layerContext, p->rgb[coli]);
      for (i = patHeight - scale; i >= 0; i -= scale)
        {
          n = patArray[k];
          for (l = 0; l < patWidth; l += scale)
            {
              if ((n % 2) == 0)
                {
                  CGContextFillRect(layerContext, CGRectMake(l, i, scale, scale));
                }
              n >>= 1;
            }
          k++;
        }
    }

  CGColorSpaceRef patternSpace;
  CGFloat alpha = gkss->alpha;
  static const CGPatternCallbacks callbacks = {0, &drawPatternCell, NULL};

  patternSpace = CGColorSpaceCreatePattern(NULL);
  CGContextSetFillColorSpace(context, patternSpace);
  CGColorSpaceRelease(patternSpace);

  CGPatternRef pattern = CGPatternCreate(patternLayers + index, CGRectMake(0, 0, patWidth, patHeight),
                                         CGAffineTransformMake(1, 0, 0, 1, 0, 0), patWidth, patHeight,
                                         kCGPatternTilingConstantSpacing, true, &callbacks);

  CGContextSetFillPattern(context, pattern, &alpha);
  CGPatternRelease(pattern);
  CGContextAddPath(context, shape);
  CGContextFillPath(context);
  end_context(context);
}

static void fill_routine(int n, double *px, double *py, int tnr)
{
  double x, y;
  int i;

  if (n > num_points)
    {
      while (n > num_points) num_points += NUM_POINTS;
      points = (CGPoint *)gks_realloc(points, num_points * sizeof(CGPoint));
    }

  for (i = 0; i < n; ++i)
    {
      if (is_NaN(px[i]) && is_NaN(py[i]))
        {
          NDC_to_DC(0, 0, points[i].x, points[i].y);
          continue;
        }
      WC_to_NDC(px[i], py[i], tnr, x, y);
      seg_xform(&x, &y);
      NDC_to_DC(x, y, points[i].x, points[i].y);
    }

  CGMutablePathRef shape = CGPathCreateMutable();
  CGPathAddLines(shape, NULL, points, n);
  CGPathCloseSubpath(shape);

  if (pattern_ > -1)
    {
      draw_pattern(pattern_, color_, shape, context);
    }
  else
    {
      CGContextAddPath(context, shape);
      CGContextFillPath(context);
    }

  CGPathRelease(shape);
}

- (void)fillarea:(int)n:(double *)px:(double *)py
{
  int fl_inter, fl_style, fl_color, i = 0;
  double x, y;

  if (n > num_points)
    {
      while (n > num_points) num_points += NUM_POINTS;
      points = (CGPoint *)gks_realloc(points, num_points * sizeof(CGPoint));
    }

  for (i = 0; i < n; ++i)
    {
      if (is_NaN(px[i]) && is_NaN(py[i]))
        {
          NDC_to_DC(0, 0, points[i].x, points[i].y);
          continue;
        }
      WC_to_NDC(px[i], py[i], gkss->cntnr, x, y);
      seg_xform(&x, &y);
      NDC_to_DC(x, y, points[i].x, points[i].y);
    }

  fl_inter = gkss->asf[10] ? gkss->ints : predef_ints[gkss->findex - 1];
  fl_style = gkss->asf[11] ? gkss->styli : predef_styli[gkss->findex - 1];
  fl_color = gkss->asf[12] ? gkss->facoli : 1;

  [self set_stroke_color:fl_color:context];

  if (fl_inter == GKS_K_INTSTYLE_HOLLOW)
    {
      begin_context(context);
      CGContextBeginPath(context);
      CGContextSetLineWidth(context, p->nominal_size);
      CGContextAddLines(context, points, n);
      CGContextClosePath(context);
      CGContextDrawPath(context, kCGPathStroke);
      end_context(context);
    }
  else if (fl_inter == GKS_K_INTSTYLE_SOLID)
    {
      begin_context(context);
      [self set_fill_color:fl_color:context];
      CGContextBeginPath(context);
      CGContextSetLineWidth(context, p->nominal_size);
      CGContextAddLines(context, points, n);
      CGContextClosePath(context);
      CGContextDrawPath(context, kCGPathEOFill);
      end_context(context);
    }
  else if (fl_inter == GKS_K_INTSTYLE_PATTERN || fl_inter == GKS_K_INTSTYLE_HATCH)
    {
      [self set_fill_color:fl_color:context];
      if (fl_inter == GKS_K_INTSTYLE_HATCH) fl_style += HATCH_STYLE;
      if (fl_style >= PATTERNS) fl_style = 1;

      pattern_ = fl_style;
      color_ = fl_color;
      fill_routine(n, px, py, gkss->cntnr);
      pattern_ = -1;
    }
}

static void to_DC(int n, double *x, double *y)
{
  int i;
  double xn, yn;

  for (i = 0; i < n; i++)
    {
      WC_to_NDC(x[i], y[i], gkss->cntnr, xn, yn);
      seg_xform(&xn, &yn);
      NDC_to_DC(xn, yn, x[i], y[i]);
    }
}

- (void)draw_path:(int)n:(double *)px:(double *)py:(int)nc:(int *)codes
{
  int i, j;
  double x[3], y[3], w, h, a1, a2;
  double cur_x = 0, cur_y = 0;
  double start_x = 0, start_y = 0;

  begin_context(context);

  CGContextSetLineWidth(context, gkss->bwidth * p->nominal_size);
  [self set_stroke_color:gkss->bcoli:context];
  [self set_fill_color:gkss->facoli:context];

  j = 0;
  for (i = 0; i < nc; ++i)
    {
      switch (codes[i])
        {
        case 'M':
        case 'm':
          x[0] = px[j];
          y[0] = py[j];
          if (codes[i] == 'm')
            {
              x[0] += cur_x;
              y[0] += cur_y;
            }
          cur_x = start_x = x[0];
          cur_y = start_y = y[0];
          to_DC(1, x, y);
          CGContextMoveToPoint(context, x[0], y[0]);
          j += 1;
          break;
        case 'L':
        case 'l':
          x[0] = px[j];
          y[0] = py[j];
          if (codes[i] == 'l')
            {
              x[0] += cur_x;
              y[0] += cur_y;
            }
          cur_x = x[0];
          cur_y = y[0];
          to_DC(1, x, y);
          CGContextAddLineToPoint(context, x[0], y[0]);
          j += 1;
          break;
        case 'Q':
        case 'q':
          x[0] = px[j];
          y[0] = py[j];
          if (codes[i] == 'q')
            {
              x[0] += cur_x;
              y[0] += cur_y;
            }
          x[1] = px[j + 1];
          y[1] = py[j + 1];
          if (codes[i] == 'q')
            {
              x[1] += cur_x;
              y[1] += cur_y;
            }
          cur_x = x[1];
          cur_y = y[1];
          to_DC(2, x, y);
          CGContextAddQuadCurveToPoint(context, x[0], y[0], x[1], y[1]);
          j += 2;
          break;
        case 'C':
        case 'c':
          x[0] = px[j];
          y[0] = py[j];
          if (codes[i] == 'c')
            {
              x[0] += cur_x;
              y[0] += cur_y;
            }
          x[1] = px[j + 1];
          y[1] = py[j + 1];
          if (codes[i] == 'c')
            {
              x[1] += cur_x;
              y[1] += cur_y;
            }
          x[2] = px[j + 2];
          y[2] = py[j + 2];
          if (codes[i] == 'c')
            {
              x[2] += cur_x;
              y[2] += cur_y;
            }
          cur_x = x[2];
          cur_y = y[2];
          to_DC(3, x, y);
          CGContextAddCurveToPoint(context, x[0], y[0], x[1], y[1], x[2], y[2]);
          j += 3;
          break;
        case 'A':
        case 'a':
          {
            double rx, ry, cx, cy;
            rx = fabs(px[j]);
            ry = fabs(py[j]);
            a1 = px[j + 1];
            a2 = py[j + 1];
            cx = cur_x - rx * cos(a1);
            cy = cur_y - ry * sin(a1);
            x[0] = cx - rx;
            y[0] = cy - ry;
            x[1] = cx + rx;
            y[1] = cy + ry;
            cur_x = cx + rx * cos(a2);
            cur_y = cy + ry * sin(a2);
          }
          to_DC(2, x, y);
          w = x[1] - x[0];
          h = y[1] - y[0];
          if (fabs(w - h) > FEPS) // stupid workaround for 10.15.3
            {
              CGMutablePathRef path = CGPathCreateMutable();
              CGAffineTransform m = CGAffineTransformMakeTranslation(x[0] + 0.5 * w, y[0] + 0.5 * h);
              m = CGAffineTransformConcat(CGAffineTransformMakeScale(1.0, h / w), m);
              CGPathAddArc(path, &m, 0, 0, 0.5 * w, a1, a2, a1 > a2);
              CGContextAddPath(context, path);
              CFRelease(path);
            }
          else
            CGContextAddArc(context, x[0] + 0.5 * w, y[0] + 0.5 * h, 0.5 * w, a1, a2, a1 > a2);
          j += 3;
          break;
        case 's':
          CGContextClosePath(context);
          cur_x = start_x;
          cur_y = start_y;
          CGContextDrawPath(context, kCGPathStroke);
          break;
        case 'S':
          CGContextDrawPath(context, kCGPathStroke);
          break;
        case 'f':
          CGContextClosePath(context);
          cur_x = start_x;
          cur_y = start_y;
          CGContextDrawPath(context, kCGPathFill);
          break;
        case 'F':
          CGContextClosePath(context);
          cur_x = start_x;
          cur_y = start_y;
          CGContextDrawPath(context, kCGPathFillStroke);
          break;
        case 'Z':
          CGContextClosePath(context);
          cur_x = start_x;
          cur_y = start_y;
          break;
        case '\0':
          break;
        default:
          gks_perror("invalid path code ('%c')", codes[i]);
          exit(1);
        }
    }

  end_context(context);
}


- (void)draw_lines:(int)n:(double *)px:(double *)py:(int *)attributes
{
  int i, j = 0, rgba;
  double x, y;
  int xim1, yim1, xi, yi;
  float line_width;
  CGFloat color[4];

  begin_context(context);

  if (colorSpace == NULL)
    {
      colorSpace = CGColorSpaceCreateDeviceRGB();
    }

  WC_to_NDC(px[0], py[0], gkss->cntnr, x, y);
  seg_xform(&x, &y);
  NDC_to_DC(x, y, xi, yi);

  for (i = 1; i < n; i++)
    {
      xim1 = xi;
      yim1 = yi;
      WC_to_NDC(px[i], py[i], gkss->cntnr, x, y);
      seg_xform(&x, &y);
      NDC_to_DC(x, y, xi, yi);

      CGContextBeginPath(context);
      CGContextSetLineJoin(context, kCGLineJoinMiter);
      CGContextSetLineCap(context, kCGLineCapRound);
      line_width = 0.01 * attributes[j++];
      CGContextSetLineWidth(context, line_width * p->nominal_size);

      rgba = attributes[j++];
      color[0] = (rgba & 0xff) / 255.0;
      color[1] = ((rgba >> 8) & 0xff) / 255.0;
      color[2] = ((rgba >> 16) & 0xff) / 255.0;
      color[3] = gkss->alpha;
      CGContextSetStrokeColorSpace(context, colorSpace);
      CGContextSetStrokeColor(context, color);

      CGContextMoveToPoint(context, xim1, yim1);
      CGContextAddLineToPoint(context, xi, yi);
      CGContextDrawPath(context, kCGPathStroke);
    }

  end_context(context);
}


- (void)draw_markers:(int)n:(double *)px:(double *)py:(int *)attributes
{
  int mk_type, mk_color = 0;
  double x, y, mk_size;
  double *clrt = gkss->viewport[gkss->cntnr];
  int i, j = 0, rgba, draw;
  CGFloat color[4];

  mk_type = gkss->asf[3] ? gkss->mtype : gkss->mindex;

  begin_context(context);

  if (colorSpace == NULL)
    {
      colorSpace = CGColorSpaceCreateDeviceRGB();
    }
  CGContextSetStrokeColorSpace(context, colorSpace);
  CGContextSetFillColorSpace(context, colorSpace);

  for (i = 0; i < n; i++)
    {
      WC_to_NDC(px[i], py[i], gkss->cntnr, x, y);
      seg_xform(&x, &y);

      if (gkss->clip == GKS_K_CLIP)
        draw = (x >= clrt[0] && x <= clrt[1] && y >= clrt[2] && y <= clrt[3]);
      else
        draw = 1;

      mk_size = 0.01 * attributes[j++];
      rgba = attributes[j++];
      color[0] = (rgba & 0xff) / 255.0;
      color[1] = ((rgba >> 8) & 0xff) / 255.0;
      color[2] = ((rgba >> 16) & 0xff) / 255.0;
      color[3] = gkss->alpha;

      if (draw) [self draw_marker:x:y:mk_type:mk_size:mk_color:context:color];
    }

  end_context(context);
}


- (void)gdp:(int)n:(double *)px:(double *)py:(int)primid:(int)nc:(int *)codes
{
  switch (primid)
    {
    case GKS_K_GDP_DRAW_PATH:
      [self draw_path:n:px:py:nc:codes];
      break;
    case GKS_K_GDP_DRAW_LINES:
      [self draw_lines:n:px:py:codes];
      break;
    case GKS_K_GDP_DRAW_MARKERS:
      [self draw_markers:n:px:py:codes];
      break;
    default:
      gks_perror("invalid drawing primitive ('%d')", primid);
      exit(1);
    }
}


- (void)cellarray:(double)
             xmin:(double)xmax
                 :(double)ymin
                 :(double)ymax
                 :(int)dx
                 :(int)dy
                 :(int)dimx
                 :(int *)colia
                 :(int)true_color
{
  double x1, y1, x2, y2;
  int ix1, ix2, iy1, iy2;
  int x, y, width, height;
  int i, j, ix, iy, ind;
  int swapx, swapy, *tmpptr;
  CGColorSpaceRef cs;
  CGContextRef bitmap;
  CGImageRef image;
  const CGFloat *colors;
  double scale = [self.window backingScaleFactor];

  WC_to_NDC(xmin, ymax, gkss->cntnr, x1, y1);
  seg_xform(&x1, &y1);
  NDC_to_DC(x1, y1, x1, y1);
  ix1 = round(x1);
  iy1 = round(y1);

  WC_to_NDC(xmax, ymin, gkss->cntnr, x2, y2);
  seg_xform(&x2, &y2);
  NDC_to_DC(x2, y2, x2, y2);
  ix2 = round(x2);
  iy2 = round(y2);

  width = abs(ix2 - ix1) * scale;
  height = abs(iy2 - iy1) * scale;
  if (width == 0 || height == 0) return;

  x = min(ix1, ix2) * scale;
  y = min(iy1, iy2) * scale;

  swapx = ix1 > ix2;
  swapy = iy1 > iy2;

  begin_context(context);
  if (!true_color)
    {
      tmpptr = (int *)gks_malloc(dx * dy * sizeof(int));
      for (i = 0; i < dx; i++)
        for (j = 0; j < dy; j++)
          {
            ix = swapx ? dx - i - 1 : i;
            iy = swapy ? dy - j - 1 : j;
            tmpptr[iy * dx + ix] = colia[j * dimx + i];
          }

      if (dx != width || dy != height)
        {
          colia = gks_resize(tmpptr, dx, dy, width, height);
          free(tmpptr);
        }
      else
        colia = tmpptr;

      cs = CGColorSpaceCreateDeviceRGB();
      for (i = 0; i < width * height; i++)
        {
          ind = colia[i];
          ind = FIX_COLORIND(ind);
          colors = CGColorGetComponents(p->rgb[ind]);
          colia[i] = (int)(gkss->alpha * colors[0] * 255) + ((int)(gkss->alpha * colors[1] * 255) << 8) +
                     ((int)(gkss->alpha * colors[2] * 255) << 16) + ((int)(gkss->alpha * 255) << 24);
        }
    }
  else
    {
      unsigned char *pixels = (unsigned char *)gks_malloc(width * height * 4);
      gks_resample((const unsigned char *)colia, pixels, dx, dy, width, height, dimx, swapx, swapy,
                   gkss->resample_method);

      cs = CGColorSpaceCreateDeviceRGB();
      for (i = 0; i < width * height; i++)
        {
          unsigned char red = pixels[i * 4 + 0];
          unsigned char green = pixels[i * 4 + 1];
          unsigned char blue = pixels[i * 4 + 2];
          unsigned char alpha = pixels[i * 4 + 3];
          /* Combine pixel alpha component and global transparency */
          float combined_alpha = alpha * gkss->alpha;
          /* Pre-multiply alpha */
          pixels[i * 4 + 0] = red * combined_alpha / 255.0;
          pixels[i * 4 + 1] = green * combined_alpha / 255.0;
          pixels[i * 4 + 2] = blue * combined_alpha / 255.0;
          pixels[i * 4 + 3] = combined_alpha;
        }
      colia = (int *)pixels;
    }

  bitmap = CGBitmapContextCreate(colia, width, height, 8, 4 * width, cs, kCGImageAlphaPremultipliedLast);
  CGContextScaleCTM(context, 1 / scale, 1 / scale);
  image = CGBitmapContextCreateImage(bitmap);
  CGContextDrawImage(context, CGRectMake(x, y, width, height), image);

  CGImageRelease(image);
  CGContextRelease(bitmap);
  CGColorSpaceRelease(cs);

  free(colia);

  end_context(context);
}

- (void)drawimage:(int)x:(int)y:(int)width:(int)height:(int *)bitmap
{
  CGColorSpaceRef cs;
  CGContextRef bmp;
  CGImageRef image;

  begin_context(context);
  cs = CGColorSpaceCreateDeviceRGB();
  bmp = CGBitmapContextCreate(bitmap, width, height, 8, 4 * width, cs, kCGImageAlphaPremultipliedLast);
  image = CGBitmapContextCreateImage(bmp);
  CGContextDrawImage(context, CGRectMake(x, y, width, height), image);
  CGImageRelease(image);
  CGContextRelease(bmp);
  CGColorSpaceRelease(cs);
  end_context(context);
}

- (NSString *)stringForText:(const char *)text withFontFamilyID:(int)family
{
  NSString *string;
  if (family == 30)
    { // ZapfDingbatsITC
      int i;
      int nchars = strlen(text);
      string = [NSString string];
      for (i = 0; i < nchars; i++)
        {
          string = [string stringByAppendingFormat:@"%d", dingbats[text[i]]];
        }
    }
  else if (family == 12)
    { // Symbols
      string = [NSString stringWithCString:text encoding:NSSymbolStringEncoding];
    }
  else
    {
      string = [NSString stringWithCString:text encoding:NSUTF8StringEncoding];
      if (!string)
        {
          /* if string creation failed, replace all invalid bytes with question marks */
          int i;
          const char *utf8_str = text;
          char *text_without_invalid_bytes = gks_malloc(strlen(text) + 1);
          for (i = 0; utf8_str[i] != 0; i++)
            {
              if ((utf8_str[i] & 0x80) == 0x00)
                {
                  text_without_invalid_bytes[i] = utf8_str[i];
                }
              else if ((utf8_str[i] & 0xe0) == 0xc0 && (utf8_str[i + 1] & 0xc0) == 0x80)
                {
                  text_without_invalid_bytes[i] = utf8_str[i];
                  text_without_invalid_bytes[i + 1] = utf8_str[i + 1];
                  i += 1;
                }
              else if ((utf8_str[i] & 0xf0) == 0xe0 && (utf8_str[i + 1] & 0xc0) == 0x80 &&
                       (utf8_str[i + 2] & 0xc0) == 0x80)
                {
                  text_without_invalid_bytes[i] = utf8_str[i];
                  text_without_invalid_bytes[i + 1] = utf8_str[i + 1];
                  text_without_invalid_bytes[i + 2] = utf8_str[i + 2];
                  i += 2;
                }
              else if ((utf8_str[i] & 0xf8) == 0xf0 && (utf8_str[i + 1] & 0xc0) == 0x80 &&
                       (utf8_str[i + 2] & 0xc0) == 0x80 && (utf8_str[i + 3] & 0xc0) == 0x80)
                {
                  text_without_invalid_bytes[i] = utf8_str[i];
                  text_without_invalid_bytes[i + 1] = utf8_str[i + 1];
                  text_without_invalid_bytes[i + 2] = utf8_str[i + 2];
                  text_without_invalid_bytes[i + 3] = utf8_str[i + 3];
                  i += 3;
                }
              else
                {
                  /* invalid byte combination */
                  text_without_invalid_bytes[i] = '?';
                  continue;
                }
            }
          text_without_invalid_bytes[i] = 0;
          string = [NSString stringWithCString:text_without_invalid_bytes encoding:NSUTF8StringEncoding];
          gks_free(text_without_invalid_bytes);
        }
      if (!string)
        {
          /* if string creation failed again, replace all non-ASCII bytes with question marks */
          int i;
          const char *utf8_str = text;
          char *text_without_nonascii_bytes = gks_malloc(strlen(text) + 1);
          for (i = 0; utf8_str[i] != 0; i++)
            {
              if ((utf8_str[i] & 0x80) == 0x00)
                {
                  text_without_nonascii_bytes[i] = utf8_str[i];
                }
              else
                {
                  /* invalid byte combination */
                  text_without_nonascii_bytes[i] = '?';
                  continue;
                }
            }
          text_without_nonascii_bytes[i] = 0;
          string = [NSString stringWithCString:text_without_nonascii_bytes encoding:NSASCIIStringEncoding];
          gks_free(text_without_nonascii_bytes);
        }
    }
  return string;
}


- (void)text:(double)px:(double)py:(char *)text
{
  int tx_font, tx_prec, tx_color, nchars;
  double xn, yn, xstart, ystart, xrel, yrel, ax, ay;
  NSString *fontName;

  nchars = strlen(text);

  tx_font = gkss->asf[6] ? gkss->txfont : predef_font[gkss->tindex - 1];
  tx_prec = gkss->asf[6] ? gkss->txprec : predef_prec[gkss->tindex - 1];
  tx_color = gkss->asf[9] ? gkss->txcoli : 1;

  [self set_stroke_color:tx_color:context];

  begin_context(context);

  if (tx_prec == GKS_K_TEXT_PRECISION_STRING)
    {
      _FontInfo info = [self set_font:tx_font];
      fontName = info.fontfamily;
      float fontsize = info.fontsize;


      CGFontRef cgfont; // Check if CGFont is already cached
      if (cgfontrefs[p->family] == NULL)
        {
          cgfontrefs[p->family] = CGFontCreateWithFontName((CFStringRef)fontName);
        }
      cgfont = cgfontrefs[p->family];
      CTFontRef font = CTFontCreateWithGraphicsFont(cgfont, fontsize, &CGAffineTransformIdentity, NULL);
      NSString *string = [self stringForText:text withFontFamilyID:p->family];
      if (!string)
        {
          return;
        }
      CFStringRef cfstring = (__bridge CFStringRef)string;
      CFStringRef keys[] = {kCTFontAttributeName, kCTForegroundColorFromContextAttributeName};
      CFTypeRef values[] = {font, kCFBooleanTrue};
      CFDictionaryRef attributes = CFDictionaryCreate(kCFAllocatorDefault, (const void **)&keys, (const void **)&values,
                                                      sizeof(keys) / sizeof(keys[0]), &kCFTypeDictionaryKeyCallBacks,
                                                      &kCFTypeDictionaryValueCallBacks);
      CFAttributedStringRef attrString = CFAttributedStringCreate(kCFAllocatorDefault, cfstring, attributes);
      CTLineRef line = CTLineCreateWithAttributedString(attrString);
      CGRect bounds = CTLineGetImageBounds(line, context);
      double stringWidth = bounds.size.width;

      // Calculate the required transformation
      WC_to_NDC(px, py, gkss->cntnr, xn, yn);
      seg_xform(&xn, &yn);
      NDC_to_DC(xn, yn, xstart, ystart);
      xrel = stringWidth * xfac[gkss->txal[0]];
      yrel = p->capheight * yfac[gkss->txal[1]];
      CharXform(xrel, yrel, ax, ay);
      xstart += ax;
      ystart += ay;

      update_color(tx_color);

      // Setup the rendering properties and draw the text line
      CGContextSetTextDrawingMode(context, kCGTextFill);
      CGContextSetFillColorWithColor(context, p->rgb[tx_color]);
      CGContextSetStrokeColorWithColor(context, p->rgb[tx_color]);
      if (p->angle != 0)
        {
          CGContextTranslateCTM(context, xstart, ystart);
          CGContextRotateCTM(context, p->angle);
          CGContextTranslateCTM(context, -xstart, -ystart);
        }
      CGContextSetTextPosition(context, xstart, ystart);
      CTLineDraw(line, context);

      CFRelease(attributes);
      CFRelease(attrString);
      CFRelease(line);
      CFRelease(font);
    }
#ifndef NO_FT
  else if (tx_prec == GKS_K_TEXT_PRECISION_CHAR)
    {
      int *bitmap;
      int x, y, w, h;

      WC_to_NDC(px, py, gkss->cntnr, xn, yn);
      seg_xform(&xn, &yn);
      NDC_to_DC(xn, yn, x, y);
      h = p->c;
      bitmap = gks_ft_render(&x, &y, &w, &h, gkss, text, nchars);
      if (bitmap != NULL)
        {
          [self drawimage:x:y:w:h:bitmap];
          free(bitmap);
        }
      else
        {
          gks_emul_text(px, py, nchars, text, line_routine, fill_routine);
        }
    }
#endif
  else
    {
      gks_emul_text(px, py, nchars, text, line_routine, fill_routine);
    }

  end_context(context);
}

- (_FontInfo)set_font:(int)font
{
  double scale, ux, uy;
  int fontsize;
  double width, height, capheight;

  font = abs(font);
  if (font >= 101 && font <= 129)
    font -= 100;
  else if (font >= 1 && font <= 32)
    font = map[font - 1];
  else
    font = 9;

  WC_to_NDC_rel(gkss->chup[0], gkss->chup[1], gkss->cntnr, ux, uy);
  seg_xform_rel(&ux, &uy);

  p->angle = -atan2(ux, uy);

  scale = sqrt(gkss->chup[0] * gkss->chup[0] + gkss->chup[1] * gkss->chup[1]);
  ux = gkss->chup[0] / scale * gkss->chh;
  uy = gkss->chup[1] / scale * gkss->chh;
  WC_to_NDC_rel(ux, uy, gkss->cntnr, ux, uy);

  width = 0;
  height = sqrt(ux * ux + uy * uy);
  seg_xform_rel(&width, &height);

  height = sqrt(width * width + height * height);
  capheight = nint(height * (fabs(p->c) + 1));
  p->capheight = nint(capheight);

  fontsize = nint(capheight / capheights[font - 1]);
  p->family = font - 1;

  _FontInfo info;
  info.fontsize = fontsize;
  info.fontfamily = [NSString stringWithCString:fonts[p->family] encoding:NSASCIIStringEncoding];
  return info;
}

@end
