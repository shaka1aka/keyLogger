#ifndef BADCLIENT_H
#define BADCLIENT_H

#include <stddef.h>
#include <openssl/sha.h>

#define PROC_FILE "/proc/hidden_bridge"
#define BUF_SIZE 1024
#define EXTRAS_LEN 3 + 1 + 1

int get_mac_string(const char *ifname, char *mac_str, size_t mac_str_size);

int hash_string_sha256(const char *input, char *out_hex, size_t out_hex_size);

#endif /* BADCLIENT_H */