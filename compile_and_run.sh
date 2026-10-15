#!/bin/bash

set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"

if [[ "$SCRIPT_DIR" == *[[:space:]:]* ]]; then
    printf '%s\n' "LD_PRELOAD cannot represent library paths containing whitespace or colons; move the checkout." >&2
    exit 1
fi

bash "$SCRIPT_DIR/build.sh"
export OGL_INJECT_SHADER="${OGL_INJECT_SHADER:-$SCRIPT_DIR/fragment.shader}"
export LD_PRELOAD="$SCRIPT_DIR/ogl_inject.so${LD_PRELOAD:+:$LD_PRELOAD}"
exec glxgears "$@"
