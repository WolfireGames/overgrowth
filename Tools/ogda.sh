#!/bin/bash

./Ogda.bin.x86_64 -i ../Data/ -o BuildData/ -j ../Deploy/release.xml --print-missing --manifest-input manifest.xml --manifest-output manifest.xml --perform-removes --force-removes --remove-unlisted --threads 8 2> rmlist.txt

for i in $(cat rmlist.txt | cut -d: -f 2 | sort -h -r); do sed -i -e "${i}d" ../Deploy/release.xml; done
