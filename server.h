#ifndef SERVER_H
#define SERVER_H

#include <stddef.h>
#include <netinet/in.h>

#define BUF_SIZE 1024
#define BACKLOG 23
#define FILENAME_LEN 256
#define VICTIM_ID_LEN 128

typedef struct
{
    int socket_fd;
    char client_ip[INET_ADDRSTRLEN];
    int client_port;
} client_info_t;

#endif /* SERVER_H */