#ifndef INTERPRETER_H
#define INTERPRETER_H

typedef enum interp_result { INTERP_OK, INTERP_ERR } interp_result;

typedef struct interpreter interpreter;

interpreter *interpreter_new(void);
void interpreter_delete(interpreter *interp);
/* Bind a `_flux_cli_opts` dict at global scope before script evaluation.
 * Pass NULL `output_format` for an empty dict. Flux::run applies these
 * over the in-file config at run time, so --junit etc. override. */
interp_result interpreter_bind_cli_opts(interpreter *interp,
                                        const char *output_format);
interp_result interpreter_eval(interpreter *interp, const char *script);
interp_result interpreter_eval_file(interpreter *interp, const char *filename);
interp_result interpreter_execute(interpreter *interp);
void interpreter_print_error(interpreter *interp);

#endif
