#ifdef __unix__
#define _POSIX_C_SOURCE 200112L
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#if !defined(VMS) && !defined(_WIN32)
#include <unistd.h>
#endif

#ifdef hpux
#include <sys/utsname.h>
#endif

#if defined(_WIN32) && !defined(__GNUC__)
#include <windows.h>
#endif

#include "gks.h"
#include "gkscore.h"

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif
#define NINT(a) (int)((a) + 0.5)

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define SIZE_INCREMENT 32768

#define PATTERNS 120

#define LLX 16
#define LLY 16

#define WC_to_NDC(xw, yw, tnr, xn, yn) \
  xn = a[tnr] * (xw) + b[tnr];         \
  yn = c[tnr] * (yw) + d[tnr]

#define WC_to_NDC_rel(xw, yw, tnr, xn, yn) \
  xn = a[tnr] * (xw);                      \
  yn = c[tnr] * (yw)

#define NDC_to_DC(xn, yn, xd, yd) \
  xd = p->a * (xn) + p->b;        \
  yd = p->c * (yn) + p->d

#define nint(a) ((int)(a + 0.5))

static gks_state_list_t *gkss;

static double a[MAX_TNR], b[MAX_TNR], c[MAX_TNR], d[MAX_TNR];

static const char *show[] = {"lj", "lj", "ct", "rj"};
static double yfac[] = {0., -1.2, -1.0, -0.5, 0., 0.2};

static int predef_font[] = {1, 1, 1, -2, -3, -4};
static int predef_prec[] = {0, 1, 2, 2, 2, 2};
static int predef_ints[] = {0, 1, 3, 3, 3};
static int predef_styli[] = {1, 1, 1, 2, 3};

static int map[] = {22, 9,  5, 14, 18, 26, 13, 1, 24, 11, 7, 16, 20, 28, 13, 3,
                    23, 10, 6, 15, 19, 27, 13, 2, 25, 12, 8, 17, 21, 29, 13, 4};

static double caps[] = {0.662, 0.653, 0.676, 0.669, 0.718, 0.718, 0.718, 0.718, 0.562, 0.562, 0.562,
                        0.562, 0.667, 0.681, 0.681, 0.681, 0.681, 0.722, 0.722, 0.722, 0.722, 0.740,
                        0.740, 0.740, 0.740, 0.692, 0.692, 0.681, 0.681, 0.587, 0.692};

static const char *fonts[] = {"Times-Roman",
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
                              "Bookman-Light",
                              "Bookman-LightItalic",
                              "Bookman-Demi",
                              "Bookman-DemiItalic",
                              "NewCenturySchlbk-Roman",
                              "NewCenturySchlbk-Italic",
                              "NewCenturySchlbk-Bold",
                              "NewCenturySchlbk-BoldItalic",
                              "AvantGarde-Book",
                              "AvantGarde-BookOblique",
                              "AvantGarde-Demi",
                              "AvantGarde-DemiOblique",
                              "Palatino-Roman",
                              "Palatino-Italic",
                              "Palatino-Bold",
                              "Palatino-BoldItalic",
                              "ZapfChancery-MediumItalic",
                              "ZapfDingbats"};

static const char *dc[3][3] = {{"F", "E", "D"}, {"G", "I", "C"}, {"H", "A", "B"}};

typedef struct ws_state_list_t
{
  int conid, wtype, state;
  int empty, init, pages;

  int ix, iy;
  double a, b, c, d, e, f, g, h, mw, mh;
  int ytrans, res;
  double magstep;
  int stroke, limit, np;

  double red[MAX_COLOR + 1], green[MAX_COLOR + 1], blue[MAX_COLOR + 1];
  int color, fcol;

  double ysize;

  int len, size, column, saved_len, saved_column;
  char *buffer;

  unsigned char ascii85_buffer[10];
  char a85line[100];
  long a85offset;

  double window[4], viewpt[4];

  int ltype;
  double cwidth, csize, cangle, cheight;
  int font;

  double width, height, nominal_size;
} ws_state_list;

static ws_state_list *p;

static void set_norm_xform(int tnr, double *wn, double *vp)
{
  a[tnr] = (vp[1] - vp[0]) / (wn[1] - wn[0]);
  b[tnr] = vp[0] - wn[0] * a[tnr];
  c[tnr] = (vp[3] - vp[2]) / (wn[3] - wn[2]);
  d[tnr] = vp[2] - wn[2] * c[tnr];
}

static void init_norm_xform(void)
{
  int tnr;

  for (tnr = 0; tnr < MAX_TNR; tnr++) set_norm_xform(tnr, gkss->window[tnr], gkss->viewport[tnr]);
}

static void seg_xform(double *x, double *y)
{
  double xx;

  xx = *x * gkss->mat[0][0] + *y * gkss->mat[0][1] + gkss->mat[2][0];
  *y = *x * gkss->mat[1][0] + *y * gkss->mat[1][1] + gkss->mat[2][1];
  *x = xx;
}

static void seg_xform_rel(double *x, double *y)
{
  double xx;

  xx = *x * gkss->mat[0][0] + *y * gkss->mat[0][1];
  *y = *x * gkss->mat[1][0] + *y * gkss->mat[1][1];
  *x = xx;
}

static int lastop(const char *op)
{
  int len;

  len = strlen(op);
  if (len < p->len)
    return strncmp(p->buffer + p->len - len, op, len);
  else
    return -1;
}

static void packb(const char *buff)
{
  int len, i;

  len = strlen(buff);

  p->saved_len = p->len;
  p->saved_column = p->column;

  if (buff[0] == '%')
    {
      if (p->column != 0)
        {
          p->buffer[p->len++] = '\n';
          p->column = 0;
        }
    }
  else if (len > 78 - p->column)
    {
      if (p->len != 0)
        {
          p->buffer[p->len++] = '\n';
          p->column = 0;
        }
    }

  if (len + 2 > p->size - p->len)
    {
      p->size += SIZE_INCREMENT;
      p->buffer = (char *)realloc(p->buffer, p->size);
    }

  if (p->column != 0)
    {
      p->buffer[p->len++] = ' ';
      p->column++;
    }

  for (i = 0; i < len; i++)
    {
      p->buffer[p->len++] = buff[i];
      p->column++;
    }

  if (buff[0] == '%')
    {
      p->buffer[p->len++] = '\n';
      p->column = 0;
    }
}

static char *Ascii85Tuple(unsigned char *data)
{
  static char tuple[6];
  long i, x;
  unsigned long code, quantum;

  code = ((((unsigned long)data[0] << 8) | (unsigned long)data[1]) << 16) | ((unsigned long)data[2] << 8) |
         (unsigned long)data[3];
  if (code == 0L)
    {
      tuple[0] = 'z';
      tuple[1] = '\0';
      return (tuple);
    }
  quantum = 85UL * 85UL * 85UL * 85UL;
  for (i = 0; i < 4; i++)
    {
      x = (long)(code / quantum);
      code -= quantum * x;
      tuple[i] = (char)(x + (int)'!');
      quantum /= 85L;
    }
  tuple[4] = (char)((code % 85L) + (int)'!');
  tuple[5] = '\0';
  return (tuple);
}

static void Ascii85Initialize(void)
{
  p->a85offset = 0L;
  p->a85line[0] = '\0';
}

static void Ascii85Flush(void)
{
  char *tuple;

  packb(p->a85line);
  if (p->a85offset > 0)
    {
      p->ascii85_buffer[p->a85offset] = '\0';
      p->ascii85_buffer[p->a85offset + 1] = '\0';
      p->ascii85_buffer[p->a85offset + 2] = '\0';
      tuple = Ascii85Tuple(p->ascii85_buffer);
      packb(*tuple == 'z' ? (char *)"!!!!" : tuple);
    }
  packb("~>");
}

static void Ascii85Encode(unsigned char code)
{
  long n, i = 0;
  char *q;
  unsigned char *c;
  char b[100];

  p->ascii85_buffer[p->a85offset] = code;
  p->a85offset++;
  if (p->a85offset < 4) return;
  c = p->ascii85_buffer;
  for (n = p->a85offset; n >= 4; n -= 4)
    {
      for (q = Ascii85Tuple(c); *q; q++) b[i++] = *q;
      c += 8;
    }
  p->a85offset = n;
  c -= 4;
  b[i] = '\0';
  strcat(p->a85line, b);
  if (strlen(p->a85line) >= 75)
    {
      packb(p->a85line);
      p->a85line[0] = '\0';
    }
  for (n = 0; n < 4; n++) p->ascii85_buffer[n] = (*c++);
}

