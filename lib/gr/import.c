#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#include "gr.h"

#ifdef _MSC_VER
#ifndef NAN
static const unsigned long __nan[2] = {0xffffffff, 0x7fffffff};
#define NAN (*(const float *)__nan)
#endif
#else
#ifndef NAN
#define NAN 0.0 / 0.0
#endif
#endif

#define BUFFSIZE 8192

static char *format[] = {
    "axes:ffffiif",
    "axes3d:ffffffiiif",
    "camerainteraction:ffff",
    "cellarray:ffffiiiiiiI",
    "colorbar:",
    "contour:iiiFFFFi",
    "contourf:iiiFFFFi",
    "destroycontext:i",
    "drawarc:ffffii",
    "drawarrow:ffff",
    "drawimage:ffffiiIi",
    "drawpath:iVBi",
    "drawrect:ffff",
    "fillarc:ffffii",
    "fillarea:iFF",
    "fillrect:ffff",
    "gdp:iFFiiI",
    "grid:ffffii",
    "grid3d:ffffffiii",
    "herrorbars:iFFFF",
    "hexbin:iFFi",
    "mathtex:ffs",
    "polyline:iFF",
    "polyline3d:iFFF",
    "polymarker:iFF",
    "quiver:iiFFFFi",
    "restorestate:",
    "savestate:",
    "selectclipxform:i",
    "selectcontext:i",
    "selntran:i",
    "setarrowsize:f",
    "setarrowstyle:i",
    "setbordercolorind:i",
    "setborderwidth:f",
    "setcharexpan:f",
    "setcharheight:f",
    "setcharspace:f",
    "setcharup:ff",
    "setclip:i",
    "setcolormap:i",
    "setcolorrep:ifff",
    "setfillcolorind:i",
    "setfillintstyle:i",
    "setfillstyle:i",
    "setlinecolorind:i",
    "setlinetype:i",
    "setlinewidth:f",
    "setmarkercolorind:i",
    "setmarkersize:f",
    "setmarkertype:i",
    "setorthographicprojection:ffffff",
    "setperspectiveprojection:fff",
    "setprojectiontype:i",
    "setscale:i",
    "setscalefactors3d: fff",
    "setspace:ffii",
    "settextalign:ii",
    "settextcolorind:i",
    "settextencoding:i",
    "settextfontprec:ii",
    "settextpath:i",
    "settransformationparameters:fffffffff",
    "settransparency:f",
    "setviewport:ffff",
    "setwindow:ffff",
    "setwindow3d:ffffff",
    "setwsviewport:ffff",
    "setwswindow:ffff",
    "shadelines:iFFiii",
    "shadepoints:iFFiii",
    "spline:iFFii",
    "surface:iiFFFi",
    "text:ffs",
    "textext:ffs",
    "titles3d:sss",
    "tricont:iFFFiF",
    "trisurf:iFFF",
    "uselinespec:s",
    "verrorbars:iFFFF",
};

static int nel = sizeof(format) / sizeof(format[0]);

static double f_arg[9], *f_arr[4];

static int i_arg[6], *i_arr, i_arr_size, f_arr_size[4], v_arr_size, b_arr_size;

static vertex_t *v_arr;

static unsigned char *b_arr;

static char *s_arg[3];

static int i_argc, f_argc, s_argc, i_arrc, f_arrp, f_arrc, v_arrc, b_arrc;

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

static int binsearch(char *str[], int nel, char *value)
{
  int position, begin = 0, end = nel - 1, cond = 0;
  char key[50];

  while (begin <= end)
    {
      position = (begin + end) / 2;
      strcpy(key, str[position]);
      strtok(key, ":");
      if ((cond = strcmp(key, value)) == 0)
        return position;
      else if (cond < 0)
        begin = position + 1;
      else
        end = position - 1;
    }
  return nel;
}

static double ascii2double(const char *s)
{
  if (strcmp(s, "nan") == 0 || strcmp(s, "1.#QNAN") == 0)
    return NAN;
  else
    return atof(s);
}

