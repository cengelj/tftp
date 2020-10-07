#ifndef _CONNECTION_H__
#define _CONNECTION_H__

#ifndef SUBMITTY
#include "../unpv13e/lib/unp.h"
#endif

#ifdef SUBMITTY
#include "unp.h"
#endif

// Enum for simplifying code checking
enum opcode { RRQ = 1,
              WRQ = 2,
              DATA = 3,
              ACK = 4,
              ERROR = 5 };
typedef enum opcode OP;

struct connection {
    int port;                     // Port, also functionally the TID
    struct sockaddr_in* address;  // struct storing informtation about the server
    int socketfd;                 // the file descriptor for the given connection
};
typedef struct connection Connection;

/**
 * @name construct_connection()
 * @brief constructs and returns a new connection, dynamically allocated.
 * @param port an int indicating the port number for the connection to listen on
 * @returns a dynamically allocated connection
 */
Connection* construct_connection(int port);

/**
 * @name get_char_string()
 * @brief extracts a substring from the given string and returns it
 * @param char** str: Pointer to a cstring, double pointer is used so that the string position can be updated
 * @param char div: Character to end reading at
 * @param int maxsize: The size of the buffer originally, as well as a hard cap to the amount that can be read in. 
 * @returns the new char string. Also modifies str so that it begins one after max read. 
 */
char* get_char_string(char** str, char div, int maxsize);

/**
 * @name get_data()
 * @brief retrieves a UDP datagram from the client
 * @param Connection* connect: The connection to receive on
 * @param char* buffer: an unallocated buffer
 * @param int* msize: The max size of the datagram in bytes, upon successful return is set to number of bytes received
 * @param int should_to: An int indicating if this action should be able to timeout
 * @returns the datagram as a char array or NULL if an error occurs
 */
char* get_datagram(Connection* connect, struct sockaddr_in* from_address, int* msize, int should_to);

/**
 * @name find_file()
 * @brief retrieves a file with the given filename
 * @param char* filename: the name of the file
 * @param char* mode: the mode. EX: "R", "W", "WR", etc
 * @returns FILE* the file having been found
 */
FILE* find_file(char* filename, char* mode);
#endif