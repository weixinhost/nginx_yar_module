#include "ngx_yar_module_handler.h"
#include "ngx_yar_module_impl.h"
#include <dlfcn.h>

static void* ngx_http_yar_create_loc_conf(ngx_conf_t *cf);

static char* ngx_http_yar_conf_yar_method_path(ngx_conf_t *cf, ngx_command_t *cmd,void *conf);
static char* ngx_http_yar_conf_on(ngx_conf_t *cf, ngx_command_t *cmd,void *conf);
static char* ngx_http_yar_conf_debug(ngx_conf_t *cf, ngx_command_t *cmd,void *conf);
static char* ngx_http_yar_conf_timeout(ngx_conf_t *cf, ngx_command_t *cmd,void *conf);

ngx_int_t ngx_http_yar_read_request_handler(ngx_http_request_t *r);
void ngx_http_yar_handler(ngx_http_request_t *r);

static ngx_command_t ngx_http_yar_commands[] = {

        {
                ngx_string("yar_method_path"),
                NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
                ngx_http_yar_conf_yar_method_path,
                NGX_HTTP_LOC_CONF_OFFSET,
                offsetof(ngx_http_yar_loc_conf_t, yar_method_path),
                NULL
        },

        {
                ngx_string("yar_on"),
                NGX_HTTP_LOC_CONF|NGX_CONF_NOARGS|NGX_CONF_TAKE1,
                ngx_http_yar_conf_on,
                NGX_HTTP_LOC_CONF_OFFSET,
                offsetof(ngx_http_yar_loc_conf_t, on),
                NULL
        },

        {
                ngx_string("yar_debug"),
                NGX_HTTP_LOC_CONF|NGX_CONF_NOARGS|NGX_CONF_TAKE1,
                ngx_http_yar_conf_debug,
                NGX_HTTP_LOC_CONF_OFFSET,
                offsetof(ngx_http_yar_loc_conf_t, debug),
                NULL
        },

        {
                ngx_string("yar_timeout"),
                NGX_HTTP_LOC_CONF|NGX_CONF_NOARGS|NGX_CONF_TAKE1,
                ngx_http_yar_conf_timeout,
                NGX_HTTP_LOC_CONF_OFFSET,
                offsetof(ngx_http_yar_loc_conf_t, timeout),
                NULL
        },


        ngx_null_command
};


static ngx_http_module_t ngx_http_yar_module_ctx = {
        NULL,                          /* preconfiguration */
        NULL,                          /* postconfiguration */
        NULL,                          /* create main configuration */
        NULL,                          /* init main configuration */

        NULL,                          /* create server configuration */
        NULL,                          /* merge server configuration */

        ngx_http_yar_create_loc_conf, /* create location configuration */

        NULL                            /* merge location configuration */
};

ngx_module_t ngx_http_yar_module = {
        NGX_MODULE_V1,
        &ngx_http_yar_module_ctx,    /* module context */
        ngx_http_yar_commands,       /* module directives */
        NGX_HTTP_MODULE,               /* module type */
        NULL,                          /* init master */
        NULL,                          /* init module */
        NULL,                          /* init process */
        NULL,                          /* init thread */
        NULL,                          /* exit thread */
        NULL,                          /* exit process */
        NULL,                          /* exit master */
        NGX_MODULE_V1_PADDING
};

static void *ngx_http_yar_create_loc_conf(ngx_conf_t *cf)
{
    ngx_http_yar_loc_conf_t* local_conf = NULL;

    local_conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_yar_loc_conf_t));

    if (local_conf == NULL)
    {
        return NULL;
    }

    ngx_str_null(&local_conf->yar_method_path);

    local_conf->yar_method_handler = NULL;

    local_conf->on      = 0;

    local_conf->debug   = 0;

    return local_conf;
}

static char* ngx_http_yar_conf_yar_method_path(ngx_conf_t *cf, ngx_command_t *cmd,void *conf){

    ngx_http_yar_loc_conf_t *local_conf = conf;

    char* rv = ngx_conf_set_str_slot(cf, cmd, local_conf);

    if(local_conf->yar_method_path.len > 0){

        local_conf->yar_method_handler = (void *) dlopen ((const char *) local_conf->yar_method_path.data,
                                                   RTLD_NOW | RTLD_LOCAL);

        local_conf->dlerror = dlerror();

    }

    return rv;

}

static char* ngx_http_yar_conf_on(ngx_conf_t *cf, ngx_command_t *cmd,void *conf){

    ngx_http_yar_loc_conf_t *local_conf = conf;

    local_conf->on = 1;

    ngx_http_core_loc_conf_t *clcf;

    clcf = ngx_http_conf_get_module_loc_conf (cf, ngx_http_core_module);

    clcf->handler = ngx_http_yar_read_request_handler;

    return NULL;
}


static char* ngx_http_yar_conf_debug(ngx_conf_t *cf, ngx_command_t *cmd,void *conf){

    ngx_http_yar_loc_conf_t *local_conf = conf;

    local_conf->debug = 1;

    return NULL;
}

//todo not implements set.
static char* ngx_http_yar_conf_timeout(ngx_conf_t *cf, ngx_command_t *cmd,void *conf){

    ngx_http_yar_loc_conf_t *local_conf = conf;

    local_conf->timeout = 1;

    return NULL;
}

void ngx_http_yar_handler(ngx_http_request_t *r){

    ngx_str_t error;

    ngx_str_t *client_request_body = ngx_http_yar_read_client_post_body(r);

    yar_request *request = NULL;

    yar_response *response = NULL;


    if(!client_request_body || client_request_body->len < sizeof(yar_header)){

        ngx_str_set(&error,"read request body error.");

        goto send_error;

    }

    request = ngx_http_yar_get_yar_request(r,client_request_body);

    if(!request){

        ngx_str_set(&error,"parse yar protocol error.");

        goto send_error;

    }

    if(request->mlen < 1){

        ngx_str_set(&error,"call undefined method.");

        goto send_error;

    }

    response = ngx_http_yar_get_yar_response(r,request);

    if(!response){

        ngx_str_set(&error,"get response error.");

        goto send_error;

    }

    if (!yar_response_pack(response, &(response->payload), sizeof(yar_header) + sizeof(YAR_PACKAGER))) {

        ngx_str_set(&error,"protocol pack response error.");

        goto send_error;

    }


    yar_header response_header;

    yar_protocol_render(&response_header, request->id, NULL, NULL, response->payload.size - sizeof(yar_header), 0);

    memcpy(response->payload.data, (char *)&response_header, sizeof(yar_header));

    memcpy(response->payload.data + sizeof(yar_header), YAR_PACKAGER, sizeof(YAR_PACKAGER));


    ngx_str_t reply;

    reply.data = (u_char *)response->payload.data;

    reply.len = response->payload.size;

    ngx_http_yar_send_response(r,&reply);

    goto clean_resource;

    while(0){

        send_error:
        {

            ngx_http_yar_send_response (r, &error);

            goto clean_resource;
        };

        return ;

    }

    clean_resource:
    {
        if (request) {

            yar_request_free (request);

        }

        if (response) {

            yar_response_free (response);

        }
    }

}


ngx_int_t ngx_http_yar_read_request_handler(ngx_http_request_t *r){

    ngx_int_t rc;

    rc = ngx_http_read_client_request_body(r,ngx_http_yar_handler);

    if (rc >= NGX_HTTP_SPECIAL_RESPONSE) {
        return rc;
    }

    return NGX_OK;

}
