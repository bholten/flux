#include <stdlib.h>
#include <string.h>

#include <curl/curl.h>
#include <curl/easy.h>
#include <tcl/tcl.h>
#include <tcl/tclDecls.h>

#include "dsl.h"
#include "flux.h"

size_t write_callback(char *ptr, size_t size, size_t nmemb, void *userdata) {

  return size;
}

struct flux {
  CURL *curl;
  Tcl_Interp *interp;
  struct curl_slist *headers;
};

flux *flux_new(void) {
  flux *req = calloc(1, sizeof(*req));

  if (!req) return NULL;

  CURL *curl = curl_easy_init();

  // clang-format off
  #if DEBUG
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
  #endif
  // clang-format on

  if (!curl) {
    free(req);
    return NULL;
  }

  Tcl_Interp *interp = Tcl_CreateInterp();

  if (!interp) {
    curl_easy_cleanup(curl);
    free(req);
    return NULL;
  }

  if (Tcl_Eval(interp, (char *)src_dsl_tcl) != TCL_OK) {
    Tcl_DeleteInterp(interp);
    Tcl_Finalize();
    curl_easy_cleanup(curl);
    free(req);
    return NULL;
  }

  req->curl = curl;
  req->interp = interp;
  req->headers = NULL;

  return req;
}

void flux_delete(flux *req) {
  curl_easy_cleanup(req->curl);
  curl_slist_free_all(req->headers);
  Tcl_DeleteInterp(req->interp);
  Tcl_Finalize();
  free(req);
}

void flux_set_url(flux *req, const char *url) {
  curl_easy_setopt(req->curl, CURLOPT_URL, url);
}

void flux_set_header(flux *req, const char *header) {
  req->headers = curl_slist_append(req->headers, header);
  curl_easy_setopt(req->curl, CURLOPT_HTTPHEADER, req->headers);
}

int flux_send(flux *req) {
  CURLcode result = curl_easy_perform(req->curl);

  if (CURLE_OK == result) {
    curl_easy_reset(req->curl);
    return 0;
  }

  curl_easy_reset(req->curl);
  return -1;
}

void flux_set_verb(flux *req, verb v) {
  switch (v) {
  case GET: curl_easy_setopt(req->curl, CURLOPT_CUSTOMREQUEST, "GET"); return;
  case PUT: curl_easy_setopt(req->curl, CURLOPT_CUSTOMREQUEST, "PUT"); return;
  case POST: curl_easy_setopt(req->curl, CURLOPT_CUSTOMREQUEST, "POST"); return;
  case DELETE:
    curl_easy_setopt(req->curl, CURLOPT_CUSTOMREQUEST, "DELETE");
    return;
  case PATCH:
    curl_easy_setopt(req->curl, CURLOPT_CUSTOMREQUEST, "PATCH");
    return;
  case OPTION:
    curl_easy_setopt(req->curl, CURLOPT_CUSTOMREQUEST, "OPTION");
    return;
  default: return;
  }
}

int flux_interpret(flux *req, const char *file) {
  if (Tcl_EvalFile(req->interp, file) != TCL_OK) {
    return 0;
  }

  return 1;
}

void flux_get_error(flux *req, const char *filename) {
  int line = Tcl_GetErrorLine(req->interp);
  const char *err = Tcl_GetStringResult(req->interp);
  fprintf(stderr, "[flux] %s:%i %s\n", filename, line, err);
}