static unsigned int LZWEncodeImage(unsigned int number_pixels, unsigned char *pixels)
{
#define LZWClr 256UL /* Clear Table Marker */
#define LZWEod 257UL /* End of Data marker */
#define OutputCode(code)                                    \
  {                                                         \
    accumulator += code << (32 - code_width - number_bits); \
    number_bits += code_width;                              \
    while (number_bits >= 8)                                \
      {                                                     \
        Ascii85Encode((unsigned char)(accumulator >> 24));  \
        accumulator = accumulator << 8;                     \
        number_bits -= 8;                                   \
      }                                                     \
  }

  typedef struct _TableType
  {
    long prefix, suffix, next;
  } TableType;

  long index;
  long i;
  TableType *table;
  unsigned long accumulator, number_bits, code_width, last_code, next_index;

  /*
   * Allocate string table.
   */

  table = (TableType *)malloc((1 << 12) * sizeof(*table));
  if (table == (TableType *)NULL) return (0);

  /*
   * Initialize variables.
   */
  accumulator = 0;
  code_width = 9;
  number_bits = 0;
  last_code = 0;
  Ascii85Initialize();
  OutputCode(LZWClr);
  for (index = 0; index < 256; index++)
    {
      table[index].prefix = (-1);
      table[index].suffix = (short)index;
      table[index].next = (-1);
    }
  next_index = LZWEod + 1;
  code_width = 9;
  last_code = (unsigned long)pixels[0];
  for (i = 1; i < (long)number_pixels; i++)
    {
      /*
       * Find string.
       */
      index = (long)last_code;
      while (index != -1)
        if ((table[index].prefix != (long)last_code) || (table[index].suffix != (long)pixels[i]))
          index = table[index].next;
        else
          {
            last_code = (unsigned long)index;
            break;
          }
      if (last_code != (unsigned long)index)
        {
          /*
           * Add string.
           */
          OutputCode(last_code);
          table[next_index].prefix = (long)last_code;
          table[next_index].suffix = (short)pixels[i];
          table[next_index].next = table[last_code].next;
          table[last_code].next = (long)next_index;
          next_index++;
          /*
           * Did we just move up to next bit width?
           */
          if ((next_index >> code_width) != 0)
            {
              code_width++;
              if (code_width > 12)
                {
                  /*
                   * Did we overflow the max bit width?
                   */
                  code_width--;
                  OutputCode(LZWClr);
                  for (index = 0; index < 256; index++)
                    {
                      table[index].prefix = (-1);
                      table[index].suffix = index;
                      table[index].next = (-1);
                    }
                  next_index = LZWEod + 1;
                  code_width = 9;
                }
            }
          last_code = (unsigned long)pixels[i];
        }
    }
  /*
   * Flush tables.
   */
  OutputCode(last_code);
  OutputCode(LZWEod);
  if (number_bits != 0) Ascii85Encode((unsigned char)(accumulator >> 24));

  Ascii85Flush();
  free(table);

  return (0);
}

static void set_xform(double *wn, double *vp)
{
  p->e = (vp[1] - vp[0]) / (wn[1] - wn[0]);
  p->f = (6750 - 1) / 0.28575;
  p->h = (4650 - 1) / 0.19685;
  p->g = (vp[3] - vp[2]) / (wn[3] - wn[2]);

  p->a = p->e * p->f;
  p->b = p->f * (vp[0] - wn[0] * p->e);
  p->c = p->g * p->h;
  p->d = p->h * (vp[2] - wn[2] * p->g);

  p->mw = p->a * (wn[1] - wn[0]);
  p->mh = p->c * (wn[3] - wn[2]);

  p->width = p->a;
  p->height = p->c;
  p->nominal_size = MIN(p->width, p->height) / 500.0 * 72 / 600;

  p->stroke = 0;
}

static void bounding_box(int landscape, double magstep)
{
  char buffer[50];
  int ix1, ix2, iy1, iy2;
  double magn;

  if (fabs(magstep) > FEPS)
    magn = pow(1.2, magstep);
  else
    magn = 1.0;

  ix1 = LLX;
  iy1 = LLY;
  ix2 = ix1 + NINT((landscape ? p->mh : p->mw) * 72 / 600 * magn);
  iy2 = iy1 + NINT((landscape ? p->mw : p->mh) * 72 / 600 * magn);
  if (gkss->version < 5)
    p->ytrans = landscape ? iy2 : iy1;
  else
    p->ytrans = -ix2;
  p->res = (ix2 - ix1) + (iy2 - iy1);

  sprintf(buffer, "%%%%BoundingBox: %d %d %d %d", ix1, iy1, ix2, iy2);
  packb(buffer);
  if (gkss->version > 4)
    {
      sprintf(buffer, "%%%%Orientation: %s", landscape ? "Landscape" : "Portrait");
      packb(buffer);
    }
}

static void move(double x, double y)
{
  char buffer[50];

  p->ix = NINT(p->a * x + p->b);
  p->iy = NINT(p->c * y + p->d);

  if (p->stroke)
    {
      packb("sk");
      p->stroke = 0;
    }
  sprintf(buffer, "np %d %d m", p->ix, p->iy);
  packb(buffer);
  p->np = 1;
}

static void draw(double x, double y)
{
  char buffer[50];
  int jx, jy, rx, ry;

  jx = p->ix;
  jy = p->iy;
  p->ix = NINT(p->a * x + p->b);
  p->iy = NINT(p->c * y + p->d);

  if (p->np == 1 || p->ix != jx || p->iy != jy)
    {
      rx = p->ix - jx;
      ry = p->iy - jy;
      if (abs(rx) > 1 || abs(ry) > 1)
        {
          sprintf(buffer, "%d %d rl", rx, ry);
          packb(buffer);
        }
      else
        packb(dc[rx + 1][ry + 1]);
      p->np++;

      if (p->limit)
        {
          if (p->np == p->limit)
            {
              packb("sk");
              p->stroke = 0;
              sprintf(buffer, "%d %d m", p->ix, p->iy);
              packb(buffer);
              p->np = 1;
            }
          else
            p->stroke = 1;
        }
    }
}

static void moveto(double x, double y)
{
  char buffer[20];

  p->ix = NINT(x);
  p->iy = NINT(y);

  sprintf(buffer, "%d %d m", p->ix, p->iy);
  packb(buffer);
}

static void amoveto(double angle, double x, double y)
{
  char buffer[30];

  p->ix = NINT(x);
  p->iy = NINT(y);

  sprintf(buffer, "%.4g %d %d am", angle, p->ix, p->iy);
  packb(buffer);
}

static void set_linetype(int ltype, double lwidth)
{
  char buffer[100], dash[80];

  if (ltype != p->ltype || (fabs(lwidth - p->cwidth) > FEPS))
    {
      p->ltype = ltype;
      gks_get_dash(ltype, lwidth * 8, dash);
      sprintf(buffer, "%s 0 setdash", dash);
      packb(buffer);
    }
}

static void set_linewidth(double width)
{
  char buffer[20];

  if (fabs(width - p->cwidth) > FEPS)
    {
      p->cwidth = fabs(width);
      sprintf(buffer, "%.4g lw", p->cwidth * 600 / 72 * p->nominal_size);
      packb(buffer);
    }
}

static void set_markersize(double size)
{
  char buffer[20];

  if (fabs(size - p->csize) > FEPS)
    {
      p->csize = fabs(size);
      sprintf(buffer, "%.4g ms", p->csize * p->nominal_size);
      packb(buffer);
    }
}

static void set_markerangle(double angle)
{
  char buffer[20];

  if (fabs(angle - p->cangle) > FEPS)
    {
      p->cangle = fabs(angle);
      sprintf(buffer, "%.4g ma", p->cangle);
      packb(buffer);
    }
}

