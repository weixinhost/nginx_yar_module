/**
 * Yar - Concurrent RPC Server for PHP, C etc
 *
 * Copyright (C) 2012-2012 Xinchen Hui <laruence at gmail dot com>
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>   	/* for fprintf */
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h> /* for sockets */
#include <sys/un.h>  	/* for un */
#include <netdb.h>  	/* for gethostbyname */

#include "yar_common.h"
#include "yar_pack.h"
#include "yar_protocol.h"
#include "yar_response.h"
#include "yar_request.h"
#include "yar_client.h"

void yar_client_destroy(yar_client *client) /* {{{ */ {
    if (client->fd) {
        close(client->fd);
    }

    free(client);
}
/* }}} */

static yar_response * yar_client_caller(yar_client *client, char *method, uint num_args, yar_packager *parameters[]) /* {{{ */ {
    int	bytes_sent, select_result;
    uint bytes_left, totolly_read;
    yar_header *response_header = NULL;
    yar_response *response;
    yar_request  *request;
    yar_header header = {0};
    yar_payload payload = {0};
    struct timeval tv;
    fd_set readfds, writefds;

    if (client->timeout){
        tv.tv_sec = client->timeout;
    } else {
        tv.tv_sec = 1; /* default 1 second */
    }
    tv.tv_usec = 0;

    request = calloc(1, sizeof(yar_request));
    request->id = 1000; /* dummy id */
    request->method = strdup(method);
    request->mlen = strlen(method);
    if (num_args) {
        uint i;
        yar_packager *packager = yar_pack_start_array(num_args);
        for (i=0;i<num_args;i++) {
            yar_pack_push_packager(packager, parameters[i]);
        }
        yar_request_set_parameters(request, packager);
        yar_pack_free(packager);
    }

    if (!yar_request_pack(request, &payload, sizeof(yar_header) + sizeof(YAR_PACKAGER))) {

        yar_request_free(request);
        free(request);
        return NULL;
    }

    yar_protocol_render(&header, request->id, YAR_CLIENT_NAME, NULL, payload.size - sizeof(yar_header), client->persistent? YAR_PROTOCOL_PERSISTENT : 0);

    memcpy(payload.data, (char *)&header, sizeof(yar_header));
    memcpy(payload.data + sizeof(yar_header), YAR_PACKAGER, sizeof(YAR_PACKAGER));
    yar_request_free(request);
    free(request);

    FD_ZERO(&writefds);
    FD_SET(client->fd, &writefds);

    bytes_left = payload.size;

    write_wait:
    if ((select_result = select(client->fd + 1, NULL, &writefds, NULL, &tv)) == 0) {

        free(payload.data);
        return NULL;
    } else if (select == -1) {

        free(payload.data);
        return NULL;
    }

    if (!FD_ISSET(client->fd, &writefds)) {
        goto write_wait;
    } else {
        do {
            bytes_sent = send(client->fd, payload.data + payload.size - bytes_left, bytes_left, 0);
        } while (bytes_sent == -1 && errno == EINTR);

        if (bytes_sent < 0) {

            free(payload.data);
            return NULL;
        }

        bytes_left -= bytes_sent;
        if (bytes_left) {
            goto write_wait;
        }
    }

    free(payload.data);

    FD_ZERO(&readfds);
    FD_SET(client->fd, &readfds);

    response = calloc(1, sizeof(yar_response));
    totolly_read = 0;

    read_wait:
    if ((select_result = select(client->fd + 1, &readfds, NULL, NULL, &tv)) == 0) {

        yar_response_free(response);
        free(response);
        return NULL;
    } else  if (select_result == -1) {

        yar_response_free(response);
        free(response);
        return NULL;
    }

    if (!FD_ISSET(client->fd, &readfds)) {
        goto read_wait;
    } else {
        int bytes_read = 0;
        if (!response_header) {
            char buf[sizeof(yar_header)];
            do {
                bytes_read = recv(client->fd, buf, sizeof(buf), 0);
            } while (bytes_read == -1 && errno == EINTR);

            if (bytes_read == 0) {

                yar_response_free(response);
                free(response);
                return NULL;
            } else if (bytes_read < 0) {

                yar_response_free(response);
                free(response);
                return NULL;
            }

            response_header = (yar_header *)buf;
            if (!yar_protocol_parse(response_header)) {

                yar_response_free(response);
                free(response);
                return NULL;
            }
            if (!response->payload.data) {
                response->payload.data = malloc(sizeof(yar_header) + response_header->body_len);
                response->payload.size = sizeof(yar_header) + response_header->body_len;
            }
            memcpy(response->payload.data + totolly_read, buf, bytes_read);
            totolly_read += bytes_read;
        }

        do {
            bytes_read = recv(client->fd, response->payload.data + bytes_read, response->payload.size - bytes_read, 0);
        } while (bytes_read == -1 && errno == EINTR);

        if (bytes_read == 0) {

            yar_response_free(response);
            free(response);
            return NULL;
        } else if (bytes_read < 0) {

            yar_response_free(response);
            free(response);
            return NULL;
        }

        totolly_read += bytes_read;
        if (totolly_read < response_header->body_len + sizeof(yar_header)) {
            goto read_wait;
        }

        if (!yar_response_unpack(response, response->payload.data, response->payload.size, sizeof(yar_header) + sizeof(YAR_PACKAGER))) {

            yar_response_free(response);
            free(response);
            return NULL;
        }

        return response;
    }

}
/* }}} */



