#!/bin/bash
mkdir -p build

# Clear Previous Build Targets
cmake --build build --target clean

# Build with Options
cmake -S . -B build \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build --parallel $(($(nproc) - 1)) --target all # Parallelize make for faster builds

# Run the Binary
cd build

# CLI
./main --cli

# GUI
./main --gui
