#ifndef HTTP_H
#define HTTP_H

#include <stdbool.h>
#include <stddef.h>

typedef enum http_result { HTTP_OK, HTTP_ERROR } http_result;

typedef struct http http;

http *http_new(void);
void http_delete(http *h);
int http_init(http *h);
void http_reset(http *h);
bool http_alive(http *h);
size_t http_sizeof(void);

void http_get_info_response_code(http *h, long *status);
void http_get_info_content_type(http *h, char **content_type);
void http_get_info_effective_url(http *h, char **effective_url);
void http_get_info_total_time(http *h, double *total_time);

http_result http_set_header_callback(http *h, void *cb);
http_result http_set_header_data(http *h, void *data);
http_result http_set_write_callback(http *h, void *cb);
http_result http_set_write_data(http *h, void *data);

http_result http_set_verb(http *h, const char *verb);
http_result http_set_url(http *h, const char *url);
http_result http_set_header(http *h, const char *header);
http_result http_set_body(http *h, const char *body);

// Connection Settings
http_result http_set_option_accept_timeout_ms(http *h, long timeout_ms);
http_result http_set_option_connection_timeout_ms(http *h, long timeout_ms);
http_result http_set_option_interface(http *h, const char *interface);
http_result http_set_option_low_speed_limit(http *h, long low_speed_limit);
http_result http_set_option_low_speed_time(http *h, long low_speed_time);
http_result http_set_option_tcp_keep_alive(http *h, long keep_alive);
http_result http_set_option_tcp_keep_idle(http *h, long keep_idle);
http_result http_set_option_tcp_keep_intvl(http *h, long keep_intvl);

// HTTP
http_result http_set_option_accept_encoding(http *h, const char *encoding);
http_result http_set_option_http_version(http *h, long http_version);

// TLS/mTLS
http_result http_set_option_ssl_verify_peer(http *h, long verify);
http_result http_set_option_ssl_verify_host(http *h, long verify);

http_result http_set_option_ca_info(http *h, char *path);
http_result http_set_option_ca_path(http *h, char *path);
// stopped here:
http_result http_set_option_ssl_cert(http *h, int ssl_cert);
http_result http_set_option_ssl_cert_type(http *h, int ssl_cert_type);
http_result http_set_option_ssl_key(http *h, int ssl_key);
http_result http_set_option_ssl_key_type(http *h, int ssl_key_type);
http_result http_set_option_key_password(http *h, int key_password);
http_result http_set_option_ssl_version(http *h, int ssl_version);
http_result http_set_option_ssl_cipher_list(http *h, int cipher_list);
http_result http_set_option_tls13_ciphers(http *h, int tls13_ciphers);

// Proxies
http_result http_set_option_proxy(http *h, int proxy);
http_result http_set_option_proxy_port(http *h, int proxy_port);
http_result http_set_option_proxy_type(http *h, int proxy_type);
http_result http_set_option_proxy_username(http *h, int username);
http_result http_set_option_proxy_password(http *h, int password);
http_result http_set_option_no_proxy(http *h, int no_proxy);
http_result http_set_option_proxy_ssl_verify_peer(http *h, int verify_peer);
http_result http_set_option_proxy_ssl_verify_host(http *h, int verify_host);
http_result http_set_option_proxy_ca_info(http *h, int ca_info);
http_result http_set_option_proxy_ca_path(http *h, int ca_path);
http_result http_set_option_proxy_ssl_version(http *h, int ssl_version);

// Redirects
http_result http_set_option_follow_location(http *h, int follow_location);
http_result http_set_option_max_redirects(http *h, int max_redirects);
http_result http_set_option_post_redirect(http *h, int post_redirect);

// Auth
http_result http_set_option_http_auth(http *h, int http_auth);
http_result http_set_option_username(http *h, int username);
http_result http_set_option_password(http *h, int password);
http_result http_set_option_xoauth2_bearer(http *h, int xoauth2_bearer);

// Observability
http_result http_set_option_verbose(http *h, long verbose);
http_result http_set_option_debug_function(http *h, int debug_function);
http_result http_set_option_header(http *h, int header);
http_result http_set_option_header_function(http *h, int header_function);

// Power users
http_result http_set_option_resolve(http *h, int resolve);
http_result http_set_option_doh_url(http *h, int doh_url);
http_result http_set_option_dns_servers(http *h, int dns_servers);
http_result http_set_option_fresh_connect(http *h, int fresh_connect);
http_result http_set_option_forbid_reuse(http *h, int forbid_reuse);
http_result http_set_option_expect_100_timeout_ms(http *h, int expect);
http_result http_set_option_max_recv_speed_large(http *h, int max_rcv);
http_result http_set_option_max_send_speed_large(http *h, int max_send_speed);
http_result http_set_option_cookie_file(http *h, int cookie_file);
http_result http_set_option_cookie_jar(http *h, int cookie_jar);

http_result http_send(http *h);

#endif
