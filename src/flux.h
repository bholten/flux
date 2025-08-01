#ifndef FLUX_H
#define FLUX_H

#include <stdint.h>

typedef struct flux flux;

typedef enum verb { GET, POST, PUT, DELETE, PATCH, OPTION } verb;

flux *flux_new(void);
void flux_delete(flux *req);

int flux_send(flux *req);
void flux_set_verb(flux *req, verb v);
void flux_set_url(flux *req, const char *url);
void flux_set_header(flux *req, const char *header);
void flux_set_body(flux *req, const uint8_t *body);
int flux_interpret(flux *req, const char *file);
void flux_get_error(flux *req, const char *file);

#endif
