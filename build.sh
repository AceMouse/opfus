#!/bin/bash

set -ex 
FREETYPE_FLAGS=$(pkg-config --cflags freetype2)
gcc obfus.c -o obfus -lm $FREETYPE_FLAGS -L/usr/local/lib -lfreetype -g
