
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>

#ifdef _WIN32
#include <windows.h>
#endif

#include "gkscore.h"

char *gks_a_error_info = NULL; /* for compatibility with GLI/GKS */
int gks_errno = 0;
FILE *gks_a_error_file = NULL;

void gks_perror(const char *format, ...)
{
  va_list ap;

  if (gks_a_error_file == NULL) gks_a_error_file = stderr;

  fprintf(gks_a_error_file, "GKS: ");

  va_start(ap, format);
  vfprintf(gks_a_error_file, format, ap);
  va_end(ap);

  fprintf(gks_a_error_file, "\n");
}

void gks_fatal_error(const char *args, ...)
{
  gks_perror(args);
  exit(-1);
}

const char *gks_function_name(int routine)
{
  const char *name;

  switch (routine)
    {
    case 0:
      name = "OPEN_GKS";
      break;
    case 1:
      name = "CLOSE_GKS";
      break;
    case 2:
      name = "OPEN_WS";
      break;
    case 3:
      name = "CLOSE_WS";
      break;
    case 4:
      name = "ACTIVATE_WS";
      break;
    case 5:
      name = "DEACTIVATE_WS";
      break;
    case 6:
      name = "CLEAR_WS";
      break;
    case 7:
      name = "REDRAW_SEG_ON_WS";
      break;
    case 8:
      name = "UPDATE_WS";
      break;
    case 9:
      name = "SET_DEFERRAL_STATE";
      break;
    case 10:
      name = "MESSAGE";
      break;
    case 11:
      name = "ESCAPE";
      break;
    case 12:
      name = "POLYLINE";
      break;
    case 13:
      name = "POLYMARKER";
      break;
    case 14:
      name = "TEXT";
      break;
    case 15:
      name = "FILLAREA";
      break;
    case 16:
      name = "CELLARRAY";
      break;
    case 17:
      name = "GDP";
      break;
    case 18:
      name = "SET_PLINE_INDEX";
      break;
    case 19:
      name = "SET_PLINE_LINETYPE";
      break;
    case 20:
      name = "SET_PLINE_LINEWIDTH";
      break;
    case 21:
      name = "SET_PLINE_COLOR_INDEX";
      break;
    case 22:
      name = "SET_PMARK_INDEX";
      break;
    case 23:
      name = "SET_PMARK_TYPE";
      break;
    case 24:
      name = "SET_PMARK_SIZE";
      break;
    case 25:
      name = "SET_PMARK_COLOR_INDEX";
      break;
    case 26:
      name = "SET_TEXT_INDEX";
      break;
    case 27:
      name = "SET_TEXT_FONTPREC";
      break;
    case 28:
      name = "SET_TEXT_EXPFAC";
      break;
    case 29:
      name = "SET_TEXT_SPACING";
      break;
    case 30:
      name = "SET_TEXT_COLOR_INDEX";
      break;
    case 31:
      name = "SET_TEXT_HEIGHT";
      break;
    case 32:
      name = "SET_TEXT_UPVEC";
      break;
    case 33:
      name = "SET_TEXT_PATH";
      break;
    case 34:
      name = "SET_TEXT_ALIGN";
      break;
    case 35:
      name = "SET_FILL_INDEX";
      break;
    case 36:
      name = "SET_FILL_INT_STYLE";
      break;
    case 37:
      name = "SET_FILL_STYLE_INDEX";
      break;
    case 38:
      name = "SET_FILL_COLOR_INDEX";
      break;
    case 41:
      name = "SET_ASF";
      break;
    case 48:
      name = "SET_COLOR_REP";
      break;
    case 49:
      name = "SET_WINDOW";
      break;
    case 50:
      name = "SET_VIEWPORT";
      break;
    case 52:
      name = "SELECT_XFORM";
      break;
    case 53:
      name = "SET_CLIPPING";
      break;
    case 54:
      name = "SET_WS_WINDOW";
      break;
    case 55:
      name = "SET_WS_VIEWPORT";
      break;
    case 56:
      name = "CREATE_SEG";
      break;
    case 57:
      name = "CLOSE_SEG";
      break;
    case 59:
      name = "DELETE_SEG";
      break;
    case 61:
      name = "ASSOC_SEG_WITH_WS";
      break;
    case 62:
      name = "COPY_SEG_TO_WS";
      break;
    case 64:
      name = "SET_SEG_XFORM";
      break;
    case 69:
      name = "INITIALIZE_LOCATOR";
      break;
    case 81:
      name = "REQUEST_LOCATOR";
      break;
    case 82:
      name = "REQUEST_STROKE";
      break;
    case 84:
      name = "REQUEST_CHOICE";
      break;
    case 86:
      name = "REQUEST_STRING";
      break;
    case 102:
      name = "GET_ITEM";
      break;
    case 103:
      name = "READ_ITEM";
      break;
    case 104:
      name = "INTERPRET_ITEM";
      break;
    case 105:
      name = "EVAL_XFORM_MATRIX";
      break;
    case 106:
      name = "SET_ENCODING";
      break;
    case 107:
      name = "INQ_ENCODING";
      break;
    case 108:
      name = "SET_RESAMPLE_METHOD";
      break;
    case 200:
      name = "SET_TEXT_SLANT";
      break;
    case 201:
      name = "DRAW_IMAGE";
      break;
    case 202:
      name = "SET_SHADOW";
      break;
    case 203:
      name = "SET_TRANSPARENCY";
      break;
    case 204:
      name = "SET_COORD_XFORM";
      break;
    case 205:
      name = "CONFIGURE_WS";
      break;
    case 206:
      name = "SET_BORDER_WIDTH";
      break;
    case 207:
      name = "SET_BORDER_COLOR_INDEX";
      break;
    case 208:
      name = "SELECT_CLIP_XFORM";
      break;
    case 250:
      name = "BEGIN_SELECTION";
      break;
    case 251:
      name = "END_SELECTION";
      break;
    case 252:
      name = "MOVE_SELECTION";
      break;
    case 253:
      name = "RESIZE_SELECTION";
      break;
    case 254:
      name = "INQ_BBOX";
      break;
    default:
      name = "?";
    }

  return name;
}

