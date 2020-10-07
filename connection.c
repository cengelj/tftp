#include "connection.h"

/*
    ------- Connection Main Functions -------
*/

// "private" helper function to bind the address struct to a socket
int bind_connect(int socket, struct sockaddr_in serveraddr) {
    if (bind(socket, (struct sockaddr*)&serveraddr, sizeof(serveraddr)) < 0) {
        perror("bind() failed");
        return 0;
    }
    return 1;
}

Connection* construct_connection(int port) {
    // Allocate memory for the connection and init the socket
    Connection* c = (Connection*)malloc(sizeof(Connection));
    if ((c->socketfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        perror("socket() failed");
        return NULL;
    }

    // Setup the address struct for UDP
    c->address = (struct sockaddr_in*)malloc(sizeof(struct sockaddr_in));
    c->address->sin_addr.s_addr = htonl(INADDR_ANY);
    c->address->sin_family = AF_INET;
    c->address->sin_port = htons(port);

    if (!bind_connect(c->socketfd, *c->address))
        return NULL;
    return c;
}

/*
    ------- Helper Functions -------
*/
char* get_char_string(char** str, char div, int maxsize) {
    char* ret_string = (char*)malloc(maxsize * sizeof(char));
    int i;
    for (i = 0; *((*str) + i) != div && i < maxsize; i++) {
        ret_string[i] = (*((*str) + i));
    }
    ret_string[i] = '\0';
    if (i != maxsize)  // If terminated due to div byte, add one to move to valid data
        *str += 1;
    *str += i;
    return ret_string;
}

FILE* find_file(char* filename, char* mode) {
    // Assumes that the file will be in the same directory as the server
    FILE* file = fopen(filename, mode);
    return file ? file : NULL;
}

char* get_datagram(Connection* connect, struct sockaddr_in* from_address, int* msize, int should_to) {
    char* buffer = (char*)malloc(*msize * sizeof(char));
    socklen_t from_len = sizeof(*from_address);
    int n;
    errno = 0;
    if (should_to)
        alarm(1);
    if ((n = recvfrom(connect->socketfd, buffer, *msize - 1, 0, (struct sockaddr*)from_address, (socklen_t*)&from_len)) == -1) {
        if (errno != EINTR)
            perror("recvfrom() failed");
        return NULL;
    }
    if (should_to)
        alarm(0);
    buffer[n] = '\0';
    *msize = n;
    return buffer;
}