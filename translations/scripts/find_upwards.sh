#!/bin/bash
#
# Finds a file upwards in the directory tree and returns the location
#
# Usage       : ./find_upwards.sh <file name>
#               The script will search the file with the given name upwards and
#               and return the paths (without the file name)
#
#************************************************************************************************

start_dir="$PWD"

while [[ "$PWD" != / ]] ; do
    if [ -f "$PWD/$1" ]; then
        echo "$PWD"
        cd "${start_dir}"
        exit 0
    else
        cd ..
    fi
done

cd "${start_dir}"
exit 1
