#!/bin/bash
sudo ./subtitle -f fonts/7x13B.bdf -C 255,255,255 -s -0 --led-slowdown-gpio=5 --led-cols=64 --led-rows=32 --led-chain=6 --led-row-addr-type=0 -x 50 -y 0 --led-brightness=100 --led-no-hardware-pulse --led-pixel-mapper "Rotate:180" -i ./input.txt
