#!/bin/sh
set -eu

if [ "$(id -u)" -ne 0 ]; then
    echo "Run as root." >&2
    exit 1
fi

src_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
dest_dir=/opt/raspyjack-badge-bridge
unit=/etc/systemd/system/raspyjack-badge-bridge.service
defaults=/etc/default/raspyjack-badge-bridge

install -d -m 0755 "$dest_dir"
install -m 0755 "$src_dir/raspyjack_badge_bridge.py" "$dest_dir/raspyjack_badge_bridge.py"
install -m 0644 "$src_dir/README.md" "$dest_dir/README.md"
install -m 0644 "$src_dir/PROTOCOL.md" "$dest_dir/PROTOCOL.md"
install -m 0644 "$src_dir/optional-raspyjack-mirror-rate.patch" "$dest_dir/optional-raspyjack-mirror-rate.patch"
install -m 0644 "$src_dir/raspyjack-badge-bridge.service" "$unit"
if [ ! -e "$defaults" ]; then
    install -m 0644 "$src_dir/defaults.env" "$defaults"
fi

systemctl daemon-reload
systemctl enable --now raspyjack-badge-bridge.service
echo "Installed. Configure $defaults only if your paths differ."
