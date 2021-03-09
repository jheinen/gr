#include <stdlib.h>
#include <math.h>

#include "gks.h"
#include "gkscore.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define POINT_INC 1000

#ifndef NO_FT

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_XFREE86_H
#include FT_OUTLINE_H
#include FT_BBOX_H
#include FT_TRUETYPE_TABLES_H

#define nint(a) ((int)(a + 0.5))

#define WC_to_NDC(xw, yw, tnr, xn, yn)     \
  xn = gkss->a[tnr] * (xw) + gkss->b[tnr]; \
  yn = gkss->c[tnr] * (yw) + gkss->d[tnr]

#define NDC_to_WC(xn, yn, tnr, xw, yw)     \
  xw = ((xn)-gkss->b[tnr]) / gkss->a[tnr]; \
  yw = ((yn)-gkss->d[tnr]) / gkss->c[tnr]

const static FT_String *gks_font_list_pfb[] = {
    "NimbusRomNo9L-Regu", /* 1: Times New Roman */
    "NimbusRomNo9L-ReguItal",
    "NimbusRomNo9L-Medi",
    "NimbusRomNo9L-MediItal",
    "NimbusSanL-Regu", /* 5: Helvetica */
    "NimbusSanL-ReguItal",
    "NimbusSanL-Bold",
    "NimbusSanL-BoldItal",
    "NimbusMonL-Regu", /* 9: Courier */
    "NimbusMonL-ReguObli",
    "NimbusMonL-Bold",
    "NimbusMonL-BoldObli",
    "StandardSymL",     /* 13: Symbol */
    "URWBookmanL-Ligh", /* 14: Bookman Light */
    "URWBookmanL-LighItal",
    "URWBookmanL-DemiBold",
    "URWBookmanL-DemiBoldItal",
    "CenturySchL-Roma", /* 18: New Century Schoolbook Roman */
    "CenturySchL-Ital",
    "CenturySchL-Bold",
    "CenturySchL-BoldItal",
    "URWGothicL-Book", /* 22: Avant Garde Book */
    "URWGothicL-BookObli",
    "URWGothicL-Demi",
    "URWGothicL-DemiObli",
    "URWPalladioL-Roma", /* 26: Palatino */
    "URWPalladioL-Ital",
    "URWPalladioL-Bold",
    "URWPalladioL-BoldItal",
    "URWChanceryL-MediItal", /* 30: Zapf Chancery */
    "Dingbats"               /* 31: Zapf Dingbats */
};

const static FT_String *gks_font_list_ttf[] = {NULL,         NULL,        NULL, NULL, NULL, NULL, NULL, NULL,
                                               NULL,         NULL,        NULL, NULL, NULL, NULL, NULL, NULL,
                                               NULL,         NULL,        NULL, NULL, NULL, NULL, NULL, NULL,
                                               NULL,         NULL,        NULL, NULL, NULL, NULL, NULL, "CMUSerif-Math",
                                               "DejaVuSans", "PingFangSC"};

static FT_Face font_face_cache_pfb[] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                                        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                                        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};

static FT_Face font_face_cache_ttf[] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                                        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                                        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};

/* TODO: Add fallback fonts for non-Latin languages */
static const char *fallback_font_list[] = {NULL};
static int fallback_font_reference_list[] = {232};

static FT_Face fallback_font_faces[] = {NULL};
static const int NUM_FALLBACK_FACES = sizeof(fallback_font_faces) / sizeof(fallback_font_faces[0]);

const static int map[] = {22, 9,  5, 14, 18, 26, 13, 1, 24, 11, 7, 16, 20, 28, 13, 3,
                          23, 10, 6, 15, 19, 27, 13, 2, 25, 12, 8, 17, 21, 29, 13, 4};

const static double caps[] = {0.662, 0.653, 0.676, 0.669, 0.718, 0.718, 0.718, 0.718, 0.562, 0.562, 0.562,
                              0.562, 0.667, 0.681, 0.681, 0.681, 0.681, 0.722, 0.722, 0.722, 0.722, 0.740,
                              0.740, 0.740, 0.740, 0.692, 0.692, 0.681, 0.681, 0.587, 0.692};

static FT_Bool init = 0;
static FT_Library library;

double horiAdvance = 0, vertAdvance = 0;

static int npoints = 0, maxpoints = 0;
static double *xpoint = NULL, *ypoint = NULL;

static int num_opcodes = 0;
static int *opcodes = NULL;

static long pen_x = 0;

static FT_Error set_glyph(FT_Face face, FT_UInt codepoint, FT_UInt *previous, FT_Vector *pen, FT_Bool vertical,
                          FT_Matrix *rotation, FT_Vector *bearing, FT_Int halign, FT_GlyphSlot *glyph_slot_ptr);
static void gks_ft_init_fallback_faces();
static void utf_to_unicode(FT_Bytes str, FT_UInt *unicode_string, FT_UInt *length);
static FT_Long ft_min(FT_Long a, FT_Long b);
static FT_Long ft_max(FT_Long a, FT_Long b);

static FT_Long ft_min(FT_Long a, FT_Long b)
{
  return a > b ? b : a;
}

static FT_Long ft_max(FT_Long a, FT_Long b)
{
  return a > b ? a : b;
}

int gks_ft_bearing_x_direction = -1;

DLLEXPORT void gks_ft_set_bearing_x_direction(int direction)
{
  gks_ft_bearing_x_direction = direction;
}

DLLEXPORT void gks_ft_inq_bearing_x_direction(int *direction)
{
  *direction = gks_ft_bearing_x_direction;
}

