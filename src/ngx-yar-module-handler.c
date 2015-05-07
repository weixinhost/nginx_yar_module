#include <ngx_core.h>
#include <ngx_http.h>
#include <nginx.h>
#include <ngx_yar/yar.h>
#include <dlfcn.h>
typedef struct
{

    ngx_str_t yar_method_path;

    void*     yar_method_lib;

    ngx_int_t on;

    ngx_int_t debug;

} ngx_http_yar_loc_conf_t;

static void *ngx_http_yar_create_loc_conf(ngx_conf_t *cf);


static char* ngx_http_yar_conf_yar_method_path(ngx_conf_t *cf, ngx_command_t *cmd,void *conf);
static char* ngx_http_yar_conf_on(ngx_conf_t *cf, ngx_command_t *cmd,void *conf);
static char* ngx_http_yar_conf_debug(ngx_conf_t *cf, ngx_command_t *cmd,void *conf);
static char* ngx_http_yar_conf_timeout(ngx_conf_t *cf, ngx_command_t *cmd,void *conf);
ngx_int_t ngx_http_test_handler(ngx_http_request_t *r);

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
                ngx_http_yar_conf_debug,
                NGX_HTTP_LOC_CONF_OFFSET,
                offsetof(ngx_http_yar_loc_conf_t, timeout),
                NULL
        },


        ngx_null_command
};

static ngx_http_module_t ngx_http_yar_module_ctx = {
        NULL,                          /* preconfiguration */
        NULL,           /* postconfiguration */
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

    local_conf->yar_method_lib = NULL;

    local_conf->on      = 0;

    local_conf->debug   = 0;

    return local_conf;
}


static char* ngx_http_yar_conf_yar_method_path(ngx_conf_t *cf, ngx_command_t *cmd,void *conf){

    ngx_http_yar_loc_conf_t *local_conf = conf;

    char* rv = ngx_conf_set_str_slot(cf, cmd, local_conf);

    if(local_conf->yar_method_path.len > 0){

        my_conf->yar_method_lib = (void *) dlopen ((const char *) local_conf->yar_method_path.data,
                                                   RTLD_NOW | RTLD_LOCAL);

        if(!my_conf->yar_method_lib){

            ngx_log_error(NGX_LOG_EMERG, r->connection->log, 0, "open yar_method_path:%s error. %s",(char *)local_conf->yar_method_path.data,dlerror());

        }
    }

    return rv;

}

static char* ngx_http_yar_conf_on(ngx_conf_t *cf, ngx_command_t *cmd,void *conf){

    ngx_http_yar_loc_conf_t *local_conf = conf;

    local_conf->on = 1;

    ngx_http_core_loc_conf_t *clcf;

    clcf = ngx_http_conf_get_module_loc_conf (cf, ngx_http_core_module);

    clcf->handler = ngx_http_yar_post_handler;

    return NULL;
}


static char* ngx_http_yar_conf_debug(ngx_conf_t *cf, ngx_command_t *cmd,void *conf){

    local_conf->debug = 1;

    return NULL;
}



ngx_int_t ngx_http_test_handler(ngx_http_request_t *r){



}

















