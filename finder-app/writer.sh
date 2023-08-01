#!/bin/bash

writefile=$1
writestr=$2

if ! [ $# -eq 2 ]; then
    echo "Number of input arguments must be 2!"
    return 1
fi

mkdir -p $(dirname "${writefile}")

touch "${writefile}"
echo "${writestr}" > "${writefile}"
