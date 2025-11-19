#!/bin/bash

set -e

echo "[*] Building Hopper server..."

zig build -Doptimize=ReleaseSafe

echo "[*] Installing Hopper server..."

hopper_server=$(realpath zig-out/bin/hopper.server)
cp "$hopper_server" /usr/local/bin/hopper.server

echo "[*] Installing Hopper client..."

pip install -e .

echo "[✓] Done."