/* load a glyph into the slot and compute bearing */
static FT_Error set_glyph(FT_Face face, FT_UInt codepoint, FT_UInt *previous, FT_Vector *pen, FT_Bool vertical,
                          FT_Matrix *rotation, FT_Vector *bearing, FT_Int halign, FT_GlyphSlot *glyph_slot_ptr)
{
  FT_Error error;
  FT_UInt glyph_index;

  glyph_index = FT_Get_Char_Index(face, codepoint);
  if (FT_HAS_KERNING(face) && !FT_IS_FIXED_WIDTH(face) && *previous && !vertical && glyph_index)
    {
      FT_Vector delta;
      FT_Get_Kerning(face, *previous, glyph_index, FT_KERNING_UNFITTED, &delta);
      FT_Vector_Transform(&delta, rotation);
      pen->x += delta.x;
      pen->y += delta.y;
    }
  *previous = glyph_index;

  if (!glyph_index)
    {
      int i;
      for (i = 0; i < NUM_FALLBACK_FACES; i++)
        {
          if (!fallback_font_faces[i])
            {
              continue;
            }
          glyph_index = FT_Get_Char_Index(fallback_font_faces[i], codepoint);
          if (glyph_index != 0)
            {
              face = fallback_font_faces[i];
              break;
            }
        }
    }

  if (!glyph_index)
    {
      gks_perror("glyph missing from current font: %d", codepoint);
    }
  error = FT_Load_Glyph(face, glyph_index, vertical ? FT_LOAD_VERTICAL_LAYOUT : FT_LOAD_DEFAULT);
  if (error)
    {
      gks_perror("glyph could not be loaded: %d", codepoint);
      return 1;
    }
  *glyph_slot_ptr = face->glyph;

  error = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);
  if (error)
    {
      gks_perror("glyph could not be rendered: %c", codepoint);
      return 1;
    }

  bearing->x = FT_IS_FIXED_WIDTH(face) ? 0 : face->glyph->metrics.horiBearingX;
  bearing->y = 0;
  if (vertical)
    {
      if (halign == GKS_K_TEXT_HALIGN_RIGHT)
        {
          bearing->x += face->glyph->metrics.width;
        }
      else if (halign == GKS_K_TEXT_HALIGN_CENTER)
        {
          bearing->x += face->glyph->metrics.width / 2;
        }
      if (bearing->x != 0) FT_Vector_Transform(bearing, rotation);
      bearing->x = 64 * face->glyph->bitmap_left - bearing->x;
      bearing->y = 64 * face->glyph->bitmap_top - bearing->y;
    }
  else
    {
      if (bearing->x != 0) FT_Vector_Transform(bearing, rotation);
      pen->x += gks_ft_bearing_x_direction * bearing->x;
      pen->y -= bearing->y;
      bearing->x = 64 * face->glyph->bitmap_left;
      bearing->y = 64 * face->glyph->bitmap_top;
    }
  return 0;
}


static void symbol_to_unicode(FT_Bytes str, FT_UInt *unicode_string, int length)
{
  static int symbol2utf[256] = {
      0,    1,    2,    3,    4,    5,    6,    7,     8,    9,     10,   11,   12,   13,   14,   15,   16,   17,
      18,   18,   20,   21,   22,   23,   24,   25,    26,   27,    28,   29,   30,   31,   32,   33,   8704, 35,
      8707, 37,   38,   8715, 40,   41,   42,   43,    44,   45,    46,   47,   48,   49,   50,   51,   52,   53,
      54,   55,   56,   57,   58,   59,   60,   61,    62,   63,    8773, 913,  914,  935,  916,  917,  934,  915,
      919,  921,  977,  922,  923,  924,  925,  927,   928,  920,   929,  931,  932,  933,  962,  937,  926,  936,
      918,  91,   8756, 93,   8869, 95,   8254, 945,   946,  967,   948,  949,  966,  947,  951,  953,  981,  954,
      955,  956,  957,  959,  960,  952,  961,  963,   964,  965,   982,  969,  958,  968,  950,  123,  124,  125,
      126,  127,  128,  129,  130,  131,  132,  133,   134,  135,   136,  137,  138,  139,  140,  141,  142,  143,
      144,  145,  146,  147,  148,  149,  150,  151,   152,  153,   154,  155,  156,  157,  158,  159,  160,  978,
      8242, 8804, 8260, 8734, 402,  9827, 9830, 9829,  9824, 8596,  8592, 8593, 8594, 8595, 176,  177,  8243, 8805,
      215,  8733, 8706, 8226, 247,  8800, 8801, 8776,  8230, 9116,  9135, 8629, 8501, 8465, 8476, 8472, 8855, 8853,
      8709, 8745, 8746, 8835, 8839, 8836, 8834, 8838,  8712, 8713,  8736, 8711, 174,  169,  8482, 8719, 8730, 183,
      172,  8743, 8744, 8660, 8656, 8657, 8658, 8659,  9674, 12296, 174,  169,  8482, 8721, 9115, 9116, 9117, 9121,
      9116, 9123, 9127, 9128, 9129, 9116, 240,  12297, 8747, 9127,  9116, 9133, 9131, 9130, 9120, 9124, 9130, 9126,
      9131, 9132, 9133, 255};
  int i;

  for (i = 0; i < length; i++) unicode_string[i] = symbol2utf[str[i]];
}

/* set text string, converting UTF-8 into Unicode */
static void utf_to_unicode(FT_Bytes str, FT_UInt *unicode_string, FT_UInt *length)
{
  FT_UInt num_glyphs = 0;
  FT_UInt codepoint;
  FT_Byte following_bytes;
  FT_Byte offset;
  int i, j;

  for (i = 0; i < *length; i++)
    {
      if (str[i] < 128)
        {
          offset = 0;
          following_bytes = 0;
        }
      else if (str[i] < 128 + 64 + 32)
        {
          offset = 128 + 64;
          following_bytes = 1;
        }
      else if (str[i] < 128 + 64 + 32 + 16)
        {
          offset = 128 + 64 + 32;
          following_bytes = 2;
        }
      else if (str[i] < 128 + 64 + 32 + 16 + 8)
        {
          offset = 128 + 64 + 32 + 16;
          following_bytes = 3;
        }
      else
        {
          gks_perror("character ignored due to unicode error");
          continue;
        }
      codepoint = str[i] - offset;
      for (j = 0; j < following_bytes; j++)
        {
          codepoint = codepoint * 64;
          i++;
          if (str[i] < 128 || str[i] >= 128 + 64)
            {
              gks_perror("character ignored due to unicode error");
              continue;
            }
          codepoint += str[i] - 128;
        }
      unicode_string[num_glyphs] = codepoint;
      num_glyphs++;
    }
  unicode_string[num_glyphs] = '\0';

  *length = num_glyphs;
}

int gks_ft_init(void)
{
  FT_Error error;
  if (init) return 0;
  error = FT_Init_FreeType(&library);
  if (error)
    {
      gks_perror("could not initialize freetype library");
      return error;
    }
  init = 1;

  gks_ft_init_fallback_faces();
  return error;
}

/* deallocate memory */
void gks_ft_terminate(void)
{
  if (init)
    {
      FT_Done_FreeType(library);
    }
  init = 0;
}

static int gks_ft_convert_textfont(int textfont)
{
  textfont = abs(textfont);
  if (textfont >= 201 && textfont <= 234)
    {
      textfont -= 200;
    }
  else if (textfont >= 101 && textfont <= 131)
    {
      textfont -= 100;
    }
  else if (textfont > 1 && textfont <= 32)
    {
      textfont = map[textfont - 1];
    }
  else
    {
      textfont = 9;
    }
  textfont = textfont - 1;
  return textfont;
}