void gks_report_error(int routine, int errnum)
{
  const char *name, *message;

  name = gks_function_name(routine);

  switch (errnum)
    {
    case 0:
      message = "normal successful completion";
      break;
    case 1:
      message = "GKS not in proper state. GKS must be in the state\
 GKCL in routine %s";
      break;
    case 2:
      message = "GKS not in proper state. GKS must be in the state\
 GKOP in routine %s";
      break;
    case 3:
      message = "GKS not in proper state. GKS must be in the state\
 WSAC in routine %s";
      break;
    case 4:
      message = "GKS not in proper state. GKS must be in the state\
 SGOP in routine %s";
      break;
    case 5:
      message = "GKS not in proper state. GKS must be either in the\
 state WSAC or SGOP in routine %s";
      break;
    case 6:
      message = "GKS not in proper state. GKS must be either in the\
 state WSOP or WSAC in routine %s";
      break;
    case 7:
      message = "GKS not in proper state. GKS must be in one of the\
 states WSOP,WSAC,SGOP in routine %s";
      break;
    case 8:
      message = "GKS not in proper state. GKS must be in one of the\
 states GKOP,WSOP,WSAC,SGOP in routine %s";
      break;
    case 20:
      message = "Specified workstation identifier is invalid in\
 routine %s";
      break;
    case 21:
      message = "Specified connection identifier is invalid in\
 routine %s";
      break;
    case 22:
      message = "Specified workstation type is invalid in\
 routine %s";
      break;
    case 24:
      message = "Specified workstation is open in routine %s";
      break;
    case 25:
      message = "Specified workstation is not open in\
 routine %s";
      break;
    case 26:
      message = "Specified workstation cannot be opened in\
 routine %s";
      break;
    case 27:
      message = "Workstation Independent Segment Storage is\
 not open in routine %s";
      break;
    case 28:
      message = "Workstation Independent Segment Storage is\
 already open in routine %s";
      break;
    case 29:
      message = "Specified workstation is active in routine %s";
      break;
    case 30:
      message = "Specified workstation is not active in\
 routine %s";
      break;
    case 34:
      message = "Specified workstation is not of category MI in\
 routine %s";
      break;
    case 38:
      message = "Specified workstation is neither of category INPUT\
 nor of category OUTIN in routine %s";
      break;
    case 50:
      message = "Transformation number is invalid in routine %s";
      break;
    case 51:
      message = "Rectangle definition is invalid in routine %s";
      break;
    case 52:
      message = "Viewport is not within the NDC unit square in\
 routine %s";
      break;
    case 53:
      message = "Workstation window is not within the NDC unit square\
 in routine %s";
      break;
    case 60:
      message = "Polyline index is invalid in routine %s";
      break;
    case 62:
      message = "Linetype is invalid in routine %s";
      break;
    case 64:
      message = "Polymarker index is invalid in routine %s";
      break;
    case 65:
      message = "Colour index is invalid in routine %s";
      break;
    case 66:
      message = "Marker type is invalid in routine %s";
      break;
    case 68:
      message = "Text index is invalid in routine %s";
      break;
    case 70:
      message = "Text font is invalid in routine %s";
      break;
    case 72:
      message = "Character expansion factor is invalid in\
 routine %s";
      break;
    case 73:
      message = "Character height is invalid in routine %s";
      break;
    case 74:
      message = "Character up vector is invalid in routine %s";
      break;
    case 75:
      message = "Fill area index is invalid in routine %s";
      break;
    case 78:
      message = "Style index is invalid in routine %s";
      break;
    case 81:
      message = "Pattern size value is invalid in routine %s";
      break;
    case 84:
      message = "Dimensions of colour index array are invalid in\
 routine %s";
      break;
    case 85:
      message = "Colour index is invalid in routine %s";
      break;
    case 88:
      message = "Colour is invalid in routine %s";
      break;
    case 91:
      message = "Dimensions of color index array are invalid in\
 routine %s";
      break;
    case 100:
      message = "Number of points is invalid in routine %s";
      break;
    case 161:
      message = "Item length is invalid in routine %s";
      break;
    case 163:
      message = "Metafile item is invalid in routine %s";
      break;
    case 164:
      message = "Item type is not a valid GKS item in\
 routine %s";
      break;
    case 401:
      message = "Dimensions of image are invalid in routine %s";
      break;
    case 402:
      message = "Invalid image data pointer in routine %s";
      break;
    case 403:
      message = "String is too long in routine %s";
      break;
    case 404:
      message = "Subimage limitation reached in routine %s";
      break;
    case 501:
      message = "Resample method is invalid in routine %s";
      break;
    case 901:
      message = "Open failed in routine %s";
      break;
    default:
      message = "unknown error";
    }

  gks_errno = errnum;

  gks_perror(message, name);
}
