#!/bin/bash

# Check if an argument is provided
if [ $# -ne 1 ]; then
    echo "Usage: $0 <directory>"
    exit 1
fi

directory="$1"

# Check if the provided path is a directory
if [ ! -d "$directory" ]; then
    echo "$directory is not a directory."
    exit 1
fi

program="./build/image-unpacker"  # Replace with the actual command to run your program

# Iterate through each file in the specified directory
for file in "$directory"/*; do
    if [ -f "$file" ]; then
        echo "Running $program on $file"
        $program "$file"
    fi
done