static char *gks_ft_get_font_path(const char *font_name, const char *font_file_extension)
{
  const char *prefix;
  char *font_path;

  prefix = gks_getenv("GKS_FONTPATH");
  if (prefix == NULL)
    {
      prefix = gks_getenv("GRDIR");
    }
  if (prefix == NULL)
    {
      prefix = GRDIR;
    }

  font_path = (char *)gks_malloc(strlen(prefix) + 7 + strlen(font_name) + strlen(font_file_extension) + 1);
  strcpy(font_path, prefix);
#ifdef _WIN32
  strcat(font_path, "\\FONTS\\");
#else
  strcat(font_path, "/fonts/");
#endif
  strcat(font_path, font_name);
  strcat(font_path, font_file_extension);
  return font_path;
}

static void gks_ft_init_fallback_faces()
{
  FT_Error error;
  int i;
  for (i = 0; i < NUM_FALLBACK_FACES; i++)
    {
      if (!init) gks_ft_init();

      if (fallback_font_faces[i] == NULL)
        {
          if (fallback_font_list[i] == NULL)
            {
              /* fallback font reuses an existing, regular font face */
              fallback_font_faces[i] = gks_ft_get_face(fallback_font_reference_list[i]);
            }
          else
            {
              char *file = gks_ft_get_font_path(fallback_font_list[i], "");
              error = FT_New_Face(library, file, 0, &fallback_font_faces[i]);
              gks_free(file);
              if (error == FT_Err_Unknown_File_Format)
                {
                  gks_perror("unknown file format: %s", file);
                  fallback_font_faces[i] = NULL;
                }
              else if (error)
                {
                  gks_perror("could not open font file: %s", file);
                  fallback_font_faces[i] = NULL;
                }
            }
        }
    }
}


void *gks_ft_get_face(int textfont)
{
  FT_Error error;
  FT_Face face;
  const FT_String *font;

  if (!init) gks_ft_init();
  int use_ttf = (textfont >= 200);
  int original_textfont = textfont;
  textfont = gks_ft_convert_textfont(textfont);

  const FT_String **font_list = use_ttf ? gks_font_list_ttf : gks_font_list_pfb;
  FT_Face *font_face_cache = use_ttf ? font_face_cache_ttf : font_face_cache_pfb;

  font = font_list[textfont];

  if (font == NULL)
    {
      gks_perror("Missing font: %d\n", original_textfont);
      return NULL;
    }

  if (font_face_cache[textfont] == NULL)
    {
      char *file = gks_ft_get_font_path(font, (use_ttf ? ".ttf" : ".pfb"));
      error = FT_New_Face(library, file, 0, &face);
      gks_free(file);
      if (error == FT_Err_Unknown_File_Format)
        {
          gks_perror("unknown file format: %s", file);
          return NULL;
        }
      else if (error)
        {
          gks_perror("could not open font file: %s", file);
          return NULL;
        }
      if (strcmp(FT_Get_X11_Font_Format(face), "Type 1") == 0)
        {
          char *file = gks_ft_get_font_path(font, ".afm");
          FT_Attach_File(face, file);
          gks_free(file);
        }
      font_face_cache[textfont] = face;
    }
  else
    {
      face = font_face_cache[textfont];
    }
  return (void *)face;
}


int gks_ft_get_metrics(int font, double fontsize, unsigned int codepoint, unsigned int dpi, double *width,
                       double *height, double *depth, double *advance, double *bearing, double *xmin, double *xmax,
                       double *ymin, double *ymax)
{
  FT_Face face;
  FT_Error error;
  int hinting_factor = 8;
  int i;

  gks_ft_init();

  for (i = -1; i < NUM_FALLBACK_FACES; i++)
    {
      if (i < 0)
        {
          face = (FT_Face)gks_ft_get_face(font);
        }
      else
        {
          face = fallback_font_faces[i];
        }
      if (!face)
        {
          continue;
        }
      error = FT_Set_Char_Size(face, (long)(fontsize * 64), 0, (dpi * hinting_factor), dpi);
      if (error)
        {
          continue;
        }

      FT_Set_Transform(face, NULL, NULL);

      FT_UInt glyph_index;

      glyph_index = FT_Get_Char_Index(face, codepoint);
      if (!glyph_index)
        {
          continue;
        }
      error = FT_Load_Glyph(face, glyph_index, 2);

      if (error)
        {
          continue;
        }
      FT_Glyph newglyph;
      error = FT_Get_Glyph(face->glyph, &newglyph);
      if (error)
        {
          continue;
        }
      if (width)
        {
          *width = face->glyph->metrics.width / hinting_factor / 64.0;
        }
      if (height)
        {
          *height = face->glyph->metrics.horiBearingY / 64.0;
        }
      if (depth)
        {
          *depth = face->glyph->metrics.height / 64.0 - *height;
        }
      if (advance)
        {
          *advance = face->glyph->linearHoriAdvance / hinting_factor / 65536.0;
        }
      if (bearing)
        {
          *bearing = face->glyph->metrics.horiBearingX / hinting_factor / 64.0;
        }

      FT_BBox cbox;
      FT_Glyph_Get_CBox(newglyph, FT_GLYPH_BBOX_SUBPIXELS, &cbox);
      if (xmin)
        {
          *xmin = cbox.xMin / 64.0 / hinting_factor;
        }
      if (xmax)
        {
          *xmax = cbox.xMax / 64.0 / hinting_factor;
        }
      if (ymin)
        {
          *ymin = cbox.yMin / 64.0;
        }
      if (ymax)
        {
          *ymax = cbox.yMax / 64.0;
        }
      FT_Done_Glyph(newglyph);
      return 1;
    }
  return 0;
}

double gks_ft_get_kerning(int font, double fontsize, unsigned int dpi, unsigned int first_codepoint,
                          unsigned int second_codepoint)
{
  FT_Face face;
  FT_Error error;
  int hinting_factor = 8;
  int i;
  FT_UInt first_glyph_index;
  FT_UInt second_glyph_index;

  gks_ft_init();

  for (i = -1; i < NUM_FALLBACK_FACES; i++)
    {
      if (i < 0)
        {
          face = (FT_Face)gks_ft_get_face(font);
        }
      else
        {
          face = fallback_font_faces[i];
        }
      if (!face)
        {
          continue;
        }
      error = FT_Set_Char_Size(face, (long)(fontsize * 64), 0, (dpi * hinting_factor), dpi);
      if (error)
        {
          continue;
        }

      FT_Set_Transform(face, NULL, NULL);

      first_glyph_index = FT_Get_Char_Index(face, first_codepoint);
      if (!first_glyph_index)
        {
          continue;
        }

      second_glyph_index = FT_Get_Char_Index(face, second_codepoint);
      if (!second_glyph_index)
        {
          return 0.0;
        }
      FT_Vector kerning;
      FT_Get_Kerning(face, first_glyph_index, second_glyph_index, FT_KERNING_DEFAULT, &kerning);
      return kerning.x / 64.0 / hinting_factor;
    }
  return 0.0;
}


