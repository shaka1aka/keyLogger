# keylogger

A Linux kernel module that logs keystrokes into `/proc`, a TCP client that forwards them to a remote server, and a multi-threaded server that writes log files per victim.

## Build

### Attacker (server)

```bash
./out/server
```

---

## Run: one attacker VM + multiple victim VMs

### On attacker VM

```bash
./out/server
```

### On each victim VM

```bash
./klrun.ko
```

For each connected victim, the server will create a unique log file with their keystrokes.

## Cleanup

On each victim:

```bash
./klstop.ko
```

Stop the server with `Ctrl+C` on the attacker.

---