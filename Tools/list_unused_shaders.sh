#!/bin/bash

for i in $(ls ../Data/GLSL/*frag); do 
    v=$(basename $i .frag)
    if ! grep -q "$v" -R ../Source/ ../Data/Levels; then
        #echo $i 
        basename $i .frag
    fi
done
