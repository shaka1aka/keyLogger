#!/bin/bash

cd /home/badguy/victim

# Stop the service (if it exists/enabled)
sudo systemctl stop keylogger.service

# Clean up payload
pkill -f "./client"
sudo rmmod keylogger
make clean

echo "[+] service stopped and payload cleaned"