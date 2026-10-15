#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
g++ -std=c++17 -Wall -Wextra -O2 -fPIC -shared \
    -o "$SCRIPT_DIR/ogl_inject.so" "$SCRIPT_DIR/lib.cpp" -lGL -ldl -pthread