unsigned char *gks_ft_get_bitmap(int *x, int *y, int *width, int *height, gks_state_list_t *gkss, const char *text,
                                 int length)
{
  FT_Face face;                /* font face */
  FT_GlyphSlot glyph_slot;     /* glyph slot (might be in a fallback face) */
  FT_Vector pen;               /* glyph position */
  FT_BBox bb;                  /* bounding box */
  FT_Vector bearing;           /* individual glyph translation */
  FT_UInt previous;            /* previous glyph index */
  FT_Vector spacing;           /* amount of additional space between glyphs */
  FT_ULong textheight;         /* textheight in FreeType convention */
  FT_Error error;              /* error code */
  FT_Matrix rotation;          /* text rotation matrix */
  FT_UInt size;                /* number of pixels of the bitmap */
  FT_UInt *unicode_string;     /* unicode text string */
  FT_Int halign, valign;       /* alignment */
  FT_Byte *mono_bitmap = NULL; /* target for rendered text */
  FT_UInt num_glyphs;          /* number of glyphs */
  FT_Vector anchor;
  FT_Vector up;
  FT_Bitmap ftbitmap;
  FT_UInt codepoint;
  int i, textfont, dx, dy, value, pos_x, pos_y;
  unsigned int j, k;
  const int direction = (gkss->txp <= 3 && gkss->txp >= 0 ? gkss->txp : 0);
  const FT_Bool vertical = (direction == GKS_K_TEXT_PATH_DOWN || direction == GKS_K_TEXT_PATH_UP);

  if (!init) gks_ft_init();

  halign = gkss->txal[0];
  if (halign < 0 || halign > 3)
    {
      gks_perror("Invalid horizontal alignment");
      halign = GKS_K_TEXT_HALIGN_NORMAL;
    }
  if (halign == GKS_K_TEXT_HALIGN_NORMAL)
    {
      if (vertical)
        {
          halign = GKS_K_TEXT_HALIGN_CENTER;
        }
      else if (direction == GKS_K_TEXT_PATH_LEFT)
        {
          halign = GKS_K_TEXT_HALIGN_RIGHT;
        }
      else
        {
          halign = GKS_K_TEXT_HALIGN_LEFT;
        }
    }

  valign = gkss->txal[1];
  if (valign < 0 || valign > 5)
    {
      gks_perror("Invalid vertical alignment");
      valign = GKS_K_TEXT_VALIGN_NORMAL;
    }
  if (valign == GKS_K_TEXT_VALIGN_NORMAL)
    {
      valign = GKS_K_TEXT_VALIGN_BASE;
    }

  face = (FT_Face)gks_ft_get_face(gkss->txfont);
  if (!face)
    {
      return NULL;
    }
  textfont = gks_ft_convert_textfont(gkss->txfont);

  textheight = nint(gkss->chh * *width * 64 / caps[textfont]);
  error = FT_Set_Char_Size(face, nint(textheight * gkss->chxp), textheight, 72, 72);
  if (error) gks_perror("cannot set text height");
  for (i = 0; i < NUM_FALLBACK_FACES; i++)
    {
      if (!fallback_font_faces[i])
        {
          continue;
        }
      error = FT_Set_Char_Size(fallback_font_faces[i], nint(textheight * gkss->chxp), textheight, 72, 72);
      if (error) gks_perror("cannot set text height");
    }

  if (gkss->chup[0] != 0.0 || gkss->chup[1] != 0.0)
    {
      double s = -gkss->chup[0];
      double c = gkss->chup[1];
      double f = sqrt(s * s + c * c);
      s /= f;
      c /= f;
      rotation.xx = nint(c * 0x10000L);
      rotation.xy = nint(-s * 0x10000L);
      rotation.yx = nint(s * 0x10000L);
      rotation.yy = nint(c * 0x10000L);
      FT_Set_Transform(face, &rotation, NULL);
      for (i = 0; i < NUM_FALLBACK_FACES; i++)
        {
          if (!fallback_font_faces[i])
            {
              continue;
            }
          FT_Set_Transform(fallback_font_faces[i], &rotation, NULL);
        }
    }
  else
    {
      FT_Set_Transform(face, NULL, NULL);
      for (i = 0; i < NUM_FALLBACK_FACES; i++)
        {
          if (!fallback_font_faces[i])
            {
              continue;
            }
          FT_Set_Transform(fallback_font_faces[i], NULL, NULL);
        }
    }

  spacing.x = spacing.y = 0;
  if (gkss->chsp != 0.0)
    {
      error = FT_Load_Glyph(face, FT_Get_Char_Index(face, ' '), vertical ? FT_LOAD_VERTICAL_LAYOUT : FT_LOAD_DEFAULT);
      if (!error)
        {
          spacing.x = nint(face->glyph->advance.x * gkss->chsp);
          spacing.y = nint(face->glyph->advance.y * gkss->chsp);
        }
      else
        {
          gks_perror("cannot apply character spacing");
        }
    }

  num_glyphs = length;
  unicode_string = (FT_UInt *)gks_malloc((length + 1) * sizeof(FT_UInt));
  if (textfont + 1 == 13)
    {
      symbol_to_unicode((FT_Bytes)text, unicode_string, num_glyphs);
    }
  else
    {
      utf_to_unicode((FT_Bytes)text, unicode_string, &num_glyphs);
    }

  if (direction == GKS_K_TEXT_PATH_LEFT)
    {
      int i;
      for (i = 0; i < num_glyphs - 1 - i; i++)
        {
          int tmp = unicode_string[i];
          unicode_string[i] = unicode_string[num_glyphs - 1 - i];
          unicode_string[num_glyphs - 1 - i] = tmp;
        }
    }

  bb.xMin = bb.yMin = LONG_MAX;
  bb.xMax = bb.yMax = LONG_MIN;
  pen.x = 0;
  pen.y = 0;
  previous = 0;

  for (i = 0; i < num_glyphs; i++)
    {
      codepoint = unicode_string[i];

      error = set_glyph(face, codepoint, &previous, &pen, vertical, &rotation, &bearing, halign, &glyph_slot);
      if (error) continue;

      bb.xMin = ft_min(bb.xMin, pen.x + bearing.x);
      bb.xMax = ft_max(bb.xMax, pen.x + bearing.x + 64 * glyph_slot->bitmap.width);
      bb.yMin = ft_min(bb.yMin, pen.y + bearing.y - 64 * glyph_slot->bitmap.rows);
      bb.yMax = ft_max(bb.yMax, pen.y + bearing.y);

      if (direction == GKS_K_TEXT_PATH_DOWN)
        {
          pen.x -= glyph_slot->advance.x + spacing.x;
          pen.y -= glyph_slot->advance.y + spacing.y;
        }
      else
        {
          pen.x += glyph_slot->advance.x + spacing.x;
          pen.y += glyph_slot->advance.y + spacing.y;
        }
    }

  if (halign == GKS_K_TEXT_HALIGN_LEFT)
    {
      anchor.x = 0;
      anchor.y = 0;
    }
  else if (halign == GKS_K_TEXT_HALIGN_CENTER)
    {
      anchor.x = nint(pen.x * 0.5);
      anchor.y = nint(pen.y * 0.5);
    }
  else if (halign == GKS_K_TEXT_HALIGN_RIGHT)
    {
      anchor.x = pen.x;
      anchor.y = pen.y;
    }

  up.x = 0;
  up.y = nint(gkss->chh * *width * 64);
  FT_Vector_Transform(&up, &rotation);

  if (valign == GKS_K_TEXT_VALIGN_BOTTOM)
    {
      anchor.x += nint(-0.2 * up.x);
      anchor.y += nint(-0.2 * up.y);
    }
  else if (valign == GKS_K_TEXT_VALIGN_BASE)
    {
      anchor.x += 0;
      anchor.y += 0;
    }
  else if (valign == GKS_K_TEXT_VALIGN_HALF)
    {
      anchor.x += nint(0.5 * up.x);
      anchor.y += nint(0.5 * up.y);
    }
  else if (valign == GKS_K_TEXT_VALIGN_CAP)
    {
      anchor.x += nint(1.0 * up.x);
      anchor.y += nint(1.0 * up.y);
    }
  else if (valign == GKS_K_TEXT_VALIGN_TOP)
    {
      anchor.x += nint(1.2 * up.x);
      anchor.y += nint(1.2 * up.y);
    }

  *x += (bb.xMin - anchor.x) / 64.0;
  *y += (bb.yMin - anchor.y) / 64.0;

  *width = (int)((bb.xMax - bb.xMin) / 64);
  *height = (int)((bb.yMax - bb.yMin) / 64);
  if (bb.xMax <= bb.xMin || bb.yMax <= bb.yMin)
    {
      gks_perror("invalid bitmap size");
      gks_free(unicode_string);
      return NULL;
    }

  size = *width * *height;
  mono_bitmap = (FT_Byte *)gks_malloc(size);
  memset(mono_bitmap, 0, size);

  pen.x = 0;
  pen.y = 0;
  previous = 0;

  for (i = 0; i < num_glyphs; i++)
    {
      codepoint = unicode_string[i];

      bearing.x = bearing.y = 0;
      error = set_glyph(face, codepoint, &previous, &pen, vertical, &rotation, &bearing, halign, &glyph_slot);
      if (error) continue;

      pos_x = (pen.x + bearing.x - bb.xMin) / 64;
      pos_y = (-pen.y - bearing.y + bb.yMax) / 64;
      ftbitmap = glyph_slot->bitmap;
      for (j = 0; j < (unsigned int)ftbitmap.rows; j++)
        {
          for (k = 0; k < (unsigned int)ftbitmap.width; k++)
            {
              dx = k + pos_x;
              dy = j + pos_y;
              value = mono_bitmap[dy * *width + dx];
              value += ftbitmap.buffer[j * ftbitmap.pitch + k];
              if (value > 255)
                {
                  value = 255;
                }
              mono_bitmap[dy * *width + dx] = value;
            }
        }

      if (direction == GKS_K_TEXT_PATH_DOWN)
        {
          pen.x -= glyph_slot->advance.x + spacing.x;
          pen.y -= glyph_slot->advance.y + spacing.y;
        }
      else
        {
          pen.x += glyph_slot->advance.x + spacing.x;
          pen.y += glyph_slot->advance.y + spacing.y;
        }
    }
  gks_free(unicode_string);

  return mono_bitmap;
}

