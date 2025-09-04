#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <jim.h>

#include "dsl.h"
#include "http.h"
#include "interpreter.h"

#define NAMESPACE_EVAL_PREAMBLE "namespace eval ::flux { %s }"

#define PRELOAD_SCRIPT                                                         \
  "namespace eval ::flux {"                                                    \
  "  source \"%s\""                                                            \
  "}"

#define RUN_CMD "::flux::run"

static const size_t JIM_NAMESPACE_EVAL_PREAMBLE_LEN =
    strlen(NAMESPACE_EVAL_PREAMBLE) + 1;
static const size_t JIM_PRELOAD_LEN = strlen(PRELOAD_SCRIPT) + 1;

struct dstr {
  char *buf;
  size_t len;
  size_t cap;
};

static int dstr_grow(struct dstr *s, size_t extra) {
  size_t need = s->len + extra;
  if (need > s->cap) {
    size_t cap = s->cap ? s->cap : 8192;

    while (cap < need) {
      cap *= 2;
    }

    char *nbuf = Jim_Realloc(s->buf, cap);

    if (!nbuf) return 0;

    s->buf = nbuf;
    s->cap = cap;
  }

  return 1;
}

static int dstr_append(struct dstr *s, const char *p, size_t n) {
  if (!dstr_grow(s, n)) {
    return 0;
  }

  memcpy(s->buf + s->len, p, n);
  s->len += n;
  return 1;
}

static void dstr_free(struct dstr *s) {
  if (s->buf) {
    Jim_Free(s->buf);
  }

  s->buf = NULL;
  s->len = 0;
  s->cap = 0;
}

struct http_ctx {
  http *http;
  Jim_Obj *write_command;
  Jim_Obj *header_command;
  Jim_Interp *interp;

  struct dstr body;
  struct dstr headers;
  long status;
  char *ct;
  char *eff_url;
  double total_time;
};

struct interpreter {
  Jim_Interp *interp;
};
/*
static int eval_prefix(Jim_Interp *interp, Jim_Obj *prefix, Jim_Obj *arg1) {
  Jim_Obj *call = Jim_DuplicateObj(interp, prefix);
  Jim_IncrRefCount(call);
  printf("Hit eval prefix\n");
  Jim_ListAppendElement(interp, call, arg1);

  int rc = Jim_EvalObj(interp, call);
  Jim_DecrRefCount(interp, call);
  return rc;
}
*/
static size_t write_cb(char *ptr, size_t size, size_t nmeb, void *user_data) {
  struct http_ctx *ctx = (struct http_ctx *)user_data;
  size_t n = size * nmeb;

  if (!ctx) return 0;
  if (!dstr_append(&ctx->body, ptr, n)) return 0;

  return n;
}

static const char *j_str(Jim_Obj *obj) {
  int len;
  return Jim_GetString(obj, &len);
}

static int j_err(Jim_Interp *interp, const char *msg) {
  Jim_SetResultString(interp, msg, -1);
  return JIM_ERR;
}

static void print_error(Jim_Interp *interp) {
  Jim_Obj *result = Jim_GetResult(interp);
  const char *str = j_str(result);
  fprintf(stderr, "[flux] interpreter error: %s\n", str);
}

