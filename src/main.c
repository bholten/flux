#include <stdio.h>
#include <stdlib.h>

#include "flux.h"

int main(int argc, const char **argv) {
  flux *f = flux_new();

  if (argc < 2) {
    fprintf(stderr, "[flux] Need a file\n");
    flux_delete(f);
    return EXIT_FAILURE;
  }

  const char *file = argv[1];
  printf("[flux] file %s\n", file);

  if (!flux_interpret(f, file)) {
    printf("[flux] error\n");
    flux_get_error(f, file);
    flux_delete(f);
    return EXIT_FAILURE;
  }

  flux_result result = sync_requests(f);

  if (result != FLUX_OK) {
    fprintf(stderr, "[flux] error constructing requests\n");
    flux_get_error(f, file);
  }

  flux_delete(f);

  return EXIT_SUCCESS;
}
