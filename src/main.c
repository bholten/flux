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

  if (!flux_interpret(f, file)) {
    flux_get_error(f, file);
    flux_delete(f);
    return EXIT_FAILURE;
  }

  flux_result result = sync_requests(f);

  if (result == FLUX_OK) {
    flux_send(f);
  }

  /*
  flux_set_url(req, "https://jsonplaceholder.typicode.com/posts/1");
  flux_set_verb(req, GET);
  flux_set_header(req, "Content-Type: application/json");
  flux_set_header(req, "Accept-Encoding: gzip");
  flux_send(req);
  */

  flux_delete(f);

  return EXIT_SUCCESS;
}
