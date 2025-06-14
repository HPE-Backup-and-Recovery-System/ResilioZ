#!/bin/bash
mkdir -p build

# Clear Previous Build Targets
cmake --build build --target clean

# Build with Options
cmake -S . -B build \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build

# Run the Binary
cd build
# ./cli
./gui