int *gks_ft_render(int *x, int *y, int *width, int *height, gks_state_list_t *gkss, const char *text, int length)
{
  FT_Byte *rgba_bitmap = NULL;
  double red, green, blue;
  int tmp, size, i, j;
  int color[4];
  unsigned char *mono_bitmap;

  mono_bitmap = gks_ft_get_bitmap(x, y, width, height, gkss, text, length);
  gks_inq_rgb(gkss->txcoli, &red, &green, &blue);
  color[0] = (int)(red * 255);
  color[1] = (int)(green * 255);
  color[2] = (int)(blue * 255);
  color[3] = (int)(gkss->alpha * 255);

  size = *width * *height;
  rgba_bitmap = (FT_Byte *)gks_malloc(4 * size);
  memset(rgba_bitmap, 0, 4 * size);
  for (i = 0; i < size; i++)
    {
      for (j = 0; j < 4; j++)
        {
          tmp = rgba_bitmap[4 * i + j] + color[j] * mono_bitmap[i] / 255;
          rgba_bitmap[4 * i + j] = (FT_Byte)ft_min(tmp, 255);
        }
    }

  gks_free(mono_bitmap);
  return (int *)rgba_bitmap;
}

static char *xrealloc(void *ptr, int size)
{
  char *result = (char *)realloc(ptr, size);
  if (!result)
    {
      gks_perror("out of virtual memory");
      abort();
    }
  return (result);
}

static void reallocate(int npoints)
{
  while (npoints >= maxpoints) maxpoints += POINT_INC;

  xpoint = (double *)xrealloc(xpoint, maxpoints * sizeof(double));
  ypoint = (double *)xrealloc(ypoint, maxpoints * sizeof(double));
  opcodes = (int *)xrealloc(opcodes, maxpoints * sizeof(int));
}

static void load_glyph(FT_Face face, FT_UInt code)
{
  FT_UInt glyph_index = FT_Get_Char_Index(face, code);
  if (!glyph_index) gks_perror("glyph missing from current font: %d", code);
  FT_Error error = FT_Load_Glyph(face, glyph_index, FT_LOAD_NO_SCALE | FT_LOAD_NO_BITMAP);
  if (error) gks_perror("could not load glyph: %d\n", glyph_index);
}

