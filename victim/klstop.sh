#!/bin/bash

cd /home/badguy/victim || exit 1

# systemd - stop the service (so it stops rerunning klrun.sh)
sudo systemctl stop keylogger.service || true

# Make sure build is fully cleaned up
pkill -f "./client" || true

sudo rmmod keylogger || true

make clean

echo "[+] service stopped and payload cleaned"