#include "ngx_yar_module_impl.h"
#include <dlfcn.h>

extern  ngx_module_t ngx_http_yar_module;

ngx_str_t*      ngx_http_yar_read_client_post_body(ngx_http_request_t *r){

    u_char *p;
    u_char *data = NULL;
    size_t len = 0;
    ngx_buf_t *buf, *next;
    ngx_chain_t *cl;

    if(r->request_body == NULL || r->request_body->bufs == NULL){

    }

    cl = r->request_body->bufs;

    buf = cl->buf;

    if (cl->next == NULL) {

        len = (buf->last - buf->pos);

        p = ngx_pnalloc(r->pool, len + 1);

        if (p == NULL) {

        }

        data = p;

        ngx_memcpy(p, buf->pos, len);

        data[len] = 0;

    } else {

        next = cl->next->buf;

        len = (buf->last - buf->pos) + (next->last - next->pos);

        p = ngx_pnalloc(r->pool, len + 1);

        data = p;

        if (p == NULL) {

        }

        p = ngx_cpymem(p, buf->pos, buf->last - buf->pos);

        ngx_memcpy(p, next->pos, next->last - next->pos);

        data[len] = 0;

    }

    ngx_str_t *body = (ngx_str_t *)ngx_pnalloc(r->pool,sizeof(ngx_str_t));

    body->len = len;

    body->data = data;

    return body;

}

yar_request*    ngx_http_yar_get_yar_request(ngx_http_request_t *r,ngx_str_t *body){

    unsigned int protocol_len = sizeof(yar_header) + sizeof(YAR_PACKAGER);

    if(!body || body->len < protocol_len) {

        return NULL;
    }

    yar_header *header = (yar_header *)body->data;

    int ret = yar_protocol_parse(header);

    if(!ret) return NULL;


    if(ngx_strncmp (body->data + sizeof(yar_header),YAR_PACKAGER,sizeof(YAR_PACKAGER) - 1) != 0 ){

        return NULL;
    }

    ngx_str_t protocol_body = ngx_null_string;

    protocol_body.data = body->data + protocol_len;

    protocol_body.len = body->len - protocol_len;

    yar_request *request =  (yar_request *)ngx_pnalloc(r->pool,sizeof(yar_request));

    request->id = 0;

    request->mlen = 0;

    request->body = NULL;

    ret = yar_request_unpack (request,(char *)protocol_body.data,header->body_len,0);

    if(!ret){

        return NULL;

    }

    if(request->mlen < 1) {

        return NULL;
    }

    return request;


}

yar_response*   ngx_http_yar_get_yar_response(ngx_http_request_t *r, yar_request *request){

    if(!r || !request) return NULL;

    if(!request->id || !request->mlen || !request->method) return NULL;

    if(request->mlen > 256) return NULL;

    char method[256] = {0};

    memcpy(method,request->method,request->mlen);


    ngx_http_yar_loc_conf_t* my_conf = ngx_http_get_module_loc_conf(r, ngx_http_yar_module);

    if(!my_conf->yar_method_handler){

        return NULL;

    }

    yar_response *response = (yar_response *)ngx_pnalloc(r->pool,sizeof(yar_response));

    response->id = 0;

    response->out = NULL;

    response->payload.size = 0;

    char *cookie = NULL;

    char real_method[256] = {0};

    sprintf (real_method,"yar_method_%s",method);

    yar_method current_method = (yar_method)dlsym(my_conf->yar_method_handler,real_method);

    if(!current_method) return NULL;

    current_method(request,response,cookie);

    return response;

}

ngx_int_t       ngx_http_yar_send_response(ngx_http_request_t *r, ngx_str_t *reply){

    ngx_buf_t   *b;
    ngx_chain_t  out;

    ngx_uint_t content_length = reply->len;

    ngx_str_set (&r->headers_out.content_type, "application/msgpack");

    b = ngx_pcalloc (r->pool, sizeof (ngx_buf_t));

    out.buf = b;
    out.next = NULL;

    b->pos = reply->data;
    b->last = reply->data + content_length;
    b->memory = 1;
    b->last_buf = 1;

    r->headers_out.status = NGX_HTTP_OK;
    r->headers_out.content_length_n = content_length;
    ngx_http_send_header (r);
    ngx_http_output_filter (r, &out);

    ngx_http_finalize_request (r, NGX_OK);

    return 1;


}