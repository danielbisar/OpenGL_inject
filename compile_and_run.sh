#!/bin/bash

set -e

g++ -fPIC -lglfw -lGL -lX11 -lpthread -lXrandr -lXi -ldl --shared -O2 -o ogl_inject.so lib.cpp
 __GL_SYNC_TO_VBLANK=0 vblank_mode=0 LD_PRELOAD=/home/daniel/src/OpenGL_inject/ogl_inject.so glxgears
