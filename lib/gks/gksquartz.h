#ifndef _GKSQUARTZ_H_
#define _GKSQUARTZ_H_

#define MAX_WINDOWS 50

#define GKSTERM_FUNCTION_UNKNOWN 0
#define GKSTERM_FUNCTION_CREATE_WINDOW 1
#define GKSTERM_FUNCTION_DRAW 2
#define GKSTERM_FUNCTION_IS_ALIVE 3
#define GKSTERM_FUNCTION_CLOSE_WINDOW 4
#define GKSTERM_FUNCTION_IS_RUNNING 5
#define GKSTERM_FUNCTION_INQ_WS_STATE 6

#include <CoreGraphics/CGGeometry.h>
#include <CoreGraphics/CGColor.h>

typedef struct
{
  int state;
  int win;
  gks_display_list_t dl;
  double width, height;
  double swidth, sheight;
  double a, b, c, d;
  double window[4], viewport[4];
  double nominal_size;
  CGColorRef rgb[MAX_COLOR];
  int family, capheight;
  double angle;
  CGRect rect[MAX_TNR];
  pthread_t master_thread;
  int inactivity_counter;
  bool thread_alive;
  bool closed_by_api;
  bool empty;
} ws_state_list;

#endif