static void gkinfo(int *nchars, char *chars)
{
  char *date, host[100];
  const char *user;
  time_t elapsed_time;
#ifdef hpux
  struct utsname utsname;
#endif
#ifdef _WIN32
  char lpBuffer[100];
  DWORD nSize = 100;
#endif

  time(&elapsed_time);
  date = ctime(&elapsed_time);

#ifndef _WIN32
  user = (char *)gks_getenv("USER");
#else
  if (GetUserName(lpBuffer, &nSize) != 0)
    {
      user = lpBuffer;
      lpBuffer[nSize] = '\0';
    }
  else
    user = NULL;
#endif
  if (user == NULL) user = "(?)";

#ifdef VMS
  strcpy(host, (char *)gks_getenv("SYS$NODE"));
#else
#ifdef hpux
  uname(&utsname);
  strcpy(host, utsname.nodename);
#else
#if defined(OS2) || (defined(_WIN32) && !defined(__GNUC__))
  strcpy(host, "(unknown)"); /* FIXME */
#else
  gethostname(host, 100);
#endif /* _WIN32 */
#endif /* hpux */
#endif /* VMS */

  strtok(date, "\n");
  strtok(host, ".");

  sprintf(chars, "%s  by user  %s @ %s", date, user, host);
  *nchars = strlen(chars);
}

static void writefile(char *buffer)
{
  gks_write_file(p->conid, buffer, strlen(buffer));
}

static void ps_header(void)
{
  int nchars;
  char info[150], buffer[150];

  gkinfo(&nchars, info);
  writefile("%!PS-Adobe-2.0\n");
  if (nchars > 0)
    {
      sprintf(buffer, "\
%%%%Creator: %s, GKS 5 PostScript Device Handler\n",
              info + 35);
      writefile(buffer);
      info[24] = '\0';
      sprintf(buffer, "%%%%+CreationDate: %s\n", info);
      writefile(buffer);
    }
  else
    writefile("%%Creator: GKS 5 PostScript Device Handler\n");
  writefile("%%+Copyright @ 1993-2007, J.Heinen\n");
  sprintf(buffer, "%%%%Pages: %d\n", p->pages);
  writefile(buffer);
}

static void set_color(int color, int wtype)
{
  char buffer[50];
  double grey;
  int index;

  if (color < MAX_COLOR)
    {
      if (color != p->color)
        {
          if (lastop("sc") == 0)
            {
              p->len = p->saved_len;
              p->column = p->saved_column;
            }
          index = abs(color);
          if (wtype % 2)
            {
              grey = 0.3 * p->red[index] + 0.59 * p->green[index] + 0.11 * p->blue[index];
              sprintf(buffer, "%.4g sg", grey);
              packb(buffer);
            }
          else
            {
              sprintf(buffer, "%.4g %.4g %.4g sc", p->red[index], p->green[index], p->blue[index]);
              packb(buffer);
            }
          p->color = index;
        }
    }
}

static void set_foreground(int color, int wtype)
{
  char buffer[50];
  int index;
  double grey;

  if (color < MAX_COLOR)
    {
      if (color != p->fcol)
        {
          index = abs(color);
          if (wtype % 2)
            {
              grey = 0.3 * p->red[index] + 0.59 * p->green[index] + 0.11 * p->blue[index];
              sprintf(buffer, "/fg {%.4g sg} def", grey);
              packb(buffer);
            }
          else
            {
              sprintf(buffer, "/fg {%.4g %.4g %.4g sc} def", p->red[index], p->green[index], p->blue[index]);
              packb(buffer);
            }
          p->fcol = index;
        }
      if (color != p->color)
        {
          index = abs(color);
          packb("fg");
          p->color = index;
        }
    }
}

static void set_background(int wtype)
{
  char buffer[50];
  double grey;

  if (wtype % 2)
    {
      grey = 0.3 * p->red[0] + 0.59 * p->green[0] + 0.11 * p->blue[0];
      sprintf(buffer, "/bg {%.4g sg} def", grey);
      packb(buffer);
    }
  else
    {
      sprintf(buffer, "/bg {%.4g %.4g %.4g sc} def", p->red[0], p->green[0], p->blue[0]);
      packb(buffer);
    }
}

static void update(void)
{
  if (p->column != 0)
    {
      p->buffer[p->len++] = '\n';
      p->column = 0;
    }
}

static void set_clip(double *clrt)
{
  int i, j;
  int ix1, ix2, iy1, iy2;
  double cx1, cy1, cx2, cy2;
  char buffer[100];

  i = clrt[0] < clrt[1] ? 0 : 1;
  j = clrt[2] < clrt[3] ? 2 : 3;

  NDC_to_DC(clrt[i], clrt[j], cx1, cy1);
  NDC_to_DC(clrt[1 - i], clrt[5 - j], cx2, cy2);

  ix1 = ((int)cx1) - 2;
  iy1 = ((int)cy1) - 2;
  ix2 = NINT(cx2) + 2;
  iy2 = NINT(cy2) + 2;

  sprintf(buffer, "np %d %d m %d %d l %d %d l %d %d l cp clip", ix1, iy1, ix1, iy2, ix2, iy2, ix2, iy1);
  packb(buffer);
}

static void set_font(int font)
{

  double scale, w, h, ux, uy, chh;
  char buffer[200];
  int size;

  scale = sqrt(gkss->chup[0] * gkss->chup[0] + gkss->chup[1] * gkss->chup[1]);
  ux = gkss->chup[0] / scale * gkss->chh;
  uy = gkss->chup[1] / scale * gkss->chh;
  WC_to_NDC_rel(ux, uy, gkss->cntnr, ux, uy);

  w = 0;
  h = sqrt(ux * ux + uy * uy);
  seg_xform_rel(&w, &h);

  chh = sqrt(w * w + h * h);

  if ((font != p->font) || (fabs(chh - p->cheight) > FEPS))
    {
      p->font = abs(font);
      p->cheight = fabs(chh);

      if (p->font >= 101 && p->font <= 131)
        font = p->font - 101;
      else if (p->font >= 1 && p->font <= 32)
        font = map[p->font - 1] - 1;
      else
        font = 8;

      p->ysize = p->cheight * p->height;
      size = MIN(MAX((int)(p->ysize / caps[font]), 1), 7200);

      if (font != 12 && font != 29 && font != 30)
        {
          sprintf(buffer, "gsave /%s_ ISOLatin1Encoding", fonts[font]);
          packb(buffer);
          sprintf(buffer, "/%s encodefont pop grestore", fonts[font]);
          packb(buffer);
          sprintf(buffer, "/%s_ findfont %d scalefont setfont", fonts[font], size);
          packb(buffer);
        }
      else
        {
          sprintf(buffer, "/%s findfont %d scalefont setfont", fonts[font], size);
          packb(buffer);
        }
    }
}

static void get_magstep(double *magstep, int *dpi)
{
  char *env;

  if ((env = (char *)gks_getenv("GKS_MAGSTEP")) != NULL)
    *magstep = atof(env);
  else
    *magstep = 0;

  *dpi = 75;
}

