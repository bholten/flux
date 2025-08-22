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

struct interpreter {
  Jim_Interp *interp;
};

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
  http *h = (http *)Jim_CmdPrivData(interp);

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
    int rc = http_send(h);
    if (rc != HTTP_OK) return JIM_ERR;

    return JIM_OK;
  } else if (strcmp(sub, "cleanup") == 0) {
    if (http_alive(h)) {
      http_reset(h);
    }

    return JIM_OK;

  } else if (strcmp(sub, "reset") == 0) {
    if (h) {
      http_reset(h);
    }

    return JIM_OK;
  } else if (strcmp(sub, "header") == 0) {
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
  } else if (strcmp(sub, "body") == 0) {
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
  http *h = (http *)private_data;

  if (!h) {
    return;
  }

  http_delete(h);
  Jim_Free(h);
}

static int flux_init_command(Jim_Interp *interp, int argc,
                             Jim_Obj *const *argv) {
  (void)argc;
  (void)argv;

  http *h = http_new();
  if (http_init(h) != HTTP_OK) {
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

  if (Jim_Eval(jim, (char *)src_dsl_tcl) != JIM_OK) {
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
    printf("setting arg %s\n", argv[n]);
    Jim_Obj *obj = Jim_NewStringObj(interp->interp, argv[n], -1);
    Jim_ListAppendElement(interp->interp, list_obj, obj);
  }

  printf("setting argc %d\n", argc);
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
