#include "ngx_yar_module_impl.h"
#include "ngx_yar_module_handler.h"
#include <dlfcn.h>
#include <sys/time.h>
#include "func_timeout_call.h"

typedef void (*yar_bootstrap_method)(void *config,uint config_len);

typedef void (*yar_finalize_method)(yar_request *request, yar_response *response);

extern  ngx_module_t ngx_http_yar_module;

ngx_str_t*      ngx_http_yar_read_client_post_body(ngx_http_request_t *r){

    u_char *p;
    u_char *data = NULL;
    size_t len = 0;
    ngx_buf_t *buf, *next;
    ngx_chain_t *cl;

    if(r->request_body == NULL || r->request_body->bufs == NULL){

        ngx_log_error(NGX_LOG_WARN, r->connection->log, 0,
                      "read client request body error.");
                
        return NULL;
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

        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                      "not support packager name(%s). only support for MSGPACK",body->data + sizeof(yar_header));
        return NULL;
    }

    ngx_str_t protocol_body = ngx_null_string;

    protocol_body.data = body->data + protocol_len;

    protocol_body.len = body->len - protocol_len;

    yar_request *request =  (yar_request *)ngx_pnalloc(r->pool,sizeof(yar_request));

    memset(request,0,sizeof(yar_request));

    request->id = 0;

    request->mlen = 0;

    request->body = NULL;

    ret = yar_request_unpack (request,(char *)protocol_body.data,header->body_len,0);

    if(!ret){

        ngx_log_error(NGX_LOG_WARN, r->connection->log, 0,
                      "unpack yar request error.");

        return NULL;

    }

    if(request->mlen < 1) {

        ngx_log_error(NGX_LOG_WARN, r->connection->log, 0,
                      "yar call method name cannt be empty.");

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
    method[request->mlen] = 0;

    ngx_http_yar_loc_conf_t* my_conf = ngx_http_get_module_loc_conf(r, ngx_http_yar_module);

    if(!my_conf->yar_method_handler){
        ngx_log_error(NGX_LOG_WARN, r->connection->log, 0,
                      "cannt open yar_method_path(%s):dlerror:%s",(char *)my_conf->yar_method_path.data,my_conf->dlerror);

        return NULL;

    }

    volatile yar_response *response = (yar_response *)ngx_pnalloc(r->pool,sizeof(yar_response));

    memset((yar_response *)response,0,sizeof(yar_response));

    response->id = 0;

    response->out = NULL;

    response->payload.size = 0;

    response->payload.data = NULL;

    char *cookie = NULL;

    char real_method[256] = {0};


    sprintf (real_method,"yar_method_%s",method);

    yar_method current_method = (yar_method)dlsym(my_conf->yar_method_handler,real_method);

    if(!current_method) {

        ngx_log_error(NGX_LOG_WARN, r->connection->log, 0,
                      "yar call undefined method %s.",method);

        yar_response_free((yar_response *)response);

        return NULL;
    }


    struct timeval start;
    struct timeval end;

    gettimeofday(&start, NULL);


    char bootstrap_method[256] = {0};

    if(my_conf->bootstrap.len > 0){

        memcpy(bootstrap_method,my_conf->bootstrap.data,my_conf->bootstrap.len);

        bootstrap_method[my_conf->bootstrap.len] = 0;

        yar_bootstrap_method bootstrap = (yar_bootstrap_method)dlsym(my_conf->yar_method_handler,bootstrap_method);

        if(bootstrap){

            bootstrap(my_conf->custom_config.data,my_conf->custom_config.len);

        }else{

        }

    }

    if(my_conf->timeout > 0){

        int ret = 0;

        int try_times = my_conf->timeout_try_times < 1 ? 1 :my_conf->timeout_try_times; //more than 1

        int interval = my_conf->timeout;

        add_timeout_to_func(current_method, try_times, interval, ret, request, (yar_response *)response, cookie);

        if(ret == E_CALL_TIMEOUT){

            ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                          "yar call method %s timeout.",method);

        }

    }else{

        current_method((yar_request *)request,(yar_response *)response,cookie);

    }


    char finalize_method[256] = {0};

    if(my_conf->finalize.len > 0){

        memcpy(finalize_method,my_conf->finalize.data,my_conf->finalize.len);

        finalize_method[my_conf->finalize.len] = 0;

    }

    yar_finalize_method finalize = (yar_finalize_method)dlsym(my_conf->yar_method_handler,finalize_method);

    if(finalize){

        finalize((yar_request *)request,(yar_response *)response);

    }

    gettimeofday(&end, NULL);


    int used_sec = end.tv_sec - start.tv_sec;
    int used_usec = end.tv_usec - start.tv_usec;

    float used_msec = ((float)(used_sec * 1000 * 1000) +used_usec) / 1000;

    if(my_conf->slow_timeout > 0 && used_msec > my_conf->slow_timeout){

        ngx_log_error(NGX_LOG_WARN, r->connection->log, 0,
                      "yar call method %s too slow. [%.3f ms]",method,used_msec);

    }else{

        ngx_log_error(NGX_LOG_NOTICE, r->connection->log, 0,
                      "yar call method %s. [%.3f ms]",method,used_msec);

    }

    return (yar_response *)response;

}


ngx_int_t       ngx_http_yar_send_response(ngx_http_request_t *r, ngx_str_t *reply){

    ngx_buf_t   *b = NULL;

    ngx_chain_t  *out = ngx_pcalloc (r->pool, sizeof (ngx_chain_t));;

    ngx_uint_t content_length = reply->len;

    ngx_str_set (&r->headers_out.content_type, "application/msgpack");

      b = ngx_create_temp_buf(r->pool, content_length);
      memcpy(b->pos,reply->data,content_length);
      b->last = b->pos + content_length;
      b->last_buf = 1;

    out->buf = b;

    out->next = NULL;

    r->headers_out.status = NGX_HTTP_OK;

    r->headers_out.content_length_n = content_length;

    ngx_http_send_header (r);

    int rc =  ngx_http_output_filter (r, out);

    ngx_http_finalize_request (r, rc);

    return rc;


}