static void ps_init(int *pages)
{
  int dpi, landscape;
  int pa[33], i, j = 1, k;
  char str[17], buffer[100];

  landscape = p->wtype >= 63;

  if (gkss->version < 5)
    if (!landscape) landscape = p->viewpt[1] - p->viewpt[0] > 0.19685;

  if (*pages == 0)
    {
      get_magstep(&p->magstep, &dpi);

      bounding_box(landscape, p->magstep);
      packb("%%EndComments");
      packb("%%BeginProcSet: GKS 5");

      packb("save /GKS_save exch def");
      packb("/GKS_dict 150 dict def GKS_dict begin");

      packb("/in {72 mul} def");
      packb("/np {newpath} def");
      packb("/cp {closepath} def");
      packb("/m {moveto} def");
      packb("/l {lineto} def");
      packb("/c {curveto} def");
      packb("/A {1 0 rlineto} def");
      packb("/B {1 1 rlineto} def");
      packb("/C {0 1 rlineto} def");
      packb("/D {-1 1 rlineto} def");
      packb("/E {-1 0 rlineto} def");
      packb("/F {-1 -1 rlineto} def");
      packb("/G {0 -1 rlineto} def");
      packb("/H {1 -1 rlineto} def");
      packb("/I {1 0 rlineto -1 0 rlineto} def");
      packb("/am {np gsave translate rotate 0 0 m} def");
      packb("/gs {gsave} def");
      packb("/gr {grestore} def");
      packb("/rm {rmoveto} def");
      packb("/srm {rxy rx s mul ry s mul rm} def");
      packb("/rl {rlineto} def");
      packb("/srl {rxy rx s mul ry s mul rl} def");
      packb("/sk {stroke} def");
      packb("/csk {closepath stroke} def");
      packb("/fi {closepath eofill} def");
      packb("/el {/endangle exch def /startangle exch def\
 /yrad exch def /xrad exch def /y exch def /x exch def\
 /savematrix matrix currentmatrix def x y translate xrad yrad scale\
 0 0 1 startangle endangle arc savematrix setmatrix} def");
      packb("/eln {/endangle exch def /startangle exch def\
 /yrad exch def /xrad exch def /y exch def /x exch def\
 /savematrix matrix currentmatrix def x y translate xrad yrad scale\
 0 0 1 startangle endangle arcn savematrix setmatrix} def");
      packb("/sg {setgray} def");
      packb("/sc {setrgbcolor} def");
      packb("/fg {0 sg} def");

      set_background(p->wtype);

      packb("/lw {setlinewidth} def");
      packb("/ms {/s exch def} def");
      packb("/ma {/a exch def} def");
      packb("/ct {dup stringwidth pop 2 div neg 0 rmoveto show} def");
      packb("/rj {dup stringwidth pop neg 0 rmoveto show} def");
      packb("/lj {show} def");
      packb("/xy {/y exch def /x exch def} def");
      packb("/rxy {/ry exch def /rx exch def} def");
      packb("/sxy {gsave xy x y translate a rotate x neg y neg translate} def");
      packb("/dt {xy np fg x y s 0 360 arc fi} def");
      packb("/pl {sxy np x y m fg -24 0 srl 48 0 srl\
 -24 0 srl 0 24 srl 0 -48 srl sk gr} def");
      packb("/as {np x y m 0 24 srm 14 -43.4 srl\
 -36.8 26.8 srl 45.6 0 srl -36.8 -26.8 srl");
      packb("14 43.4 srl 14 -43.4 srl} def");
      packb("/fas {sxy fg as fill fg as csk gr} def");
      packb("/dc {sxy np x y m fg -24 24 srl 48 -48 srl\
 -24 24 srl -24 -24 srl 48 48 srl");
      packb("sk gr} def");
      packb("/hl {sxy np x y m fg -24 0 srm 48 0 srl sk gr} def");
      packb("/vl {sxy np x y m fg 0 -24 srm 0 48 srl sk gr} def");
      packb("/e5 {np x y m 0 25 srm 23.775 -17.275 srl -9.075 -27.95 srl\
 -29.4 0 srl -9.075 27.95 srl} def");
      packb("/e6 {np x y m 0 -25 srm 21.65 12.5 srl 0 25 srl -21.65 12.5 srl\
 -21.65 -12.5 srl 0 -25 srl} def");
      packb("/e7 {np x y m 0 25 srm 19.55 -9.425 srl 4.825 -21.15 srl\
 -13.525 -16.95 srl -21.7 0 srl -13.525 16.95 srl 4.825 21.15 srl} def");
      packb("/e8 {np x y m 0 -25 srm 17.675 7.325 srl 7.325 17.675 srl\
 -7.325 17.675 srl -17.675 7.325 srl -17.675 -7.325 srl -7.325 -17.675 srl\
 7.325 -17.675 srl} def");
      packb("/s4 {np x y m 0 -25 srm 7.075 17.925 srl 17.925 7.075 srl\
 -17.925 7.075 srl -7.075 17.925 srl -7.075 -17.925 srl -17.925 -7.075 srl\
 17.925 -7.075 srl} def");
      packb("/s5 {np x y m 0 25 srm 5.875 -16.9 srl 17.9 -0.375 srl\
 -14.275 -10.825 srl 5.2 -17.125 srl -14.7 10.225 srl -14.7 -10.225 srl\
 5.2 17.125 srl -14.275 10.825 srl 17.9 0.375 srl} def");
      packb("/s6 {np x y m 0 -25 srm 5 16.35 srl 16.65 -3.85 srl\
 -11.65 12.5 srl 11.65 12.5 srl -16.65 -3.85 srl -5 16.35 srl -5 -16.35 srl\
 -16.65 3.85 srl 11.65 -12.5 srl -11.65 -12.5 srl 16.65 3.85 srl} def");
      packb("/s7 {np x y m 0 25 srm 4.35 -16 srl 15.2 6.575 srl\
 -9.8 -13.35 srl 14.625 -7.8 srl -16.55 -0.65 srl 3.025 -16.3 srl\
 -10.85 12.525 srl -10.85 -12.525 srl 3.025 16.3 srl -16.55 0.65 srl\
 14.625 7.8 srl -9.8 13.35 srl 15.2 -6.575 srl} def");
      packb("/s8 {np x y m 0 -25 srm 3.825 15.75 srl 13.85 -8.425 srl\
 -8.425 13.85 srl 15.75 3.825 srl -15.75 3.825 srl 8.425 13.85 srl\
 -13.85 -8.425 srl -3.825 15.75 srl -3.825 -15.75 srl -13.85 8.425 srl\
 8.425 -13.85 srl -15.75 -3.825 srl 15.75 -3.825 srl -8.425 -13.85 srl\
 13.85 8.425 srl} def");
      packb("/ed5 {sxy fg e5 fill fg e5 csk gr} def");
      packb("/ed6 {sxy fg e6 fill fg e6 csk gr} def");
      packb("/ed7 {sxy fg e7 fill fg e7 csk gr} def");
      packb("/ed8 {sxy fg e8 fill fg e8 csk gr} def");
      packb("/st4 {sxy fg s4 fill fg s4 csk gr} def");
      packb("/st5 {sxy fg s5 fill fg s5 csk gr} def");
      packb("/st6 {sxy fg s6 fill fg s6 csk gr} def");
      packb("/st7 {sxy fg s7 fill fg s7 csk gr} def");
      packb("/st8 {sxy fg s8 fill fg s8 csk gr} def");
      packb("/sq {np x y m 0 24 srm 24 0 srl 0 -48 srl\
 -48 0 srl 0 48 srl 24 0 srl} def");
      packb("/nsq {sxy bg sq fi fg sq csk gr} def");
      packb("/fsq {sxy fg sq fi fg sq csk gr} def");
      packb("/ci {np x y 24 s mul 0 360 arc} def");
      packb("/nci {xy bg ci fi fg ci sk} def");
      packb("/fci {xy fg ci fi fg ci sk} def");
      packb("/tu {np x y m 0 28 srm -24 -42 srl 48 0 srl -24 42 srl} def");
      packb("/ntu {sxy bg tu fi fg tu csk gr} def");
      packb("/ftu {sxy fg tu fi fg tu csk gr} def");
      packb("/td {np x y m 0 -28 srm -24 42 srl 48 0 srl -24 -42 srl} def");
      packb("/ntd {sxy bg td fi fg td csk gr} def");
      packb("/ftd {sxy fg td fi fg td csk gr} def");
      packb("/dm {np x y m 0 24 srm -24 -24 srl\
 24 -24 srl 24 24 srl -24 24 srl} def");
      packb("/ndm {sxy bg dm fi fg dm csk gr} def");
      packb("/fdm {sxy fg dm fi fg dm csk gr} def");
      packb("/bt {np x y m -30 24 srl 0 -48 srl\
 60 48 srl 0 -48 srl -30 24 srl} def");
      packb("/nbt {sxy bg bt fi fg bt csk gr} def");
      packb("/fbt {sxy fg bt fi fg bt csk gr} def");
      packb("/hg {np x y m -24 30 srl 48 0 srl\
 -48 -60 srl 48 0 srl -24 30 srl} def");
      packb("/nhg {sxy bg hg fi fg hg csk gr} def");
      packb("/fhg {sxy fg hg fi fg hg csk gr} def");
      packb("/st {sxy bg as fi fg as csk gr} def");
      packb("/fst {fas} def");
      packb("/tud {sxy bg tu fi bg td fi fg tu csk fg td csk gr} def");
      packb("/tl {np x y m -14 0 srm 42 -24 srl 0 48 srl -42 -24 srl} def");
      packb("/ftl {sxy fg tl fi fg tl csk gr} def");
      packb("/tr {np x y m 28 0 srm -42 -24 srl 0 48 srl 42 -24 srl} def");
      packb("/ftr {sxy fg tr fi fg tr csk gr} def");
      packb("/opl {np x y m 0 24 srm 8 0 srl\
 0 -16 srl 16 0 srl 0 -16 srl -16 0 srl");
      packb("0 -16 srl -16 0 srl 0 16 srl -16 0 srl\
 0 16 srl 16 0 srl 0 16 srl 8 0 srl} def");
      packb("/npl {sxy bg opl fi fg opl csk gr} def");
      packb("/fpl {sxy fg opl fi fg opl csk gr} def");
      packb("/om {np x y m 0 24 srm 16 0 srl\
 8 -8 srl 0 -32 srl -8 -8 srl -32 0 srl");
      packb("-8 8 srl 0 32 srl 8 8 srl 16 0 srl} def");
      packb("/nom {sxy bg om fi fg om csk gr} def");

      for (i = 0; i < PATTERNS; i++)
        {
          gks_inq_pattern_array(i, pa);
          for (j = *pa; j < ((*pa == 32) ? 16 : (*pa == 4) ? 8 : *pa); j++)
            {
              pa[j + 1] = pa[j % *pa + 1];
            }
          for (k = 0, j = 1; j < 9; j++, k += 2)
            {
              sprintf(str + k, "%02x", pa[j]);
            }
          sprintf(buffer, "/pat%d << /PaintType 2 /PatternType 1 /TilingType 1\
 /BBox [0 0 1 1] /XStep 1",
                  i);
          packb(buffer);
          sprintf(buffer, "/YStep 1 /PaintProc {pop 8 8 false [8 0 0 8 0 0] \
{<%s>} imagemask}",
                  str);
          packb(buffer);
          packb(">> [0 8 -8 0 0 0] makepattern def");
        }

      packb("/OF /findfont load def");
      packb("/findfont {dup GKS_dict exch known");
      packb("{GKS_dict exch get}");
      packb("if GKS_dict /OF get exec} def");
      packb("mark");
      packb("/ISOLatin1Encoding 8#000 1 8#001 {StandardEncoding exch get} for");
      packb("/emdash /endash 8#004 1 8#025 {StandardEncoding exch get} for");
      packb("/quotedblleft /quotedblright 8#030 1 8#054\
 {StandardEncoding exch get} for");
      packb("/minus 8#056 1 8#217 {StandardEncoding exch get} for");
      packb("/dotlessi 8#301 1 8#317 {StandardEncoding exch get} for");
      packb("/space/exclamdown/cent/sterling/currency/yen/brokenbar/section");
      packb("/dieresis/copyright/ordfeminine/guillemotleft\
/logicalnot/hyphen/registered");
      packb("/macron/degree/plusminus/twosuperior\
/threesuperior/acute/mu/paragraph");
      packb("/periodcentered/cedilla/onesuperior/ordmasculine\
/guillemotright/onequarter");
      packb("/onehalf/threequarters/questiondown/Agrave\
/Aacute/Acircumflex/Atilde");
      packb("/Adieresis/Aring/AE/Ccedilla/Egrave/Eacute\
/Ecircumflex/Edieresis/Igrave");
      packb("/Iacute/Icircumflex/Idieresis/Eth/Ntilde/Ograve\
/Oacute/Ocircumflex/Otilde");
      packb("/Odieresis/multiply/Oslash/Ugrave/Uacute\
/Ucircumflex/Udieresis/Yacute/Thorn");
      packb("/germandbls/agrave/aacute/acircumflex/atilde\
/adieresis/aring/ae/ccedilla");
      packb("/egrave/eacute/ecircumflex/edieresis/igrave\
/iacute/icircumflex/idieresis");
      packb("/eth/ntilde/ograve/oacute/ocircumflex/otilde\
/odieresis/divide/oslash/ugrave");
      packb("/uacute/ucircumflex/udieresis/yacute/thorn\
/ydieresis");
      packb("256 array astore def cleartomark");
      packb("/encodefont {findfont dup maxlength dict begin");
      packb("{1 index /FID ne {def} {pop pop} ifelse} forall");
      packb("/Encoding exch def dup");
      packb("/FontName exch def currentdict");
      packb("definefont end} def");
      packb("end");

      packb("%%EndProcSet");
      packb("%%EndProlog");
    }

  (*pages)++;
  sprintf(buffer, "%%%%Page: %d %d", *pages, *pages);
  packb(buffer);

  packb("%%BeginPageSetup");
  packb("GKS_dict begin save /psl exch def");

  if (landscape)
    {
      if (gkss->version < 5)
        sprintf(buffer, "%d %d translate -90 rotate", LLX, p->ytrans);
      else
        sprintf(buffer, "90 rotate %d %d translate", LLX, p->ytrans);
      packb(buffer);
    }
  else
    {
      sprintf(buffer, "%d %d translate", LLX, LLY);
      packb(buffer);
    }
  if (fabs(p->magstep) > FEPS)
    {
      sprintf(buffer, "%.4g 1 in 600 div mul dup scale", pow(1.2, p->magstep));
      packb(buffer);
    }
  else
    packb("1 in 600 div dup scale");

  set_color(-1, p->wtype);
  set_foreground(-1, p->wtype);
  packb("0 setlinecap 1 setlinejoin");
  set_linewidth(-1.0);
  set_markersize(-1.0);
  packb("0 ma");
  set_font(-1);
  set_clip(p->window);
  packb("%%EndPageSetup");
  update();
}

