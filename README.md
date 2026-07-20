# keylogger

A Linux kernel module that logs keystrokes into `/proc`, a TCP client that forwards them to a remote server, and a multi-threaded server that writes log files per victim.

## Build

## Run: one attacker VM + multiple victim VMs

### Attacker VM (server)

Terminal one:

```bash
./out/server
```

Terminal two:

```bash
python3 dashboard.py
```

---

### On each victim VM

```bash
./klinstall.sh
./klservice.sh
```

For each connected victim, the server will create a unique log file with their keystrokes.

## Cleanup

On each victim:

```bash
./kluninstall
```

Stop the server with `Ctrl+C` on the attacker.

---