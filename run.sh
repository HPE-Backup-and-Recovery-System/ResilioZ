#!/bin/bash

# Clear Previous Build
rm -rf build

# Build with Options
cmake -S . -B build \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build

# Run the Binary
cd build
./main
