#!/bin/sh
set -eu

if [ "$(id -u)" -ne 0 ]; then
    echo "Run as root." >&2
    exit 1
fi

src_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
dest_dir=/opt/flipper-badge-bridge
unit=/etc/systemd/system/flipper-badge-bridge.service
defaults=/etc/default/flipper-badge-bridge

install -d -m 0755 "$dest_dir"
install -m 0755 "$src_dir/flipper_badge_bridge.py" "$dest_dir/flipper_badge_bridge.py"
install -m 0644 "$src_dir/README.md" "$dest_dir/README.md"
install -m 0644 "$src_dir/PROTOCOL.md" "$dest_dir/PROTOCOL.md"
install -m 0644 "$src_dir/flipper-badge-bridge.service" "$unit"
if [ ! -e "$defaults" ]; then
    install -m 0644 "$src_dir/defaults.env" "$defaults"
fi

systemctl daemon-reload
systemctl enable --now flipper-badge-bridge.service
echo "Installed. Configure $defaults only if discovery needs overriding."
