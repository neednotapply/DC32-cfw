#!/bin/sh
set -eu

if [ "$(id -u)" -ne 0 ]; then
    echo "Run as root." >&2
    exit 1
fi

systemctl disable --now pwnagotchi-badge-bridge.service 2>/dev/null || true
rm -f /etc/systemd/system/pwnagotchi-badge-bridge.service
rm -rf /opt/pwnagotchi-badge-bridge
if [ "${1:-}" = "--purge" ]; then
    rm -f /etc/default/pwnagotchi-badge-bridge
fi
systemctl daemon-reload
