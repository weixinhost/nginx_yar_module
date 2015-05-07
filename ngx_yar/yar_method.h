#ifndef __yar_method_H_
#define __yar_method_H_

#define MAX_YAR_METHOD_LENGTH 32


#include "yar_request.h"
#include "yar_response.h"

typedef void (*yar_method)(yar_request *request, yar_response *response, void *cookie);
typedef void(*yar_method_init_callback)(void);

typedef struct _yar_method
{

    char *name;
    size_t len;
    yar_method method;
} yar_method_t;


yar_method_t* yar_method_register(char *name,size_t len,yar_method method);
yar_method_t* yar_method_unregister(char *name, size_t len);
yar_method_t* yar_method_find(char *name, size_t len);

int yar_method_init(void);


#endif //yar_method.h_H_