static size_t yar_client_http_write_func (void *contents, size_t size, size_t nmemb, void *userp) {

    size_t realsize = size * nmemb;

    yar_client_http_write_buffer *mem = (yar_client_http_write_buffer *) userp;

    if(!mem->buf){

        mem->buf = (void *)calloc(1,realsize);

    }else{

        mem->buf = realloc(mem->buf,mem->len + realsize);

    }

    memcpy(mem->buf,contents,realsize);

    mem->len += realsize;

    return realsize;
}


static yar_response * yar_client_http_caller(yar_client *client, char *method, uint num_args, yar_packager *parameters[]){

    int	bytes_sent, select_result;
    uint bytes_left, totolly_read;
    yar_header *response_header = NULL;
    yar_response *response = NULL;
    yar_request  *request = NULL;
    yar_header header = {0};
    yar_payload payload = {0};
    struct timeval tv;
    fd_set readfds, writefds;

    if (client->timeout){
        tv.tv_sec = client->timeout;
    } else {
        tv.tv_sec = 1; /* default 1 second */
    }
    tv.tv_usec = 0;

    request = calloc(1, sizeof(yar_request));
    request->id = 1000; /* dummy id */
    request->method = strdup(method);
    request->mlen = strlen(method);
    if (num_args) {
        uint i;
        yar_packager *packager = yar_pack_start_array(num_args);
        for (i=0;i<num_args;i++) {
            yar_pack_push_packager(packager, parameters[i]);
        }
        yar_request_set_parameters(request, packager);
        yar_pack_free(packager);
    }

    printf("pack OK.\n");

    if (!yar_request_pack(request, &payload, sizeof(yar_header) + sizeof(YAR_PACKAGER))) {
        yar_request_free(request);
        free(request);

        return NULL;
    }
    printf("pack header OK.\n");

    yar_protocol_render(&header, request->id, YAR_CLIENT_NAME, NULL, payload.size - sizeof(yar_header), client->persistent? YAR_PROTOCOL_PERSISTENT : 0);

    memcpy(payload.data, (char *)&header, sizeof(yar_header));

    memcpy(payload.data + sizeof(yar_header), YAR_PACKAGER, sizeof(YAR_PACKAGER));

    yar_request_free(request);

    free(request);

    curl_global_init(CURL_GLOBAL_NOTHING);

    CURL *curl = curl_easy_init();

    curl_easy_setopt(curl, CURLOPT_AUTOREFERER, 1);

    curl_easy_setopt(curl, CURLOPT_USERAGENT, YAR_CLIENT_NAME);

    curl_easy_setopt(curl, CURLOPT_DNS_CACHE_TIMEOUT, 15 * 30);

    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);

    curl_easy_setopt(curl, CURLOPT_URL, client->hostname);

    curl_easy_setopt(curl, CURLOPT_TIMEOUT, client->timeout ? client->timeout : 5);

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, yar_client_http_write_func);

    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *) &client->write_buffer);

    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST,"POST");

    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.data);

    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, payload.size);

    int err_code =  curl_easy_perform (curl);

    if(err_code != 0 || client->write_buffer.len < sizeof(yar_header)){

        goto clean_curl;

    }

    printf("check header OK.\n");

    response_header = (yar_header *)client->write_buffer.buf;

    if (!yar_protocol_parse(response_header)) {



        goto clean_curl;

    }
    printf("parse header OK.\n");
    response = calloc(1,sizeof(yar_response));

    response->payload.data = malloc(sizeof(yar_header) + client->write_buffer.len);

    response->payload.size = sizeof(yar_header) + client->write_buffer.len;

    memcpy(response->payload.data , client->write_buffer.buf,client->write_buffer.len);

    if (!yar_response_unpack(response, response->payload.data, response->payload.size, sizeof(yar_header) + sizeof(YAR_PACKAGER))) {

        goto clean_curl;

    }

    clean_curl :
    {

        free(payload.data);


        if(client->write_buffer.buf) {

            free (client->write_buffer.buf);

            client->write_buffer.len = 0;

            client->write_buffer.buf = NULL;
        }

        curl_easy_cleanup(curl);
        curl_global_cleanup();
    };

    return response;

}



