#!/bin/sh

set -e

mkdir -p build
cd build
cmake .. -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build .
