#ifndef _REQUEST_H__
#define _REQUEST_H__

#include "connection.h"

#define PACKET_SIZE 512

struct request {
    Connection* connect;
    struct sockaddr_in* claddress;  // struct storing information about the connected client
    char* filename;                 // Name of file to work on
    OP code;                        // Code indicating what kind of info
    FILE* file;                     // The file being transfered to/from the server
    unsigned short blockno;         // The current blocknumber in the transfer
};
typedef struct request Request;

/**
 * @name construct_request()
 * @brief Constructs a new dynamically allocated request struct
 * @param char* request: The string to build the request from
 * @param int port: the port the request should occur on
 * @param struct sockaddr_in cl_addr: struct containing information about the client
 * @returns a dynamically allocated request struct OR NULL if an error occurs
 */
Request* construct_request(char* request, int port, struct sockaddr_in* cl_addr);

/**
 * @name execute_request()
 * @brief executes the given request struct
 * @note assumes that the request is valid
 * @param Request* req: The request to execute
 * @returns an int indicating (1) success or (0) failure
 */
int execute_request(Request* req);

/**
 * @name send_file()
 * @brief sends a file for a given request
 * @param Request* req: The request being fulfilled
 * @returns an int indicating (1) success or (0) failure
 */
int send_file(Request* req);

/**
 * @name receive_file()
 * @brief attempts to receive a file for a given request
 * @param Request* req: The request being fulfilled
 * @returns an int indicating (1) success or (0) failure
 */
int receive_file(Request* req);
#endif