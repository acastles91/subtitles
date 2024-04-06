#!/bin/bash
sudo ./subtitle -f fonts/6x13.bdf -C 255,255,255 -s -0 --led-slowdown-gpio=4 --led-cols=64 --led-rows=32 --led-chain=4 -x 8 -y 0 --led-brightness=50 --led-no-hardware-pulse -i ./input.txt
