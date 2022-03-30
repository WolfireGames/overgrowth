#!/bin/bash

for i in $(cat $1); do
    ./Overgrowth.bin.x86_64 -l "$i" --write-dir WriteDir/ --quit_after_load --no_dialogues
    echo "waiting for next"
    sleep 3
done
