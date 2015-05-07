#ifndef __ngx_yar_module_ctx_H_
#define __ngx_yar_module_ctx_H_


static void* ngx_http_yar_create_loc_conf(ngx_conf_t *cf);

static char *ngx_http_yar_conf_yar_method_path(ngx_conf_t *cf, ngx_command_t *cmd,void *conf);

static char *ngx_http_yar_conf_on(ngx_conf_t *cf, ngx_command_t *cmd,void *conf);

static char *ngx_http_yar_conf_debug(ngx_conf_t *cf, ngx_command_t *cmd,void *conf);

static char *ngx_http_yar_conf_timeout(ngx_conf_t *cf, ngx_command_t *cmd,void *conf);

ngx_int_t ngx_http_yar_handler(ngx_http_request_t *r);



static ngx_command_t ngx_http_yar_commands[] = {

        {


        }


};




#endif //ngx-yar-module-ctx_H_
