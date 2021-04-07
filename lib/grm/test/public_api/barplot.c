#ifndef _MSC_VER
#include <unistd.h>
#else
#include <io.h>
#endif
#include "grm.h"

#define N_SERIES 3
#define INNER_N_SERIES 3

int main()
{
  int n_y = 3;
  int n_yy = 3;
  double y[] = {4, 5, 8};
  double yy1[] = {4, 8, 2};
  double yy2[] = {7, 3, 9};
  double yy3[] = {1, 4, 6};
  double inner_yy2_1[] = {5, 2};
  double inner_yy2_2[] = {3};
  double inner_yy2_3[] = {9};
  int n_inner_yy2_1 = 2;
  double yy_pos_neg[] = {5, -5, 3, -3};
  int n_yy_pos_neg = 4;
  int c[] = {984, 992, 993};
  int c2[] = {997, 998, 999};
  double c_rgb[3][3] = {{0.5, 0.4, 0.3}, {0.3, 0.4, 0.5}, {0.4, 0.3, 0.5}};
  int inner_c[] = {989, 984};
  int n_inner_c = 2;
  double inner_c_rgb[2][3] = {{0.5, 0.4, 0.3}, {0.8, 0.1, 0.1}};
  int n_inner_c_rgb = 6;
  int bar_color = 992;
  int edge_color = 989;
  double bar_width = 0.85;
  double edge_width = 1.5;
  const char *xticklabels[3] = {"eins", "zwei", "drei"};
  const char *ylabels[3] = {"4", "5", "8"};
  const char *yy1_labels[3] = {"4", "8", "2"};
  const char *yy2_labels[3] = {"7", "3", "9"};
  const char *yy3_labels[3] = {"1", "4", "6"};
  const char *yy2_labels_for_inner[4] = {"5", "2", "3", "9"};
  const char *yy_pos_neg_labels[4] = {"5", "-5", "3", "-3"};
  int indices[2] = {1, 2};
  grm_args_t *args;
  grm_args_t *ind_bar_color[2];
  grm_args_t *ind_edge_color;
  grm_args_t *ind_edge_width;
  grm_args_t *series[N_SERIES];
  grm_args_t *inner_series[INNER_N_SERIES];
  int i, j;

  args = grm_args_new();

  /* Draw the bar plot */
  grm_args_push(args, "y", "nD", n_y, y);
  grm_args_push(args, "kind", "s", "barplot");
  grm_plot(args);
  sleep(3);

  /* Draw the bar plot with locations specified by x and y values in the bars*/
  grm_args_push(args, "xticklabels", "nS", n_y, xticklabels);
  grm_args_push(args, "ylabels", "nS", n_y, ylabels);
  grm_plot(args);
  sleep(3);

  /* Draw the bar plot with different bar_width, edge_width, edge_color and bar_color */
  grm_args_push(args, "edge_width", "d", edge_width);
  grm_args_push(args, "bar_width", "d", bar_width);
  grm_args_push(args, "edge_color", "i", edge_color);
  grm_args_push(args, "bar_color", "i", bar_color);
  grm_plot(args);
  sleep(3);
  /* or */
  grm_args_push(args, "bar_color", "ddd", 0.66, 0.66, 0.66);
  grm_args_push(args, "edge_color", "ddd", 0.33, 0.33, 0.33);
  grm_plot(args);
  sleep(3);

  /* Draw the bar plot with bars that have individual bar_color, edge_color, edge_with */
  for (j = 0; j < 2; ++j)
    {
      ind_bar_color[j] = grm_args_new();
    }
  grm_args_push(ind_bar_color[0], "indices", "nI", 2, indices);
  grm_args_push(ind_bar_color[0], "rgb", "ddd", 0.0, 0.666, 0.333);
  grm_args_push(ind_bar_color[1], "indices", "i", 3);
  grm_args_push(ind_bar_color[1], "rgb", "ddd", 0.111, 0.222, 0.333);

  ind_edge_color = grm_args_new();
  grm_args_push(ind_edge_color, "indices", "i", 3);
  grm_args_push(ind_edge_color, "rgb", "ddd", 0.9, 0.6, 0.3);

  ind_edge_width = grm_args_new();
  grm_args_push(ind_edge_width, "indices", "i", 3);
  grm_args_push(ind_edge_width, "width", "d", 5.0);

  grm_args_push(args, "ind_bar_color", "nA", 2, ind_bar_color);
  grm_args_push(args, "ind_edge_color", "a", ind_edge_color);
  grm_args_push(args, "ind_edge_width", "a", ind_edge_width);
  grm_plot(args);
  sleep(3);

  /* Draw the bar plot with colorlist */
  grm_args_delete(args);
  args = grm_args_new();
  grm_args_push(args, "y", "nD", n_y, y);
  grm_args_push(args, "kind", "s", "barplot");
  grm_args_push(args, "c", "nI", n_y, c);
  grm_plot(args);
  sleep(3);
  /* Or */
  grm_args_push(args, "c", "nD", 3 * n_y, c_rgb);
  grm_plot(args);
  sleep(3);

  /* Draw a 2D bar plot lined */
  grm_args_delete(args);
  args = grm_args_new();
  for (i = 0; i < N_SERIES; i++)
    {
      series[i] = grm_args_new();
    }
  grm_args_push(series[0], "y", "nD", n_yy, yy1);
  grm_args_push(series[0], "ylabels", "nS", n_yy, yy1_labels);
  grm_args_push(series[1], "y", "nD", n_yy, yy2);
  grm_args_push(series[1], "ylabels", "nS", n_yy, yy2_labels);
  grm_args_push(series[2], "y", "nD", n_yy, yy3);
  grm_args_push(series[2], "ylabels", "nS", n_yy, yy3_labels);

  grm_args_push(args, "kind", "s", "barplot");
  grm_args_push(args, "style", "s", "lined");
  grm_args_push(args, "series", "nA", N_SERIES, series);
  grm_plot(args);
  sleep(3);

  /* Draw a 2D bar plot stacked */
  grm_args_push(args, "style", "s", "stacked");
  grm_plot(args);
  sleep(3);

  /* Draw a 2D bar plot with colorlist */
  grm_args_delete(args);
  args = grm_args_new();
  for (i = 0; i < N_SERIES; i++)
    {
      series[i] = grm_args_new();
    }
  grm_args_push(series[0], "y", "nD", n_yy, yy1);
  grm_args_push(series[0], "c", "nI", n_yy, c);
  grm_args_push(series[1], "y", "nD", n_yy, yy2);
  grm_args_push(series[2], "y", "nD", n_yy, yy3);

  grm_args_push(args, "kind", "s", "barplot");
  grm_args_push(args, "style", "s", "stacked");
  grm_args_push(args, "series", "nA", N_SERIES, series);
  grm_plot(args);
  sleep(3);

  /* Draw a 2D bar plot stacked with positive and negative values */
  /* The positive and negative values are stacked separately */
  grm_args_delete(args);
  args = grm_args_new();

  grm_args_push(args, "y", "nD", n_yy_pos_neg, yy_pos_neg);
  grm_args_push(args, "kind", "s", "barplot");
  grm_args_push(args, "style", "s", "stacked");
  grm_args_push(args, "ylabels", "nS", n_yy_pos_neg, yy_pos_neg_labels);
  grm_plot(args);
  sleep(3);

  /* Draw a bar plot that is lined and stacked with inner color list (rgb) */
  grm_args_delete(args);
  args = grm_args_new();
  for (i = 0; i < N_SERIES; i++)
    {
      series[i] = grm_args_new();
    }
  for (i = 0; i < INNER_N_SERIES; i++)
    {
      inner_series[i] = grm_args_new();
    }

  grm_args_push(inner_series[0], "y", "nD", n_inner_yy2_1, inner_yy2_1);
  grm_args_push(inner_series[0], "c", "nD", n_inner_c_rgb, inner_c_rgb);
  grm_args_push(inner_series[1], "y", "nD", 1, inner_yy2_2);
  grm_args_push(inner_series[2], "y", "nD", 1, inner_yy2_3);

  grm_args_push(series[0], "y", "nD", n_yy, yy1);
  grm_args_push(series[0], "c", "nD", 3 * n_yy, c_rgb);
  grm_args_push(series[1], "inner_series", "nA", INNER_N_SERIES, inner_series);
  grm_args_push(series[1], "c", "nD", 3 * n_yy, c_rgb);
  grm_args_push(series[2], "y", "nD", n_yy, yy3);
  grm_args_push(series[2], "c", "nD", 3 * n_yy, c_rgb);

  grm_args_push(args, "kind", "s", "barplot");
  grm_args_push(args, "style", "s", "lined");
  grm_args_push(args, "series", "nA", N_SERIES, series);

  grm_plot(args);
  sleep(3);

  /* Draw a bar plot that is lined and stacked with inner color list and ylabels */
  grm_args_delete(args);
  args = grm_args_new();
  for (i = 0; i < N_SERIES; i++)
    {
      series[i] = grm_args_new();
    }
  for (i = 0; i < INNER_N_SERIES; i++)
    {
      inner_series[i] = grm_args_new();
    }

  grm_args_push(inner_series[0], "y", "nD", n_inner_yy2_1, inner_yy2_1);
  grm_args_push(inner_series[0], "c", "nI", n_inner_c, inner_c);
  grm_args_push(inner_series[1], "y", "nD", 1, inner_yy2_2);
  grm_args_push(inner_series[2], "y", "nD", 1, inner_yy2_3);

  grm_args_push(series[0], "y", "nD", n_yy, yy1);
  grm_args_push(series[0], "ylabels", "nS", n_yy, yy1_labels);
  grm_args_push(series[1], "inner_series", "nA", INNER_N_SERIES, inner_series);
  /* ylabels for series containing inner_series can alternatively be put in each inner_series */
  grm_args_push(series[1], "ylabels", "nS", n_yy + 1, yy2_labels_for_inner);
  grm_args_push(series[2], "y", "nD", n_yy, yy3);
  grm_args_push(series[2], "ylabels", "nS", n_yy, yy3_labels);

  grm_args_push(args, "kind", "s", "barplot");
  grm_args_push(args, "style", "s", "lined");
  grm_args_push(args, "series", "nA", N_SERIES, series);

  grm_plot(args);
  sleep(3);

  grm_args_delete(args);
  grm_finalize();
}
