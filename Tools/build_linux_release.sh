#!/bin/bash

rm -r BuildRelease32
mkdir BuildRelease32
cd BuildRelease32
schroot --chroot steamrt_scout_i386 -- cmake ../Projects -DCMAKE_BUILD_TYPE=Release -DFORCE32=Yes
schroot --chroot steamrt_scout_i386 -- make clean
schroot --chroot steamrt_scout_i386 -- make 
cd ..
