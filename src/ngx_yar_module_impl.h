#ifndef __ngx_yar_module_impl_H_
#define __ngx_yar_module_impl_H_
#include <ngx_core.h>
#include <ngx_http.h>
#include <nginx.h>
#include <ngx_yar/yar.h>
#include "ngx_yar_module_handler.h"


ngx_str_t*      ngx_http_yar_read_client_post_body(ngx_http_request_t *r);

yar_request*    ngx_http_yar_get_yar_request(ngx_http_request_t *r,ngx_str_t *body);

yar_response*   ngx_http_yar_get_yar_response(ngx_http_request_t *r, yar_request *request);

ngx_int_t       ngx_http_yar_send_response(ngx_http_request_t *r, ngx_str_t *reply);


#endif //ngx-yar-module-impl_H_
