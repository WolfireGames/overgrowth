#!/bin/bash

rm -r BuildRelease32
mkdir BuildRelease32
cd BuildRelease32
cmake ../Projects -DCMAKE_OSX_DEPLOYMENT_TARGET=10.6 -DCMAKE_BUILD_TYPE=Release
make clean
make 
cd ..
