#!/bin/sh
set -eu

if [ "$(id -u)" -ne 0 ]; then
    echo "Run as root." >&2
    exit 1
fi

src_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
dest_dir=/opt/pwnagotchi-badge-bridge
unit=/etc/systemd/system/pwnagotchi-badge-bridge.service
defaults=/etc/default/pwnagotchi-badge-bridge

install -d -m 0755 "$dest_dir"
install -m 0755 "$src_dir/pwnagotchi_badge_bridge.py" "$dest_dir/pwnagotchi_badge_bridge.py"
install -m 0644 "$src_dir/README.md" "$dest_dir/README.md"
install -m 0644 "$src_dir/PROTOCOL.md" "$dest_dir/PROTOCOL.md"
install -m 0644 "$src_dir/pwnagotchi-badge-bridge.service" "$unit"
if [ ! -e "$defaults" ]; then
    install -m 0600 "$src_dir/defaults.env" "$defaults"
fi

systemctl daemon-reload
systemctl enable --now pwnagotchi-badge-bridge.service
echo "Installed. Set PWN_USER/PWN_PASSWORD in $defaults if your local web UI requires them."
