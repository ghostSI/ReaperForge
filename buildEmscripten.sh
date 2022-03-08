#! /bin/bash

cd 3rdparty/emsdk/
source ./emsdk_env.sh
cd ../../
emcc -std=c++20 -O3 -flto -funsafe-math-optimizations -fno-exceptions -fno-rtti src/*.cpp -s USE_SDL=2 -o index.js \
--embed-file res/icon.ico
