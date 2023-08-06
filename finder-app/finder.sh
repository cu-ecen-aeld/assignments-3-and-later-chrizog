#!/bin/bash

if ! [ $# -eq 2 ]; then
    echo "Number of input arguments must be 2!"
    return 1
fi

filesdir=$1
searchstr=$2

if ! [ -e $filesdir ]; then
    echo "The diven directory ${filesdir} does not exist"
    return 1
fi

number_lines=0
number_files=0

for file in $(find $filesdir -type f);do
    file_counted=false
    while IFS= read -r line; do
        if [[ "${line}" == "${searchstr}" ]]; then
            number_lines=$((number_lines + 1))

            if [[ "${file_counted}" = false ]]; then
                number_files=$((number_files + 1))
                file_counted=true
            fi
        fi
    done < "${file}"
done

echo "The number of files are ${number_files} and the number of matching lines are ${number_lines}"
