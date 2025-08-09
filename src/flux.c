#include <stdlib.h>
#include <string.h>

#include <curl/curl.h>
#include <curl/easy.h>
#include <tcl/tcl.h>
#include <tcl/tclDecls.h>

#include "dsl.h"
#include "flux.h"

#define FLUX_CURRENT_WORKSPACE_VAR "::___flux::main::current_workspace"
#define FLUX_WORKSPACES_VAR "::___flux::main::workspaces"

#define PRELOAD_SCRIPT                                                         \
  "namespace eval ___flux::main {"                                             \
  "  source \"%s\""                                                            \
  "}"

static const size_t FLUX_MAIN_LEN = strlen("::___flux::main::") + 1;
static const size_t FLUX_PRELOAD_LEN = strlen(PRELOAD_SCRIPT) + 1;

size_t write_callback(char *ptr, size_t size, size_t nmemb, void *userdata) {

  return size;
}

struct flux {
  CURL *curl;
  Tcl_Interp *interp;
  struct curl_slist *headers;
};

flux *flux_new(void) {
  flux *f = calloc(1, sizeof(*f));

  if (!f) return NULL;

  CURL *curl = curl_easy_init();

#if DEBUG
  curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
#endif

  if (!curl) {
    free(f);
    return NULL;
  }

  Tcl_Interp *interp = Tcl_CreateInterp();

  if (!interp) {
    curl_easy_cleanup(curl);
    free(f);
    return NULL;
  }

  if (Tcl_Eval(interp, (char *)src_dsl_tcl) != TCL_OK) {
    Tcl_DeleteInterp(interp);
    Tcl_Finalize();
    curl_easy_cleanup(curl);
    free(f);
    return NULL;
  }

  f->curl = curl;
  f->interp = interp;
  f->headers = NULL;

  return f;
}

void flux_delete(flux *f) {
  curl_easy_cleanup(f->curl);
  curl_slist_free_all(f->headers);
  Tcl_DeleteInterp(f->interp);
  Tcl_Finalize();
  free(f);
}

void flux_set_url(flux *f, const char *url) {
  curl_easy_setopt(f->curl, CURLOPT_URL, url);
}

void flux_set_header(flux *f, const char *header) {
  f->headers = curl_slist_append(f->headers, header);
  curl_easy_setopt(f->curl, CURLOPT_HTTPHEADER, f->headers);
}

flux_result flux_send(flux *f) {
  CURLcode result = curl_easy_perform(f->curl);

  if (CURLE_OK == result) {
    curl_easy_reset(f->curl);

#if DEBUG
    curl_easy_setopt(f->curl, CURLOPT_VERBOSE, 1L);
#endif

    return FLUX_OK;
  }

  curl_easy_reset(f->curl);

#if DEBUG
  curl_easy_setopt(f->curl, CURLOPT_VERBOSE, 1L);
#endif

  return FLUX_ERROR;
}

void flux_set_verb(flux *f, const char *verb) {
  curl_easy_setopt(f->curl, CURLOPT_CUSTOMREQUEST, verb);
}

void flux_set_body(flux *f, const char *body) {
  curl_easy_setopt(f->curl, CURLOPT_POSTFIELDS, body);
}

flux_result flux_interpret(flux *f, const char *file) {
  size_t script_len = FLUX_PRELOAD_LEN + strlen(file);
  char script[script_len];
  snprintf(script, script_len, PRELOAD_SCRIPT, file);

  int code = Tcl_Eval(f->interp, script);

  if (code != TCL_OK) {
    return FLUX_ERROR;
  }

  if (Tcl_EvalFile(f->interp, file) != TCL_OK) {
    return FLUX_ERROR;
  }

  return FLUX_OK;
}

void flux_get_error(flux *f, const char *filename) {
  int line = Tcl_GetErrorLine(f->interp);
  const char *err = Tcl_GetStringResult(f->interp);
  fprintf(stderr, "[flux] Error  %s:%i %s\n", filename, line, err);
}

