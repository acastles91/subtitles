#!/bin/bash
sudo ./main -f fonts/8x13B.bdf -C 0,0,255 -O 255,255,255 -s -0 --led-slowdown-gpio=4 --led-cols=192 -x 8 -y 9 --led-brightness=50 -i input.txt
