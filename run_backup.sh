#!/bin/bash

# Check if running as root
if [ "$EUID" -ne 0 ]; then
    echo "This program requires root privileges to mount NFS shares."
    echo "Please run with sudo: sudo $0"
    exit 1
fi

# Get the directory where the script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"

# Change to the build directory
cd "$SCRIPT_DIR/build"

# Run the program
./main 