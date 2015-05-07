#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "yar_common.h"
#include "yar_pack.h"
#include "yar_method.h"


struct {

    size_t len;
    yar_method_t method_list[MAX_YAR_METHOD_LENGTH];

} _yar_method_list;

extern yar_method_init_callback yar_method_init_handler;



yar_method_t* yar_method_register(char *name,size_t len,yar_method method){

    if(_yar_method_list.len >= MAX_YAR_METHOD_LENGTH) return NULL;

    yar_method_t* m = &_yar_method_list.method_list[_yar_method_list.len];

    m->name = name;
    m->len = len;
    m->method = method;

    _yar_method_list.len++;
    return &_yar_method_list.method_list[_yar_method_list.len - 1];

}

yar_method_t* yar_method_unregister(char *name, size_t len){

    yar_method_t *m = yar_method_find (name,len);

    if(!m) return NULL;

    m->name = NULL;
    m->len = 0;
    m->method = NULL;

    return m;
}


yar_method_t* yar_method_find(char *name, size_t len){

    if(_yar_method_list.len < 1) return NULL;

    unsigned int i =0;

    for(;i<_yar_method_list.len;i++){

        yar_method_t *method = &(_yar_method_list.method_list[i]);

        if(strncmp(method->name,name,len) == 0){
            return method;
        }
    }

    return NULL;

}



int yar_method_init(void){


    return 1;
}