#ifndef __ngx_yar_module_handler_H_
#define __ngx_yar_module_handler_H_
#include <ngx_core.h>
#include <ngx_http.h>
#include <nginx.h>

typedef struct
{

    ngx_str_t       yar_method_path;        // for yar_method_path="*.so"

    void            *yar_method_handler;    // for dlopen(yar_method_path)

    ngx_int_t       on;                     // for yar_server=1;

    ngx_int_t       debug;                  // for yar_debug=1;

    ngx_int_t       timeout;                // for yar_timeout=5;

    ngx_int_t       timeout_try_times;

    ngx_int_t       slow_timeout;           // for yar_slow_timeout = 200    ms

    ngx_str_t       custom_config;

    ngx_str_t       bootstrap;

    ngx_str_t       finalize;

    char *       dlerror;

} ngx_http_yar_loc_conf_t;


ngx_module_t*   ngx_http_yar_get_module();

#endif //ngx_yar_module_handler_H_