static char *xml(char *s, char *fmt)
{
  char *attr, *p;

  i_argc = i_arrc = 0;
  f_argc = f_arrp = f_arrc = 0;
  v_arrc = 0;
  b_arrc = 0;
  s_argc = 0;

  while (*fmt)
    {
      while (isspace(*s)) s++;
      if (isalpha(*s))
        {
          while (isalnum(*s)) s++;
          if (*s == '=')
            {
              s++;
              if (*s == '"')
                {
                  attr = ++s;
                  while (*s && *s != '"') s++;
                  if (*s == '"')
                    {
                      *s++ = '\0';
                      switch (*fmt)
                        {
                        case 'i':
                          i_arg[i_argc++] = atoi(attr);
                          break;
                        case 'f':
                          f_arg[f_argc++] = atof(attr);
                          break;
                        case 's':
                          s_arg[s_argc++] = attr;
                          break;
                        case 'I':
                          p = strtok(attr, " \t\"");
                          while (p != NULL)
                            {
                              if (i_arrc >= i_arr_size)
                                {
                                  i_arr_size += BUFFSIZE;
                                  i_arr = (int *)xrealloc(i_arr, sizeof(int) * i_arr_size);
                                }
                              i_arr[i_arrc++] = atoi(p);
                              p = strtok(NULL, " \t\"");
                            }
                          break;
                        case 'F':
                          p = strtok(attr, " \t\"");
                          while (p != NULL)
                            {
                              if (f_arrc >= f_arr_size[f_arrp])
                                {
                                  f_arr_size[f_arrp] += BUFFSIZE;
                                  f_arr[f_arrp] =
                                      (double *)xrealloc(f_arr[f_arrp], sizeof(double) * f_arr_size[f_arrp]);
                                }
                              f_arr[f_arrp][f_arrc++] = atof(p);
                              p = strtok(NULL, " \t\"");
                            }
                          f_arrp++;
                          f_arrc = 0;
                          break;
                        case 'V':
                          p = strtok(attr, " \t\"");
                          while (p != NULL)
                            {
                              if (v_arrc >= v_arr_size)
                                {
                                  v_arr_size += BUFFSIZE;
                                  v_arr = (vertex_t *)xrealloc(v_arr, sizeof(vertex_t) * v_arr_size);
                                }
                              v_arr[v_arrc].x = ascii2double(p);
                              p = strtok(NULL, " \t\"");
                              v_arr[v_arrc].y = ascii2double(p);
                              p = strtok(NULL, " \t\"");
                              v_arrc++;
                            }
                          break;
                        case 'B':
                          p = strtok(attr, " \t\"");
                          while (p != NULL)
                            {
                              if (b_arrc >= b_arr_size)
                                {
                                  b_arr_size += BUFFSIZE;
                                  b_arr = (unsigned char *)xrealloc(b_arr, sizeof(unsigned char) * b_arr_size);
                                }
                              b_arr[b_arrc++] = (unsigned char)atoi(p);
                              p = strtok(NULL, " \t\"");
                            }
                          break;
                        }
                    }
                  else
                    fprintf(stderr, "'\"' expected\n");
                }
            }
        }
      fmt++;
    }
  return s;
}