static void end_page(int pages)
{
  char buffer[30];

  sprintf(buffer, "%%%%EndPage: %d %d", pages, pages);
  packb(buffer);
}


static void set_colortable(void)
{
  int i;

  for (i = 0; i < MAX_COLOR; i++) gks_inq_rgb(i, p->red + i, p->green + i, p->blue + i);
  p->color = -1;
}

static void set_color_rep(int color, double red, double green, double blue)
{
  if (color >= 0 && color < MAX_COLOR)
    {
      p->red[color] = red;
      p->green[color] = green;
      p->blue[color] = blue;
      p->color = -1;
    }
}

static void query_color(int index, unsigned char **buf, int wtype)
{
  double grey;

  index = FIX_COLORIND(index);

  if (wtype % 2)
    {
      grey = 0.3 * p->red[index] + 0.59 * p->green[index] + 0.11 * p->blue[index];
      **buf = (char)NINT(grey * 255);
      (*buf)++;
    }
  else
    {
      **buf = (char)NINT(p->red[index] * 255);
      (*buf)++;
      **buf = (char)NINT(p->green[index] * 255);
      (*buf)++;
      **buf = (char)NINT(p->blue[index] * 255);
      (*buf)++;
    }
}

static void rgb2color(int rgb, unsigned char **buf, int wtype)
{
  int r, g, b;
  double grey;

  r = (rgb & 0xff);
  g = (rgb & 0xff00) >> 8;
  b = (rgb & 0xff0000) >> 16;

  if (wtype % 2)
    {
      grey = 0.3 * r / 255.0 + 0.59 * g / 255.0 + 0.11 * b / 255.0;
      **buf = (char)NINT(grey * 255);
      (*buf)++;
    }
  else
    {
      **buf = (char)NINT(r);
      (*buf)++;
      **buf = (char)NINT(g);
      (*buf)++;
      **buf = (char)NINT(b);
      (*buf)++;
    }
}

static void set_connection(int conid, int wtype)
{
  p->conid = conid;
  p->wtype = wtype;

  p->window[0] = 0;
  p->window[1] = 1;
  p->window[2] = 0;
  p->window[3] = 1;

  p->viewpt[0] = 0;
  p->viewpt[1] = 0.19685;
  p->viewpt[2] = 0;
  p->viewpt[3] = p->viewpt[1];

  set_xform(p->window, p->viewpt);

  p->pages = 0;
  p->init = 0;
  p->empty = 1;
  p->color = 1;
  p->len = p->column = p->saved_len = p->saved_column = 0;
  p->font = 0;
  p->ltype = GKS_K_LINETYPE_SOLID;
  p->cwidth = p->csize = p->cangle = p->cheight = 0.0;
}

