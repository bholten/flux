#ifndef FLUX_H
#define FLUX_H

#include <stdint.h>

typedef enum flux_result { FLUX_OK, FLUX_ERROR } flux_result;

typedef struct flux flux;

flux *flux_new(void);
void flux_delete(flux *f);

void flux_set_verb(flux *f, const char *verb);
void flux_set_url(flux *f, const char *url);
void flux_set_header(flux *f, const char *header);
void flux_set_body(flux *f, const uint8_t *body);

int flux_send(flux *f);

flux_result sync_requests(flux *f);

int flux_interpret(flux *f, const char *file);
void flux_get_error(flux *f, const char *file);

#endif