static int flux_obj_command(Jim_Interp *interp, int argc,
                            Jim_Obj *const *argv) {
  struct http_ctx *h_ctx = (struct http_ctx *)Jim_CmdPrivData(interp);
  http *h = h_ctx->http;

  if (!h || !http_alive(h)) return j_err(interp, "curl handle is closed");

  if (argc < 2) {
    Jim_WrongNumArgs(interp, 1, argv, "subcommand ?args?");
    return JIM_ERR;
  }

  const char *sub = j_str(argv[1]);

  if (strcmp(sub, "configure") == 0) {
    if (((argc - 2) % 2 != 0)) {
      return j_err(interp, "configure expects option/value pairs");
    }

    for (int i = 2; i < argc; i += 2) {
      const char *opt = j_str(argv[i]);

      if (strcmp(opt, "-verbose") == 0) {
        long v;

        if (Jim_GetLong(interp, argv[i + 1], &v) != JIM_OK) {
          return JIM_ERR;
        }

        http_result rc = http_set_option_verbose(h, v);

        if (rc != HTTP_OK) return j_err(interp, "http_set_verbose failed");
      }

      else if (strcmp(opt, "-url") == 0) {
        int len;
        const char *url = Jim_GetString(argv[i + 1], &len);

        http_result rc = http_set_url(h, url);
        if (rc != HTTP_OK) return j_err(interp, "flux_set_url failed");
      }

      else if (strcmp(opt, "-verb") == 0) {
        int len;
        const char *verb = Jim_GetString(argv[i + 1], &len);

        http_result rc = http_set_verb(h, verb);
        if (rc != HTTP_OK) return j_err(interp, "flux_set_verb failed");
      }

      else if (strcmp(opt, "-accept_timeout_ms") == 0) {
        long v;

        if (Jim_GetLong(interp, argv[i + 1], &v) != JIM_OK) {
          return JIM_ERR;
        }

        http_result rc = http_set_option_accept_timeout_ms(h, v);

        if (rc != HTTP_OK) {
          return j_err(interp, "flux_set_option_accept_timeous_ms failed");
        }
      }

      else if (strcmp(opt, "-connection_timeout_ms") == 0) {
        long v;

        if (Jim_GetLong(interp, argv[i + 1], &v) != JIM_OK) {
          return JIM_ERR;
        }

        http_result rc = http_set_option_connection_timeout_ms(h, v);

        if (rc != HTTP_OK) {
          return j_err(interp, "flux_set_option_connection_timeous_ms failed");
        }
      }

      else if (strcmp(opt, "-interface") == 0) {
        int len;

        const char *interface = Jim_GetString(argv[i + 1], &len);

        http_result rc = http_set_option_interface(h, interface);

        if (rc != HTTP_OK) {
          return j_err(interp, "http_set_option_interface failed");
        }
      }

      else if (strcmp(opt, "-low_speed_limit") == 0) {
        long v;
        if (Jim_GetLong(interp, argv[i + 1], &v) != JIM_OK) {
          return JIM_ERR;
        }
        http_result rc = http_set_option_low_speed_limit(h, v);
        if (rc != HTTP_OK) {
          return j_err(interp, "http_set_option_low_speed_limit failed");
        }
      }

      else if (strcmp(opt, "-low_speed_time") == 0) {
        long v;
        if (Jim_GetLong(interp, argv[i + 1], &v) != JIM_OK) {
          return JIM_ERR;
        }
        http_result rc = http_set_option_low_speed_time(h, v);
        if (rc != HTTP_OK) {
          return j_err(interp, "http_set_option_low_speed_time failed");
        }
      }

      else if (strcmp(opt, "-tcp_keep_alive") == 0) {
        long v;
        if (Jim_GetLong(interp, argv[i + 1], &v) != JIM_OK) {
          return JIM_ERR;
        }
        http_result rc = http_set_option_tcp_keep_alive(h, v);
        if (rc != HTTP_OK) {
          return j_err(interp, "http_set_option_tcp_keep_alive failed");
        }
      }

      else if (strcmp(opt, "-tcp_keep_idle") == 0) {
        long v;
        if (Jim_GetLong(interp, argv[i + 1], &v) != JIM_OK) {
          return JIM_ERR;
        }
        http_result rc = http_set_option_tcp_keep_idle(h, v);
        if (rc != HTTP_OK) {
          return j_err(interp, "http_set_option_tcp_keep_idle failed");
        }
      }

      else if (strcmp(opt, "-tcp_keep_intvl") == 0) {
        long v;
        if (Jim_GetLong(interp, argv[i + 1], &v) != JIM_OK) {
          return JIM_ERR;
        }
        http_result rc = http_set_option_tcp_keep_intvl(h, v);
        if (rc != HTTP_OK) {
          return j_err(interp, "http_set_option_tcp_keep_intvl failed");
        }
      }

      else if (strcmp(opt, "-accept_encoding") == 0) {
        int len;

        const char *accept_encoding = Jim_GetString(argv[i + 1], &len);

        http_result rc = http_set_option_accept_encoding(h, accept_encoding);
        if (rc != HTTP_OK) {
          return j_err(interp, "http_set_option_accept_encoding failed");
        }
      }

      else if (strcmp(opt, "-http_version") == 0) {
        long v;
        if (Jim_GetLong(interp, argv[i + 1], &v) != JIM_OK) {
          return JIM_ERR;
        }
        http_result rc = http_set_option_http_version(h, v);
        if (rc != HTTP_OK) {
          return j_err(interp, "http_set_option_http_version failed");
        }
      }

      else if (strcmp(opt, "-ssl_verify_peer") == 0) {
        long v;
        if (Jim_GetLong(interp, argv[i + 1], &v) != JIM_OK) {
          return JIM_ERR;
        }

        http_result rc = http_set_option_ssl_verify_peer(h, v);
        if (rc != HTTP_OK) {
          return j_err(interp, "http_set_option_ssl_verify_peer failed");
        }
      }

      else if (strcmp(opt, "-ssl_verify_host") == 0) {
        long v;
        if (Jim_GetLong(interp, argv[i + 1], &v) != JIM_OK) {
          return JIM_ERR;
        }

        http_result rc = http_set_option_ssl_verify_host(h, v);
        if (rc != HTTP_OK) {
          return j_err(interp, "http_set_option_ssl_verify_host failed");
        }
      }

      else if (strcmp(opt, "-ca_info") == 0) {
        int len;
        const char *ca_info = Jim_GetString(argv[i + 1], &len);
        http_result rc = http_set_option_ca_info(h, (char *)ca_info);
        if (rc != HTTP_OK) {
          return j_err(interp, "http_set_option_ca_info failed");
        }
      }

      return JIM_OK;
    }
  } else if (strcmp(sub, "perform") == 0) {
    h_ctx->body.len = 0;
    h_ctx->headers.len = 0;
    h_ctx->status = 0;
    h_ctx->total_time = 0.0;

    if (h_ctx->ct) {
      Jim_Free(h_ctx->ct);
      h_ctx->ct = NULL;
    }

    if (h_ctx->eff_url) {
      Jim_Free(h_ctx->eff_url);
      h_ctx->eff_url = NULL;
    }

    int rc = http_send(h);
    if (rc != HTTP_OK) return JIM_ERR;

    http_get_info_response_code(h_ctx->http, &h_ctx->status);
    http_get_info_total_time(h_ctx->http, &h_ctx->total_time);

    {
      char *p = NULL;
      http_get_info_content_type(h_ctx->http, &p);
      if (p) {
        size_t len = strlen(p);
        h_ctx->ct = Jim_Alloc(len + 1);
        memcpy(h_ctx->ct, p, len + 1);
      }
    }

    {
      char *p = NULL;
      http_get_info_effective_url(h_ctx->http, &p);
      if (p) {
        size_t len = strlen(p);
        h_ctx->eff_url = Jim_Alloc(len + 1);
        memcpy(h_ctx->eff_url, p, len + 1);
      }
    }

    Jim_SetEmptyResult(interp);
    return JIM_OK;
  }

  else if (strcmp(sub, "result") == 0) {
    Jim_Obj *dict = Jim_NewDictObj(interp, NULL, 0);
    Jim_DictAddElement(interp, dict, Jim_NewStringObj(interp, "status", -1),
                       Jim_NewIntObj(interp, h_ctx->status));

    Jim_Obj *body = Jim_NewStringObj(
        interp, h_ctx->body.buf ? h_ctx->body.buf : "", (int)h_ctx->body.len);
    Jim_DictAddElement(interp, dict, Jim_NewStringObj(interp, "body", -1),
                       body);

    Jim_Obj *hdrs =
        Jim_NewStringObj(interp, h_ctx->headers.buf ? h_ctx->headers.buf : "",
                         (int)h_ctx->headers.len);
    Jim_DictAddElement(interp, dict,
                       Jim_NewStringObj(interp, "headers-raw", -1), hdrs);

    Jim_DictAddElement(
        interp, dict, Jim_NewStringObj(interp, "content-type", -1),
        Jim_NewStringObj(interp, h_ctx->ct ? h_ctx->ct : "", -1));
    Jim_DictAddElement(
        interp, dict, Jim_NewStringObj(interp, "effective-url", -1),
        Jim_NewStringObj(interp, h_ctx->eff_url ? h_ctx->eff_url : "", -1));
    Jim_DictAddElement(interp, dict, Jim_NewStringObj(interp, "total-time", -1),
                       Jim_NewDoubleObj(interp, h_ctx->total_time));

    Jim_SetResult(interp, dict);
    return JIM_OK;
  }

  else if (strcmp(sub, "cleanup") == 0) {
    if (http_alive(h)) {
      http_reset(h);
    }

    return JIM_OK;
  }

  else if (strcmp(sub, "reset") == 0) {
    if (h) {
      http_reset(h);
    }

    return JIM_OK;
  }

  else if (strcmp(sub, "header") == 0) {
    if (argc < 3) {
      return j_err(interp, "usage: handler header <header>");
    }

    for (int i = 2; i < argc; i++) {
      int len;
      const char *header = Jim_GetString(argv[i], &len);

      if (http_set_header(h, header) != HTTP_OK) {
        return j_err(interp, "http_set_header failed");
      }
    }

    return JIM_OK;
  }

  else if (strcmp(sub, "body") == 0) {
    if (argc != 3) {
      return j_err(interp, "usage: handler body <body>");
    }

    int len;
    const char *body = Jim_GetString(argv[2], &len);

    if (http_set_body(h, body) != HTTP_OK) {
      return j_err(interp, "http_set_body failed");
    }

    return JIM_OK;
  }

  Jim_SetResultFormatted(interp, "unknown subcommand \"%s\"", sub);
  return JIM_ERR;
}