static void marker_routine(double x, double y, int marker)
{
  double dx, dy;
  char buffer[50];
  static const char *macro[] = {"nom", " hl", " vl", "st8", "st7", "st6", "st5", "st4", "ed8", "ed7",
                                "ed6", "ed5", "fpl", "npl", "ftl", "ftr", "tud", "fst", " st", "fdm",
                                "ndm", "fhg", "nhg", "fbt", "nbt", "fsq", "nsq", "ftd", "ntd", "ftu",
                                "ntu", "fci", " dt", " dt", " pl", "fas", "nci", " dc"};

  NDC_to_DC(x, y, dx, dy);

  p->ix = NINT(dx);
  p->iy = NINT(dy);
  sprintf(buffer, "%d %d %s", p->ix, p->iy, macro[marker + 32]);
  packb(buffer);
}

static void cell_array(double xmin, double xmax, double ymin, double ymax, int dx, int dy, int dimx, int *colia,
                       int wtype, int true_color)
{
  char buffer[100];
  unsigned char *buf, *bufP;
  double x1, x2, y1, y2;
  int w, h, x, y;

  int i, j, ci, len, swap = 0;
  int tnr;

  tnr = gkss->cntnr;

  WC_to_NDC(xmin, ymax, tnr, x1, y1);
  seg_xform(&x1, &y1);
  NDC_to_DC(x1, y1, x1, y1);

  WC_to_NDC(xmax, ymin, tnr, x2, y2);
  seg_xform(&x2, &y2);
  NDC_to_DC(x2, y2, x2, y2);

  w = (int)fabs(x2 - x1);
  h = (int)fabs(y2 - y1);
  if (w == 0 || h == 0) return;
  x = (int)MIN(x1, x2);
  y = (int)MIN(y1, y2);

  packb("gsave");

  set_clip(gkss->viewport[gkss->clip == GKS_K_CLIP ? tnr : 0]);

  packb("/RawData currentfile /ASCII85Decode filter def");
  packb("/Data RawData << >> /LZWDecode filter def");

  sprintf(buffer, "%d %d translate", x, y);
  packb(buffer);

  sprintf(buffer, "%d %d scale", w, h);
  packb(buffer);

  sprintf(buffer, "/Device%s setcolorspace", wtype % 2 == 0 ? "RGB" : "Gray");
  packb(buffer);

  if (x1 > x2) swap = 1;
  if (y1 > y2) swap += 2;

  packb("{ << /ImageType 1");

  sprintf(buffer, "/Width %d /Height %d", dx, dy);
  packb(buffer);
  if (swap == 0)
    sprintf(buffer, "/ImageMatrix [%d 0 0 -%d 0 %d]", dx, dy, dy);
  else if (swap == 1)
    sprintf(buffer, "/ImageMatrix [-%d 0 0 -%d %d %d]", dx, dy, dx, dy);
  else if (swap == 2)
    sprintf(buffer, "/ImageMatrix [%d 0 0 %d 0 0]", dx, dy);
  else
    sprintf(buffer, "/ImageMatrix [-%d 0 %d %d 0 0]", dx, dx, dy);
  packb(buffer);

  sprintf(buffer, "/DataSource Data /BitsPerComponent 8 /Decode [0 1%s]", wtype % 2 == 0 ? " 0 1 0 1" : "");
  packb(buffer);

  packb(">> image Data closefile RawData flushfile } exec");

  len = dx * dy;
  if (wtype % 2 == 0) len = len * 3;

  buf = (unsigned char *)malloc(len);
  bufP = buf;
  for (j = 0; j < dy; j++)
    {
      for (i = 0; i < dx; i++)
        {
          ci = colia[j * dimx + i];
          if (!true_color)
            query_color(ci, &bufP, wtype);
          else
            rgb2color(ci, &bufP, wtype);
        }
    }
  LZWEncodeImage(len, buf);
  free(buf);

  packb("grestore");
}

static void text_routine(double *x, double *y, int nchars, char *chars)
{
  int i, j;
  double ux, uy, yrel, angle, phi;
  double xorg, yorg;
  int alh, alv, ic;
  char str[500], buffer[510];
  int prec;
  char *latin1_str = gks_malloc(nchars + 1);

  gks_utf82latin1(chars, latin1_str);
  chars = latin1_str;
  nchars = strlen(chars);

  NDC_to_DC(*x, *y, xorg, yorg);

  prec = gkss->asf[6] ? gkss->txprec : predef_prec[gkss->tindex - 1];
  alh = gkss->txal[0];
  alv = gkss->txal[1];

  WC_to_NDC_rel(gkss->chup[0], gkss->chup[1], gkss->cntnr, ux, uy);
  seg_xform_rel(&ux, &uy);
  angle = -atan2(ux, uy) * 180.0 / M_PI;

  if (prec == GKS_K_TEXT_PRECISION_STRING)
    {
      phi = angle / 180.0 * M_PI;
      yrel = p->ysize * yfac[alv - GKS_K_TEXT_VALIGN_NORMAL];
      xorg -= yrel * sin(phi);
      yorg += yrel * cos(phi);
    }

  if (fabs(angle) > FEPS)
    amoveto(angle, xorg, yorg);
  else
    moveto(xorg, yorg);

  for (i = 0, j = 0; i < nchars; i++)
    {
      ic = chars[i];
      if (ic < 0) ic += 256;
      if (ic < 127)
        {
          if (strchr("()\\", ic) != NULL) str[j++] = '\\';
          str[j++] = chars[i];
        }
      else
        {
          sprintf(str + j, "\\%03o", ic);
          j += 4;
        }
      str[j] = '\0';
    }
  sprintf(buffer, "(%s) %s", str, show[alh + GKS_K_TEXT_HALIGN_NORMAL]);
  packb(buffer);
  if (fabs(angle) > FEPS) packb("gr");

  gks_free(latin1_str);
}

static void fill_routine(int n, double *px, double *py, int tnr)
{
  double x, y;
  char buffer[50];
  int i, jx, jy, rx, ry, nan_found = 0;

  packb("gsave");

  set_clip(gkss->viewport[gkss->clip == GKS_K_CLIP ? tnr : 0]);

  WC_to_NDC(px[0], py[0], tnr, x, y);
  NDC_to_DC(x, y, p->ix, p->iy);

  sprintf(buffer, "np %d %d m", p->ix, p->iy);
  packb(buffer);
  p->np = 1;

  for (i = 1; i < n; i++)
    {
      jx = p->ix;
      jy = p->iy;
      WC_to_NDC(px[i], py[i], tnr, x, y);
      NDC_to_DC(x, y, p->ix, p->iy);

      if (i == 1 || p->ix != jx || p->iy != jy)
        {
          rx = p->ix - jx;
          ry = p->iy - jy;
          if (abs(rx) > 1 || abs(ry) > 1)
            {
              if (px[i] != px[i] && py[i] != py[i])
                {
                  nan_found = 1;
                  continue;
                }
              if (nan_found)
                {
                  sprintf(buffer, "%d %d m", p->ix, p->iy);
                  nan_found = 0;
                }
              else
                {
                  sprintf(buffer, "%d %d rl", rx, ry);
                }
              packb(buffer);
            }
          else
            packb(dc[rx + 1][ry + 1]);
          p->np++;
        }
    }

  if (p->np > 2) packb("fi");

  packb("grestore");
}

static void fillpattern_routine(int n, double *px, double *py, int tnr, int pattern)
{
  char buffer[100];

  sprintf(buffer, "gs [/Pattern /Device%s] setcolorspace %.4g %.4g %.4g pat%d setcolor",
          p->wtype % 2 == 0 ? "RGB" : "Gray", p->red[p->color], p->green[p->color], p->blue[p->color], pattern);
  packb(buffer);

  fill_routine(n, px, py, tnr);
  packb("gr");
}

