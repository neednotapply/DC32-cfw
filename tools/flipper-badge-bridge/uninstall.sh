#!/bin/sh
set -eu

if [ "$(id -u)" -ne 0 ]; then
    echo "Run as root." >&2
    exit 1
fi

systemctl disable --now flipper-badge-bridge.service 2>/dev/null || true
rm -f /etc/systemd/system/flipper-badge-bridge.service
rm -rf /opt/flipper-badge-bridge
if [ "${1:-}" = "--purge" ]; then
    rm -f /etc/default/flipper-badge-bridge
fi
systemctl daemon-reload
echo "Removed bridge files. The configuration file was retained unless --purge was supplied."
