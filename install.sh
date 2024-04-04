#!/bin/bash
cd ./include/rpi-rgb-led-matrix/lib
make
cd ../../../
g++ main.cpp -I./include/rpi-rgb-led-matrix/include/ -L./include/rpi-rgb-led-matrix/lib -lrgbmatrix -o subtitle
echo "Success!!"
