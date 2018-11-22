#!/bin/sh
# Usage: makeqrc [-prefix <prefix>] <source-directory> <target-qrc>

PREFIX=""
if [ "$1" = "-prefix" ]; then
    PREFIX="$2"
    shift 2
fi
if [ ! -d "$1" ]; then
    echo "Directory \'$1\' does not exist."
    exit 1
fi

# Remove existing files
rm -f "$2"

# Change directory due to the limitation of rcc
cd "$1"

# Generate .qrc and add "prefix" if needed
rcc -project -o "$2"
if [ -n $PREFIX ]; then
    sed -i "s/<qresource>/<qresource prefix=\"\/$PREFIX\">/g" "$2"
fi
