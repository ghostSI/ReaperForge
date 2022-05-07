#! /bin/bash

cp -r ../res .
cp -r ../songs .
cd ../3rdparty/emsdk/
source ./emsdk_env.sh
cd ../../website
emcc -std=c++20 -O3 -flto -funsafe-math-optimizations -fno-exceptions -fno-rtti ../src/*.cpp -s USE_SDL=2 -s USE_WEBGL2=1 -s ASSERTIONS=1 -s ALLOW_MEMORY_GROWTH=1 -o index.js \
--embed-file res/icon.ico \
--embed-file songs/test.psarc
