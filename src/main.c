#include <stdio.h>
#include <stdlib.h>

#include "flux.h"

int main(int argc, const char **argv) {
  flux *req = flux_new();

  if (argc < 2) {
    fprintf(stderr, "[flux] Need a file\n");
    return EXIT_FAILURE;
  }

  const char *file = argv[1];

  if (!flux_interpret(req, file)) {
    flux_get_error(req, file);
    flux_delete(req);
    return EXIT_FAILURE;
  }

  /*
  flux_set_url(req, "https://jsonplaceholder.typicode.com/posts/1");
  flux_set_verb(req, GET);
  flux_set_header(req, "Content-Type: application/json");
  flux_set_header(req, "Accept-Encoding: gzip");
  flux_send(req);
  */

  flux_delete(req);

  return EXIT_SUCCESS;
}
