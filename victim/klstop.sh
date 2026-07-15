#!/bin/bash

pkill -f "./client"

sudo rmmod keylogger

make clean

echo "[+] cleaned"