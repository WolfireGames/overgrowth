#!/bin/bash

kill -9 $(ps -e | grep Overgrowth.bin | head -1 | cut -d' ' -f 1)
