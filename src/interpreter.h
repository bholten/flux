#ifndef INTERPRETER_H
#define INTERPRETER_H

typedef enum interp_result { INTERP_OK, INTERP_ERR } interp_result;

typedef struct interpreter interpreter;

interpreter *interpreter_new(void);
void interpreter_delete(interpreter *interp);
interp_result interpreter_setup_environment(interpreter *interp, int argc,
                                            const char **argv);
interp_result interpreter_eval(interpreter *interp, const char *script);
interp_result interpreter_eval_file(interpreter *interp, const char *filename);
interp_result interpreter_execute(interpreter *interp);
void interpreter_print_error(interpreter *interp);

#endif