static void add_point(long x, long y)
{
  if (npoints >= maxpoints) reallocate(npoints);
  xpoint[npoints] = (double)(x + pen_x);
  ypoint[npoints] = (double)y;
  npoints += 1;
}

static int move_to(const FT_Vector *to, void *user)
{
  add_point(to->x, to->y);
  opcodes[num_opcodes++] = (int)'M';
  return 0;
}

static int line_to(const FT_Vector *to, void *user)
{
  add_point(to->x, to->y);
  opcodes[num_opcodes++] = (int)'L';
  return 0;
}

static int conic_to(const FT_Vector *control, const FT_Vector *to, void *user)
{
  add_point(control->x, control->y);
  add_point(to->x, to->y);
  opcodes[num_opcodes++] = (int)'Q';
  return 0;
}

static int cubic_to(const FT_Vector *control1, const FT_Vector *control2, const FT_Vector *to, void *user)
{
  add_point(control1->x, control1->y);
  add_point(control2->x, control2->y);
  add_point(to->x, to->y);
  opcodes[num_opcodes++] = (int)'C';
  return 0;
}

static void get_outline(FT_Face face, FT_UInt charcode, FT_Bool first, FT_Bool last)
{
  FT_Outline_Funcs callbacks;
  FT_GlyphSlot slot;
  FT_Glyph_Metrics metrics;
  FT_Outline outline;
  FT_Error error;

  callbacks.move_to = move_to;
  callbacks.line_to = line_to;
  callbacks.conic_to = conic_to;
  callbacks.cubic_to = cubic_to;

  callbacks.shift = 0;
  callbacks.delta = 0;

  slot = face->glyph;
  outline = slot->outline;
  metrics = slot->metrics;

  if (first) pen_x -= metrics.horiBearingX;

  error = FT_Outline_Decompose(&outline, &callbacks, NULL);
  if (error) gks_perror("could not extract the outline");

  if (num_opcodes > 0)
    {
      opcodes[num_opcodes++] = 'f';
      opcodes[num_opcodes] = '\0';
    }

  if (last && charcode != 32)
    {
      /* Use bearingX + width for the last character so that right-aligned texts are aligned to the bounding box of the
       * last glyph. If the last character is a space use the horiAdvance instead as the width is 0. */
      pen_x += metrics.horiBearingX + metrics.width;
    }
  else
    {
      pen_x += metrics.horiAdvance;
    }
}

static long get_kerning(FT_Face face, FT_UInt left_glyph, FT_UInt right_glyph)
{
  FT_UInt left_glyph_index, right_glyph_index;
  FT_Vector delta;
  FT_Error error;

  left_glyph_index = FT_Get_Char_Index(face, left_glyph);
  right_glyph_index = FT_Get_Char_Index(face, right_glyph);
  error = FT_Get_Kerning(face, left_glyph_index, right_glyph_index, FT_KERNING_UNSCALED, &delta);
  if (error)
    {
      gks_perror("could not get kerning information for %d, %d", left_glyph_index, right_glyph_index);
      delta.x = 0;
    }

  return delta.x;
}

static double get_capheight(FT_Face face)
{
  TT_PCLT *pclt;
  FT_BBox bbox;
  FT_Error error;
  long capheight;

  if (!init) gks_ft_init();

  pclt = FT_Get_Sfnt_Table(face, ft_sfnt_pclt);
  if (pclt == NULL)
    {
      /* Font does not contain CapHeight information.
       * Use use the letter 'I' to determine the height of capital letters */
      load_glyph(face, 'I');
      error = FT_Outline_Get_BBox(&face->glyph->outline, &bbox);
      if (error)
        {
          capheight = face->size->metrics.height;
          fprintf(stderr, "Couldn't get bounding box: FT_Outline_Get_BBox() failed\n");
        }
      else
        capheight = bbox.yMax - bbox.yMin;
    }
  else
    capheight = pclt->CapHeight;

  return capheight;
}

static void process_glyphs(FT_Face face, double x, double y, char *text, double phi, gks_state_list_t *gkss,
                           void (*gdp)(int, double *, double *, int, int, int *), double *bBoxX, double *bBoxY)
{
  FT_UInt unicode_string[256];
  FT_UInt length = strlen(text);
  int i, j;
  double xj, yj, cos_f, sin_f, shear_x, shear_y;
  double chh, height, theta;
  int alh;

  if (!init) gks_ft_init();

  WC_to_NDC(x, y, gkss->cntnr, x, y);
  utf_to_unicode((FT_Bytes)text, unicode_string, &length);

  pen_x = 0;
  cos_f = cos(phi);
  sin_f = sin(phi);
  chh = gkss->chh;
  height = chh / get_capheight(face);
  theta = gkss->txslant * M_PI / 180.0;
  shear_x = cos(theta);
  shear_y = sin(theta);
  alh = gkss->txal[0];

  for (i = 0; i < length; i++)
    {
      load_glyph(face, unicode_string[i]);

      if (i > 0 && FT_HAS_KERNING(face) && !FT_IS_FIXED_WIDTH(face))
        pen_x += get_kerning(face, unicode_string[i - 1], unicode_string[i]);

      get_outline(face, unicode_string[i], i == 0, i == length - 1);

      if (npoints > 0 && bBoxX == NULL && bBoxY == NULL)
        {
          for (j = 0; j < npoints; j++)
            {
              xj = xpoint[j] * height;
              yj = ypoint[j] * height;
              xj = horiAdvance + shear_x * xj + shear_y * yj;
              yj = vertAdvance + shear_x * yj;
              xpoint[j] = x + cos_f * xj - sin_f * yj;
              ypoint[j] = y + sin_f * xj + cos_f * yj;
            }

          (*gdp)(npoints, xpoint, ypoint, GKS_K_GDP_DRAW_PATH, num_opcodes, opcodes);
        }

      npoints = 0;
      num_opcodes = 0;
    }

  if (bBoxX != NULL && bBoxY != NULL)
    {
      bBoxX[0] = bBoxX[3] = bBoxX[4] = bBoxX[7] = 0;
      bBoxX[1] = bBoxX[2] = bBoxX[5] = bBoxX[6] = height * pen_x;
      /* The vertical extents should actually be determined by the font information,
       * but these values are usually oversized. */
      bBoxY[0] = bBoxY[1] = -chh * 0.3; /* face->descender; */
      bBoxY[2] = bBoxY[3] = chh * 1.2;  /* face->ascender;  */
      bBoxY[4] = bBoxY[5] = 0;
      bBoxY[6] = bBoxY[7] = chh;

      if (alh == GKS_K_TEXT_HALIGN_LEFT)
        bBoxX[8] = bBoxX[1];
      else if (alh == GKS_K_TEXT_HALIGN_RIGHT)
        bBoxX[8] = 0;
      else
        bBoxX[8] = -horiAdvance;
      bBoxY[8] = -vertAdvance;

      for (j = 0; j <= 8; j++)
        {
          xj = horiAdvance + bBoxX[j];
          yj = vertAdvance + bBoxY[j];
          bBoxX[j] = x + cos_f * xj - sin_f * yj;
          bBoxY[j] = y + sin_f * xj + cos_f * yj;
          NDC_to_WC(bBoxX[j], bBoxY[j], gkss->cntnr, bBoxX[j], bBoxY[j]);
        }
    }
}

