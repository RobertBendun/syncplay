#!/usr/bin/env bash

set -e

if [ ! -d build ]; then
	mkdir -p build && cd build && cmake ..
fi

cd build
cmake --build . -j 4
