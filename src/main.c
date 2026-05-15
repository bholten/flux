#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "interpreter.h"

static void print_usage(const char *prog) {
  fprintf(stderr, "Usage: %s [OPTIONS] <file.lcl>\n", prog);
  fprintf(stderr, "\nOptions:\n");
  fprintf(stderr, "  --junit     Output results in JUnit XML format\n");
  fprintf(stderr, "  --help      Show this help message\n");
}

int main(int argc, const char **argv) {
  const char *file = NULL;
  int output_junit = 0;
  int i;

  for (i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--junit") == 0) {
      output_junit = 1;
    } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
      print_usage(argv[0]);
      return EXIT_SUCCESS;
    } else if (argv[i][0] == '-') {
      fprintf(stderr, "[flux] Unknown option: %s\n", argv[i]);
      print_usage(argv[0]);
      return EXIT_FAILURE;
    } else {
      file = argv[i];
      break;
    }
  }

  if (!file) {
    fprintf(stderr, "[flux] Need a file\n");
    print_usage(argv[0]);
    return EXIT_FAILURE;
  }

  interpreter *interp = interpreter_new();

  if (!interp) {
    fprintf(stderr, "Failed to initialize interpreter\n");
    return EXIT_FAILURE;
  }

  if (interpreter_bind_cli_opts(interp, output_junit ? "junit" : NULL) !=
      INTERP_OK) {
    fprintf(stderr, "[flux] error binding CLI options\n");
    interpreter_delete(interp);
    return EXIT_FAILURE;
  }

  if (interpreter_eval_file(interp, file) != INTERP_OK) {
    interpreter_delete(interp);
    return EXIT_FAILURE;
  }

  interp_result result = interpreter_execute(interp);

  interpreter_delete(interp);

  if (result != INTERP_OK) {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
