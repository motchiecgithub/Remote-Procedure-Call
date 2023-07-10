#include "rpc.h"
#include "utils.h"
#include <stdlib.h>
#include <stdio.h>
#include <netdb.h>
#include <string.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>

#define MAX_HANDLE 10
#define INITIAL_THREAD_COUNT 10
#define NONBLOCKING
#define SERVICE 4
#define NODE "::1"
#define BUFFER_LEN 1000


struct rpc_server {
    int sockfd;
    rpc_handle **arr_handle;
    rpc_handler *arr_handler;
    int num;
    /* Add variable(s) for server state */
};

struct rpc_client {
    int sockfd;
    /* Add variable(s) for client state */
};

struct rpc_handle {
    char* name;
    /* Add variable(s) for handle */
};

int create_listening_socket(char *service);
void *thread_function(void *arg);

rpc_server *rpc_init_server(int port) {
    rpc_server *server = malloc(sizeof(server));
    server->arr_handle = malloc(sizeof(rpc_handle*) * MAX_HANDLE);
    server->arr_handler = malloc(sizeof(rpc_handler) * MAX_HANDLE);
    server->num = 0;
    char *service = malloc(sizeof(char) * SERVICE);
    sprintf(service, "%d", port);
    server->sockfd = create_listening_socket(service);
    return server;
}

int rpc_register(rpc_server *srv, char *name, rpc_handler handler) {
    if (handler == NULL){
        return -1;
    }
    // replace old handler with new handler 
    for (int i = 0; i < srv->num; i++){
        if (strcmp(srv->arr_handle[i]->name, name) == 0){
            srv->arr_handler[i] = handler;
            return 1;
        }
    }    
    // init handle
    srv->arr_handle[srv->num] = malloc(sizeof(rpc_handle));
    srv->arr_handle[srv->num]->name = malloc(sizeof(name));
    strcpy(srv->arr_handle[srv->num]->name, name);

    // init handler 
    srv->arr_handler[srv->num] = handler;
    srv->num += 1;
    return 1;
}

void rpc_serve_all(rpc_server *srv) {
    if (listen(srv->sockfd, 5) < 0){
        perror("listen");
    }
    // create threads for task 9
    pthread_t *thread_list = malloc(sizeof(pthread_t) * INITIAL_THREAD_COUNT);
    for (int i = 0; i < INITIAL_THREAD_COUNT; i ++) {
        pthread_create(&thread_list[i], NULL, thread_function, (void*)srv);
    }
    for (int i = 0; i < INITIAL_THREAD_COUNT; i ++) {
        pthread_join(thread_list[i], NULL);
    }
    pthread_exit(NULL);
    // free data 
    for (int i = 0; i < srv->num; i ++){
        free(srv->arr_handle[i]);
        free(srv->arr_handler[i]);
    }
    free(srv->arr_handle);
    free(srv->arr_handler);
    close(srv->sockfd);
}

rpc_client *rpc_init_client(char *addr, int port) {
    // Reference: Beej's networking guide, man pages
    int s;
    rpc_client *client = malloc(sizeof(rpc_client));
    struct addrinfo hints, *servinfo, *rp;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET6;
    hints.ai_socktype = SOCK_STREAM;
    char *service = malloc(sizeof(char) * 4);
    sprintf(service, "%d", port);
    s = getaddrinfo(addr, service, &hints, &servinfo);
    if (s !=0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
        return NULL;
    }
    for (rp = servinfo; rp != NULL; rp = rp->ai_next){
        client->sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (client->sockfd == -1) {continue;}
        if (connect(client->sockfd, rp->ai_addr, rp->ai_addrlen) != -1){
            break;
        }
        close(client->sockfd);
    }
    freeaddrinfo(servinfo);
    if (rp == NULL){
        fprintf(stderr, "client: failed to connect\n");
        return NULL;
    }
    return client;
}

rpc_handle *rpc_find(rpc_client *cl, char *name) {
    rpc_handle *result = malloc(sizeof(rpc_handle));
    int n;
    char package[256];
    package[0] = 'f';
    memcpy(package + 1, name, strlen(name));
    n = send(cl->sockfd, package, strlen(name) + 1, 0);
    if (n < 0){
        perror("write");
    }
    char buffer[256];
    n = recv(cl->sockfd, buffer, 255, 0);
    buffer[n] = '\0';
    if (n < 0){
        perror("read");
    } else if (strncmp(buffer, name, n) == 0){
        result->name = malloc(sizeof(buffer));
        strcpy(result->name, buffer);
        return result;
    }
    return NULL;
}

