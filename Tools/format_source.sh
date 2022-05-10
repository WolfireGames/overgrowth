#!/bin/bash

find ./Source/ -iname *.h -o -iname *.cpp -o -iname *.hpp| xargs clang-format -i