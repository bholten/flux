#include <stdio.h>
#include <stdlib.h>

#include "interpreter.h"

int main(int argc, const char **argv) {
  interpreter *interp = interpreter_new();

  if (!interp) {
    fprintf(stderr, "Failed to initialize interpreter\n");
    exit(1);
  }

  if (argc < 2) {
    fprintf(stderr, "[flux] Need a file\n");
    interpreter_delete(interp);
    return EXIT_FAILURE;
  }

  const char *file = argv[1];

  if (argc > 2) {
    if (interpreter_setup_environment(interp, argc - 2, argv + 2) !=
        INTERP_OK) {
      fprintf(stderr, "[flux] error setting global variables\n");
      return EXIT_FAILURE;
    }
  }

  if (interpreter_eval_file(interp, file) != INTERP_OK) {
    fprintf(stderr, "[flux] error evaluating %s\n", file);
    interpreter_print_error(interp);
    return EXIT_FAILURE;
  }

  if (interpreter_execute(interp) != INTERP_OK) {
    fprintf(stderr, "[flux] error executing run %s\n", file);
    interpreter_print_error(interp);
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