yar_client * yar_client_init(char *hostname) /* {{{ */ {
    struct sockaddr sa;
    int sockfd;
    yar_client *client = calloc(1, sizeof(yar_client));

    client->hostname = hostname;
    client->call = yar_client_caller;
    client->protocol = YAR_CLIENT_PROTOCOL_SOCK;
    client->write_buffer.buf = NULL;
    client->write_buffer.len = 0;
    client->timeout = 0;

    if (strncasecmp(hostname, "http://", sizeof("http://") -1) == 0
        || strncasecmp(hostname, "https://", sizeof("https://") -1) == 0) {

        client->protocol = YAR_CLIENT_PROTOCOL_HTTP;
        client->call = yar_client_http_caller;

        return client;

    } else if (hostname[0] == '/') {
        /* unix domain socket */
        struct sockaddr_un *usa;
        usa = (struct sockaddr_un *)&sa;
        if ((sockfd = socket(AF_UNIX, SOCK_STREAM, 0) == -1)) {

            free(client);
            return NULL;
        }
        usa->sun_family = AF_UNIX;
        memcpy(usa->sun_path, hostname, strlen(hostname) + 1);
    } else {
        /* IPV4, we don't support IPV6 for now */
        char *delim, *p, host[1024];
        int port;
        struct hostent *hptr;
        struct sockaddr_in *isa;
        if ((delim = strchr(hostname, ':'))) {
            memcpy(host, hostname, delim - hostname);
            host[delim - hostname] = '\0';
            port = atoi(delim + 1);
        } else {

            free(client);
            return NULL;
        }

        if ((hptr = gethostbyname(host)) == NULL) {

            free(client);
            return NULL;
        }

        p = hptr->h_addr;
        if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {

            free(client);
            return NULL;
        }

        switch (hptr->h_addrtype) {
            case AF_INET:
            {
                int val = 1;
                isa = (struct sockaddr_in *)&sa;
                bzero(isa, sizeof(struct sockaddr_in));

                isa->sin_family = AF_INET;
                isa->sin_port = htons(port);
                memcpy(&isa->sin_addr, p, sizeof(struct in_addr));

                setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char*)&val, sizeof(val));
            }
                break;
            default:

                close(sockfd);
                free(client);
                return NULL;
        }
    }

    if (connect(sockfd, (const struct sockaddr *)&sa, sizeof(sa)) == -1) {

        close(sockfd);
        free(client);
        return NULL;
    }

    yar_set_non_blocking(sockfd);

    client->fd = sockfd;

    return client;
}
/* }}} */

int yar_client_set_opt(yar_client *client, yar_client_opt opt, void *val) /* {{{ */ {
    switch (opt) {
        case YAR_PERSISTENT_LINK:
            client->persistent = *(int *)val;
            break;
        case YAR_CONNECT_TIMEOUT:
            client->timeout = *(int *)val;
            break;
        default:
            return 0;
    }
    return 1;
}
/* }}} */

const void * yar_client_get_opt(yar_client *client, yar_client_opt opt) /* {{{ */ {
    switch (opt) {
        case YAR_PERSISTENT_LINK:
            return &client->persistent;
            break;
        case YAR_CONNECT_TIMEOUT:
            return &client->timeout;
            break;
        default:
            return NULL;
    }
}
/* }}} */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */