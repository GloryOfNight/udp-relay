#!/bin/bash

# Create a temporary file to store the list of .cpp and .h files
temp_file="files.temp"

rm -f "$temp_file"

# Find all .cpp and .h files recursively from the current directory
find . -type f \( -name "*.cpp" -o -name "*.c" -o -name "*.cxx" -o -name "*.h" -o -name "*.hxx" \) > "$temp_file"

clang-format --files files.temp -i

# Remove the temporary file
rm -f "$temp_file"