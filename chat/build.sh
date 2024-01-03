#!/bin/bash

# Check if the number of arguments is at least 1
if [ -z "$1" ]; then
    echo "You need to provide parameter. Provide 'server' or 'client'"
    exit 1
fi

echo $1

case $1 in
    "server")
        echo "Building Server"
        ;;
    "client")
        echo "Building Client"
        ;;
*)
    echo "Unsupported parameter. Supported parameters: 'server', 'client'"; exit 1
    ;;
esac

current_type=$1;

gcc $current_type/main.c -I"$current_type" -lncurses -o $current_type/build/main

if [ $? -ne 0 ]; then
    echo "Build Failed"
    exit 1
fi

./$current_type/build/main
