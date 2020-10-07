#include "server.h"

// Entrance point
int main(int argc, char** argv) {
    if (argc != 3) {
        fprintf(stderr, "ERROR: Invalid Usage.\nUsage: a.out <lower bound> <upper bound>\nWhere the bounds define the valid range of ports\n");
        return EXIT_FAILURE;
    }
    Server* server = construct_server(atoi(argv[1]), atoi(argv[2]));
    if (!server)
        return EXIT_FAILURE;
    while (await_requests(server))
        ;
    return EXIT_SUCCESS;
}