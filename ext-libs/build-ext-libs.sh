#!/usr/bin/env bash

rm -rf dist
mkdir dist

rm -rf build
mkdir build
cd build
cmake ../EncodingDetector
make
mv libencodingdetector.a ../dist/
cd ..
rm -rf build
