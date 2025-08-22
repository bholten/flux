#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <curl/curl.h>
#include <curl/easy.h>

#include "http.h"

struct http {
  CURL *curl;
  struct curl_slist *headers;
  bool alive;
};

http *http_new(void) {
  CURL *c = curl_easy_init();

  if (!c) return NULL;

  http *h = calloc(1, sizeof(*h));

  if (!h) {
    curl_easy_cleanup(c);
    return NULL;
  }

  h->alive = true;
  h->curl = c;
  h->headers = NULL;

  return h;
}

void http_delete(http *h) {
  if (!h) return;

  if (!h->curl) {
    curl_easy_cleanup(h->curl);
  }

  free(h);
}

int http_init(http *h) {
  if (curl_global_init(CURL_GLOBAL_DEFAULT) != 0) {
    curl_easy_cleanup(h->curl);
    return -1;
  }

  return 0;
}

void http_reset(http *h) {
  curl_easy_reset(h->curl);
}

bool http_alive(http *h) {
  return h->alive;
}

size_t http_sizeof(void) {
  return sizeof(struct http);
}

typedef size_t (*write_callback)(char *, size_t, size_t, void *);

http_result http_set_write_callback(http *h, void *cb) {
  write_callback write_cb = (write_callback)cb;

  int rc = curl_easy_setopt(h->curl, CURLOPT_WRITEFUNCTION, write_cb);

  if (rc != CURLE_OK) return HTTP_ERROR;
  return HTTP_OK;
}

http_result http_set_write_data(http *h, void *data) {
  int rc = curl_easy_setopt(h->curl, CURLOPT_WRITEDATA, data);

  if (rc != CURLE_OK) return HTTP_ERROR;
  return HTTP_OK;
}

http_result http_set_option_verbose(http *h, long verbose) {
  int rc = curl_easy_setopt(h->curl, CURLOPT_VERBOSE, verbose);

  if (rc != CURLE_OK) return HTTP_ERROR;
  return HTTP_OK;
}

http_result http_set_url(http *h, const char *url) {
  int rc = curl_easy_setopt(h->curl, CURLOPT_URL, url);

  if (rc != CURLE_OK) return HTTP_ERROR;
  return HTTP_OK;
}

http_result http_set_header(http *h, const char *header) {
  h->headers = curl_slist_append(h->headers, header);
  int rc = curl_easy_setopt(h->curl, CURLOPT_HTTPHEADER, h->headers);

  if (rc != CURLE_OK) return HTTP_ERROR;
  return HTTP_OK;
}

http_result http_set_verb(http *h, const char *verb) {
  int rc = curl_easy_setopt(h->curl, CURLOPT_CUSTOMREQUEST, verb);

  if (rc != CURLE_OK) return HTTP_ERROR;
  return HTTP_OK;
}

http_result http_set_body(http *h, const char *body) {
  int rc = curl_easy_setopt(h->curl, CURLOPT_POSTFIELDS, body);

  if (rc != CURLE_OK) return HTTP_ERROR;
  return HTTP_OK;
}

http_result http_options_set_timeout(http *h, int timeout) {
  int rc = curl_easy_setopt(h->curl, CURLOPT_TIMEOUT_MS, timeout);

  if (rc != CURLE_OK) return HTTP_ERROR;
  return HTTP_OK;
}

http_result http_set_option_accept_timeout_ms(http *h, long timeout_ms) {
  int rc = curl_easy_setopt(h->curl, CURLOPT_ACCEPTTIMEOUT_MS, timeout_ms);

  if (rc != CURLE_OK) return HTTP_ERROR;
  return HTTP_OK;
}

http_result http_set_option_connection_timeout_ms(http *h, long timeout_ms) {
  int rc = curl_easy_setopt(h->curl, CURLOPT_CONNECTTIMEOUT_MS, timeout_ms);

  if (rc != CURLE_OK) return HTTP_ERROR;
  return HTTP_OK;
}

