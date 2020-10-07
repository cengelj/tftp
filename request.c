#include "request.h"

/*
    ------- Header Formatting Functions -------
*/

/**
 * Creates the basic header of all the packets. Utilized by chained functions to build the right header
 */
void make_header(char* packet, OP code, unsigned short blockno) {
    *((unsigned short*)packet) = htons(code);
    *(((unsigned short*)packet) + 1) = htons(blockno);
}

/**
 * Format a packet with a request header specifically
 */
void make_req_header(Request* req, char* packet, OP code) {
    make_header(packet, code, req->blockno);
}

/**
 * Format a packet with a write header specifically
 */
void format_write_block(Request* req, char* packet, OP code) {
    make_req_header(req, packet, code);
}

/**
 * Format a packet with a read header specifically
 */
int format_read_block(Request* req, char* packet, OP code) {
    make_req_header(req, packet, code);
    return fread(packet + 4, 1, PACKET_SIZE, req->file);
}

/*
    ------- Simple Helper Functions -------
*/

/**
 * Simple function to ignore the SIGALRM and use as just an interrupt
 */
void ignore_alarm(int signalnum) {
    usleep(1);
}

/**
 * Parses the OP Code from a block of char data
 */
OP parse_request_code(char* code) {
    unsigned short* code_ptr = (unsigned short*)code;
    switch (ntohs(*code_ptr)) {
        case 1:
            return RRQ;
        case 2:
            return WRQ;
        case 3:
            return DATA;
        case 4:
            return ACK;
        case 5:
        default:
            return ERROR;
    }
}

/*
    ------- Generalized File Helper Functions -------
*/

/**
 * Wait for a response from the client
 * NOTE: int* msgsize is a pointer as the value will be changed to reflect the 
 * number of bytes received in response. 
 */
char* wait_for_response(Request* req, int elapsed_time, int* msgsize) {
    if (elapsed_time == 10)
        return NULL;
    char* response = get_datagram(req->connect, req->claddress, msgsize, 1);
    if (response == NULL) {  // Should only return NULL for an error or for a timeout via signal
        // Let's assume it isn't an error...
        return NULL;
    }
    return response;
}

/**
 * Create an ACK packet according to the given request
 */
char* create_ack(Request* req) {
    char* packet = (char*)malloc(5 * sizeof(char));
    format_write_block(req, packet, ACK);
    return packet;
}

/**
 * Attempt to send an ACK to the client
 */
int send_ack(Request* req) {
    char* packet = create_ack(req);
    if (sendto(req->connect->socketfd, packet, 4, 0, (struct sockaddr*)req->claddress, sizeof(*req->claddress)) == -1) {
        perror("sendto() failed");
        return 0;
    }
    return 1;
}

/*
    ------- Send File Functions -------
*/

/**
 * Verify that the packet recieved is a valid ACK
 */
int verify_ack(char* buffer, unsigned short exp_blockno) {
    OP code = parse_request_code(buffer);
    unsigned short blockno = ntohs(*((unsigned short*)buffer + 1));
    return code == ACK && blockno == exp_blockno;
}

/**
 * Wait for an ack. A wrapper around the wait for response function specifically to 
 * wait for an ACK packet. 
 */
int wait_for_ack(Request* req, int elapsed_time) {
    int msize = 5;
    char* response = wait_for_response(req, elapsed_time, &msize);
    if (response && verify_ack(response, req->blockno)) {
        free(response);
        return 1;
    }
    free(response);
    return 0;
}

/**
 * Attempt to transfer a packet to a remote host specfied by request
 */
int transfer_packet(Request* req) {
    int retrans = 0, bytes_read = 0;
    char* packet = NULL;
    do {
        if (packet == NULL) {
            packet = (char*)malloc((PACKET_SIZE + 5) * sizeof(char));
            bytes_read = format_read_block(req, packet, DATA);
        }
        if (sendto(req->connect->socketfd, packet, bytes_read + 4, 0, (struct sockaddr*)req->claddress, sizeof(*req->claddress)) == -1) {
            perror("sendto() failed");
            return 0;
        }
        if (wait_for_ack(req, retrans))
            retrans = 0;
        else {
            retrans++;
        }
    } while (retrans && retrans < 10);
    if (retrans == 10)
        fprintf(stderr, "ERROR: Connection timed out\n");
    free(packet);
    req->blockno++;
    return retrans == 10 ? 0 : bytes_read;
}

