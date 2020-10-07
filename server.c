#include "server.h"

Server* construct_server(int start_port, int end_port) {
    if (start_port > end_port) {
        fprintf(stderr, "ERROR: Invalid port numbers\nRequired: lower_bound > upper_bound\nUsage: a.out <lower bound> <upper bound>\n");
        return NULL;
    }
    Server* server = (Server*)malloc(sizeof(Server));
    server->connect = construct_connection(start_port);

    // Set the remaining values, and return
    server->start_port = start_port + 1;
    server->end_port = end_port;
    return server;
}

int await_requests(Server* server) {
    int msize = (PACKET_SIZE * 2) + 5;
    struct sockaddr_in* from_address = (struct sockaddr_in*)calloc(1, sizeof(struct sockaddr_in));
    char* buffer;
    if (!(buffer = get_datagram(server->connect, from_address, &msize, 0))) {
        return 0;
    }
#ifdef DEBUG
    printf("MAIN SERVER: Rcvd msg: [%s]\n", buffer);
#endif
    Request* req = construct_request(buffer, server->start_port, from_address);
    if (req) {
        server->start_port++;
        pid_t pid = fork();
        if (pid == -1) {
            perror("fork() failed");
            return 0;
        } else if (pid == 0) {
            execute_request(req);
            exit(0);
        } else {
            free(req);
        }
    }
    free(buffer);
    return 1;
}