static void line_routine(int n, double *px, double *py, int ltype, int tnr)
{
  p->limit = 1000;
  gks_emul_polyline(n, px, py, ltype, tnr, move, draw);
  if (p->stroke)
    {
      packb("sk");
      p->stroke = 0;
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

static void draw_path(int n, double *px, double *py, int nc, int *codes)
{
  char buffer[100];
  int i, j, np;
  double x[3], y[3], w, h, a1, a2;
  double cur_x = 0, cur_y = 0, start_x = 0, start_y = 0;
  double x1, y1, x2, y2;

  sprintf(buffer, "np ");
  packb(buffer);
  np = 0;

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
          start_x = cur_x = x[0];
          start_y = cur_y = y[0];
          to_DC(1, x, y);
          sprintf(buffer, "%s%.2f %.2f m", np ? "np " : "", x[0], y[0]);
          packb(buffer);
          np = 0;
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
          sprintf(buffer, "%.2f %.2f l", x[0], y[0]);
          packb(buffer);
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
          x[2] = cur_x;
          y[2] = cur_y;
          cur_x = x[1];
          cur_y = y[1];
          to_DC(3, x, y);
          x1 = x[2] + (2.0 / 3.0) * (x[0] - x[2]);
          y1 = y[2] + (2.0 / 3.0) * (y[0] - y[2]);
          x2 = x[1] + (2.0 / 3.0) * (x[0] - x[1]);
          y2 = y[1] + (2.0 / 3.0) * (y[0] - y[1]);
          sprintf(buffer, "%.2f %.2f %.2f %.2f %.2f %.2f c", x1, y1, x2, y2, x[1], y[1]);
          packb(buffer);
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
          sprintf(buffer, "%.2f %.2f %.2f %.2f %.2f %.2f c", x[0], y[0], x[1], y[1], x[2], y[2]);
          packb(buffer);
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
            x[0] = cx;
            y[0] = cy;
            x[1] = cx + rx;
            y[1] = cy + ry;
            cur_x = cx + rx * cos(a2);
            cur_y = cy + ry * sin(a2);
          }
          to_DC(2, x, y);
          w = x[1] - x[0];
          h = y[1] - y[0];
          a1 *= 180 / M_PI;
          a2 *= 180 / M_PI;
          if (a2 < a1)
            {
              sprintf(buffer, "%.2f %.2f %.2f %.2f %.2f %.2f eln", x[0], y[0], w, h, a1, a2);
            }
          else
            {
              sprintf(buffer, "%.2f %.2f %.2f %.2f %.2f %.2f el", x[0], y[0], w, h, a1, a2);
            }
          packb(buffer);
          j += 3;
          break;
        case 's':
          sprintf(buffer, "cp");
          cur_x = start_x;
          cur_y = start_y;
          packb(buffer);
        case 'S':
          set_linewidth(gkss->bwidth);
          sprintf(buffer, "%.4g %.4g %.4g sc sk", p->red[gkss->bcoli], p->green[gkss->bcoli], p->blue[gkss->bcoli]);
          packb(buffer);
          np = 1;
          break;
        case 'f':
          sprintf(buffer, "%.4g %.4g %.4g sc fi", p->red[gkss->facoli], p->green[gkss->facoli], p->blue[gkss->facoli]);
          cur_x = start_x;
          cur_y = start_y;
          packb(buffer);
          np = 1;
          break;
        case 'F':
          sprintf(buffer, "gs %.4g %.4g %.4g sc fi gr", p->red[gkss->facoli], p->green[gkss->facoli],
                  p->blue[gkss->facoli]);
          packb(buffer);
          set_linewidth(gkss->bwidth);
          sprintf(buffer, "%.4g %.4g %.4g sc csk", p->red[gkss->bcoli], p->green[gkss->bcoli], p->blue[gkss->bcoli]);
          cur_x = start_x;
          cur_y = start_y;
          packb(buffer);
          np = 1;
          break;
        case 'Z':
          sprintf(buffer, "cp");
          cur_x = start_x;
          cur_y = start_y;
          np = 0;
          break;
        case '\0':
          break;
        default:
          gks_perror("invalid path code ('%c')", codes[i]);
          exit(1);
        }
    }
}

static void draw_lines(int n, double *px, double *py, int *attributes)
{
  int i, j = 0, rgba, ln_color = MAX_COLOR;
  double x, y;
  int xim1, yim1, xi, yi;
  double line_width;
  char buffer[50];

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

      line_width = 0.01 * attributes[j++];
      rgba = attributes[j++];
      p->red[ln_color] = (rgba & 0xff) / 255.0;
      p->green[ln_color] = ((rgba >> 8) & 0xff) / 255.0;
      p->blue[ln_color] = ((rgba >> 16) & 0xff) / 255.0;

      packb("np 1 setlinecap");
      set_linewidth(line_width);
      set_color(-ln_color, p->wtype);

      sprintf(buffer, "%d %d m %d %d l sk", xim1, yim1, xi, yi);
      packb(buffer);
    }

  packb("0 setlinecap");
}

static void draw_markers(int n, double *px, double *py, int *attributes)
{
  int i, j = 0, rgba;
  int mk_type, mk_color = MAX_COLOR;
  double mk_size, x, y;

  mk_type = gkss->asf[3] ? gkss->mtype : gkss->mindex;

  for (i = 0; i < n; i++)
    {
      WC_to_NDC(px[i], py[i], gkss->cntnr, x, y);
      seg_xform(&x, &y);

      mk_size = 0.01 * attributes[j++];
      rgba = attributes[j++];
      p->red[mk_color] = (rgba & 0xff) / 255.0;
      p->green[mk_color] = ((rgba >> 8) & 0xff) / 255.0;
      p->blue[mk_color] = ((rgba >> 16) & 0xff) / 255.0;

      set_markersize(mk_size);
      set_foreground(-mk_color, p->wtype);

      marker_routine(x, y, mk_type);
    }
}

static void draw_triangles(int n, double *px, double *py, int ntri, int *tri)
{
  double x, y;
  int i, j, k, rgba, ln_color = MAX_COLOR;
  double tri_x[3], tri_y[3];
  char buffer[200];

  j = 0;
  for (i = 0; i < ntri / 4; ++i)
    {
      for (k = 0; k < 3; ++k)
        {
          WC_to_NDC(px[tri[j] - 1], py[tri[j] - 1], gkss->cntnr, x, y);
          seg_xform(&x, &y);
          NDC_to_DC(x, y, tri_x[k], tri_y[k]);
          j++;
        }

      rgba = tri[j++];
      p->red[ln_color] = (rgba & 0xff) / 255.0;
      p->green[ln_color] = ((rgba >> 8) & 0xff) / 255.0;
      p->blue[ln_color] = ((rgba >> 16) & 0xff) / 255.0;

      packb("np");
      set_linewidth(gkss->lwidth);
      set_color(-ln_color, p->wtype);

      sprintf(buffer, "%.2f %.2f m %.2f %.2f l %.2f %.2f l csk", tri_x[0], tri_y[0], tri_x[1], tri_y[1], tri_x[2],
              tri_y[2]);
      packb(buffer);
    }
}

static void gdp(int n, double *px, double *py, int primid, int nc, int *codes)
{
  switch (primid)
    {
    case GKS_K_GDP_DRAW_PATH:
      draw_path(n, px, py, nc, codes);
      break;
    case GKS_K_GDP_DRAW_LINES:
      draw_lines(n, px, py, codes);
      break;
    case GKS_K_GDP_DRAW_MARKERS:
      draw_markers(n, px, py, codes);
      break;
    case GKS_K_GDP_DRAW_TRIANGLES:
      draw_triangles(n, px, py, nc, codes);
      break;
    default:
      gks_perror("invalid drawing primitive ('%d')", primid);
      exit(1);
    }
}

