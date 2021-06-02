
#include <stdio.h>
#include <stdlib.h>

#include "gks.h"

int main(int argc, char *argv[])
{
  int asf[13];
  double x[10], y[10], cpx, cpy, tx[4], ty[4];
  int i, j, k;
  int colia[8][10];
  int inp_status, tnr, errind;
  int count = 1, wstype = GKS_K_WSTYPE_DEFAULT;
  int width, height;
  double dpr;

  for (i = 0; i < 13; i++) asf[i] = GKS_K_ASF_INDIVIDUAL;

  k = 0;
  for (i = 0; i < 8; i++)
    for (j = 0; j < 10; j++)
      {
        colia[i][j] = k++;
      }

  gks_open_gks(6);
  gks_set_asf(asf);
  if (argc > 1)
    {
      count = atoi(argv[1]);
      wstype = count > 0 ? GKS_K_WSTYPE_WISS : GKS_K_WSTYPE_MO;
      gks_open_ws(1, GKS_K_CONID_DEFAULT, wstype);
      gks_activate_ws(1);
      gks_create_seg(1);
    }
  else
    {
      gks_open_ws(2, GKS_K_CONID_DEFAULT, GKS_K_WSTYPE_DEFAULT);
      gks_activate_ws(2);
    }
  gks_set_text_fontprec(12, GKS_K_TEXT_PRECISION_STROKE);

  /* Text */
  gks_set_text_height(.02);
  gks_set_text_fontprec(-1, GKS_K_TEXT_PRECISION_STROKE);
  gks_text(.6, .7, "Font-1");
  gks_set_text_fontprec(-2, GKS_K_TEXT_PRECISION_STROKE);
  gks_text(.6, .65, "Font-2");
  gks_set_text_fontprec(-3, GKS_K_TEXT_PRECISION_STROKE);
  gks_text(.6, .6, "Font-3");
  gks_set_text_fontprec(-4, GKS_K_TEXT_PRECISION_STROKE);
  gks_text(.6, .55, "Font-4");
  gks_set_text_fontprec(-5, GKS_K_TEXT_PRECISION_STROKE);
  gks_text(.6, .5, "Font-5");
  gks_set_text_fontprec(-6, GKS_K_TEXT_PRECISION_STROKE);
  gks_text(.6, .45, "Font-6");
  gks_set_text_fontprec(-7, GKS_K_TEXT_PRECISION_STROKE);
  gks_text(.6, .4, "Font-7");
  gks_set_text_fontprec(-8, GKS_K_TEXT_PRECISION_STROKE);
  gks_text(.6, .35, "Font-8");
  gks_set_text_fontprec(-9, GKS_K_TEXT_PRECISION_STROKE);
  gks_text(.6, .3, "Font-9");
  gks_set_text_fontprec(-10, GKS_K_TEXT_PRECISION_STROKE);
  gks_text(.6, .25, "Font-10");
  gks_set_text_fontprec(-11, GKS_K_TEXT_PRECISION_STROKE);
  gks_text(.6, .2, "Font-11");
  gks_set_text_fontprec(-12, GKS_K_TEXT_PRECISION_STROKE);
  gks_text(.8, .7, "Font-12");
  gks_set_text_fontprec(-13, GKS_K_TEXT_PRECISION_STROKE);
  gks_text(.8, .65, "Font-13");
  gks_set_text_fontprec(-14, GKS_K_TEXT_PRECISION_STROKE);
  gks_text(.8, .6, "Font-14");
  gks_set_text_fontprec(-15, GKS_K_TEXT_PRECISION_STROKE);
  gks_text(.8, .55, "Font-15");
  gks_set_text_fontprec(-16, GKS_K_TEXT_PRECISION_STROKE);
  gks_text(.8, .5, "Font-16");
  gks_set_text_fontprec(-17, GKS_K_TEXT_PRECISION_STROKE);
  gks_text(.8, .45, "Font-17");
  gks_set_text_fontprec(-18, GKS_K_TEXT_PRECISION_STROKE);
  gks_text(.8, .4, "Font-18");
  gks_set_text_fontprec(-19, GKS_K_TEXT_PRECISION_STROKE);
  gks_text(.8, .35, "Font-19");
  gks_set_text_fontprec(-20, GKS_K_TEXT_PRECISION_STROKE);
  gks_text(.8, .3, "Font-20");
  gks_set_text_fontprec(-21, GKS_K_TEXT_PRECISION_STROKE);
  gks_text(.8, .25, "Font-21");
  gks_set_text_fontprec(-22, GKS_K_TEXT_PRECISION_STROKE);
  gks_text(.8, .2, "Font-22");
  gks_set_text_fontprec(-23, GKS_K_TEXT_PRECISION_STROKE);
  gks_text(.6, .15, "Font-23");
  gks_set_text_fontprec(-24, GKS_K_TEXT_PRECISION_STROKE);
  gks_text(.8, .15, "Font-24");

  /* Colors */
  gks_set_text_fontprec(-5, GKS_K_TEXT_PRECISION_STROKE);
  gks_set_text_color_index(0);
  gks_text(.45, .35, "White");
  gks_set_text_color_index(1);
  gks_text(.45, .32, "Black");
  gks_set_text_color_index(2);
  gks_text(.45, .29, "Red");
  gks_set_text_color_index(3);
  gks_text(.45, .26, "Green");
  gks_set_text_color_index(4);
  gks_text(.45, .23, "Blue");
  gks_set_text_color_index(5);
  gks_text(.45, .20, "Cyan");
  gks_set_text_color_index(6);
  gks_text(.45, .17, "Yellow");
  gks_set_text_color_index(7);
  gks_text(.45, .14, "Magenta");

  /* Linetypes */
  x[0] = 0.10;
  x[1] = 0.26;
  y[1] = y[0] = 0.95;

  for (j = -8; j <= 4; j++)
    {
      if (j)
        {
          gks_set_pline_linetype(j);
          y[1] = y[0] -= 0.02;
          gks_polyline(2, x, y);
        }
    }

  for (i = 1; i <= 3; i++)
    {
      y[0] = y[0] - 0.03;
      gks_set_pline_linewidth(1.0 * i);
      for (j = 1; j <= 4; j++)
        {
          if (j)
            {
              gks_set_pline_linetype(j);
              y[1] = y[0] -= 0.02;
              gks_polyline(2, x, y);
            }
        }
    }

  /* Markertypes */
  gks_set_pmark_size(3.5);

  x[0] = 0.25;
  y[0] = 0.95;
  for (j = -32; j <= -21; j++)
    {
      gks_set_pmark_type(j);
      x[0] += 0.06;
      gks_polymarker(1, x, y);
    }

  x[0] = 0.25;
  y[0] = 0.875;
  for (j = -20; j <= -9; j++)
    {
      gks_set_pmark_type(j);
      x[0] += 0.06;
      gks_polymarker(1, x, y);
    }

  x[0] = 0.25;
  y[0] = 0.8;
  for (j = -8; j <= 4; j++)
    {
      if (j != 0)
        {
          gks_set_pmark_type(j);
          x[0] += 0.06;
          gks_polymarker(1, x, y);
        }
    }

  /* Fill Areas */
  x[0] = 0.02;
  x[1] = 0.12;
  x[2] = 0.12;
  x[3] = 0.02;
  x[4] = x[0];
  y[0] = 0.02;
  y[1] = 0.02;
  y[2] = 0.12;
  y[3] = 0.12;
  y[4] = y[0];

  gks_set_fill_style_index(4);
  for (j = 0; j <= 3; j++)
    {
      for (i = 0; i < 5; i++)
        {
          x[i] += 0.1;
          y[i] += 0.1;
        }
      gks_set_fill_int_style(j);
      gks_fillarea(5, x, y);
    }

  /* Patterns */
  x[0] = 0.02;
  x[1] = 0.07;
  x[2] = 0.07;
  x[3] = 0.02;
  x[4] = x[0];
  y[0] = 0.2;
  y[1] = 0.2;
  y[2] = 0.25;
  y[3] = 0.25;
  y[4] = y[0];

  gks_set_fill_int_style(2);
  for (j = 4; j <= 15; j++)
    {
      for (i = 0; i < 5; i++)
        {
          y[i] += 0.06;
        }
      gks_set_fill_style_index(j - 3);
      gks_fillarea(5, x, y);
    }

  /* Cell Array */
  gks_cellarray(0.332, 0.75, 0.532, 0.55, 8, 10, 1, 1, 8, 10, colia[0]);

  /* More Text */
  gks_set_text_height(0.08);
  gks_set_text_align(2, 3);
  gks_set_text_color_index(1);
  gks_set_text_fontprec(12, GKS_K_TEXT_PRECISION_STROKE);
  gks_text(.5, .05, "Hello World");

  gks_inq_text_extent(2, .5, .05, "Hello World", &errind, &cpx, &cpy, tx, ty);
  if (errind == GKS_K_NO_ERROR)
    {
      gks_set_fill_int_style(0);
      gks_fillarea(4, tx, ty);
    }

  gks_set_text_height(0.024);
  gks_set_text_align(1, 1);
  gks_set_text_fontprec(3, GKS_K_TEXT_PRECISION_STRING);
  gks_set_text_upvec(-1, 0);
  gks_set_text_color_index(200);
  gks_text(.05, .15, "up");
  gks_set_text_upvec(0, -1);
  gks_set_text_color_index(400);
  gks_text(.05, .15, "left");
  gks_set_text_upvec(1, 0);
  gks_set_text_color_index(600);
  gks_text(.05, .15, "down");
  gks_set_text_color_index(800);
  gks_set_text_upvec(0, 1);
  gks_text(.05, .15, "right");

  if (argc > 1)
    {
      gks_close_seg();
      gks_deactivate_ws(1);
      gks_open_ws(2, GKS_K_CONID_DEFAULT, GKS_K_WSTYPE_DEFAULT);
      gks_activate_ws(2);

      while (count--)
        {
          gks_clear_ws(2, GKS_K_CLEAR_ALWAYS);
          gks_copy_seg_to_ws(2, 1);
          gks_update_ws(2, GKS_K_POSTPONE_FLAG);
          printf("%d\n", count);
        }

      if (wstype == GKS_K_WSTYPE_MO) gks_update_ws(1, GKS_K_PERFORM_FLAG | GKS_K_WRITE_PAGE_FLAG);
    }
  else
    {
      gks_update_ws(2, GKS_K_PERFORM_FLAG | GKS_K_WRITE_PAGE_FLAG);
#ifndef EMSCRIPTEN
      puts("Press RETURN to continue ...");
      getchar();

      gks_set_pmark_type(2);
      gks_request_locator(2, 1, &inp_status, &tnr, x, y);
      while (inp_status == 1)
        {
          gks_polymarker(1, x, y);
          gks_request_locator(2, 1, &inp_status, &tnr, x, y);
        }
      printf("%g %g\n", x[0], y[0]);
#endif
    }

  gks_inq_vp_size(2, &errind, &width, &height, &dpr);
  printf("width=%d height=%d dpr=%f\n", width, height, dpr);

  gks_emergency_close();

  return 0;
}
