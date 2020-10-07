#ifndef _SERVER_H__
#define _SERVER_H__
#include "request.h"

/**
 * Server struct
 * Stores the server that handles
 * new incoming requests
 */
struct server {
    Connection* connect;

    // Port Range
    int start_port;  // Defines the next avaliable port
    int end_port;    // Defines the last avaliable port
};
typedef struct server Server;

/**
 * @name construct_server()
 * @brief constructs a new dynamically allocated server struct from the 
 * given parameters. 
 * @param start_port an int describing the first avaliable port
 * @param end_port an int describing the last avaliable port
 * @returns Server* a dynamically allocated server struct
 */
Server* construct_server(int start_port, int end_port);

/**
 * @name await_requests()
 * @brief waits for a new connection request from a client
 * @param server: The server to listen on
 * @return an int indicating (1) success or (0) failure
 */
int await_requests(Server* server);
#endif