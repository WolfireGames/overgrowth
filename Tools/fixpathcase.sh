#!/bin/bash

if [ $# -ne 2 ]; then
    echo "Usage: fixcasein.sh <filelist> <file>"
    echo "Make sure the working directory is relative to the paths"
    exit 1
fi

cat $1 | while read f
do
    base=$(basename "$f")
    match=$(find * -iwholename "$f")

    dest=$(echo $match | sed -e 's/[\/&]/\\&/g')
    sour=$(echo $f | sed -e 's/[]\/$*.^|[]/\\&/g')

    echo converting $sour to $dest
    
    sed -i -e "s/$sour/$dest/" "$2"
done