static void gr(int id)
{
  switch (id)
    {
    case 0:
      gr_axes(f_arg[0], f_arg[1], f_arg[2], f_arg[3], i_arg[0], i_arg[1], f_arg[4]);
      break;
    case 1:
      gr_axes3d(f_arg[0], f_arg[1], f_arg[2], f_arg[3], f_arg[4], f_arg[5], i_arg[0], i_arg[1], i_arg[2], f_arg[6]);
      break;
    case 2:
      gr_camerainteraction(f_arg[0], f_arg[1], f_arg[2], f_arg[3]);
      break;
    case 3:
      gr_cellarray(f_arg[0], f_arg[1], f_arg[2], f_arg[3], i_arg[0], i_arg[1], i_arg[2], i_arg[3], i_arg[4], i_arg[5],
                   i_arr);
      break;
    case 4:
      gr_colorbar();
      break;
    case 5:
      gr_contour(i_arg[0], i_arg[1], i_arg[2], f_arr[0], f_arr[1], f_arr[2], f_arr[3], i_arg[3]);
      break;
    case 6:
      gr_contourf(i_arg[0], i_arg[1], i_arg[2], f_arr[0], f_arr[1], f_arr[2], f_arr[3], i_arg[3]);
      break;
    case 7:
      gr_destroycontext(i_arg[0]);
      break;
    case 8:
      gr_drawarc(f_arg[0], f_arg[1], f_arg[2], f_arg[3], i_arg[0], i_arg[1]);
      break;
    case 9:
      gr_drawarrow(f_arg[0], f_arg[1], f_arg[2], f_arg[3]);
      break;
    case 10:
      gr_drawimage(f_arg[0], f_arg[1], f_arg[2], f_arg[3], i_arg[0], i_arg[1], i_arr, i_arg[2]);
      break;
    case 11:
      gr_drawpath(i_arg[0], v_arr, b_arrc != 0 ? b_arr : NULL, i_arg[1]);
      break;
    case 12:
      gr_drawrect(f_arg[0], f_arg[1], f_arg[2], f_arg[3]);
      break;
    case 13:
      gr_fillarc(f_arg[0], f_arg[1], f_arg[2], f_arg[3], i_arg[0], i_arg[1]);
      break;
    case 14:
      gr_fillarea(i_arg[0], f_arr[0], f_arr[1]);
      break;
    case 15:
      gr_fillrect(f_arg[0], f_arg[1], f_arg[2], f_arg[3]);
      break;
    case 16:
      gr_gdp(i_arg[0], f_arr[0], f_arr[1], i_arg[1], i_arg[2], i_arr);
      break;
    case 17:
      gr_grid(f_arg[0], f_arg[1], f_arg[2], f_arg[3], i_arg[0], i_arg[1]);
      break;
    case 18:
      gr_grid3d(f_arg[0], f_arg[1], f_arg[2], f_arg[3], f_arg[4], f_arg[5], i_arg[0], i_arg[1], i_arg[2]);
      break;
    case 19:
      gr_herrorbars(i_arg[0], f_arr[0], f_arr[1], f_arr[2], f_arr[3]);
      break;
    case 20:
      gr_hexbin(i_arg[0], f_arr[0], f_arr[1], i_arg[1]);
      break;
    case 21:
      gr_mathtex(f_arg[0], f_arg[1], s_arg[0]);
      break;
    case 22:
      gr_polyline(i_arg[0], f_arr[0], f_arr[1]);
      break;
    case 23:
      gr_polyline3d(i_arg[0], f_arr[0], f_arr[1], f_arr[2]);
      break;
    case 24:
      gr_polymarker(i_arg[0], f_arr[0], f_arr[1]);
      break;
    case 25:
      gr_quiver(i_arg[0], i_arg[1], f_arr[0], f_arr[1], f_arr[2], f_arr[3], i_arg[2]);
      break;
    case 26:
      gr_restorestate();
      break;
    case 27:
      gr_savestate();
      break;
    case 28:
      gr_selectclipxform(i_arg[0]);
      break;
    case 29:
      gr_selectcontext(i_arg[0]);
      break;
    case 30:
      gr_selntran(i_arg[0]);
      break;
    case 31:
      gr_setarrowsize(f_arg[0]);
      break;
    case 32:
      gr_setarrowstyle(i_arg[0]);
      break;
    case 33:
      gr_setbordercolorind(i_arg[0]);
      break;
    case 34:
      gr_setborderwidth(f_arg[0]);
      break;
    case 35:
      gr_setcharexpan(f_arg[0]);
      break;
    case 36:
      gr_setcharheight(f_arg[0]);
      break;
    case 37:
      gr_setcharspace(f_arg[0]);
      break;
    case 38:
      gr_setcharup(f_arg[0], f_arg[1]);
      break;
    case 39:
      gr_setclip(i_arg[0]);
      break;
    case 40:
      gr_setcolormap(i_arg[0]);
      break;
    case 41:
      gr_setcolorrep(i_arg[0], f_arg[0], f_arg[1], f_arg[2]);
      break;
    case 42:
      gr_setfillcolorind(i_arg[0]);
      break;
    case 43:
      gr_setfillintstyle(i_arg[0]);
      break;
    case 44:
      gr_setfillstyle(i_arg[0]);
      break;
    case 45:
      gr_setlinecolorind(i_arg[0]);
      break;
    case 46:
      gr_setlinetype(i_arg[0]);
      break;
    case 47:
      gr_setlinewidth(f_arg[0]);
      break;
    case 48:
      gr_setmarkercolorind(i_arg[0]);
      break;
    case 49:
      gr_setmarkersize(f_arg[0]);
      break;
    case 50:
      gr_setmarkertype(i_arg[0]);
      break;
    case 51:
      gr_setorthographicprojection(f_arg[0], f_arg[1], f_arg[2], f_arg[3], f_arg[4], f_arg[5]);
      break;
    case 52:
      gr_setperspectiveprojection(f_arg[0], f_arg[1], f_arg[2]);
      break;
    case 53:
      gr_setprojectiontype(i_arg[0]);
      break;
    case 54:
      gr_setscale(i_arg[0]);
      break;
    case 55:
      gr_setscalefactors3d(f_arg[0], f_arg[1], f_arg[2]);
      break;
    case 56:
      gr_setspace(f_arg[0], f_arg[1], i_arg[0], i_arg[1]);
      break;
    case 57:
      gr_settextalign(i_arg[0], i_arg[1]);
      break;
    case 58:
      gr_settextcolorind(i_arg[0]);
      break;
    case 59:
      gr_settextencoding(i_arg[0]);
      break;
    case 60:
      gr_settextfontprec(i_arg[0], i_arg[1]);
      break;
    case 61:
      gr_settextpath(i_arg[0]);
      break;
    case 62:
      gr_settransformationparameters(f_arg[0], f_arg[1], f_arg[2], f_arg[3], f_arg[4], f_arg[5], f_arg[6], f_arg[7],
                                     f_arg[8]);
      break;
    case 63:
      gr_settransparency(f_arg[0]);
      break;
    case 64:
      gr_setviewport(f_arg[0], f_arg[1], f_arg[2], f_arg[3]);
      break;
    case 65:
      gr_setwindow(f_arg[0], f_arg[1], f_arg[2], f_arg[3]);
      break;
    case 66:
      gr_setwindow3d(f_arg[0], f_arg[1], f_arg[2], f_arg[3], f_arg[4], f_arg[5]);
      break;
    case 67:
      gr_setwsviewport(f_arg[0], f_arg[1], f_arg[2], f_arg[3]);
      break;
    case 68:
      gr_setwswindow(f_arg[0], f_arg[1], f_arg[2], f_arg[3]);
      break;
    case 69:
      gr_shadelines(i_arg[0], f_arr[0], f_arr[1], i_arg[1], i_arg[2], i_arg[3]);
      break;
    case 70:
      gr_shadepoints(i_arg[0], f_arr[0], f_arr[1], i_arg[1], i_arg[2], i_arg[3]);
      break;
    case 71:
      gr_spline(i_arg[0], f_arr[0], f_arr[1], i_arg[1], i_arg[2]);
      break;
    case 72:
      gr_surface(i_arg[0], i_arg[1], f_arr[0], f_arr[1], f_arr[2], i_arg[2]);
      break;
    case 73:
      gr_text(f_arg[0], f_arg[1], s_arg[0]);
      break;
    case 74:
      gr_textext(f_arg[0], f_arg[1], s_arg[0]);
      break;
    case 75:
      gr_titles3d(s_arg[0], s_arg[1], s_arg[2]);
      break;
    case 76:
      gr_tricontour(i_arg[0], f_arr[0], f_arr[1], f_arr[2], i_arg[2], f_arr[3]);
      break;
    case 77:
      gr_trisurface(i_arg[0], f_arr[0], f_arr[1], f_arr[2]);
      break;
    case 78:
      gr_uselinespec(s_arg[0]);
      break;
    case 79:
      gr_verrorbars(i_arg[0], f_arr[0], f_arr[1], f_arr[2], f_arr[3]);
      break;
    }
}