static void flux_cmd_delete_proc(Jim_Interp *interp, void *private_data) {
  (void)interp;
  struct http_ctx *h_ctx = (struct http_ctx *)private_data;

  if (!h_ctx) {
    return;
  }

  http_delete(h_ctx->http);
  dstr_free(&h_ctx->body);
  dstr_free(&h_ctx->headers);

  if (h_ctx->ct) {
    Jim_Free(h_ctx->ct);
  }

  if (h_ctx->eff_url) {
    Jim_Free(h_ctx->eff_url);
  }

  Jim_Free(h_ctx);
}

static int flux_init_command(Jim_Interp *interp, int argc,
                             Jim_Obj *const *argv) {
  (void)argc;
  (void)argv;
  // TODO not sure if this will work:
  struct http_ctx *h = Jim_Alloc(sizeof(*h));
  memset(h, 0, sizeof(*h));
  h->http = http_new();
  h->header_command = NULL;
  h->write_command = NULL;

  http_set_write_callback(h->http, write_cb);
  http_set_write_data(h->http, h);

  if (http_init(h->http) != HTTP_OK) {
    fprintf(stderr, "[flux] [error] could not initialize curl\n");
    return INTERP_ERR;
  }

  char name[64];
  snprintf(name, sizeof(name), "flux%p", (void *)h);

  Jim_CreateCommand(interp, name, flux_obj_command, h, flux_cmd_delete_proc);
  Jim_SetResultString(interp, name, -1);
  return JIM_OK;
}

