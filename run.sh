#!/bin/bash

# Navigate to the project root
cd "$(dirname "$0")"

echo "Starting build process..."

# Create and navigate to the build directory
mkdir -p build
cd build

# Run CMake and Make
cmake ..
make

# Change back to the project root directory
cd ..

# Check if the executable was created in the project root and run it
if [ -f "./main" ]; then
    echo "Build successful. Running the application..."
    # Execute the program from the project root
    ./main
else
    echo "Build failed. Executable 'main' not found in the project root."
    exit 1
fi
