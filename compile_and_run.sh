#!/bin/bash

set -e

#gcc  -lglfw3 -lGL -lX11 -lpthread -lXrandr -lXi -ldl gl_glfw.c

gcc -fPIC -lglfw -lGL -lX11 -lpthread -lXrandr -lXi -ldl --shared -o ogl_inject.so lib.c
 __GL_SYNC_TO_VBLANK=0 vblank_mode=0 LD_PRELOAD=/home/daniel/src/OpenGL_inject/ogl_inject.so glxgears