http_result http_set_option_interface(http *h, const char *interface) {
  int rc = curl_easy_setopt(h->curl, CURLOPT_INTERFACE, interface);

  if (rc != CURLE_OK) return HTTP_ERROR;
  return HTTP_OK;
}

http_result http_set_option_low_speed_limit(http *h, long low_speed_limit) {
  int rc = curl_easy_setopt(h->curl, CURLOPT_LOW_SPEED_LIMIT, low_speed_limit);

  if (rc != CURLE_OK) return HTTP_ERROR;
  return HTTP_OK;
}

http_result http_set_option_low_speed_time(http *h, long low_speed_time) {
  int rc = curl_easy_setopt(h->curl, CURLOPT_LOW_SPEED_TIME, low_speed_time);

  if (rc != CURLE_OK) return HTTP_ERROR;
  return HTTP_OK;
}

http_result http_set_option_tcp_keep_alive(http *h, long keep_alive) {
  int rc = curl_easy_setopt(h->curl, CURLOPT_TCP_KEEPALIVE, keep_alive);

  if (rc != CURLE_OK) return HTTP_ERROR;
  return HTTP_OK;
}

http_result http_set_option_tcp_keep_idle(http *h, long keep_idle) {
  int rc = curl_easy_setopt(h->curl, CURLOPT_TCP_KEEPIDLE, keep_idle);

  if (rc != CURLE_OK) return HTTP_ERROR;
  return HTTP_OK;
}

http_result http_set_option_tcp_keep_intvl(http *h, long keep_intvl) {
  int rc = curl_easy_setopt(h->curl, CURLOPT_TCP_KEEPINTVL, keep_intvl);

  if (rc != CURLE_OK) return HTTP_ERROR;
  return HTTP_OK;
}

// HTTP options
http_result http_set_option_accept_encoding(http *h, const char *encoding) {
  int rc = curl_easy_setopt(h->curl, CURLOPT_ACCEPT_ENCODING, encoding);

  if (rc != CURLE_OK) return HTTP_ERROR;
  return HTTP_OK;
}

// TODO maybe change API here
http_result http_set_option_http_version(http *h, long http_version) {
  int rc = curl_easy_setopt(h->curl, CURLOPT_HTTP_VERSION, http_version);

  if (rc != CURLE_OK) return HTTP_ERROR;
  return HTTP_OK;
}

// SSL nonsense
http_result http_set_option_ssl_verify_peer(http *h, long verify) {
  int rc = curl_easy_setopt(h->curl, CURLOPT_SSL_VERIFYPEER, verify);

  if (rc != CURLE_OK) return HTTP_ERROR;
  return HTTP_OK;
}

http_result http_set_option_ssl_verify_host(http *h, long verify) {
  int rc = curl_easy_setopt(h->curl, CURLOPT_SSL_VERIFYHOST, verify);

  if (rc != CURLE_OK) return HTTP_ERROR;
  return HTTP_OK;
}

http_result http_set_option_ca_info(http *h, char *ca_info) {
  int rc = curl_easy_setopt(h->curl, CURLOPT_CAINFO, ca_info);

  if (rc != CURLE_OK) return HTTP_ERROR;
  return HTTP_OK;
}

http_result http_set_option_ca_path(http *h, char *ca_path) {
  int rc = curl_easy_setopt(h->curl, CURLOPT_CAPATH, ca_path);

  if (rc != CURLE_OK) return HTTP_ERROR;
  return HTTP_OK;
}

http_result http_send(http *h) {
  CURLcode result = curl_easy_perform(h->curl);

  if (CURLE_OK == result) {
    curl_easy_reset(h->curl);
    curl_slist_free_all(h->headers);

    return HTTP_OK;
  }

  curl_slist_free_all(h->headers);
  curl_easy_reset(h->curl);

  return HTTP_ERROR;
}
