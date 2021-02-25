#ifdef __unix__
#define _XOPEN_SOURCE 500
#endif
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef _MSC_VER
#include <unistd.h>
#else
#include <io.h>
#endif

#include "grm.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static void test_subplots(void)
{
  double plots[4][2][1000];
  int n = sizeof(plots[0][0]) / sizeof(plots[0][0][0]);
  grm_args_t *args, *subplots[4];
  int i, j;

  printf("filling argument container...\n");

  for (i = 0; i < 2; ++i)
    {
      for (j = 0; j < n; ++j)
        {
          plots[i][0][j] = j * 2 * M_PI / n;
          plots[i][1][j] = sin((j * (i + 1) * 2) * M_PI / n);
        }
    }
  for (i = 0; i < 2; ++i)
    {
      for (j = 0; j < n; ++j)
        {
          plots[2 + i][0][j] = j * 2 * M_PI / n;
          plots[2 + i][1][j] = cos((j * (i + 1) * 2) * M_PI / n);
        }
    }

  for (i = 0; i < 4; ++i)
    {
      subplots[i] = grm_args_new();
      grm_args_push(subplots[i], "x", "nD", n, plots[i][0]);
      grm_args_push(subplots[i], "y", "nD", n, plots[i][1]);
      grm_args_push(subplots[i], "subplot", "dddd", 0.5 * (i % 2), 0.5 * (i % 2 + 1), 0.5 * (i / 2), 0.5 * (i / 2 + 1));
    }

  args = grm_args_new();
  grm_args_push(args, "subplots", "nA", 4, subplots);

  printf("plotting data...\n");

  grm_plot(args);
  printf("Press any key to continue...\n");
  getchar();

  sleep(10);

  grm_plot(args);
  printf("Press any key to continue...\n");
  getchar();

  grm_args_delete(args);
}

static void test_plot(void)
{
  test_subplots();
  grm_finalize();
}

int main(void)
{
  test_plot();

  return 0;
}
