#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// clang-format off
#include <lcl.h>
#include <lcl-crypto.h>
#include <lcl-curl.h>
#include <lcl-io.h>
#include <lcl-json.h>
#include <lcl-regex.h>
#include <lcl-time.h>
// clang-format on

#include "generated/flux.h"
#include "interpreter.h"

static const lcl_embedded_lib flux_lib = {"lib/flux.lcl", lib_flux_lcl,
                                          sizeof(lib_flux_lcl)};

struct interpreter {
  lcl_interp *interp;
};

static void print_lcl_error(lcl_interp *interp) {
  const char *file = lcl_interp_error_file(interp);
  int line = lcl_interp_error_line(interp);
  const char *msg = lcl_interp_error_msg(interp);

  if (file && msg) {
    fprintf(stderr, "[flux] error at %s:%d: %s\n", file, line, msg);
  } else if (msg) {
    fprintf(stderr, "[flux] error: %s\n", msg);
  }
}

interpreter *interpreter_new(void) {
  interpreter *interp = calloc(1, sizeof(*interp));
  if (!interp) {
    return NULL;
  }

  lcl_interp *lcl = lcl_interp_new();

  if (!lcl) {
    free(interp);
    return NULL;
  }

  interp->interp = lcl;
  lcl_register_core(lcl);
  lcl_register_io(lcl);
  lcl_register_json(lcl);
  lcl_register_curl(lcl);
  lcl_register_crypto(lcl);
  lcl_register_time(lcl);
  lcl_register_regex(lcl);

  if (lcl_register_embedded_lib(lcl, &flux_lib) != 0) {
    fprintf(stderr, "Warning: Flux library could not be loaded\n");
  }

  return interp;
}

void interpreter_delete(interpreter *interp) {
  if (!interp) {
    return;
  }

  if (interp->interp) {
    lcl_interp_free(interp->interp);
  }

  free(interp);
}

interp_result interpreter_bind_cli_opts(interpreter *interp,
                                        const char *output_format) {
  if (!interp) {
    return INTERP_ERR;
  }

  lcl_value *opts = lcl_dict_new();

  if (!opts) {
    return INTERP_ERR;
  }

  if (output_format) {
    lcl_value *fmt = lcl_string_new(output_format);

    if (!fmt) {
      lcl_ref_dec(opts);
      return INTERP_ERR;
    }

    if (lcl_dict_put(&opts, "output_format", fmt) != LCL_OK) {
      lcl_ref_dec(fmt);
      lcl_ref_dec(opts);
      return INTERP_ERR;
    }

    lcl_ref_dec(fmt);
  }

  if (lcl_define_take(interp->interp, "_flux_cli_opts", opts) != LCL_OK) {
    return INTERP_ERR;
  }

  return INTERP_OK;
}

interp_result interpreter_eval(interpreter *interp, const char *script) {
  if (!interp || !script) {
    return INTERP_ERR;
  }

  lcl_value *result = NULL;

  if (lcl_eval_string(interp->interp, script, &result) != LCL_RC_OK) {
    print_lcl_error(interp->interp);
    return INTERP_ERR;
  }

  if (result) {
    lcl_ref_dec(result);
  }

  return INTERP_OK;
}

interp_result interpreter_eval_file(interpreter *interp, const char *filename) {
  if (!interp || !filename) {
    return INTERP_ERR;
  }

  lcl_value *result = NULL;

  if (lcl_eval_file(interp->interp, filename, &result) != LCL_RC_OK) {
    print_lcl_error(interp->interp);
    return INTERP_ERR;
  }

  if (result) {
    lcl_ref_dec(result);
  }

  return INTERP_OK;
}

interp_result interpreter_execute(interpreter *interp) {
  if (!interp) {
    return INTERP_ERR;
  }

  lcl_value *result = NULL;

  if (lcl_eval_string(interp->interp, "[Flux::run]", &result) != LCL_RC_OK) {
    print_lcl_error(interp->interp);
    return INTERP_ERR;
  }

  interp_result ret = INTERP_OK;

  if (result) {
    lcl_value *failed_val = NULL;

    if (lcl_dict_get(result, "failed", &failed_val) == LCL_OK && failed_val) {
      long failed = 0;

      if (lcl_value_to_int(failed_val, &failed) == LCL_OK && failed > 0) {
        ret = INTERP_ERR;
      }

      lcl_ref_dec(failed_val);
    }

    lcl_ref_dec(result);
  }

  return ret;
}

void interpreter_print_error(interpreter *interp) {
  if (!interp || !interp->interp) {
    return;
  }

  print_lcl_error(interp->interp);
}
