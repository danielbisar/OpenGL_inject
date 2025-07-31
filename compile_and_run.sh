#!/bin/bash

set -e

LOCAL_SCRIPT_DIR="$(cd $(dirname "$BASH_SOURCE") && pwd)"

g++ -fPIC -lGL -lX11 -lpthread -lXrandr -lXi -ldl --shared -O2 -o ogl_inject.so lib.cpp
 __GL_SYNC_TO_VBLANK=0 vblank_mode=0 LD_PRELOAD="$LOCAL_SCRIPT_DIR/ogl_inject.so" glxgears
