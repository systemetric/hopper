#!/bin/bash

set -e

CC=gcc

echo "[*] Building Hopper server..."

mkdir -p build/
$CC -o build/hopper.server hopper/server/server.c hopper/server/pipe.c hopper/server/handler.c -Ihopper/server -Wall -Wextra -g

echo "[*] Installing Hopper server..."

hopper_server=$(realpath build/hopper.server)
ln -s "$hopper_server" /usr/bin/hopper.server

echo "[*] Installing Hopper client..."

pip install -e .

echo "[✓] Done."