void gks_ft_text(double x, double y, char *text, gks_state_list_t *gkss,
                 void (*gdp)(int, double *, double *, int, int, int *))
{
  double bBoxX[9], bBoxY[9];
  double phi;
  int alh, alv;
  double chux, chuy;
  FT_Face face = (FT_Face)gks_ft_get_face(gkss->txfont);

  alh = gkss->txal[0];
  alv = gkss->txal[1];
  chux = gkss->chup[0];
  chuy = gkss->chup[1];

  process_glyphs(face, x, y, text, 0, gkss, gdp, bBoxX, bBoxY);
  switch (alh)
    {
    case GKS_K_TEXT_HALIGN_LEFT:
      horiAdvance = 0;
      break;
    case GKS_K_TEXT_HALIGN_CENTER:
      horiAdvance = -0.5 * (bBoxX[1] - bBoxX[0]);
      break;
    case GKS_K_TEXT_HALIGN_RIGHT:
      horiAdvance = -(bBoxX[1] - bBoxX[0]);
      break;
    default:
      horiAdvance = 0;
      break;
    }
  switch (alv)
    {
    case GKS_K_TEXT_VALIGN_TOP:
      vertAdvance = bBoxY[4] - bBoxY[2];
      break;
    case GKS_K_TEXT_VALIGN_CAP:
      vertAdvance = bBoxY[4] - bBoxY[6];
      break;
    case GKS_K_TEXT_VALIGN_HALF:
      vertAdvance = (bBoxY[4] - bBoxY[6]) * 0.5;
      break;
    case GKS_K_TEXT_VALIGN_BASE:
      vertAdvance = 0;
      break;
    case GKS_K_TEXT_VALIGN_BOTTOM:
      vertAdvance = bBoxY[4] - bBoxY[0];
      break;
    default:
      vertAdvance = 0;
    }

  phi = -atan2(chux, chuy); /* character up vector */
  process_glyphs(face, x, y, text, phi, gkss, gdp, NULL, NULL);
}

void gks_ft_inq_text_extent(double x, double y, char *text, gks_state_list_t *gkss,
                            void (*gdp)(int, double *, double *, int, int, int *), double *bBoxX, double *bBoxY)
{
  double phi;
  double chux, chuy;
  FT_Face face = (FT_Face)gks_ft_get_face(gkss->txfont);

  chux = gkss->chup[0];
  chuy = gkss->chup[1];

  phi = -atan2(chux, chuy); /* character up vector */
  process_glyphs(face, x, y, text, phi, gkss, gdp, bBoxX, bBoxY);
}

static void process_glyphs3d(FT_Face face, double x, double y, double z, char *text, int axis, double phi,
                             gks_state_list_t *gkss, double heightFactor, double *scaleFactors,
                             void (*gdp)(int, double *, double *, int, int, int *),
                             void (*wc3towc)(double *, double *, double *), double *bBoxX, double *bBoxY)
{
  FT_UInt unicode_string[256];
  FT_UInt length = strlen(text);
  int i, j;
  double xj, yj, zj, cos_f, sin_f, shear_x, shear_y;
  double chh, height, theta;
  int alh;

  if (!init) gks_ft_init();

  utf_to_unicode((FT_Bytes)text, unicode_string, &length);

  pen_x = 0;
  cos_f = cos(phi);
  sin_f = sin(phi);
  chh = gkss->chh;
  chh /= heightFactor;
  height = chh / get_capheight(face);
  theta = gkss->txslant * M_PI / 180.0;
  shear_x = cos(theta);
  shear_y = sin(theta);

  alh = gkss->txal[0];

  for (i = 0; i < length; i++)
    {
      load_glyph(face, unicode_string[i]);

      if (i > 0 && FT_HAS_KERNING(face) && !FT_IS_FIXED_WIDTH(face))
        pen_x += get_kerning(face, unicode_string[i - 1], unicode_string[i]);

      get_outline(face, unicode_string[i], i == 0, i == length - 1);

      if (npoints > 0 && bBoxX == NULL && bBoxY == NULL)
        {
          for (j = 0; j < npoints; j++)
            {
              xj = xpoint[j] * height;
              yj = ypoint[j] * height;
              xj = horiAdvance + (axis < 0 ? -1 : 1) * (shear_x * xj + shear_y * yj);
              yj = vertAdvance + shear_x * yj;
              xpoint[j] = cos_f * xj - sin_f * yj;
              ypoint[j] = sin_f * xj + cos_f * yj;

              if (axis == 1 || axis == -1)
                {
                  xj = x - ypoint[j] / scaleFactors[0];
                  yj = y + xpoint[j] / scaleFactors[1];
                  zj = z;
                }
              else if (axis == 2 || axis == -2)
                {
                  xj = x + xpoint[j] / scaleFactors[0];
                  yj = y + ypoint[j] / scaleFactors[1];
                  zj = z;
                }
              else if (axis == 3 || axis == -3)
                {
                  xj = x;
                  yj = y + xpoint[j] / scaleFactors[1];
                  zj = z + ypoint[j] / scaleFactors[2];
                }
              else if (axis == 4 || axis == -4)
                {
                  xj = x + xpoint[j] / scaleFactors[0];
                  yj = y;
                  zj = z + ypoint[j] / scaleFactors[2];
                }

              (*wc3towc)(&xj, &yj, &zj);

              xpoint[j] = xj;
              ypoint[j] = yj;
            }

          (*gdp)(npoints, xpoint, ypoint, GKS_K_GDP_DRAW_PATH, num_opcodes, opcodes);
        }

      npoints = 0;
      num_opcodes = 0;
    }

  if (bBoxX != NULL && bBoxY != NULL)
    {
      bBoxX[0] = bBoxX[3] = bBoxX[4] = bBoxX[7] = 0;
      bBoxX[1] = bBoxX[2] = bBoxX[5] = bBoxX[6] = height * pen_x;
      /* The vertical extents should actually be determined by the font information,
       * but these values are usually oversized. */
      bBoxY[0] = bBoxY[1] = -chh * 0.3; /* face->descender; */
      bBoxY[2] = bBoxY[3] = chh * 1.2;  /* face->ascender;  */
      bBoxY[4] = bBoxY[5] = 0;
      bBoxY[6] = bBoxY[7] = chh;
      for (j = 8; j < 16; j++)
        {
          bBoxX[j] = bBoxX[j - 8];
          bBoxY[j] = bBoxY[j - 8];
        }

      for (j = 0; j < 16; j++)
        {
          xj = horiAdvance + (axis < 0 ? -1 : 1) * bBoxX[j];
          yj = vertAdvance + bBoxY[j];
          bBoxX[j] = cos_f * xj - sin_f * yj;
          bBoxY[j] = sin_f * xj + cos_f * yj;

          if (j >= 8)
            {
              if (axis == 1 || axis == -1)
                {
                  xj = x - bBoxY[j] / scaleFactors[0];
                  yj = y + bBoxX[j] / scaleFactors[1];
                  zj = z;
                }
              else if (axis == 2 || axis == -2)
                {
                  xj = x + bBoxX[j] / scaleFactors[0];
                  yj = y + bBoxY[j] / scaleFactors[1];
                  zj = z;
                }
              else if (axis == 3 || axis == -3)
                {
                  xj = x;
                  yj = y + bBoxX[j] / scaleFactors[1];
                  zj = z + bBoxY[j] / scaleFactors[2];
                }
              else if (axis == 4 || axis == -4)
                {
                  xj = x + bBoxX[j] / scaleFactors[0];
                  yj = y;
                  zj = z + bBoxY[j] / scaleFactors[2];
                }

              (*wc3towc)(&xj, &yj, &zj);

              bBoxX[j] = xj;
              bBoxY[j] = yj;
            }
          else
            {
              bBoxX[j] += x;
              bBoxY[j] += y;
            }
        }
    }
}

