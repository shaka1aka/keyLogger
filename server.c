#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define BUF_SIZE 1024
#define BACKLOG 2

typedef struct
{
    int socket_fd; // Client socket
    char client_ip[INET_ADDRSTRLEN]; // Text form of client ip
    int client_port; // Client port (for debug)
} client_info_t;

void *handle_client(void *arg)
{
    client_info_t *client = (client_info_t *)arg;
    char buffer[BUF_SIZE];

    printf("(+) Client connected from %s:%d\n", client->client_ip, client->client_port);

    while (1)
    {
        memset(buffer, 0, sizeof(buffer));

        int bytes_received = recv(client->socket_fd, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received <= 0)
        {
            printf("(-) Client %s:%d disconnected.\n", client->client_ip, client->client_port);
            break;
        }

        printf("%.*s", bytes_received, buffer); // Print raw bytes from this client
        fflush(stdout);
    }

    close(client->socket_fd);
    free(client);
    pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
    int server_fd;
    struct sockaddr_in server_addr;
    int port = 8080; // Default port

    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-port") == 0)
        {
            port = atoi(argv[i + 1]);
            i++;
        }
    }

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

        // Dynamically allocate memory for the client fd.
        // Prevents a race condition where the main thread overwrites
        // the client_data variable before the new thread has a chance to read it.
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