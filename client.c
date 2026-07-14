#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#define PROC_FILE "/proc/hidden_bridge"
#define BUF_SIZE 1024

int main(void)
{
    char buffer[BUF_SIZE]; // To store the text we get from kernel
    int fd; // Will hold the id number Linux gives us when we open the file
    int bytes_read; // How many bytes the kernel gave

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
            printf("%.*s", bytes_read, buffer); // Just print the raw bytes from buffer
            fflush(stdout); // Force the terminal to instantly print the text (kernel doesnt add \n)
        }

        close(fd);
    }

    return 0;
}