/**
 * Attempt to send a file to the remote host specified by request
 */
int send_file(Request* req) {
    int bytes_read = 0;
    do {
        bytes_read = transfer_packet(req);
#ifdef DEBUG
        printf("Bytes Read: %d\n", bytes_read);
#endif
    } while (bytes_read == PACKET_SIZE && bytes_read != 0);
    fclose(req->file);
    return bytes_read == 0 ? 0 : 1;
}

/*
    ------- Receive File Functions -------
*/

/**
 * Attempt to receive a packet from a remote host specified in request
 */
int receive_packet(Request* req) {
    req->blockno++;
    int retrans = 0, bytes_written = 0, elapsed_time = 0;
    char* packet;
    do {
        int size = PACKET_SIZE + 5;
        packet = wait_for_response(req, elapsed_time, &size);
        if (packet) {  // This technically assumes that the packet recieved is correct for the sent ACK, might be problematic
            bytes_written = size - 4;
            retrans = 0;
        } else {
            retrans++;
            if (!send_ack(req))
                return 0;
        }
    } while (retrans && retrans < 10);
    if (retrans == 10)
        fprintf(stderr, "ERROR: Connection timed out\n");
    else {
        fwrite(packet + 4, 1, bytes_written, req->file);
    }
    if (retrans == 10 || !send_ack(req))
        return 0;
    return bytes_written;
}

int receive_file(Request* req) {
    if (!send_ack(req))
        return 0;
    int bytes_written = 0;
    do {
        bytes_written = receive_packet(req);
#ifdef DEBUG
        printf("Bytes Written: %d\n", bytes_written);
#endif
    } while (bytes_written == PACKET_SIZE && bytes_written != 0);
    fclose(req->file);
    return bytes_written == 0 ? 0 : 1;
}

/*
    ------- Request Main Functions -------
*/

int execute_request(Request* req) {
    siginterrupt(SIGALRM, 1);
    signal(SIGALRM, ignore_alarm);
    if (req->code == RRQ) {
        req->blockno = 1;
        if ((req->file = find_file(req->filename, "r")) == NULL) {
            fprintf(stderr, "ERROR: File Not Found\n");
            char* packet = (char*)malloc(19 * sizeof(char));
            make_header(packet, ERROR, 1);
            sprintf(packet + 4, "File not found.");  // 15
            if (sendto(req->connect->socketfd, packet, 20, 0, (struct sockaddr*)req->claddress, sizeof(*req->claddress)) == -1) {
                return 0;
            }
            return 0;
        }
        if (!send_file(req))
            return 0;
    } else if (req->code == WRQ) {
        req->blockno = 0;
        if ((req->file = find_file(req->filename, "w")) == NULL) {
            fprintf(stderr, "ERROR: Problem creating file\n");
            return 0;
        }
        if (!receive_file(req))
            return 0;
    }
    return 1;
}

Request* construct_request(char* request, int port, struct sockaddr_in* cl_addr) {
    Request* req = (Request*)malloc(sizeof(Request));
    req->file = NULL;
    req->blockno = 0;
    req->claddress = (struct sockaddr_in*)calloc(1, sizeof(struct sockaddr_in));
    req->claddress = cl_addr;

    req->code = parse_request_code(request);
    char* req_itr = request += 2;
    if (req->code < DATA) {
        // Find filename
        req->filename = get_char_string(&req_itr, '\0', PACKET_SIZE);
        char* mode = get_char_string(&req_itr, '\0', PACKET_SIZE);
        if (!strcmp(mode, "octet")) {
            req->connect = construct_connection(port);
            free(mode);
            return req;
        } else {
            fprintf(stderr, "SERVER: Rcvd invalid mode\n");
            free(mode);
        }
    } else {
        fprintf(stderr, "SERVER: Rcvd invalid request\n");
    }
    if (req->claddress)
        free(req->claddress);
    if (req->filename)
        free(req->filename);
    if (req)
        free(req);
    return NULL;
}