void gks_ft_text3d(double x, double y, double z, char *text, int axis, gks_state_list_t *gkss, double heightFactor,
                   double *scaleFactors, void (*gdp)(int, double *, double *, int, int, int *),
                   void (*wc3towc)(double *, double *, double *))
{
  double bBoxX[16], bBoxY[16];
  double phi;
  int alh, alv;
  double chux, chuy;
  FT_Face face = (FT_Face)gks_ft_get_face(gkss->txfont);

  alh = gkss->txal[0];
  alv = gkss->txal[1];
  chux = gkss->chup[0];
  chuy = gkss->chup[1];

  process_glyphs3d(face, x, y, z, text, axis, 0, gkss, heightFactor, scaleFactors, gdp, wc3towc, bBoxX, bBoxY);
  switch (alh)
    {
    case GKS_K_TEXT_HALIGN_LEFT:
      horiAdvance = 0;
      break;
    case GKS_K_TEXT_HALIGN_CENTER:
      horiAdvance = -0.5 * (bBoxX[1] - bBoxX[0]);
      break;
    case GKS_K_TEXT_HALIGN_RIGHT:
      horiAdvance = -(bBoxX[1] - bBoxX[0]);
      break;
    default:
      horiAdvance = 0;
      break;
    }
  switch (alv)
    {
    case GKS_K_TEXT_VALIGN_TOP:
      vertAdvance = bBoxY[4] - bBoxY[2];
      break;
    case GKS_K_TEXT_VALIGN_CAP:
      vertAdvance = bBoxY[4] - bBoxY[6];
      break;
    case GKS_K_TEXT_VALIGN_HALF:
      vertAdvance = (bBoxY[4] - bBoxY[6]) * 0.5;
      break;
    case GKS_K_TEXT_VALIGN_BASE:
      vertAdvance = 0;
      break;
    case GKS_K_TEXT_VALIGN_BOTTOM:
      vertAdvance = bBoxY[4] - bBoxY[0];
      break;
    default:
      vertAdvance = 0;
    }

  phi = -atan2(chux, chuy); /* character up vector */
  process_glyphs3d(face, x, y, z, text, axis, phi, gkss, heightFactor, scaleFactors, gdp, wc3towc, NULL, NULL);
}

void gks_ft_inq_text3d_extent(double x, double y, double z, char *text, int axis, gks_state_list_t *gkss,
                              double heightFactor, double *scaleFactors,
                              void (*gdp)(int, double *, double *, int, int, int *),
                              void (*wc3towc)(double *, double *, double *), double *bBoxX, double *bBoxY)
{
  double phi;
  double chux, chuy;
  FT_Face face = (FT_Face)gks_ft_get_face(gkss->txfont);

  chux = gkss->chup[0];
  chuy = gkss->chup[1];

  phi = -atan2(chux, chuy); /* character up vector */
  process_glyphs3d(face, x, y, z, text, axis, phi, gkss, heightFactor, scaleFactors, gdp, wc3towc, bBoxX, bBoxY);
}

#else

static int init = 0;

int gks_ft_init(void)
{
  gks_perror("FreeType support not compiled in");
  init = 1;
  return 1;
}

int *gks_ft_render(int *x, int *y, int *width, int *height, gks_state_list_t *gkss, const char *text, int length)
{
  if (!init) gks_ft_init();
  return NULL;
}

void gks_ft_terminate(void) {}

void gks_ft_text(double x, double y, char *text, gks_state_list_t *gkss,
                 void (*gdp)(int, double *, double *, int, int, int *))
{
  if (!init) gks_ft_init();
}

void gks_ft_inq_text_extent(double x, double y, char *text, gks_state_list_t *gkss,
                            void (*gdp)(int, double *, double *, int, int, int *), double *bBoxX, double *bBoxY)
{
  if (!init) gks_ft_init();
}

void gks_ft_text3d(double x, double y, double z, char *text, int axis, gks_state_list_t *gkss, double heightFactor,
                   double *scaleFactors, void (*gdp)(int, double *, double *, int, int, int *),
                   void (*wc3towc)(double *, double *, double *))
{
  if (!init) gks_ft_init();
}

void gks_ft_inq_text3d_extent(double x, double y, double z, char *text, int axis, gks_state_list_t *gkss,
                              double heightFactor, double *scaleFactors,
                              void (*gdp)(int, double *, double *, int, int, int *),
                              void (*wc3towc)(double *, double *, double *), double *bBoxX, double *bBoxY)
{
  if (!init) gks_ft_init();
}

#endif