int create_request_command(ClientData cd, Tcl_Interp *interp, int argc,
                           const char **argv) {

  return TCL_OK;
}

flux_result sync_requests(flux *f) {
  Tcl_Obj *workspaces =
      Tcl_GetVar2Ex(f->interp, FLUX_WORKSPACES_VAR, NULL, TCL_LEAVE_ERR_MSG);
  int size;

  if (Tcl_ListObjLength(f->interp, workspaces, &size) != TCL_OK) {
    return FLUX_ERROR;
  }

  printf("[flux] found %d workspaces\n", size);

  for (int i = 0; i < size; i++) {
    Tcl_Obj *namespace_obj;

    if (Tcl_ListObjIndex(f->interp, workspaces, i, &namespace_obj) != TCL_OK) {
      return FLUX_ERROR;
    }

    const char *namespace = Tcl_GetString(namespace_obj);

    if (!namespace) {
      return FLUX_ERROR;
    }

    const size_t namespace_len = strlen(namespace) + FLUX_MAIN_LEN + 10;
    char buff[namespace_len];
    snprintf(buff, namespace_len, "::___flux::main::%s::requests", namespace);

    Tcl_Obj *requests = Tcl_GetVar2Ex(f->interp, buff, NULL, TCL_LEAVE_ERR_MSG);
    int req_count;

    if (Tcl_ListObjLength(f->interp, requests, &req_count) != TCL_OK) {
      return FLUX_ERROR;
    }

    printf("[flux] [%s] found %d requests\n", namespace, req_count);

    for (int j = 0; j < req_count; j++) {
      Tcl_Obj *request_obj;

      if (Tcl_ListObjIndex(f->interp, requests, j, &request_obj) != TCL_OK) {
        return FLUX_ERROR;
      }

      Tcl_Obj *url_obj;
      Tcl_Obj *verb_obj;
      Tcl_Obj *data_obj;
      Tcl_Obj *headers_obj;

      const char *url = NULL;

      if (Tcl_DictObjGet(f->interp, request_obj, Tcl_NewStringObj("url", -1),
                         &url_obj) == TCL_OK &&
          url_obj != NULL) {
        url = Tcl_GetString(url_obj);
        printf("[flux] [%s] setting url %s\n", namespace, url);
        flux_set_url(f, url);
      }

      if (!url) return FLUX_ERROR;

      if (Tcl_DictObjGet(f->interp, request_obj, Tcl_NewStringObj("verb", -1),
                         &verb_obj) == TCL_OK &&
          verb_obj != NULL) {
        const char *verb = Tcl_GetString(verb_obj);

        printf("[flux] [%s] setting verb %s\n", namespace, verb);
        flux_set_verb(f, verb);
      }

      if (Tcl_DictObjGet(f->interp, request_obj, Tcl_NewStringObj("data", -1),
                         &data_obj) == TCL_OK &&
          data_obj != NULL) {
        const char *data = Tcl_GetString(data_obj);

        printf("[flux] [%s] setting body %s\n", namespace, data);
        flux_set_body(f, data);
      }

      if (Tcl_DictObjGet(f->interp, request_obj,
                         Tcl_NewStringObj("headers", -1),
                         &headers_obj) == TCL_OK &&
          headers_obj != NULL) {
        int headers_len;

        if (Tcl_ListObjLength(f->interp, headers_obj, &headers_len) != TCL_OK) {
          return TCL_ERROR;
        }

        for (int k = 0; k < headers_len; k++) {
          Tcl_Obj *header_obj;

          if (Tcl_ListObjIndex(f->interp, headers_obj, k, &header_obj) !=
              TCL_OK) {
            return TCL_ERROR;
          }

          const char *header = Tcl_GetString(header_obj);
          printf("[flux] [%s] [%s] adding header %s\n", namespace, url, header);
          flux_set_header(f, header);
        }
      }

      printf("[flux] [%s] [%s] sending request\n", namespace, url);
      flux_send(f);
    }
  }

  return FLUX_OK;
}