interpreter *interpreter_new(void) {
  interpreter *interp = calloc(1, sizeof(*interp));

  if (!interp) {
    return NULL;
  }

  Jim_Interp *jim = Jim_CreateInterp();

  if (!jim) {
    free(interp);
    return NULL;
  }

  Jim_RegisterCoreCommands(jim);
  Jim_InitStaticExtensions(jim);

  if (Jim_Eval(jim, (char *)tcl_dsl_tcl) != JIM_OK) {
    fprintf(stderr, "[flux] failed to load Flux stdlib\n");
    print_error(jim);
    Jim_FreeInterp(jim);
    free(interp);
    return NULL;
  }

  if (Jim_CreateCommand(jim, "flux.init", flux_init_command, NULL, NULL) !=
      JIM_OK) {
    fprintf(stderr, "[flux] failed to create flux.init command\n");
    print_error(jim);
    Jim_FreeInterp(jim);
    free(interp);
    return NULL;
  }

  interp->interp = jim;
  return interp;
}

void interpreter_delete(interpreter *interp) {
  if (!interp) return;

  if (interp->interp) {
    Jim_FreeInterp(interp->interp);
  }

  free(interp);
}

interp_result interpreter_setup_environment(interpreter *interp, int argc,
                                            const char **argv) {
  int n;
  Jim_Obj *list_obj = Jim_NewListObj(interp->interp, NULL, 0);

  for (n = 0; n < argc; n++) {
    Jim_Obj *obj = Jim_NewStringObj(interp->interp, argv[n], -1);
    Jim_ListAppendElement(interp->interp, list_obj, obj);
  }

  if (Jim_SetGlobalVariableStr(interp->interp, "argc",
                               Jim_NewIntObj(interp->interp, argc)) != JIM_OK) {
    return INTERP_ERR;
  }

  if (Jim_SetGlobalVariableStr(interp->interp, "argv", list_obj) != JIM_OK) {
    return INTERP_ERR;
  }

  return INTERP_OK;
}

interp_result interpreter_eval(interpreter *interp, const char *script) {
  if (!interp) return INTERP_ERR;
  if (!script) return INTERP_ERR;

  size_t script_len = JIM_NAMESPACE_EVAL_PREAMBLE_LEN + strlen(script);
  char script_ns[script_len];
  snprintf(script_ns, script_len, PRELOAD_SCRIPT, script);

  if (Jim_Eval(interp->interp, script_ns) != JIM_OK) {
    print_error(interp->interp);
    return INTERP_ERR;
  }

  return INTERP_OK;
}

interp_result interpreter_eval_file(interpreter *interp, const char *filename) {
  if (!interp) return INTERP_ERR;
  if (!filename) return INTERP_ERR;

  size_t script_len = JIM_PRELOAD_LEN + strlen(filename);
  char script_ns[script_len];
  snprintf(script_ns, script_len, PRELOAD_SCRIPT, filename);

  if (Jim_Eval(interp->interp, script_ns) != JIM_OK) {
    print_error(interp->interp);
    return INTERP_ERR;
  }

  return INTERP_OK;
}

interp_result interpreter_execute(interpreter *interp) {
  if (Jim_Eval(interp->interp, RUN_CMD) != JIM_OK) {
    print_error(interp->interp);
    return INTERP_ERR;
  }

  return INTERP_OK;
}

void interpreter_print_error(interpreter *interp) {
  print_error(interp->interp);
}
