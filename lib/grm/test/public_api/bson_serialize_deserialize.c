#include <grm.h>

int main(void)
{
  int i = 1986;
  double d = 5.05;
  char c = 'y';
  int y_i[] = {1986, 1986, 1986};
  double y_d[] = {5.05, 5.05, 5.05};
  char *y_s[] = {"eins", "zwei", "drei"};
  char y_c[] = {'a', 'b', 'c', '\0'};
  grm_args_t *args, *subarg, *subargs[2];

  args = grm_args_new();
  subarg = grm_args_new();
  subargs[0] = grm_args_new();
  subargs[1] = grm_args_new();

  //    grm_args_push(args, "hello", "s", "world");
  //    grm_args_push(args, "y", "i", i);
  //    grm_args_push(args, "y", "d", d);
  //    grm_args_push(args, "y", "c", c);
  //
  //    grm_args_push(args, "y", "nI", sizeof(y_i) / sizeof(y_i[0]), y_i);
  //    grm_args_push(args, "y", "nD", sizeof(y_d) / sizeof(y_d[0]), y_d);
  //    grm_args_push(args, "y", "nS", sizeof(y_s) / sizeof(y_s[0]), y_s);
  //    grm_args_push(args, "y", "nC", 4, y_c);
  //
  //    grm_args_push(subarg, "hello", "s", "world");
  //    grm_args_push(args, "sub", "a", subarg);
  //
  //  grm_args_push(subargs[0], "hello", "s", "world");
  //  grm_args_push(subargs[1], "y", "d", d);
  //  grm_args_push(args, "subs", "nA", 2, subargs);

  grm_args_push(subargs[0], "eins", "s", "world");
  grm_args_push(subargs[0], "zwei", "s", "world");
  grm_args_push(subargs[1], "x", "d", d);
  grm_args_push(subargs[1], "y", "d", d);
  grm_args_push(args, "subs", "nA", 2, subargs);

  grm_dump(args, stdout);
  //    grm_dump_json(args, stdout);
  //    grm_dump_bson(args, stdout);
  grm_dump_bson_and_parse(args, stdout);
  //    grm_args_delete(args);

  return 0;
}
