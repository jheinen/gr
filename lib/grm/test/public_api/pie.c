#include <stdio.h>

#include "grm.h"

#define array_size(x) (sizeof(x) / sizeof((x)[0]))

static void test_quiver(void)
{
  const double x[] = {188.6, 107.8, 100.3, 99.0};
  const char *labels[] = {"Czech Republic", "Austria", "Romania", "Germany"};
  grm_args_t *args;

  printf("plot a pie chart with x\n");
  args = grm_args_new();
  grm_args_push(args, "x", "nD", array_size(x), x);
  grm_args_push(args, "kind", "s", "pie");
  grm_args_push(args, "labels", "nS", array_size(labels), labels);
  grm_args_push(args, "title", "s", "Beer consumption per capita in 2018 (litres per year)");
  grm_plot(args);

  printf("Press any key to continue...\n");
  getchar();

  grm_args_delete(args);
}


int main(void)
{
  test_quiver();
  grm_finalize();

  return 0;
}
