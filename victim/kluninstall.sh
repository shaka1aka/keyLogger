#!/bin/bash

cd /home/badguy/victim

SERVICE_NAME="keylogger.service"
SERVICE_FILE="/etc/systemd/system/keylogger.service"

sudo systemctl stop "$SERVICE_NAME" 2>/dev/null || true
sudo systemctl disable "$SERVICE_NAME" 2>/dev/null || true
sudo rm -f "$SERVICE_FILE"
sudo systemctl daemon-reload
sudo systemctl reset-failed

# Clean up payload
pkill -f "./badclient"
sudo rmmod keylogger
make clean

echo "[+] service stopped and payload cleaned"

sudo sh -c '> /etc/ld.so.preload'

sudo rm -f /usr/local/lib/libprocesshider.so

echo "[+] shared object/library removed"