int gr_drawgraphics(char *string)
{
  char *s = string, *el, *fmt;
  int i, id;

  i_arr = (int *)xmalloc(sizeof(int) * BUFFSIZE);
  i_arr_size = BUFFSIZE;
  for (i = 0; i < 4; i++)
    {
      f_arr[i] = (double *)xmalloc(sizeof(double) * BUFFSIZE);
      f_arr_size[i] = BUFFSIZE;
    }
  v_arr = (vertex_t *)xmalloc(sizeof(vertex_t) * BUFFSIZE);
  v_arr_size = BUFFSIZE;
  b_arr = (unsigned char *)xmalloc(sizeof(unsigned char) * BUFFSIZE);
  b_arr_size = BUFFSIZE;

  while (*s)
    {
      if (*s == '<')
        {
          el = ++s;
          if (isalpha(*s))
            {
              while (isalnum(*s)) s++;
              *s++ = '\0';
              id = binsearch(format, nel, el);
              if (id < nel)
                {
                  fmt = format[id] + strlen(el) + 1;
                  s = xml(s, fmt);
                  gr(id);
                }
              else if (strcmp(el, "gr") != 0)
                fprintf(stderr, "%s: unknown XML element\n", el);
            }
        }
      while (*s && *s != '\n') s++;
      if (*s == '\n') s++;
    }

  free(b_arr);
  free(v_arr);
  for (i = 0; i < 4; i++) free(f_arr[i]);
  free(i_arr);

  return 0;
}

int gr_importgraphics(char *path)
{
  FILE *stream;
  char *buff;
  int nbytes, off, ret;

  stream = fopen(path, "r");
  if (stream != NULL)
    {
      buff = (char *)xmalloc(BUFSIZ);
      off = 0;
      nbytes = BUFSIZ;
      while ((ret = fread(buff + off, 1, BUFSIZ, stream)) > 0)
        {
          nbytes += BUFSIZ;
          off += ret;
          buff = (char *)xrealloc(buff, nbytes);
        }
      fclose(stream);
      buff[off + ret] = '\0';

      ret = gr_drawgraphics(buff);
      free(buff);
    }
  else
    {
      fprintf(stderr, "%s: can't import GR file\n", path);
      ret = -1;
    }
  return ret;
}