void gks_drv_ps(int fctid, int dx, int dy, int dimx, int *ia, int lr1, double *r1, int lr2, double *r2, int lc,
                char *chars, void **ptr)
{
  int style, color, pattern, ltype;
  double yres, width, size, x, y, angle;
  int font, tnr, prec;
  int nchars;

  p = (ws_state_list *)*ptr;

  switch (fctid)
    {
      /* open workstation */
    case 2:
      gkss = (gks_state_list_t *)*ptr;

      p = (ws_state_list *)calloc(1, sizeof(struct ws_state_list_t));

      p->size = SIZE_INCREMENT;
      p->buffer = (char *)calloc(1, p->size);

      init_norm_xform();
      set_connection(ia[1], ia[2]);
      set_colortable();

      *ptr = p;
      break;

      /* close workstation */
    case 3:
      if (p->init)
        {
          if (!p->empty) packb("showpage");
          packb("psl restore end % GKS_dict");
          end_page(p->pages);
          packb("%%Trailer");
          packb("GKS_save restore");
        }
      if (p->pages == 0) packb("%%Trailer");
      update();

      ps_header();
      gks_write_file(p->conid, p->buffer, p->len);

      free(p->buffer);
      free(p);
      break;

      /* activate workstation */
    case 4:
      p->state = GKS_K_WS_ACTIVE;
      break;

      /* deactivate workstation */
    case 5:
      p->state = GKS_K_WS_INACTIVE;
      break;

      /* clear workstation */
    case 6:
      if (p->init)
        {
          if (!p->empty)
            {
              packb("showpage");
              p->empty = 1;
            }
          packb("psl restore end % GKS_dict");
          end_page(p->pages);
          p->init = 0;
        }
      break;

      /* update workstation */
    case 8:
      break;

      /* polyline */
    case 12:
      if (p->state == GKS_K_WS_ACTIVE)
        {
          if (!p->init)
            {
              ps_init(&p->pages);
              p->init = 1;
            }
          tnr = gkss->cntnr;
          gks_set_dev_xform(gkss, p->window, p->viewpt);
          ltype = gkss->asf[0] ? gkss->ltype : gkss->lindex;
          width = gkss->asf[1] ? gkss->lwidth : 1;
          color = gkss->asf[2] ? gkss->plcoli : 1;
          if (ltype != GKS_K_LINETYPE_SOLID) set_linetype(ltype, width);
          set_linewidth(width);
          set_color(color, p->wtype);
          if (gkss->clip_tnr != 0)
            {
              packb("gsave");
              set_clip(gkss->viewport[gkss->clip_tnr]);
            }
          line_routine(ia[0], r1, r2, ltype, tnr);
          if (gkss->clip_tnr != 0)
            {
              packb("grestore");
            }
          if (ltype != GKS_K_LINETYPE_SOLID) set_linetype(GKS_K_LINETYPE_SOLID, 1.0);
          p->empty = 0;
        }
      break;

      /* polymarker */
    case 13:
      if (p->state == GKS_K_WS_ACTIVE)
        {
          if (!p->init)
            {
              ps_init(&p->pages);
              p->init = 1;
            }
          gks_set_dev_xform(gkss, p->window, p->viewpt);
          size = gkss->asf[4] ? gkss->mszsc : 1;
          x = 0.0;
          y = 1.0;
          seg_xform_rel(&x, &y);
          size *= sqrt(x * x + y * y);
          set_markersize(size);
          angle = -atan2(x, y) * 180.0 / M_PI;
          set_markerangle(angle);
          set_linewidth(1.0);
          color = gkss->asf[5] ? gkss->pmcoli : 1;
          set_foreground(color, p->wtype);
          if (gkss->clip_tnr != 0)
            {
              packb("gsave");
              set_clip(gkss->viewport[gkss->clip_tnr]);
            }
          gks_emul_polymarker(ia[0], r1, r2, marker_routine);
          if (gkss->clip_tnr != 0)
            {
              packb("grestore");
            }
          p->empty = 0;
        }
      break;

      /* text */
    case 14:
      if (p->state == GKS_K_WS_ACTIVE)
        {
          if (!p->init)
            {
              ps_init(&p->pages);
              p->init = 1;
            }
          tnr = gkss->cntnr;
          gks_set_dev_xform(gkss, p->window, p->viewpt);
          font = gkss->asf[6] ? gkss->txfont : predef_font[gkss->tindex - 1];
          prec = gkss->asf[6] ? gkss->txprec : predef_prec[gkss->tindex - 1];
          if (prec != GKS_K_TEXT_PRECISION_STROKE)
            set_font(font);
          else
            set_linewidth(1.0);
          color = gkss->asf[9] ? gkss->txcoli : 1;
          set_color(color, p->wtype);
          if (gkss->clip_tnr != 0)
            {
              packb("gsave");
              set_clip(gkss->viewport[gkss->clip_tnr]);
            }
          nchars = strlen(chars);
          if (prec == GKS_K_TEXT_PRECISION_STRING)
            {
              double px, py;
              WC_to_NDC(*r1, *r2, tnr, px, py);
              seg_xform(&px, &py);
              text_routine(&px, &py, nchars, chars);
            }
          else
            {
              gks_emul_text(r1[0], r2[0], nchars, chars, line_routine, fill_routine);
            }
          if (gkss->clip_tnr != 0)
            {
              packb("grestore");
            }
          p->empty = 0;
        }
      break;

      /* fill area */
    case 15:
      if (p->state == GKS_K_WS_ACTIVE)
        {
          if (!p->init)
            {
              ps_init(&p->pages);
              p->init = 1;
            }
          tnr = gkss->cntnr;
          gks_set_dev_xform(gkss, p->window, p->viewpt);
          style = gkss->asf[10] ? gkss->ints : predef_ints[gkss->findex - 1];
          color = gkss->asf[12] ? gkss->facoli : 1;
          set_color(color, p->wtype);
          set_linewidth(1.0);
          if (gkss->clip_tnr != 0)
            {
              packb("gsave");
              set_clip(gkss->viewport[gkss->clip_tnr]);
            }
          if (style == GKS_K_INTSTYLE_SOLID)
            fill_routine(ia[0], r1, r2, tnr);
          else if (style == GKS_K_INTSTYLE_PATTERN)
            {
              pattern = gkss->asf[11] ? gkss->styli : predef_styli[gkss->findex - 1];
              fillpattern_routine(ia[0], r1, r2, tnr, pattern);
            }
          else
            {
              yres = 1.0 / 4650.0;
              gks_emul_fillarea(ia[0], r1, r2, tnr, line_routine, yres);
            }
          if (gkss->clip_tnr != 0)
            {
              packb("grestore");
            }
          p->empty = 0;
        }
      break;

      /* cell array */
    case 16:
    case DRAW_IMAGE:
      if (p->state == GKS_K_WS_ACTIVE)
        {
          int true_color = fctid == DRAW_IMAGE;

          if (!p->init)
            {
              ps_init(&p->pages);
              p->init = 1;
            }
          gks_set_dev_xform(gkss, p->window, p->viewpt);
          if (gkss->clip_tnr != 0)
            {
              packb("gsave");
              set_clip(gkss->viewport[gkss->clip_tnr]);
            }
          cell_array(r1[0], r1[1], r2[0], r2[1], dx, dy, dimx, ia, p->wtype, true_color);
          if (gkss->clip_tnr != 0)
            {
              packb("grestore");
            }
          p->empty = 0;
        }
      break;

      /* GDP */
    case 17:
      if (p->state == GKS_K_WS_ACTIVE)
        {
          if (!p->init)
            {
              ps_init(&p->pages);
              p->init = 1;
            }
          gks_set_dev_xform(gkss, p->window, p->viewpt);
          if (gkss->clip_tnr != 0)
            {
              packb("gsave");
              set_clip(gkss->viewport[gkss->clip_tnr]);
            }
          gdp(ia[0], r1, r2, ia[1], ia[2], ia + 3);
          if (gkss->clip_tnr != 0)
            {
              packb("grestore");
            }
          p->empty = 0;
        }
      break;

      /* set color representation */
    case 48:
      set_color_rep(ia[1], r1[0], r1[1], r1[2]);
      break;

    case 49:
      /* set window */
      set_norm_xform(*ia, gkss->window[*ia], gkss->viewport[*ia]);
      break;

    case 50:
      /* set viewport */
      set_norm_xform(*ia, gkss->window[*ia], gkss->viewport[*ia]);
      break;

      /* set workstation window */
    case 54:
      p->window[0] = r1[0];
      p->window[1] = r1[1];
      p->window[2] = r2[0];
      p->window[3] = r2[1];
      set_xform(p->window, p->viewpt);
      init_norm_xform();
      if (p->init) set_clip(p->window);
      break;

      /* set workstation viewport */
    case 55:
      p->viewpt[0] = r1[0];
      p->viewpt[1] = r1[1];
      p->viewpt[2] = r2[0];
      p->viewpt[3] = r2[1];
      set_xform(p->window, p->viewpt);
      init_norm_xform();
      break;

    default:;
    }
}
