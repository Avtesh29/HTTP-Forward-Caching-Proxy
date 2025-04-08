#include "cache.h"

#include "client_socket.h"
#include "iowrapper.h"
#include "listener_socket.h"
#include "prequest.h"
#include "a5protocol.h"

#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdlib.h> 
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <sys/file.h>
#include <sys/stat.h>
#include <regex.h>

void handle_connection(uintptr_t);
char* handle_get(int, char*, int, int*);
int process_args (int, char**, int*, char**, int*);

// globals:
Listener_Socket_t *sock = NULL;
Cache_t* c = NULL;
regex_t head_reg;
regex_t cl_reg;
regex_t num_reg;
regex_t end_reg;

void usage(FILE *stream, char *exec) {
    fprintf(stream, "usage: %s <port> <mode> <n>\n", exec);
}


int main(int argc, char **argv) {


    int port = 0;
    char* mode = NULL;
    int n = 0;

    if (process_args(argc, argv, &port, &mode, &n) > 0) {
        return EXIT_FAILURE;
    }

    sock = ls_new(port);

    c = createCache(mode, n);

    int head = regcomp(&head_reg, "\r\n", REG_EXTENDED);
    int cl = regcomp(&cl_reg, CONTENT_LENGTH, REG_EXTENDED);
    int num = regcomp(&num_reg, CONTENT_NUM, REG_EXTENDED);
    int end = regcomp(&end_reg, "\r\n\r\n", REG_EXTENDED);

    if (head || cl || num || end) {
        return EXIT_FAILURE;
    }

    if (sock) {
        while (1) {
            uintptr_t connfd = ls_accept(sock);
            assert(connfd > 0);
            handle_connection(connfd);
        }
    }

    regfree(&head_reg);
    regfree(&cl_reg);
    regfree(&num_reg);
    regfree(&end_reg);
    destroyCache(&c);
    return EXIT_SUCCESS;
}

int process_args(int argc, char** argv, int* port, char** mode, int* n) {
    // check number of args
    if (argc < 4 || argc > 4) {
        usage(stderr, argv[0]); 
        return 1;
    }

     // parse port number
     char *endptr = NULL;
     (*port) = (size_t) strtoull(argv[1], &endptr, 10);
     if ((endptr && *endptr != '\0') || ((*port) < 1 || (*port) > 65535)) {
         fprintf(stderr, "invalid port number: %s\n", argv[1]);
         return 1;
     }
 
     // get mode and check validity
     (*mode) = argv[2];
     if ((strcmp((*mode), "FIFO") != 0) && (strcmp((*mode), "LRU") != 0)) {
        fprintf(stderr, "invalid mode: %s\n", argv[2]);
        return 1;
     }

     // get cache size similar method to port
     char *endptr2 = NULL;
     (*n) = (size_t) strtoull(argv[3], &endptr2, 10);
     if ((endptr2 && *endptr2 != '\0') || ((*n) < 0 || (*n) > 1024)) {
         fprintf(stderr, "invalid cache size: %s\n", argv[3]);
         return 1;
     }

    return 0;
}

void handle_connection(uintptr_t connfd) {

    // parse the proxy request from a conn fd.
    Prequest_t *preq = prequest_new(connfd);
    if (preq) {
        char *host = NULL, *uri = NULL;
        int32_t port;

        host = prequest_get_host(preq);
        uri = prequest_get_uri(preq);
        port = prequest_get_port(preq);

        LLnode_t* found;
        if (c == NULL) {
            found = NULL;
        }
        else{
            found = findNode(c, host, uri, port);
        }
        if (found != NULL) {
            fprintf(stderr, "Hit!\n");
            char* res = cacheHit(c, found);
            if (res != NULL) {
                setCachedTrue(found, res, found->res_l, head_reg, cl_reg, num_reg, end_reg);
                write_n_bytes(connfd, found->response, found->res_l);
            }   
        }
        else {
            fprintf(stderr, "Miss!\n");
            int32_t client_sock = cs_new(host, port);
            if (client_sock < 0) {
                fprintf(stderr, "Cannot connect to host %s:%d\n", host, port);
                close(connfd);
                return;
            }

            int res_l = 0;
            char* response = handle_get(client_sock, uri, connfd, &res_l); 
            cacheMiss(c, host, uri, port, response, res_l);

            if (response != NULL) {
                free(response);
            }
            close(client_sock);
        }

        printList(c->list);
        prequest_delete(&preq);
    } else {
        fprintf(stderr, "Bad request from %lu\n", connfd);
        return;
    }

    close(connfd);
}

char* handle_get(int client_sock, char* uri, int initial_conn, int* res_l) {

    // write GET request
    char request[2048];
    sprintf(request, "GET /%s HTTP/1.1\r\nConnection: Close\r\n\r\n", uri);

    // take request and write exactly req_length bytes to socket
    size_t req_length = strlen(request);
    write_n_bytes(client_sock, request, req_length);

    char buf[2048*10];
    char* response = (char*)malloc(sizeof(char)*MAX_CACHE_ENTRY);
    int bytes_read = 0;
    int total_read = 0;

    while ((bytes_read = read(client_sock, buf, 2048*10)) > 0) {
        memcpy(response + total_read, buf, bytes_read);
        write_n_bytes(initial_conn, buf, bytes_read);
        total_read += bytes_read;
    }

    response[total_read] = '\0';
    (*res_l) = total_read;
    return response;
}   


