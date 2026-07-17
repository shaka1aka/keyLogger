#!/bin/bash

cd /home/badguy/victim

# Stop the service (if it exists/enabled)
sudo systemctl stop keylogger.service

# Clean up payload
pkill -f "./badclient"
sudo rmmod keylogger
make clean

echo "[+] service stopped and payload cleaned"