rpc_data *rpc_call(rpc_client *cl, rpc_handle *h, rpc_data *payload) {
    if (payload == NULL ||
        (payload->data2_len != 0 && payload->data2 == NULL) ||
        (payload->data2_len == 0 && payload->data2 != NULL)){
        return NULL;
    }
    int n;
    unsigned char *data = serialize(payload);
    // send payload with the name of handle to use
    int len = DATA1_SIZE + DATA2_LEN_SIZE + payload->data2_len;
    memcpy(data + len, h->name, strlen(h->name));
    n = send(cl->sockfd, data, len + strlen(h->name), 0);
    if (n < 0){
        perror("write");
    }
    unsigned char buffer2[256];
    n = recv(cl->sockfd, buffer2, 255, 0);
    if (n < 0){
        perror("read");
        // exit(EXIT_FAILURE);
    }
    if (n < DATA1_SIZE + DATA2_LEN_SIZE){
        perror("Invalid function call");
        return NULL;
    } else {
        rpc_data *response_data = decoding(buffer2);
        return response_data;
    }
    return NULL;
}

void rpc_close_client(rpc_client *cl) {
    if (cl->sockfd){
        close(cl->sockfd);
    }
}

void rpc_data_free(rpc_data *data) {
    if (data == NULL) {
        return;
    }
    if (data->data2 != NULL) {
        free(data->data2);
    }
    free(data);
}

int create_listening_socket(char *service){
    // Reference: Beej's networking guide, man pages
    int re, s, sockfd;
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET6;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    s = getaddrinfo(NODE, service, &hints, &res);
    if (s != 0){
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
    }

    sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd < 0){
        perror("socket");
    }

    // reuse port if possible 
    re = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &re, sizeof(int)) < 0){
        perror("setsockopt");
    }

    if (bind(sockfd, res->ai_addr, res->ai_addrlen) < 0){
        perror("bind");
    }
    freeaddrinfo(res);
    return sockfd;
}

void *thread_function(void *arg){
    rpc_server *srv = (rpc_server*)arg;
    int n;
    struct sockaddr_in client_addr;
    socklen_t client_addr_size;
    client_addr_size = sizeof client_addr;
    while (1){
        int newsockfd = accept(srv->sockfd, (struct sockaddr*)&client_addr, &client_addr_size);
        if (newsockfd < 0){
            perror("accept");
            break;
        }
        while (1){
            char buffer[BUFFER_LEN];
            n = recv(newsockfd, buffer, BUFFER_LEN - 1, 0);
            buffer[n] = '\0';
            if (n == 0){
                break;
            }
            if (n < 0){
                perror("read");
                break;
            }
            if (buffer[0] == 'f' && n != 1){
                // check for find() function
                int found = 0;
                for (int i = 0; i < srv->num; i++){
                    int len = strlen(srv->arr_handle[i]->name);
                    if (strncmp(buffer + 1, srv->arr_handle[i]->name, len) == 0){
                        found = 1;
                        n = send(newsockfd, srv->arr_handle[i]->name, len, 0);
                        if (n < 0){
                            perror("write");
                        }
                    }
                }
                if (found == 0){
                    n = send(newsockfd, "######", 6, 0);
                    if (n < 0){
                        perror("write");
                    }
                }
            } else if (n >= 16){
                // check for call() function
                unsigned char *buf = (unsigned char *)buffer;
                rpc_data *data_recv = decoding(buf); // decode from byte arr to struct 
                if (data_recv == NULL ||
                    (data_recv->data2_len == 0 && data_recv->data2 != NULL) ||
                    (data_recv->data2_len != 0 && data_recv->data2 == NULL)){
                        perror("function return NULL");
                } else {
                    size_t data_len = DATA1_SIZE + DATA2_LEN_SIZE + data_recv->data2_len;
                    /* check within handler list if there is match handler and return result */
                    int is_found = 0;
                    for (int i = 0; i < srv->num; i++){
                        int handle_len = strlen(srv->arr_handle[i]->name);
                        if (strncmp(buffer + data_len, srv->arr_handle[i]->name, handle_len) == 0){
                            is_found = 1;
                            rpc_data *response = srv->arr_handler[i](data_recv);
                            if (response == NULL ||
                                (response->data2_len == 0 && response->data2 != NULL) ||
                                (response->data2_len != 0 && response->data2 == NULL)){
                                    is_found = 0;
                                    break;
                            }
                            unsigned char *package = serialize(response);
                            int size = DATA1_SIZE + DATA2_LEN_SIZE + response->data2_len;
                            n = send(newsockfd, package, size, 0);
                            if (n < 0){
                                perror("write");
                            }
                        }
                    }
                    if (is_found == 0){
                        // return fail to client 
                        // however "#" is still in ASCII 32 - 126
                        n = send(newsockfd, "######", 6, 0);
                        if (n < 0){
                            perror("write");
                        }
                    }
                }
                
            }
        }
        close(newsockfd);
    }
    return NULL;
}