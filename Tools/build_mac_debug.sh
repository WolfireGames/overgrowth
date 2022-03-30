#!/bin/bash

rm -r BuildDebug32
mkdir BuildDebug32
cd BuildDebug32
cmake ../Projects -DCMAKE_OSX_DEPLOYMENT_TARGET=10.6 -DCMAKE_BUILD_TYPE=Debug
make clean
make 
cd ..
